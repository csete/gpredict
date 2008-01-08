#include <stdlib.h>
#include <goocanvas.h>


GtkWidget *canvas;
GooCanvasItem *move_item;
int num_added_widgets = 0;
GList *added_widget_items;

static void
add_widget_clicked (GtkWidget *button, gpointer data)
{
  GooCanvasItem *root, *witem;
  GtkWidget *widget;

  if (num_added_widgets % 2)
    widget = gtk_label_new ("Hello World");
  else
    widget = gtk_entry_new ();

  root = goo_canvas_get_root_item (GOO_CANVAS (canvas));
  witem = goo_canvas_widget_new (root, widget,
				 num_added_widgets * 50,
				 num_added_widgets * 50,
				 200, 50, NULL);

  added_widget_items = g_list_prepend (added_widget_items, witem);
  num_added_widgets++;
}


static void
remove_widget_clicked (GtkWidget *button, gpointer data)
{
  GooCanvasItem *witem;
  GList *elem;

  if (!added_widget_items)
    return;

  witem = added_widget_items->data;

  goo_canvas_item_remove (witem);

  elem = added_widget_items;
  added_widget_items = added_widget_items->next;
  g_list_free1 (elem);
				
  num_added_widgets--;
}


static void
move_widget_clicked (GtkWidget *button, gpointer data)
{
  static gint move_index = 0;
  static gdouble moves[][4] = {
    { 50, 50, -1, -1 },
    { 300, 100, -1, -1 },
    { 200, 200, -1, -1 },
    { 400, 100, -1, -1 },
  };

  g_object_set (move_item,
		"x", moves[move_index][0],
		"y", moves[move_index][1],
		"width", moves[move_index][2],
		"height", moves[move_index][3],
		NULL);
  move_index = (move_index + 1) % 4;
}


static void
change_anchor_clicked (GtkWidget *button, gpointer data)
{
  static GtkAnchorType anchor = GTK_ANCHOR_CENTER;

  g_print ("Setting anchor to: %i\n", anchor);
  g_object_set (move_item,
		"anchor", anchor,
		NULL);
  anchor++;
  if (anchor > GTK_ANCHOR_EAST)
    anchor = GTK_ANCHOR_CENTER;
}


static void
change_widget_clicked (GtkWidget *button, gpointer data)
{
  GooCanvasItem *witem;
  GtkWidget *old_widget, *widget;

  if (!added_widget_items)
    return;

  witem = added_widget_items->data;

  g_object_get (witem, "widget", &old_widget, NULL);

  if (GTK_IS_ENTRY (old_widget))
    widget = gtk_label_new ("Hello World");
  else
    widget = gtk_entry_new ();

  g_object_set (witem, "widget", widget, NULL);
}


static void
hide_item_clicked (GtkWidget *button, gpointer data)
{
  g_object_set (move_item,
		"visibility", GOO_CANVAS_ITEM_INVISIBLE,
		NULL);
}


static void
show_item_clicked (GtkWidget *button, gpointer data)
{
  g_object_set (move_item,
		"visibility", GOO_CANVAS_ITEM_VISIBLE,
		NULL);
}


static void
hide_canvas_clicked (GtkWidget *button, gpointer data)
{
  gtk_widget_hide (canvas);
}


static void
show_canvas_clicked (GtkWidget *button, gpointer data)
{
  gtk_widget_show (canvas);
}


static void
change_transform_clicked (GtkWidget *button, gpointer data)
{
  goo_canvas_item_translate (move_item, 25.0, 25.0);
}


static gboolean
on_delete_event (GtkWidget *window,
		 GdkEvent  *event,
		 gpointer   unused_data)
{
  gtk_main_quit ();
  return FALSE;
}


static gboolean
on_focus_in (GooCanvasItem *item,
	     GooCanvasItem *target,
	     GdkEventFocus *event,
	     gpointer data)
{
  gchar *id;

  id = g_object_get_data (G_OBJECT (item), "id");

  g_print ("%s received focus-in event\n", id ? id : "unknown");

  /* Note that this is only for testing. Setting item properties to indicate
     focus isn't a good idea for real apps, as there may be multiple views. */
  g_object_set (item, "stroke-color", "black", NULL);

  return FALSE;
}


static gboolean
on_focus_out (GooCanvasItem *item,
	      GooCanvasItem *target,
	      GdkEventFocus *event,
	      gpointer data)
{
  gchar *id;

  id = g_object_get_data (G_OBJECT (item), "id");

  g_print ("%s received focus-out event\n", id ? id : "unknown");

  /* Note that this is only for testing. Setting item properties to indicate
     focus isn't a good idea for real apps, as there may be multiple views. */
  g_object_set (item, "stroke-pattern", NULL, NULL);

  return FALSE;
}


static gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gpointer data)
{
  GooCanvas *canvas;
  gchar *id;

  id = g_object_get_data (G_OBJECT (item), "id");

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
  gchar *id;

  id = g_object_get_data (G_OBJECT (item), "id");

  g_print ("%s received key-press event\n", id ? id : "unknown");

  return FALSE;
}


static void
create_focus_box (GtkWidget     *canvas,
		  gdouble        x,
		  gdouble        y,
		  gdouble        width,
		  gdouble        height,
		  gchar         *color)
{
  GooCanvasItem *root, *item;

  root = goo_canvas_get_root_item (GOO_CANVAS (canvas));
  item = goo_canvas_rect_new (root, x, y, width, height,
			      "stroke-pattern", NULL,
			      "fill-color", color,
			      "line-width", 5.0,
			      "can-focus", TRUE,
			      NULL);
  g_object_set_data (G_OBJECT (item), "id", color);

  g_signal_connect (item, "focus_in_event",
		    (GtkSignalFunc) on_focus_in, NULL);
  g_signal_connect (item, "focus_out_event",
		    (GtkSignalFunc) on_focus_out, NULL);

  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, NULL);

  g_signal_connect (item, "key_press_event",
		    (GtkSignalFunc) on_key_press, NULL);
}


int
main (int argc, char *argv[])
{
  GtkWidget *window, *vbox, *hbox, *w, *scrolled_win;
  GtkWidget *label, *entry, *textview;
  GtkTextBuffer *buffer;
  GooCanvasItem *root, *witem;

  /* Initialize GTK+. */
  gtk_set_locale ();
  gtk_init (&argc, &argv);

  /* Create the window and widgets. */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 600);
  gtk_widget_show (window);
  g_signal_connect (window, "delete_event", (GtkSignalFunc) on_delete_event,
		    NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  w = gtk_button_new_with_label ("Add Widget");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) add_widget_clicked, NULL);

  w = gtk_button_new_with_label ("Remove Widget");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) remove_widget_clicked, NULL);

  w = gtk_button_new_with_label ("Move Widget");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) move_widget_clicked, NULL);

  w = gtk_button_new_with_label ("Change Anchor");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) change_anchor_clicked, NULL);

  w = gtk_button_new_with_label ("Change Widget");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) change_widget_clicked, NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  w = gtk_button_new_with_label ("Hide Canvas");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) hide_canvas_clicked, NULL);

  w = gtk_button_new_with_label ("Show Canvas");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) show_canvas_clicked, NULL);

  w = gtk_button_new_with_label ("Hide Item");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) hide_item_clicked, NULL);

  w = gtk_button_new_with_label ("Show Item");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) show_item_clicked, NULL);

  w = gtk_button_new_with_label ("Change Transform");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) change_transform_clicked, NULL);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_IN);
  gtk_widget_show (scrolled_win);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);

  canvas = goo_canvas_new ();
  GTK_WIDGET_SET_FLAGS (canvas, GTK_CAN_FOCUS);
  gtk_widget_set_size_request (canvas, 600, 450);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 1000, 1000);
  gtk_container_add (GTK_CONTAINER (scrolled_win), canvas);

  root = goo_canvas_get_root_item (GOO_CANVAS (canvas));

  /* Add a few simple items. */
  label = gtk_label_new ("Hello World");
  witem = goo_canvas_widget_new (root, label, 50, 50, 200, 100, NULL);
  g_object_set_data (G_OBJECT (witem), "id", "hello");

  entry = gtk_entry_new ();
  move_item = goo_canvas_widget_new (root, entry, 50, 250, 200, 50, NULL);
  g_object_set_data (G_OBJECT (move_item), "id", "entry1");

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "Size: -1 x -1");
  witem = goo_canvas_widget_new (root, entry, 50, 300, -1, -1, NULL);
  g_object_set_data (G_OBJECT (witem), "id", "entry2");

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "Size: 100 x -1");
  witem = goo_canvas_widget_new (root, entry, 50, 350, 100, -1, NULL);
  g_object_set_data (G_OBJECT (witem), "id", "entry3");

  /* Use a textview so we can see the width & height of the widget. */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  textview = gtk_text_view_new ();
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
  gtk_text_buffer_set_text (GTK_TEXT_BUFFER (buffer), "Size: -1 x 100", -1);
  gtk_widget_show (textview);
  gtk_container_add (GTK_CONTAINER (scrolled_win), textview);
  gtk_widget_set_size_request (scrolled_win, 160, 50);
  witem = goo_canvas_widget_new (root, scrolled_win, 50, 400, -1, 100, NULL);
  g_object_set_data (G_OBJECT (witem), "id", "scrolledwin");

  /* Create a vbox item with several child entry widgets to check focus
     traversal.*/
  vbox = gtk_vbox_new (FALSE, 4);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  witem = goo_canvas_widget_new (root, vbox, 50, 600, -1, -1, NULL);
  g_object_set_data (G_OBJECT (witem), "id", "vbox");

  /* Create a few normal canvas items that take keyboard focus. */
  create_focus_box (canvas, 110, 80, 50, 30, "red");
  create_focus_box (canvas, 300, 160, 50, 30, "orange");
  create_focus_box (canvas, 500, 50, 50, 30, "yellow");


  gtk_widget_show (canvas);

  /* Pass control to the GTK+ main event loop. */
  gtk_main ();

  return 0;
}


