#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


static gboolean
on_motion_notify (GooCanvasItem *item,
		  GooCanvasItem *target,
		  GdkEventMotion *event,
		  gpointer data)
{
  char *item_id = NULL;

  if (target)
    item_id = g_object_get_data (G_OBJECT (target), "id");

  if (item_id)
    g_print ("%s item received 'motion-notify' signal\n", item_id);

  return FALSE;
}


static void
setup_item_signals (GooCanvasItem *item)
{
  g_signal_connect (item, "motion_notify_event",
		    (GtkSignalFunc) on_motion_notify, NULL);
}


static void
create_events_area (GtkWidget              *canvas,
		    gint                    area_num,
		    GooCanvasPointerEvents  pointer_events,
		    gchar                  *label)
{
  gint row = area_num / 3, col = area_num % 3;
  gdouble x = col * 200, y = row * 150;
  GooCanvasItem *root, *rect;
  char *view_id;
  GooCanvasLineDash *dash;

  root = goo_canvas_get_root_item (GOO_CANVAS (canvas));

  dash = goo_canvas_line_dash_new (2, 5.0, 5.0);

  /* Create invisible item. */
  rect = goo_canvas_rect_new (root, x + 45, y + 35, 30, 30,
			      "fill-color", "red",
			      "visibility", GOO_CANVAS_ITEM_INVISIBLE,
			      "line-width", 5.0,
			      "pointer-events", pointer_events,
			      NULL);
  view_id = g_strdup_printf ("%s invisible", label);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);
  setup_item_signals (rect);

  /* Display a thin rect around it to indicate it is there. */
#if 1
  rect = goo_canvas_rect_new (root, x + 42.5, y + 32.5, 36, 36,
			      "line-dash", dash,
			      "line-width", 1.0,
			      "stroke-color", "gray",
			      NULL);
#endif

  /* Create unpainted item. */
  rect = goo_canvas_rect_new (root, x + 85, y + 35, 30, 30,
			      "stroke-pattern", NULL,
			      "line-width", 5.0,
			      "pointer-events", pointer_events,
			      NULL);
  view_id = g_strdup_printf ("%s unpainted", label);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);
  setup_item_signals (rect);

  /* Display a thin rect around it to indicate it is there. */
#if 1
  rect = goo_canvas_rect_new (root, x + 82.5, y + 32.5, 36, 36,
			      "line-dash", dash,
			      "line-width", 1.0,
			      "stroke-color", "gray",
			      NULL);
#endif

  /* Create stroked item. */
  rect = goo_canvas_rect_new (root, x + 125, y + 35, 30, 30,
			      "line-width", 5.0,
			      "pointer-events", pointer_events,
			      NULL);
  view_id = g_strdup_printf ("%s stroked", label);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);
  setup_item_signals (rect);

  /* Create filled item. */
  rect = goo_canvas_rect_new (root, x + 60, y + 75, 30, 30,
			      "fill-color", "red",
			      "stroke-pattern", NULL,
			      "line-width", 5.0,
			      "pointer-events", pointer_events,
			      NULL);
  view_id = g_strdup_printf ("%s filled", label);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);
  setup_item_signals (rect);

  /* Create stroked & filled item. */
  rect = goo_canvas_rect_new (root, x + 100, y + 75, 30, 30,
			      "fill-color", "red",
			      "line-width", 5.0,
			      "pointer-events", pointer_events,
			      NULL);
  view_id = g_strdup_printf ("%s stroked & filled", label);
  g_object_set_data_full (G_OBJECT (rect), "id", view_id, g_free);
  setup_item_signals (rect);

  goo_canvas_text_new (root, label, x + 100, y + 130, -1, GTK_ANCHOR_CENTER,
		       "font", "Sans 12",
		       "fill-color", "blue",
		       NULL);

  goo_canvas_line_dash_unref (dash);
}


GtkWidget *
create_events_page (void)
{
  GtkWidget *vbox, *alignment, *frame, *label, *canvas;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  /* Instructions */

  label = gtk_label_new ("Move the mouse over the items to check they receive the right motion events.\nThe first 2 items in each group are 1) invisible and 2) visible but unpainted.");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* Frame and canvas */

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
  gtk_widget_show (alignment);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (alignment), frame);
  gtk_widget_show (frame);

  canvas = goo_canvas_new ();

  gtk_widget_set_size_request (canvas, 600, 450);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 600, 450);
  gtk_container_add (GTK_CONTAINER (frame), canvas);
  gtk_widget_show (canvas);

  create_events_area (canvas, 0, GOO_CANVAS_EVENTS_NONE, "none");
  create_events_area (canvas, 1, GOO_CANVAS_EVENTS_VISIBLE_PAINTED, "visible-painted");
  create_events_area (canvas, 2, GOO_CANVAS_EVENTS_VISIBLE_FILL, "visible-fill");
  create_events_area (canvas, 3, GOO_CANVAS_EVENTS_VISIBLE_STROKE, "visible-stroke");
  create_events_area (canvas, 4, GOO_CANVAS_EVENTS_VISIBLE, "visible");
  create_events_area (canvas, 5, GOO_CANVAS_EVENTS_PAINTED, "painted");
  create_events_area (canvas, 6, GOO_CANVAS_EVENTS_FILL, "fill");
  create_events_area (canvas, 7, GOO_CANVAS_EVENTS_STROKE, "stroke");
  create_events_area (canvas, 8, GOO_CANVAS_EVENTS_ALL, "all");

  return vbox;
}
