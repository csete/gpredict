#include <config.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


#define PIECE_SIZE 50


static void
free_stuff (GtkObject *obj, gpointer data)
{
	g_free (data);
}

static void
test_win (GooCanvasItemModel **board)
{
	int i;
#if 0
	GtkWidget *dlg;
#endif

	for (i = 0; i < 15; i++)
		if (!board[i] || (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (board[i]), "piece_num")) != i))
			return;

#if 0
	dlg=gnome_ok_dialog ("You stud, you win!");
	gtk_window_set_modal(GTK_WINDOW(dlg),TRUE);
	gnome_dialog_run (GNOME_DIALOG (dlg));
#endif
}

static char *
get_piece_color (int piece)
{
	static char buf[50];
	int x, y;
	int r, g, b;

	y = piece / 4;
	x = piece % 4;

	r = ((4 - x) * 255) / 4;
	g = ((4 - y) * 255) / 4;
	b = 128;

	sprintf (buf, "#%02x%02x%02x", r, g, b);

	return buf;
}

static gboolean
piece_enter_notify (GooCanvasItem *item,
		    GooCanvasItem *target,
		    GdkEventCrossing *event,
		    gpointer data)
{
  GooCanvasItemModel *model, *text;

#if 0
  g_print ("In piece_enter_notify\n");
#endif

  model = goo_canvas_item_get_model (item);
  text = g_object_get_data (G_OBJECT (model), "text");

  g_object_set (text,
		"fill_color", "white",
		NULL);

  return FALSE;
}


static gboolean
piece_leave_notify (GooCanvasItem *item,
		    GooCanvasItem *target,
		    GdkEvent *event,
		    gpointer data)
{
  GooCanvasItemModel *model, *text;

#if 0
  g_print ("In piece_leave_notify\n");
#endif

  model = goo_canvas_item_get_model (item);
  text = g_object_get_data (G_OBJECT (model), "text");

  g_object_set (text,
		"fill_color", "black",
		NULL);

  return FALSE;
}


static gboolean
piece_button_press (GooCanvasItem *item,
		    GooCanvasItem *target,
		    GdkEventButton *event,
		    gpointer data)
{
        GooCanvas *canvas;
	GooCanvasItemModel **board;
	GooCanvasItemModel *model, *text;
	int num, pos, newpos;
	int x, y;
	double dx = 0.0, dy = 0.0;
	int move;

	model = goo_canvas_item_get_model (item);
	canvas = goo_canvas_item_get_canvas (item);
	board = g_object_get_data (G_OBJECT (canvas), "board");
	num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "piece_num"));
	pos = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "piece_pos"));
	text = g_object_get_data (G_OBJECT (model), "text");

#if 0
	g_print ("In piece_event pos: %i,%i num: %i\n", pos % 4, pos / 4,
		 num + 1);
#endif

	y = pos / 4;
	x = pos % 4;

	move = TRUE;

	if ((y > 0) && (board[(y - 1) * 4 + x] == NULL)) {
	  dx = 0.0;
	  dy = -1.0;
	  y--;
	} else if ((y < 3) && (board[(y + 1) * 4 + x] == NULL)) {
	  dx = 0.0;
	  dy = 1.0;
	  y++;
	} else if ((x > 0) && (board[y * 4 + x - 1] == NULL)) {
	  dx = -1.0;
	  dy = 0.0;
	  x--;
	} else if ((x < 3) && (board[y * 4 + x + 1] == NULL)) {
	  dx = 1.0;
	  dy = 0.0;
	  x++;
	} else
	  move = FALSE;

	if (move) {
	  newpos = y * 4 + x;
	  board[pos] = NULL;
	  board[newpos] = model;
	  g_object_set_data (G_OBJECT (model), "piece_pos", GINT_TO_POINTER (newpos));
	  goo_canvas_item_model_translate (model, dx * PIECE_SIZE,
					   dy * PIECE_SIZE);
	  test_win (board);
	}

	return FALSE;
}


static void
on_item_created (GooCanvas          *canvas,
		 GooCanvasItem      *item,
		 GooCanvasItemModel *model,
		 gpointer            data)
{
  if (goo_canvas_item_model_get_parent (model)
      && GOO_IS_CANVAS_GROUP_MODEL (model))
    {
      g_signal_connect (item, "enter_notify_event",
			(GtkSignalFunc) piece_enter_notify,
			NULL);
      g_signal_connect (item, "leave_notify_event",
			(GtkSignalFunc) piece_leave_notify,
			NULL);
      g_signal_connect (item, "button_press_event",
			(GtkSignalFunc) piece_button_press,
			NULL);
    }
}


#define SCRAMBLE_MOVES 256

static void
scramble (GtkObject *object, gpointer data)
{
	GooCanvas *canvas;
	GooCanvasItemModel **board;
	int i;
	int pos, oldpos;
	int dir;
	int x, y;

	srand (time (NULL));

	canvas = data;
	board = g_object_get_data (G_OBJECT (canvas), "board");

	/* First, find the blank spot */

	for (pos = 0; pos < 16; pos++)
		if (board[pos] == NULL)
			break;

	/* "Move the blank spot" around in order to scramble the pieces */

	for (i = 0; i < SCRAMBLE_MOVES; i++) {
retry_scramble:
		dir = rand () % 4;

		x = y = 0;

		if ((dir == 0) && (pos > 3)) /* up */
			y = -1;
		else if ((dir == 1) && (pos < 12)) /* down */
			y = 1;
		else if ((dir == 2) && ((pos % 4) != 0)) /* left */
			x = -1;
		else if ((dir == 3) && ((pos % 4) != 3)) /* right */
			x = 1;
		else
			goto retry_scramble;

		oldpos = pos + y * 4 + x;
		board[pos] = board[oldpos];
		board[oldpos] = NULL;
		g_object_set_data (G_OBJECT (board[pos]), "piece_pos",
				   GINT_TO_POINTER (pos));

		goo_canvas_item_model_translate (board[pos], -x * PIECE_SIZE,
						 -y * PIECE_SIZE);
		pos = oldpos;
	}
}

GtkWidget *
create_canvas_fifteen (void)
{
	GtkWidget *vbox;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *canvas;
	GtkWidget *button;
	GooCanvasItemModel **board;
	GooCanvasItemModel *root, *rect, *text;
	int i, x, y;
	char buf[20];

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

	alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, TRUE, TRUE, 0);
	gtk_widget_show (alignment);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (alignment), frame);
	gtk_widget_show (frame);

	/* Create the canvas and board */

	canvas = goo_canvas_new ();

	g_signal_connect (canvas, "item_created",
			  (GtkSignalFunc) on_item_created, NULL);

	root = goo_canvas_group_model_new (NULL, NULL);
	goo_canvas_set_root_item_model (GOO_CANVAS (canvas), root);
	g_object_unref (root);

	gtk_widget_set_size_request (canvas,
				     PIECE_SIZE * 4 + 1, PIECE_SIZE * 4 + 1);
	goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0,
			       PIECE_SIZE * 4 + 1, PIECE_SIZE * 4 + 1);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);

	board = g_new (GooCanvasItemModel *, 16);
	g_object_set_data (G_OBJECT (canvas), "board", board);
	g_signal_connect (canvas, "destroy",
			  (GtkSignalFunc) free_stuff, board);

	for (i = 0; i < 15; i++) {
		y = i / 4;
		x = i % 4;

		board[i] = goo_canvas_group_model_new (root, NULL);
		goo_canvas_item_model_translate (board[i], x * PIECE_SIZE,
						 y * PIECE_SIZE);

		rect = goo_canvas_rect_model_new (board[i], 0, 0,
						  PIECE_SIZE, PIECE_SIZE,
						  "fill_color", get_piece_color (i),
						  "stroke_color", "black",
						  "line-width", 1.0,
						  NULL);

		sprintf (buf, "%d", i + 1);

		text = goo_canvas_text_model_new (board[i], buf,
						  PIECE_SIZE / 2.0, PIECE_SIZE / 2.0,
						  -1, GTK_ANCHOR_CENTER,
						  "font", "Sans bold 24",
						  "fill_color", "black",
						  NULL);

		g_object_set_data (G_OBJECT (board[i]), "text", text);

		g_object_set_data (G_OBJECT (board[i]), "piece_num", GINT_TO_POINTER (i));
		g_object_set_data (G_OBJECT (board[i]), "piece_pos", GINT_TO_POINTER (i));

	}

	board[15] = NULL;

	/* Scramble button */

	button = gtk_button_new_with_label ("Scramble");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (button), "board", board);
	g_signal_connect (button, "clicked",
			  (GtkSignalFunc) scramble,
			  canvas);
	gtk_widget_show (button);

	return vbox;
}
