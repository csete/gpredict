#include <config.h>
#include <stdlib.h>
#include <goocanvas.h>


#define DEMO_RECT_ITEM 0
#define DEMO_TEXT_ITEM 1

static GooCanvas *canvas;

static gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gpointer data)
{
  gchar *id = g_object_get_data (G_OBJECT (item), "id");

  g_print ("%s received 'button-press' signal at %g, %g (root: %g, %g)\n",
	   id ? id : "unknown", event->x, event->y,
	   event->x_root, event->y_root);

  return TRUE;
}


static void
create_demo_item (GooCanvasItemModel *table,
		  gint           demo_item_type,
		  gint           row,
		  gint           column,
		  gint           rows,
		  gint           columns,
		  gchar         *text)
{
  GooCanvasItemModel *model;
  GooCanvasItem *item;

  switch (demo_item_type)
    {
    case DEMO_RECT_ITEM:
      model = goo_canvas_rect_model_new (table, 0, 0, 38, 19,
					 "fill-color", "red",
					 NULL);
      break;
    case DEMO_TEXT_ITEM:
      model = goo_canvas_text_model_new (table, text, 0, 0, -1, GTK_ANCHOR_NW,
					 NULL);
      break;
    }

  goo_canvas_item_model_set_child_properties (table, model,
					      "row", row,
					      "column", column,
					      "rows", rows,
					      "columns", columns,
#if 1
					      "x-expand", TRUE,
					      "x-fill", TRUE,
					      "y-expand", TRUE,
					      "y-fill", TRUE,
#endif
					      NULL);

  item = goo_canvas_get_item (canvas, model);
  g_object_set_data (G_OBJECT (item), "id", text);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, NULL);
}


static GooCanvasItemModel*
create_table (GooCanvasItemModel *parent,
	      gint           row,
	      gint           column,
	      gint           embedding_level,
	      gdouble        x,
	      gdouble        y,
	      gdouble        rotation,
	      gdouble        scale,
	      gint           demo_item_type)
{
  GooCanvasItemModel *table;

  /* Add a few simple items. */
  table = goo_canvas_table_model_new (parent,
				      "row-spacing", 4.0,
				      "column-spacing", 4.0,
				      NULL);
  goo_canvas_item_model_translate (table, x, y);
#if 1
  goo_canvas_item_model_rotate (table, rotation, 0, 0);
#endif
#if 1
  goo_canvas_item_model_scale (table, scale, scale);
#endif

  if (row != -1)
    goo_canvas_item_model_set_child_properties (parent, table,
						"row", row,
						"column", column,
#if 1
						"x-expand", TRUE,
						"x-fill", TRUE,
#endif
#if 0
						"y-expand", TRUE,
						"y-fill", TRUE,
#endif
					  NULL);

  if (embedding_level)
    {
      gint level = embedding_level - 1;
      create_table (table, 0, 0, level, 50, 50, 0, 0.7, demo_item_type);
      create_table (table, 0, 1, level, 50, 50, 45, 1.0, demo_item_type);
      create_table (table, 0, 2, level, 50, 50, 90, 1.0, demo_item_type);
      create_table (table, 1, 0, level, 50, 50, 135, 1.0, demo_item_type);
      create_table (table, 1, 1, level, 50, 50, 180, 1.5, demo_item_type);
      create_table (table, 1, 2, level, 50, 50, 225, 1.0, demo_item_type);
      create_table (table, 2, 0, level, 50, 50, 270, 1.0, demo_item_type);
      create_table (table, 2, 1, level, 50, 50, 315, 1.0, demo_item_type);
      create_table (table, 2, 2, level, 50, 50, 360, 2.0, demo_item_type);
    }
  else
    {
      create_demo_item (table, demo_item_type, 0, 0, 1, 1, "(0,0)");
      create_demo_item (table, demo_item_type, 0, 1, 1, 1, "(1,0)");
      create_demo_item (table, demo_item_type, 0, 2, 1, 1, "(2,0)");
      create_demo_item (table, demo_item_type, 1, 0, 1, 1, "(0,1)");
      create_demo_item (table, demo_item_type, 1, 1, 1, 1, "(1,1)");
      create_demo_item (table, demo_item_type, 1, 2, 1, 1, "(2,1)");
      create_demo_item (table, demo_item_type, 2, 0, 1, 1, "(0,2)");
      create_demo_item (table, demo_item_type, 2, 1, 1, 1, "(1,2)");
      create_demo_item (table, demo_item_type, 2, 2, 1, 1, "(2,2)");
    }

  return table;
}


static void
create_demo_table (GooCanvasItemModel *root,
		   gdouble        x,
		   gdouble        y,
		   gdouble        width,
		   gdouble        height)
{
  GooCanvasItemModel *table, *square, *circle, *triangle;
  GooCanvasItem *item;

  table = goo_canvas_table_model_new (root,
				      "row-spacing", 4.0,
				      "column-spacing", 4.0,
				      "width", width,
				      "height", height,
				      NULL);
  goo_canvas_item_model_translate (table, x, y);

  square = goo_canvas_rect_model_new (table, 0.0, 0.0, 50.0, 50.0,
				      "fill-color", "red",
				      NULL);
  goo_canvas_item_model_set_child_properties (table, square,
					      "row", 0,
					      "column", 0,
					      "x-shrink", TRUE,
					      NULL);
  item = goo_canvas_get_item (canvas, square);
  g_object_set_data (G_OBJECT (item), "id", "Red square");
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, NULL);

  circle = goo_canvas_ellipse_model_new (table, 0.0, 0.0, 25.0, 25.0,
					 "fill-color", "blue",
					 NULL);
  goo_canvas_item_model_set_child_properties (table, circle,
					      "row", 0,
					      "column", 1,
					      "x-shrink", TRUE,
					      NULL);
  item = goo_canvas_get_item (canvas, circle);
  g_object_set_data (G_OBJECT (item), "id", "Blue circle");
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, NULL);

  triangle = goo_canvas_polyline_model_new (table, TRUE, 3,
					    25.0, 0.0, 0.0, 50.0, 50.0, 50.0,
					    "fill-color", "yellow",
					    NULL);
  goo_canvas_item_model_set_child_properties (table, triangle,
					      "row", 0,
					      "column", 2,
					      "x-shrink", TRUE,
					      NULL);
  item = goo_canvas_get_item (canvas, triangle);
  g_object_set_data (G_OBJECT (item), "id", "Yellow triangle");
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, NULL);
}


static void
create_width_for_height_table (GooCanvasItemModel *root,
			       gdouble        x,
			       gdouble        y,
			       gdouble        width,
			       gdouble        height,
			       gdouble        rotation)
{
  GooCanvasItemModel *table, *model;
  GooCanvasItem *item;
  gchar *text = "This is a long paragraph that will have to be split over a few lines so we can see if its allocated height changes when its allocated width is changed.";

  table = goo_canvas_table_model_new (root,
#if 1
				      "width", width,
				      "height", height,
#endif
				      NULL);
  goo_canvas_item_model_translate (table, x, y);
  goo_canvas_item_model_rotate (table, rotation, 0, 0);

  model = goo_canvas_rect_model_new (table, 0.0, 0.0, width - 2, 10.0,
				     "fill-color", "red",
				     NULL);
  goo_canvas_item_model_set_child_properties (table, model,
					      "row", 0,
					      "column", 0,
					      "x-shrink", TRUE,
					      NULL);

#if 1
  model = goo_canvas_text_model_new (table, text, 0, 0, -1, GTK_ANCHOR_NW, NULL);
  goo_canvas_item_model_set_child_properties (table, model,
					      "row", 1,
					      "column", 0,
					      "x-expand", TRUE,
					      "x-fill", TRUE,
					      "x-shrink", TRUE,
					      "y-expand", TRUE,
					      "y-fill", TRUE,
					      NULL);
  item = goo_canvas_get_item (canvas, model);
  g_object_set_data (G_OBJECT (item), "id", "Text Item");
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, NULL);
#endif

  model = goo_canvas_rect_model_new (table, 0.0, 0.0, width - 2, 10.0,
				     "fill-color", "red",
				     NULL);
  goo_canvas_item_model_set_child_properties (table, model,
					      "row", 2,
					      "column", 0,
					      "x-shrink", TRUE,
					      NULL);
}


GtkWidget *
create_table_page (void)
{
  GtkWidget *vbox, *scrolled_win;
  GooCanvasItemModel *root, *table;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_IN);
  gtk_widget_show (scrolled_win);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);

  canvas = (GooCanvas*) goo_canvas_new ();
  gtk_widget_set_size_request ((GtkWidget*) canvas, 600, 450);
  goo_canvas_set_bounds (canvas, 0, 0, 1000, 2000);
  gtk_container_add (GTK_CONTAINER (scrolled_win), (GtkWidget*) canvas);

  root = goo_canvas_group_model_new (NULL, NULL);
  goo_canvas_set_root_item_model (canvas, root);
  g_object_unref (root);

#if 1
  create_demo_table (root, 400, 200, -1, -1);
  create_demo_table (root, 400, 260, 100, -1);
#endif

#if 1
  create_table (root, -1, -1, 0, 10, 10, 0, 1.0, DEMO_TEXT_ITEM);
  create_table (root, -1, -1, 0, 180, 10, 30, 1.0, DEMO_TEXT_ITEM);
  create_table (root, -1, -1, 0, 350, 10, 60, 1.0, DEMO_TEXT_ITEM);
  create_table (root, -1, -1, 0, 500, 10, 90, 1.0, DEMO_TEXT_ITEM);
#endif

#if 1
  table = create_table (root, -1, -1, 0, 30, 150, 0, 1.0, DEMO_TEXT_ITEM);
  g_object_set (table, "width", 300.0, "height", 100.0, NULL);
#endif

#if 1
  create_table (root, -1, -1, 1, 200, 200, 30, 0.8, DEMO_TEXT_ITEM);
#endif

#if 0
  table = create_table (root, -1, -1, 0, 10, 700, 0, 1.0, DEMO_WIDGET_ITEM);
  g_object_set (table, "width", 300.0, "height", 200.0, NULL);
#endif

  create_width_for_height_table (root, 100, 1000, 200, -1, 0);
#if 1
  create_width_for_height_table (root, 100, 1200, 300, -1, 0);

  create_width_for_height_table (root, 500, 1000, 200, -1, 30);
  create_width_for_height_table (root, 500, 1200, 300, -1, 30);
#endif

  gtk_widget_show ((GtkWidget*) canvas);

  return vbox;
}


