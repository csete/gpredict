#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


static gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gchar *id)
{
  g_print ("%s received 'button-press' signal at %g, %g (root: %g, %g)\n",
	   id, event->x, event->y, event->x_root, event->y_root);

  return TRUE;
}


static void
setup_canvas (GooCanvas *canvas)
{
  GooCanvasItemModel *root, *model, *table;
  GooCanvasItem *item;

  root = goo_canvas_group_model_new (NULL, NULL);
  goo_canvas_set_root_item_model (canvas, root);
  g_object_unref (root);

  /* Plain items without clip path. */
  model = goo_canvas_ellipse_model_new (root, 0, 0, 50, 30,
					"fill-color", "blue",
					NULL);
  goo_canvas_item_model_translate (model, 100, 100);
  goo_canvas_item_model_rotate (model, 30, 0, 0);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Blue ellipse (unclipped)");

  model = goo_canvas_rect_model_new (root, 200, 50, 100, 100,
				     "fill-color", "red",
				     "clip-fill-rule", CAIRO_FILL_RULE_EVEN_ODD,
				     NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Red rectangle (unclipped)");

  model = goo_canvas_rect_model_new (root, 380, 50, 100, 100,
				     "fill-color", "yellow",
				     NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Yellow rectangle (unclipped)");

  model = goo_canvas_text_model_new (root, "Sample Text", 520, 100, -1, GTK_ANCHOR_NW,
				     NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Text (unclipped)");


  /* Clipped items. */
  model = goo_canvas_ellipse_model_new (root, 0, 0, 50, 30,
					"fill-color", "blue",
					"clip-path", "M 0 0 h 100 v 100 h -100 Z",
					NULL);
  goo_canvas_item_model_translate (model, 100, 300);
  goo_canvas_item_model_rotate (model, 30, 0, 0);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Blue ellipse");

  model = goo_canvas_rect_model_new (root, 200, 250, 100, 100,
				     "fill-color", "red",
				     "clip-path", "M 250 300 h 100 v 100 h -100 Z",
				     "clip-fill-rule", CAIRO_FILL_RULE_EVEN_ODD,
				     NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Red rectangle");

  model = goo_canvas_rect_model_new (root, 380, 250, 100, 100,
				     "fill-color", "yellow",
				     "clip-path", "M480,230 l40,100 l-80 0 z",
				     NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Yellow rectangle");

  model = goo_canvas_text_model_new (root, "Sample Text", 520, 300, -1, GTK_ANCHOR_NW,
				     "clip-path", "M535,300 h75 v40 h-75 z",
				     NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Text (unclipped)");


  /* Table with clipped items. */
  table = goo_canvas_table_model_new (root, NULL);

  goo_canvas_item_model_translate (table, 200, 400);
  goo_canvas_item_model_rotate (table, 30, 0, 0);

  model = goo_canvas_ellipse_model_new (table, 0, 0, 50, 30,
					"fill-color", "blue",
					"clip-path", "M 0 0 h 100 v 100 h -100 Z",
					NULL);
  goo_canvas_item_model_translate (model, 100, 300);
  goo_canvas_item_model_rotate (model, 30, 0, 0);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Blue ellipse");

  model = goo_canvas_rect_model_new (table, 200, 250, 100, 100,
				     "fill-color", "red",
				     "clip-path", "M 250 300 h 100 v 100 h -100 Z",
				     "clip-fill-rule", CAIRO_FILL_RULE_EVEN_ODD,
				     NULL);
  goo_canvas_item_model_set_child_properties (table, model,
					      "column", 1,
					      NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Red rectangle");

  model = goo_canvas_rect_model_new (table, 380, 250, 100, 100,
				     "fill-color", "yellow",
				     "clip-path", "M480,230 l40,100 l-80 0 z",
				     NULL);
  goo_canvas_item_model_set_child_properties (table, model,
					      "column", 2,
					      NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Yellow rectangle");

  model = goo_canvas_text_model_new (table, "Sample Text", 520, 300, -1, GTK_ANCHOR_NW,
				     "clip-path", "M535,300 h75 v40 h-75 z",
				     NULL);
  goo_canvas_item_model_set_child_properties (table, model,
					      "column", 3,
					      NULL);
  item = goo_canvas_get_item (canvas, model);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, "Text (unclipped)");
}


GtkWidget *
create_clipping_page (void)
{
  GtkWidget *vbox, *scrolled_win, *canvas;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_IN);
  gtk_widget_show (scrolled_win);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled_win);

  canvas = goo_canvas_new ();
  gtk_widget_set_size_request (canvas, 600, 450);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 1000, 1000);
  gtk_widget_show (canvas);
  gtk_container_add (GTK_CONTAINER (scrolled_win), canvas);

  setup_canvas (GOO_CANVAS (canvas));

  return vbox;
}
