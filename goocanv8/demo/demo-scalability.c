#include <config.h>
#include <math.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


#define N_COLS 5
#define N_ROWS 20
#define PADDING 10

#if 1
#define USE_PIXMAP 
#endif

GtkWidget *
create_canvas_scalability (void)
{
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *scrolled_win, *canvas;
	GtkWidget *table;
	GdkPixbuf *pixbuf;
	GooCanvasItem *root, *item;
	int i, j, width, height;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 4);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
	gtk_widget_show (table);
	
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_table_attach (GTK_TABLE (table), frame,
			  0, 1, 0, 1,
			  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show (frame);

	/* Create the canvas and board */

	pixbuf = gdk_pixbuf_new_from_file("toroid.png", NULL);
#ifdef USE_PIXMAP
	width = gdk_pixbuf_get_width (pixbuf) + 3;
	height = gdk_pixbuf_get_height (pixbuf) +1;
#else
	width = 37;
	height = 19;
#endif
	
	canvas = goo_canvas_new ();

	root = goo_canvas_get_root_item (GOO_CANVAS (canvas));

	gtk_widget_set_size_request (canvas, 600, 450);
	goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0,
			       N_COLS * (width + PADDING),
			       N_ROWS * (height + PADDING));
	gtk_widget_show (canvas);


	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled_win);
	gtk_container_add (GTK_CONTAINER (frame), scrolled_win);

	gtk_container_add (GTK_CONTAINER (scrolled_win), canvas);


	for (i = 0; i < N_COLS; i++) {
		for (j = 0; j < N_ROWS; j++) {
#ifdef USE_PIXMAP
		  item = goo_canvas_image_new (root, pixbuf,
					       i * (width + PADDING),
					       j * (height + PADDING),
					      NULL);
#else
		  item = goo_canvas_rect_new (root,
					      i * (width + PADDING),
					      j * (height + PADDING),
					      width, height);
		  g_object_set (item,
				"fill_color", (j%2)?"mediumseagreen":"steelblue",
				NULL);
#endif
			
		}
	}

	return vbox;
}
