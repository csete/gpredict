#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


static gboolean
on_focus_in (GooCanvasItem *item,
	     GooCanvasItem *target,
	     GdkEventFocus *event,
	     gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (item);
  gchar *id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received focus-in event\n", id ? id : "unknown");

  /* Note that this is only for testing. Setting item properties to indicate
     focus isn't a good idea for real apps, as there may be multiple views. */
  g_object_set (model, "stroke-color", "black", NULL);

  return FALSE;
}


static gboolean
on_focus_out (GooCanvasItem *item,
	      GooCanvasItem *target,
	      GdkEventFocus *event,
	      gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (item);
  gchar *id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received focus-out event\n", id ? id : "unknown");

  /* Note that this is only for testing. Setting item properties to indicate
     focus isn't a good idea for real apps, as there may be multiple views. */
  g_object_set (model, "stroke-pattern", NULL, NULL);

  return FALSE;
}


static gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (item);
  gchar *id = g_object_get_data (G_OBJECT (model), "id");
  GooCanvas *canvas;

  g_print ("%s received button-press event\n", id ? id : "unknown");

  canvas = goo_canvas_item_get_canvas (item);
  goo_canvas_grab_focus (canvas, item);

  return TRUE;
}


static gboolean
on_key_press (GooCanvasItem *item,
	      GooCanvasItem *target,
	      GdkEventKey *event,
	      gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (item);
  gchar *id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received key-press event\n", id ? id : "unknown");

  return FALSE;
}


static void
on_item_created (GooCanvas          *view,
		 GooCanvasItem      *item,
		 GooCanvasItemModel *model,
		 gpointer            data)
{
  if (GOO_IS_CANVAS_RECT_MODEL (model))
    {
      g_object_set (model, "can-focus", TRUE, NULL);

      g_signal_connect (item, "focus_in_event",
			(GtkSignalFunc) on_focus_in, NULL);
      g_signal_connect (item, "focus_out_event",
			(GtkSignalFunc) on_focus_out, NULL);

      g_signal_connect (item, "button_press_event",
			(GtkSignalFunc) on_button_press, NULL);

      g_signal_connect (item, "key_press_event",
			(GtkSignalFunc) on_key_press, NULL);
    }
}


static void
create_focus_box (GtkWidget     *canvas,
		  gdouble        x,
		  gdouble        y,
		  gdouble        width,
		  gdouble        height,
		  gchar         *color)
{
  GooCanvasItemModel *root, *item;

  root = goo_canvas_get_root_item_model (GOO_CANVAS (canvas));
  item = goo_canvas_rect_model_new (root, x, y, width, height,
				    "stroke-pattern", NULL,
				    "fill-color", color,
				    "line-width", 5.0,
				    "can-focus", TRUE,
				    NULL);
  g_object_set_data (G_OBJECT (item), "id", color);
}


static void
setup_canvas (GtkWidget *canvas)
{
  GooCanvasItemModel *root;

  root = goo_canvas_group_model_new (NULL, NULL);
  goo_canvas_set_root_item_model (GOO_CANVAS (canvas), root);
  g_object_unref (root);

  create_focus_box (canvas, 110, 80, 50, 30, "red");
  create_focus_box (canvas, 300, 160, 50, 30, "orange");
  create_focus_box (canvas, 500, 50, 50, 30, "yellow");
  create_focus_box (canvas, 70, 400, 50, 30, "blue");
  create_focus_box (canvas, 130, 200, 50, 30, "magenta");
  create_focus_box (canvas, 200, 160, 50, 30, "green");
  create_focus_box (canvas, 450, 450, 50, 30, "cyan");
  create_focus_box (canvas, 300, 350, 50, 30, "grey");
  create_focus_box (canvas, 900, 900, 50, 30, "gold");
  create_focus_box (canvas, 800, 150, 50, 30, "thistle");
  create_focus_box (canvas, 600, 800, 50, 30, "azure");
  create_focus_box (canvas, 700, 250, 50, 30, "moccasin");
  create_focus_box (canvas, 500, 100, 50, 30, "cornsilk");
  create_focus_box (canvas, 200, 750, 50, 30, "plum");
  create_focus_box (canvas, 400, 800, 50, 30, "orchid");
}


GtkWidget *
create_focus_page (void)
{
  GtkWidget *vbox, *label, *scrolled_win, *canvas;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Use Tab, Shift+Tab or the arrow keys to move the keyboard focus between the canvas items.");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_IN);
  gtk_widget_show (scrolled_win);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled_win);

  canvas = goo_canvas_new ();
  GTK_WIDGET_SET_FLAGS (canvas, GTK_CAN_FOCUS);
  gtk_widget_set_size_request (canvas, 600, 450);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 1000, 1000);
  gtk_widget_show (canvas);
  gtk_container_add (GTK_CONTAINER (scrolled_win), canvas);

  g_signal_connect (canvas, "item_created",
		    (GtkSignalFunc) on_item_created, NULL);

  setup_canvas (canvas);

  return vbox;
}
