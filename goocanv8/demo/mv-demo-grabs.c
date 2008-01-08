#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


static gboolean
on_widget_expose (GtkWidget *widget,
		  GdkEventExpose *event,
		  char *item_id)
{
  g_print ("%s received 'expose' signal\n", item_id);

  gtk_paint_box (widget->style, widget->window, GTK_STATE_NORMAL,
		 GTK_SHADOW_IN, &event->area, widget, NULL, 0, 0,
		 widget->allocation.width, widget->allocation.height);

  return FALSE;
}


static gboolean
on_widget_enter_notify (GtkWidget *widget,
			GdkEventCrossing *event,
			char *item_id)
{
  g_print ("%s received 'enter-notify' signal\n", item_id);
  return TRUE;
}


static gboolean
on_widget_leave_notify (GtkWidget *widget,
			GdkEventCrossing *event,
			char *item_id)
{
  g_print ("%s received 'leave-notify' signal\n", item_id);
  return TRUE;
}


static gboolean
on_widget_motion_notify (GtkWidget *widget,
			 GdkEventMotion *event,
			 char *item_id)
{
  g_print ("%s received 'motion-notify' signal (window: %p)\n", item_id,
	   event->window);

  if (event->is_hint)
    gdk_window_get_pointer (event->window, NULL, NULL, NULL);

  return TRUE;
}


static gboolean
on_widget_button_press (GtkWidget *widget,
			GdkEventButton *event,
			char *item_id)
{
  g_print ("%s received 'button-press' signal\n", item_id);

  if (strstr (item_id, "explicit"))
    {
      GdkGrabStatus status;
      GdkEventMask mask = GDK_BUTTON_PRESS_MASK
	| GDK_BUTTON_RELEASE_MASK
	| GDK_POINTER_MOTION_MASK
	| GDK_POINTER_MOTION_HINT_MASK
	| GDK_ENTER_NOTIFY_MASK
	| GDK_LEAVE_NOTIFY_MASK;

      status = gdk_pointer_grab (widget->window, FALSE, mask, FALSE, NULL,
				 event->time);
      if (status == GDK_GRAB_SUCCESS)
	g_print ("grabbed pointer\n");
      else
	g_print ("pointer grab failed\n");
    }

  return TRUE;
}


static gboolean
on_widget_button_release (GtkWidget *widget,
			  GdkEventButton *event,
			  char *item_id)
{
  g_print ("%s received 'button-release' signal\n", item_id);

  if (strstr (item_id, "explicit"))
    {
      GdkDisplay *display;

      display = gtk_widget_get_display (widget);
      gdk_display_pointer_ungrab (display, event->time);
      g_print ("released pointer grab\n");
    }

  return TRUE;
}




static gboolean
on_enter_notify (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventCrossing *event,
		 gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);
  char *item_id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received 'enter-notify' signal\n", item_id);
  return FALSE;
}


static gboolean
on_leave_notify (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventCrossing *event,
		 gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);
  char *item_id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received 'leave-notify' signal\n", item_id);
  return FALSE;
}


static gboolean
on_motion_notify (GooCanvasItem *item,
		  GooCanvasItem *target,
		  GdkEventMotion *event,
		  gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);
  char *item_id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received 'motion-notify' signal\n", item_id);
  return FALSE;
}


static gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);
  char *item_id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received 'button-press' signal\n", item_id);

  if (strstr (item_id, "explicit"))
    {
      GooCanvas *canvas;
      GdkGrabStatus status;
      GdkEventMask mask = GDK_BUTTON_PRESS_MASK
	| GDK_BUTTON_RELEASE_MASK
	| GDK_POINTER_MOTION_MASK
	| GDK_POINTER_MOTION_HINT_MASK
	| GDK_ENTER_NOTIFY_MASK
	| GDK_LEAVE_NOTIFY_MASK;

      canvas = goo_canvas_item_get_canvas (item);
      status = goo_canvas_pointer_grab (canvas, item, mask, NULL, event->time);
      if (status == GDK_GRAB_SUCCESS)
	g_print ("grabbed pointer\n");
      else
	g_print ("pointer grab failed\n");
    }

  return FALSE;
}


static gboolean
on_button_release (GooCanvasItem *item,
		   GooCanvasItem *target,
		   GdkEventButton *event,
		   gpointer data)
{
  GooCanvasItemModel *model = goo_canvas_item_get_model (target);
  char *item_id = g_object_get_data (G_OBJECT (model), "id");

  g_print ("%s received 'button-release' signal\n", item_id);

  if (strstr (item_id, "explicit"))
    {
      GooCanvas *canvas;

      canvas = goo_canvas_item_get_canvas (item);
      goo_canvas_pointer_ungrab (canvas, item, event->time);
      g_print ("released pointer grab\n");
    }

  return FALSE;
}


static void
create_fixed (GtkTable *table, gint row, gchar *text, gchar *id)
{
  GtkWidget *label, *fixed, *drawing_area;
  char *view_id;

  label = gtk_label_new (text);
  gtk_table_attach (table, label, 0, 1, row, row + 1,
		    0, 0, 0, 0);
  gtk_widget_show (label);

  fixed = gtk_fixed_new ();
  gtk_fixed_set_has_window (GTK_FIXED (fixed), TRUE);
  gtk_widget_set_events (fixed,
			 GDK_EXPOSURE_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK
			 | GDK_KEY_PRESS_MASK
			 | GDK_KEY_RELEASE_MASK
			 | GDK_ENTER_NOTIFY_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_FOCUS_CHANGE_MASK);
  gtk_widget_set_size_request (fixed, 200, 100);
  gtk_table_attach (GTK_TABLE (table), fixed, 1, 2, row, row + 1,
		    0, 0, 0, 0);
  gtk_widget_show (fixed);

  view_id = g_strdup_printf ("%s-background", id);
  g_signal_connect (fixed, "expose_event",
		    (GtkSignalFunc) on_widget_expose, view_id);

  g_signal_connect (fixed, "enter_notify_event",
		    (GtkSignalFunc) on_widget_enter_notify, view_id);
  g_signal_connect (fixed, "leave_notify_event",
		    (GtkSignalFunc) on_widget_leave_notify, view_id);
  g_signal_connect (fixed, "motion_notify_event",
		    (GtkSignalFunc) on_widget_motion_notify, view_id);
  g_signal_connect (fixed, "button_press_event",
		    (GtkSignalFunc) on_widget_button_press, view_id);
  g_signal_connect (fixed, "button_release_event",
		    (GtkSignalFunc) on_widget_button_release, view_id);


  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_events (drawing_area,
			 GDK_EXPOSURE_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK
			 | GDK_KEY_PRESS_MASK
			 | GDK_KEY_RELEASE_MASK
			 | GDK_ENTER_NOTIFY_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_FOCUS_CHANGE_MASK);
			 
  gtk_widget_set_size_request (drawing_area, 60, 60);
  gtk_fixed_put (GTK_FIXED (fixed), drawing_area, 20, 20);
  gtk_widget_show (drawing_area);

  view_id = g_strdup_printf ("%s-left", id);
  g_signal_connect (drawing_area, "expose_event",
		    (GtkSignalFunc) on_widget_expose, view_id);

  g_signal_connect (drawing_area, "enter_notify_event",
		    (GtkSignalFunc) on_widget_enter_notify, view_id);
  g_signal_connect (drawing_area, "leave_notify_event",
		    (GtkSignalFunc) on_widget_leave_notify, view_id);
  g_signal_connect (drawing_area, "motion_notify_event",
		    (GtkSignalFunc) on_widget_motion_notify, view_id);
  g_signal_connect (drawing_area, "button_press_event",
		    (GtkSignalFunc) on_widget_button_press, view_id);
  g_signal_connect (drawing_area, "button_release_event",
		    (GtkSignalFunc) on_widget_button_release, view_id);


  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_events (drawing_area,
			 GDK_EXPOSURE_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK
			 | GDK_KEY_PRESS_MASK
			 | GDK_KEY_RELEASE_MASK
			 | GDK_ENTER_NOTIFY_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_FOCUS_CHANGE_MASK);
			 
  gtk_widget_set_size_request (drawing_area, 60, 60);
  gtk_fixed_put (GTK_FIXED (fixed), drawing_area, 120, 20);
  gtk_widget_show (drawing_area);

  view_id = g_strdup_printf ("%s-right", id);
  g_signal_connect (drawing_area, "expose_event",
		    (GtkSignalFunc) on_widget_expose, view_id);

  g_signal_connect (drawing_area, "enter_notify_event",
		    (GtkSignalFunc) on_widget_enter_notify, view_id);
  g_signal_connect (drawing_area, "leave_notify_event",
		    (GtkSignalFunc) on_widget_leave_notify, view_id);
  g_signal_connect (drawing_area, "motion_notify_event",
		    (GtkSignalFunc) on_widget_motion_notify, view_id);
  g_signal_connect (drawing_area, "button_press_event",
		    (GtkSignalFunc) on_widget_button_press, view_id);
  g_signal_connect (drawing_area, "button_release_event",
		    (GtkSignalFunc) on_widget_button_release, view_id);
}


static void
on_item_created (GooCanvas          *canvas,
		 GooCanvasItem      *item,
		 GooCanvasItemModel *model,
		 gpointer            data)
{
  if (GOO_IS_CANVAS_RECT (model))
    {
      g_signal_connect (item, "enter_notify_event",
			(GtkSignalFunc) on_enter_notify, NULL);
      g_signal_connect (item, "leave_notify_event",
			(GtkSignalFunc) on_leave_notify, NULL);
      g_signal_connect (item, "motion_notify_event",
			(GtkSignalFunc) on_motion_notify, NULL);
      g_signal_connect (item, "button_press_event",
			(GtkSignalFunc) on_button_press, NULL);
      g_signal_connect (item, "button_release_event",
			(GtkSignalFunc) on_button_release, NULL);
    }
}


static void
create_canvas (GtkTable *table, gint row, gchar *text, gchar *id)
{
  GtkWidget *label, *canvas;
  GooCanvasItemModel *root, *rect;
  char *view_id;

  label = gtk_label_new (text);
  gtk_table_attach (table, label, 0, 1, row, row + 1,
		    0, 0, 0, 0);
  gtk_widget_show (label);

  canvas = goo_canvas_new ();

  g_signal_connect (canvas, "item_created",
		    (GtkSignalFunc) on_item_created, NULL);

  gtk_widget_set_size_request (canvas, 200, 100);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 200, 100);
  gtk_table_attach (table, canvas, 1, 2, row, row + 1, 0, 0, 0, 0);
  gtk_widget_show (canvas);

  root = goo_canvas_group_model_new (NULL, NULL);
  goo_canvas_set_root_item_model (GOO_CANVAS (canvas), root);
  g_object_unref (root);

  rect = goo_canvas_rect_model_new (root, 0, 0, 200, 100,
				    "stroke-pattern", NULL,
				    "fill-color", "yellow",
				    NULL);
  view_id = g_strdup_printf ("%s-yellow", id);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);

  rect = goo_canvas_rect_model_new (root, 20, 20, 60, 60,
				    "stroke-pattern", NULL,
				    "fill-color", "blue",
				    NULL);
  view_id = g_strdup_printf ("%s-blue", id);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);

  rect = goo_canvas_rect_model_new (root, 120, 20, 60, 60,
				    "stroke-pattern", NULL,
				    "fill-color", "red",
				    NULL);
  view_id = g_strdup_printf ("%s-red", id);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);
}


GtkWidget *
create_grabs_page (void)
{
  GtkWidget *table, *label;

  table = gtk_table_new (5, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_widget_show (table);

  label = gtk_label_new ("Move the mouse over the widgets and canvas items on the right to see what events they receive.\nClick buttons to start explicit or implicit pointer grabs and see what events they receive now.\n(They should all receive the same events.)");
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1, 0, 0, 0, 0);
  gtk_widget_show (label);

  /* Drawing area with explicit grabs. */
  create_fixed (GTK_TABLE (table), 1,
		"Widget with Explicit Grabs:",
		"widget-explicit");

  /* Drawing area with implicit grabs. */
  create_fixed (GTK_TABLE (table), 2,
		"Widget with Implicit Grabs:",
		"widget-implicit");

  /* Canvas with explicit grabs. */
  create_canvas (GTK_TABLE (table), 3,
		 "Canvas with Explicit Grabs:",
		 "canvas-explicit");

  /* Canvas with implicit grabs. */
  create_canvas (GTK_TABLE (table), 4,
		 "Canvas with Implicit Grabs:",
		 "canvas-implicit");

  return table;
}
