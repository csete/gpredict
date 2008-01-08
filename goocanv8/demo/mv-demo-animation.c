#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <goocanvas.h>


static GooCanvasItemModel *root = NULL;
static GooCanvasItemModel *ellipse1, *ellipse2, *rect1, *rect2, *rect3, *rect4;


static void
start_animation_clicked (GtkWidget *button, gpointer data)
{
  /* Absolute. */
  goo_canvas_item_model_set_simple_transform (ellipse1, 100, 100, 1, 0);
  goo_canvas_item_model_animate (ellipse1, 500, 100, 2, 720, TRUE, 2000, 40,
			   GOO_CANVAS_ANIMATE_BOUNCE);

  goo_canvas_item_model_set_simple_transform (rect1, 100, 200, 1, 0);
  goo_canvas_item_model_animate (rect1, 100, 200, 1, 350, TRUE, 40 * 36, 40,
			   GOO_CANVAS_ANIMATE_RESTART);

  goo_canvas_item_model_set_simple_transform (rect3, 200, 200, 1, 0);
  goo_canvas_item_model_animate (rect3, 200, 200, 3, 0, TRUE, 400, 40,
			   GOO_CANVAS_ANIMATE_BOUNCE);

  /* Relative. */
  goo_canvas_item_model_set_simple_transform (ellipse2, 100, 400, 1, 0);
  goo_canvas_item_model_animate (ellipse2, 400, 0, 2, 720, FALSE, 2000, 40,
			   GOO_CANVAS_ANIMATE_BOUNCE);

  goo_canvas_item_model_set_simple_transform (rect2, 100, 500, 1, 0);
  goo_canvas_item_model_animate (rect2, 0, 0, 1, 350, FALSE, 40 * 36, 40,
			   GOO_CANVAS_ANIMATE_RESTART);

  goo_canvas_item_model_set_simple_transform (rect4, 200, 500, 1, 0);
  goo_canvas_item_model_animate (rect4, 0, 0, 3, 0, FALSE, 400, 40,
			   GOO_CANVAS_ANIMATE_BOUNCE);
}


static void
stop_animation_clicked (GtkWidget *button, gpointer data)
{
  goo_canvas_item_model_stop_animation (ellipse1);
  goo_canvas_item_model_stop_animation (ellipse2);
  goo_canvas_item_model_stop_animation (rect1);
  goo_canvas_item_model_stop_animation (rect2);
  goo_canvas_item_model_stop_animation (rect3);
  goo_canvas_item_model_stop_animation (rect4);
}


static GooCanvasItemModel*
create_canvas_model (void)
{
  if (root)
    return root;

  root = goo_canvas_group_model_new (NULL, NULL);

  /* Absolute. */
  ellipse1 = goo_canvas_ellipse_model_new (root, 0, 0, 25, 15,
					   "fill-color", "blue",
					   NULL);
  goo_canvas_item_model_translate (ellipse1, 100, 100);

  rect1 = goo_canvas_rect_model_new (root, -10, -10, 20, 20,
				     "fill-color", "blue",
				     NULL);
  goo_canvas_item_model_translate (rect1, 100, 200);

  rect3 = goo_canvas_rect_model_new (root, -10, -10, 20, 20,
				     "fill-color", "blue",
				     NULL);
  goo_canvas_item_model_translate (rect3, 200, 200);

  /* Relative. */
  ellipse2 = goo_canvas_ellipse_model_new (root, 0, 0, 25, 15,
					   "fill-color", "red",
					   NULL);
  goo_canvas_item_model_translate (ellipse2, 100, 400);

  rect2 = goo_canvas_rect_model_new (root, -10, -10, 20, 20,
				     "fill-color", "red",
				     NULL);
  goo_canvas_item_model_translate (rect2, 100, 500);

  rect4 = goo_canvas_rect_model_new (root, -10, -10, 20, 20,
				     "fill-color", "red",
				     NULL);
  goo_canvas_item_model_translate (rect4, 200, 500);

  return root;
}


GtkWidget *
create_animation_page (void)
{
  GtkWidget *vbox, *hbox, *w, *scrolled_win, *canvas;
  GooCanvasItemModel *root;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  w = gtk_button_new_with_label("Start Animation");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) start_animation_clicked,
		    NULL);

  w = gtk_button_new_with_label("Stop Animation");
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);
  g_signal_connect (w, "clicked", (GtkSignalFunc) stop_animation_clicked,
		    NULL);


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

  root = create_canvas_model ();
  goo_canvas_set_root_item_model (GOO_CANVAS (canvas), root);

  return vbox;
}
