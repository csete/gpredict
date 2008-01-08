#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <goocanvas.h>

#define LEFT    50.0
#define RIGHT  350.0
#define MIDDLE 150.0
#define DEFAULT_WIDTH   2
#define DEFAULT_SHAPE_A 4
#define DEFAULT_SHAPE_B 5
#define DEFAULT_SHAPE_C 4


static void
set_dimension (GooCanvas *canvas, char *arrow_name, char *text_name,
	       double x1, double y1, double x2, double y2,
	       double tx, double ty, int dim)
{
	GooCanvasPoints *points;
	char buf[100];

	points = goo_canvas_points_new (2);
	points->coords[0] = x1;
	points->coords[1] = y1;
	points->coords[2] = x2;
	points->coords[3] = y2;

	g_object_set (g_object_get_data (G_OBJECT (canvas), arrow_name),
		      "points", points,
		      NULL);

	sprintf (buf, "%d", dim);
	g_object_set (g_object_get_data (G_OBJECT (canvas), text_name),
		      "text", buf,
		      "x", tx,
		      "y", ty,
		      NULL);

	goo_canvas_points_unref (points);
}

static void
move_drag_box (GooCanvasItem *item, double x, double y)
{
	g_object_set (item,
		      "x", x - 5.0,
		      "y", y - 5.0,
		      NULL);
}


static void
set_arrow_shape (GooCanvas *canvas)
{
	int width;
	int shape_a, shape_b, shape_c;
	GooCanvasPoints *points;
	char buf[100];

	width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (canvas), "width"));
	shape_a = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (canvas), "shape_a"));
	shape_b = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (canvas), "shape_b"));
	shape_c = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (canvas), "shape_c"));

	/* Big arrow */

	g_object_set (g_object_get_data (G_OBJECT (canvas), "big_arrow"),
		      "line-width", 10.0 * width,
		      "arrow-tip-length", (double) (shape_a),
		      "arrow-length", (double) (shape_b),
		      "arrow-width", (double) (shape_c),
		      NULL);

	/* Outline */

	points = goo_canvas_points_new (5);
	points->coords[0] = RIGHT - 10 * shape_a * width;
	points->coords[1] = MIDDLE - 10 * width / 2;
	points->coords[2] = RIGHT - 10 * shape_b * width;
	points->coords[3] = MIDDLE - 10 * (shape_c * width / 2.0);
	points->coords[4] = RIGHT;
	points->coords[5] = MIDDLE;
	points->coords[6] = points->coords[2];
	points->coords[7] = MIDDLE + 10 * (shape_c * width / 2.0);
	points->coords[8] = points->coords[0];
	points->coords[9] = MIDDLE + 10 * width / 2;
	g_object_set (g_object_get_data (G_OBJECT (canvas), "outline"),
		      "points", points,
		      NULL);
	goo_canvas_points_unref (points);

	/* Drag boxes */

	move_drag_box (g_object_get_data (G_OBJECT (canvas), "width_drag_box"),
		       LEFT,
		       MIDDLE - 10 * width / 2.0);

	move_drag_box (g_object_get_data (G_OBJECT (canvas), "shape_a_drag_box"),
		       RIGHT - 10 * shape_a * width,
		       MIDDLE);

	move_drag_box (g_object_get_data (G_OBJECT (canvas), "shape_b_c_drag_box"),
		       RIGHT - 10 * shape_b * width,
		       MIDDLE - 10 * (shape_c * width / 2.0));

	/* Dimensions */

	set_dimension (canvas, "width_arrow", "width_text",
		       LEFT - 10,
		       MIDDLE - 10 * width / 2.0,
		       LEFT - 10,
		       MIDDLE + 10 * width / 2.0,
		       LEFT - 15,
		       MIDDLE,
		       width);

	set_dimension (canvas, "shape_a_arrow", "shape_a_text",
		       RIGHT - 10 * shape_a * width,
		       MIDDLE + 10 * (shape_c * width / 2.0) + 10,
		       RIGHT,
		       MIDDLE + 10 * (shape_c * width / 2.0) + 10,
		       RIGHT - 10 * shape_a * width / 2.0,
		       MIDDLE + 10 * (shape_c * width / 2.0) + 15,
		       shape_a);

	set_dimension (canvas, "shape_b_arrow", "shape_b_text",
		       RIGHT - 10 * shape_b * width,
		       MIDDLE + 10 * (shape_c * width / 2.0) + 35,
		       RIGHT,
		       MIDDLE + 10 * (shape_c * width / 2.0) + 35,
		       RIGHT - 10 * shape_b * width / 2.0,
		       MIDDLE + 10 * (shape_c * width / 2.0) + 40,
		       shape_b);

	set_dimension (canvas, "shape_c_arrow", "shape_c_text",
		       RIGHT + 10,
		       MIDDLE - 10 * shape_c * width / 2.0,
		       RIGHT + 10,
		       MIDDLE + 10 * shape_c * width / 2.0,
		       RIGHT + 15,
		       MIDDLE,
		       shape_c);

	/* Info */

	sprintf (buf, "line-width: %d", width);
	g_object_set (g_object_get_data (G_OBJECT (canvas), "width_info"),
		      "text", buf,
		      NULL);

	sprintf (buf, "arrow-tip-length: %d (* line-width)", shape_a);
	g_object_set (g_object_get_data (G_OBJECT (canvas), "shape_a_info"),
		      "text", buf,
		      NULL);

	sprintf (buf, "arrow-length: %d (* line-width)", shape_b);
	g_object_set (g_object_get_data (G_OBJECT (canvas), "shape_b_info"),
		      "text", buf,
		      NULL);
	sprintf (buf, "arrow-width: %d (* line-width)", shape_c);
	g_object_set (g_object_get_data (G_OBJECT (canvas), "shape_c_info"),
		      "text", buf,
		      NULL);

	/* Sample arrows */

	g_object_set (g_object_get_data (G_OBJECT (canvas), "sample_1"),
		      "line-width", (double) width,
		      "arrow-tip-length", (double) shape_a,
		      "arrow-length", (double) shape_b,
		      "arrow-width", (double) shape_c,
		      NULL);
	g_object_set (g_object_get_data (G_OBJECT (canvas), "sample_2"),
		      "line-width", (double) width,
		      "arrow-tip-length", (double) shape_a,
		      "arrow-length", (double) shape_b,
		      "arrow-width", (double) shape_c,
		      NULL);
	g_object_set (g_object_get_data (G_OBJECT (canvas), "sample_3"),
		      "line-width", (double) width,
		      "arrow-tip-length", (double) shape_a,
		      "arrow-length", (double) shape_b,
		      "arrow-width", (double) shape_c,
		      NULL);
}


static void
create_drag_box (GtkWidget *canvas,
		 GooCanvasItemModel *root,
		 char *box_name)
{
	GooCanvasItemModel *box;

	box = goo_canvas_rect_model_new (root, 0, 0, 10, 10,
					 "fill_color", "black",
					 "stroke_color", "black",
					 "line_width", 1.0,
					 NULL);
	g_object_set_data (G_OBJECT (canvas), box_name, box);
}


static void
create_dimension (GtkWidget *canvas,
		  GooCanvasItemModel *root,
		  char *arrow_name,
		  char *text_name,
		  GtkAnchorType anchor)
{
	GooCanvasItemModel *item;

	item = goo_canvas_polyline_model_new (root, FALSE, 0,
					      "fill_color", "black",
					      "start-arrow", TRUE,
					      "end-arrow", TRUE,
					      NULL);
	g_object_set_data (G_OBJECT (canvas), arrow_name, item);

	item = goo_canvas_text_model_new (root, NULL, 0, 0, -1, anchor,
					  "fill_color", "black",
					  "font", "Sans 12",
					  NULL);
	g_object_set_data (G_OBJECT (canvas), text_name, item);
}

static void
create_info (GtkWidget *canvas,
	     GooCanvasItemModel *root,
	     char *info_name,
	     double x,
	     double y)
{
	GooCanvasItemModel *item;

	item = goo_canvas_text_model_new (root, NULL, x, y, -1, GTK_ANCHOR_NW,
					  "fill_color", "black",
					  "font", "Sans 14",
					  NULL);
	g_object_set_data (G_OBJECT (canvas), info_name, item);
}

static void
create_sample_arrow (GtkWidget *canvas,
		     GooCanvasItemModel *root,
		     char *sample_name,
		     double x1,
		     double y1,
		     double x2,
		     double y2)
{
	GooCanvasItemModel *item;

	item = goo_canvas_polyline_model_new_line (root, x1, y1, x2, y2,
						   "start-arrow", TRUE,
						   "end-arrow", TRUE,
						   NULL);
	g_object_set_data (G_OBJECT (canvas), sample_name, item);
}


static gboolean
on_enter_notify (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventCrossing *event,
		 gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);

  g_object_set (model,
		"fill_color", "red",
		NULL);

  return TRUE;
}


static gboolean
on_leave_notify (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEvent *event,
		 gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);

  g_object_set (model,
		"fill_color", "black",
		NULL);

  return TRUE;
}


static gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gpointer data)
{
  GooCanvas *canvas;
  GdkCursor *fleur;

  fleur = gdk_cursor_new (GDK_FLEUR);
  canvas = goo_canvas_item_get_canvas (item);
  goo_canvas_pointer_grab (canvas, item,
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   fleur,
			   event->time);
  gdk_cursor_unref (fleur);

  return TRUE;
}


static gboolean
on_button_release (GooCanvasItem *item,
		   GooCanvasItem *target,
		   GdkEventButton *event,
		   gpointer data)
{
  GooCanvas *canvas;

  canvas = goo_canvas_item_get_canvas (item);
  goo_canvas_pointer_ungrab (canvas, item, event->time);

  return TRUE;
}


static gboolean
on_motion (GooCanvasItem *item,
	   GooCanvasItem *target,
	   GdkEventMotion *event,
	   gpointer data)
{
  GooCanvas *canvas = goo_canvas_item_get_canvas (item);
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);
  int x, y, width, shape_a, shape_b, shape_c;
  gboolean change = FALSE;

  if (!(event->state & GDK_BUTTON1_MASK))
    return FALSE;

  if (model == g_object_get_data (G_OBJECT (canvas), "width_drag_box"))
    {
      y = event->y;
      width = (MIDDLE - y) / 5;
      if (width < 0)
	return FALSE;
      g_object_set_data (G_OBJECT (canvas), "width", GINT_TO_POINTER (width));
      set_arrow_shape (canvas);
    }
  else if (model == g_object_get_data (G_OBJECT (canvas), "shape_a_drag_box"))
    {
      x = event->x;
      width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (canvas), "width"));
      shape_a = (RIGHT - x) / 10 / width;
      if ((shape_a < 0) || (shape_a > 30))
	return FALSE;
      g_object_set_data (G_OBJECT (canvas), "shape_a",
			 GINT_TO_POINTER (shape_a));
      set_arrow_shape (canvas);
    }
  else if (model == g_object_get_data (G_OBJECT (canvas), "shape_b_c_drag_box"))
    {
      width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (canvas), "width"));

      x = event->x;
      shape_b = (RIGHT - x) / 10 / width;
      if ((shape_b >= 0) && (shape_b <= 30)) {
	g_object_set_data (G_OBJECT (canvas), "shape_b",
			   GINT_TO_POINTER (shape_b));
	change = TRUE;
      }

      y = event->y;
      shape_c = (MIDDLE - y) * 2 / 10 / width;
      if (shape_c >= 0) {
	g_object_set_data (G_OBJECT (canvas), "shape_c",
			   GINT_TO_POINTER (shape_c));
	change = TRUE;
      }

      if (change)
	set_arrow_shape (canvas);
    }

  return TRUE;
}


static void
on_item_created (GooCanvas          *canvas,
		 GooCanvasItem      *item,
		 GooCanvasItemModel *model,
		 gpointer            data)
{
  if (GOO_IS_CANVAS_RECT_MODEL (model))
    {
      g_signal_connect (item, "enter_notify_event",
			(GtkSignalFunc) on_enter_notify,
			NULL);
      g_signal_connect (item, "leave_notify_event",
			(GtkSignalFunc) on_leave_notify,
			NULL);
      g_signal_connect (item, "button_press_event",
			(GtkSignalFunc) on_button_press,
			NULL);
      g_signal_connect (item, "button_release_event",
			(GtkSignalFunc) on_button_release,
			NULL);
      g_signal_connect (item, "motion_notify_event",
			(GtkSignalFunc) on_motion,
			NULL);
    }
}


GtkWidget *
create_canvas_arrowhead (void)
{
	GtkWidget *vbox;
	GtkWidget *w;
	GtkWidget *frame;
	GtkWidget *canvas;
	GooCanvasItemModel *root, *item;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

	w = gtk_label_new ("This demo allows you to edit arrowhead shapes.  Drag the little boxes\n"
			   "to change the shape of the line and its arrowhead.  You can see the\n"
			   "arrows at their normal scale on the right hand side of the window.");
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	w = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 0);
	gtk_widget_show (w);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (w), frame);
	gtk_widget_show (frame);

	canvas = goo_canvas_new ();

	g_signal_connect (canvas, "item_created",
			  (GtkSignalFunc) on_item_created, NULL);

	root = goo_canvas_group_model_new (NULL, NULL);
	goo_canvas_set_root_item_model (GOO_CANVAS (canvas), root);
	g_object_unref (root);

	gtk_widget_set_size_request (canvas, 500, 350);
	goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 500, 350);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);


	g_object_set_data (G_OBJECT (canvas), "width",
			   GINT_TO_POINTER (DEFAULT_WIDTH));
	g_object_set_data (G_OBJECT (canvas), "shape_a",
			   GINT_TO_POINTER (DEFAULT_SHAPE_A));
	g_object_set_data (G_OBJECT (canvas), "shape_b",
			   GINT_TO_POINTER (DEFAULT_SHAPE_B));
	g_object_set_data (G_OBJECT (canvas), "shape_c",
			   GINT_TO_POINTER (DEFAULT_SHAPE_C));

	/* Big arrow */

	item = goo_canvas_polyline_model_new_line (root,
						   LEFT, MIDDLE, RIGHT, MIDDLE,
						   "stroke_color", "mediumseagreen",
						   "end-arrow", TRUE,
						   NULL);
	g_object_set_data (G_OBJECT (canvas), "big_arrow", item);

	/* Arrow outline */

	item = goo_canvas_polyline_model_new (root, TRUE, 0,
					      "stroke_color", "black",
					      "line-width", 2.0,
					      "line-cap", CAIRO_LINE_CAP_ROUND,
					      "line-join", CAIRO_LINE_JOIN_ROUND,
					      NULL);
	g_object_set_data (G_OBJECT (canvas), "outline", item);

	/* Drag boxes */

	create_drag_box (canvas, root, "width_drag_box");
	create_drag_box (canvas, root, "shape_a_drag_box");
	create_drag_box (canvas, root, "shape_b_c_drag_box");

	/* Dimensions */

	create_dimension (canvas, root, "width_arrow", "width_text", GTK_ANCHOR_E);
	create_dimension (canvas, root, "shape_a_arrow", "shape_a_text", GTK_ANCHOR_N);
	create_dimension (canvas, root, "shape_b_arrow", "shape_b_text", GTK_ANCHOR_N);
	create_dimension (canvas, root, "shape_c_arrow", "shape_c_text", GTK_ANCHOR_W);

	/* Info */

	create_info (canvas, root, "width_info", LEFT, 260);
	create_info (canvas, root, "shape_a_info", LEFT, 280);
	create_info (canvas, root, "shape_b_info", LEFT, 300);
	create_info (canvas, root, "shape_c_info", LEFT, 320);

	/* Division line */

	goo_canvas_polyline_model_new_line (root, RIGHT + 50, 0, RIGHT + 50, 1000,
					    "fill_color", "black",
					    "line-width", 2.0,
					    NULL);

	/* Sample arrows */

	create_sample_arrow (canvas, root, "sample_1",
			     RIGHT + 100, 30, RIGHT + 100, MIDDLE - 30);
	create_sample_arrow (canvas, root, "sample_2",
			     RIGHT + 70, MIDDLE, RIGHT + 130, MIDDLE);
	create_sample_arrow (canvas, root, "sample_3",
			     RIGHT + 70, MIDDLE + 30, RIGHT + 130, MIDDLE + 120);

	/* Done! */
	
	set_arrow_shape (GOO_CANVAS (canvas));
	return vbox;
}
