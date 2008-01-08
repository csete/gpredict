#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


static gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (item);
  GooCanvasItemModel *parent1, *parent2, *parent;

  if (event->button != 1 || event->type != GDK_BUTTON_PRESS)
    return FALSE;

  parent1 = g_object_get_data (G_OBJECT (model), "parent1");
  parent2 = g_object_get_data (G_OBJECT (model), "parent2");

  parent = goo_canvas_item_model_get_parent (model);
  g_object_ref (model);
  goo_canvas_item_model_remove (model);
  if (parent == parent1)
    goo_canvas_item_model_add_child (parent2, model, -1);
  else
    goo_canvas_item_model_add_child (parent1, model, -1);
  g_object_unref (model);

  return TRUE;
}


static void
on_item_created (GooCanvas          *canvas,
		 GooCanvasItem      *item,
		 GooCanvasItemModel *model,
		 gpointer            data)
{
  if (g_object_get_data (G_OBJECT (model), "parent1"))
    {
      g_signal_connect (item, "button_press_event",
			(GtkSignalFunc) on_button_press, NULL);
    }
}


GtkWidget *
create_canvas_features (void)
{
	GtkWidget *vbox;
	GtkWidget *w;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *canvas;
	GooCanvasItemModel *root, *item;
	GooCanvasItemModel *parent1;
	GooCanvasItemModel *parent2;
	GooCanvasItemModel *group;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

	/* Instructions */

	w = gtk_label_new ("Reparent test:  click on the items to switch them between parents");
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	/* Frame and canvas */

	alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
	gtk_widget_show (alignment);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (alignment), frame);
	gtk_widget_show (frame);

	canvas = goo_canvas_new ();

	g_signal_connect (canvas, "item_created",
			  (GtkSignalFunc) on_item_created, NULL);

	root = goo_canvas_group_model_new (NULL, NULL);

	gtk_widget_set_size_request (canvas, 400, 200);
	goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 300, 200);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);

	/* First parent and box */

	parent1 = goo_canvas_group_model_new (root, NULL);

	goo_canvas_rect_model_new (parent1, 0, 0, 200, 200,
				   "fill_color", "tan",
				   NULL);

	/* Second parent and box */

	parent2 = goo_canvas_group_model_new (root, NULL);
	goo_canvas_item_model_translate (parent2, 200, 0);

	goo_canvas_rect_model_new (parent2, 0, 0, 200, 200,
				   "fill_color", "#204060",
				   NULL);

	/* Big circle to be reparented */

	item = goo_canvas_ellipse_model_new (parent1, 100, 100, 90, 90,
					     "stroke_color", "black",
					     "fill_color", "mediumseagreen",
					     "line-width", 3.0,
					     NULL);
	g_object_set_data (G_OBJECT (item), "parent1", parent1);
	g_object_set_data (G_OBJECT (item), "parent2", parent2);
#if 0
	g_signal_connect (G_OBJECT (item), "event",
			  (GtkSignalFunc) item_event,
			  NULL);
#endif

	/* A group to be reparented */

	group = goo_canvas_group_model_new (parent2, NULL);
	goo_canvas_item_model_translate (group, 100, 100);

	goo_canvas_ellipse_model_new (group, 0, 0, 50, 50,
				      "stroke_color", "black",
				      "fill_color", "wheat",
				      "line_width", 3.0,
				      NULL);
	goo_canvas_ellipse_model_new (group, 0, 0, 25, 25,
				      "fill_color", "steelblue",
				      NULL);

	g_object_set_data (G_OBJECT (group), "parent1", parent1);
	g_object_set_data (G_OBJECT (group), "parent2", parent2);
#if 0
	g_signal_connect (G_OBJECT (group), "event",
			    (GtkSignalFunc) item_event,
			    NULL);
#endif

	goo_canvas_set_root_item_model (GOO_CANVAS (canvas), root);
	g_object_unref (root);

	/* Done */

	return vbox;
}
