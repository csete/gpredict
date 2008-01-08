/*
 * GooCanvas Demo. Copyright (C) 2006 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * demo-item.c - a simple demo item.
 */
#include <config.h>
#include "goocanvas.h"
#include "demo-item.h"


/* Use the GLib convenience macro to define the type. GooDemoItem is the
   class struct, goo_demo_item is the function prefix, and our class is a
   subclass of GOO_TYPE_CANVAS_ITEM_SIMPLE. */
G_DEFINE_TYPE (GooDemoItem, goo_demo_item, GOO_TYPE_CANVAS_ITEM_SIMPLE)


/* The standard object initialization function. */
static void
goo_demo_item_init (GooDemoItem *demo_item)
{
  demo_item->x = 0.0;
  demo_item->y = 0.0;
  demo_item->width = 0.0;
  demo_item->height = 0.0;
}


/* The convenience function to create new items. This should start with a 
   parent argument and end with a variable list of object properties to fit
   in with the standard canvas items. */
GooCanvasItem*
goo_demo_item_new (GooCanvasItem      *parent,
		   gdouble             x,
		   gdouble             y,
		   gdouble             width,
		   gdouble             height,
		   ...)
{
  GooCanvasItem *item;
  GooDemoItem *demo_item;
  const char *first_property;
  va_list var_args;

  item = g_object_new (GOO_TYPE_DEMO_ITEM, NULL);

  demo_item = (GooDemoItem*) item;
  demo_item->x = x;
  demo_item->y = y;
  demo_item->width = width;
  demo_item->height = height;

  va_start (var_args, height);
  first_property = va_arg (var_args, char*);
  if (first_property)
    g_object_set_valist ((GObject*) item, first_property, var_args);
  va_end (var_args);

  if (parent)
    {
      goo_canvas_item_add_child (parent, item, -1);
      g_object_unref (item);
    }

  return item;
}


/* The update method. This is called when the canvas is initially shown and
   also whenever the object is updated and needs to change its size and/or
   shape. It should calculate its new bounds in its own coordinate space,
   storing them in simple->bounds. */
static void
goo_demo_item_update  (GooCanvasItemSimple *simple,
		       cairo_t             *cr)
{
  GooDemoItem *demo_item = (GooDemoItem*) simple;

  /* Compute the new bounds. */
  simple->bounds.x1 = demo_item->x;
  simple->bounds.y1 = demo_item->y;
  simple->bounds.x2 = demo_item->x + demo_item->width;
  simple->bounds.y2 = demo_item->y + demo_item->height;
}


/* The paint method. This should draw the item on the given cairo_t, using
   the item's own coordinate space. */
static void
goo_demo_item_paint (GooCanvasItemSimple   *simple,
		     cairo_t               *cr,
		     const GooCanvasBounds *bounds)
{
  GooDemoItem *demo_item = (GooDemoItem*) simple;

  cairo_move_to (cr, demo_item->x, demo_item->y);
  cairo_line_to (cr, demo_item->x, demo_item->y + demo_item->height);
  cairo_line_to (cr, demo_item->x + demo_item->width,
		 demo_item->y + demo_item->height);
  cairo_line_to (cr, demo_item->x + demo_item->width, demo_item->y);
  cairo_close_path (cr);
  goo_canvas_style_set_fill_options (simple->simple_data->style, cr);
  cairo_fill (cr);
}


/* Hit detection. This should check if the given coordinate (in the item's
   coordinate space) is within the item. If it is it should return TRUE,
   otherwise it should return FALSE. */
static gboolean
goo_demo_item_is_item_at (GooCanvasItemSimple *simple,
			  gdouble              x,
			  gdouble              y,
			  cairo_t             *cr,
			  gboolean             is_pointer_event)
{
  GooDemoItem *demo_item = (GooDemoItem*) simple;

  if (x < demo_item->x || (x > demo_item->x + demo_item->width)
      || y < demo_item->y || (y > demo_item->y + demo_item->height))
    return FALSE;

  return TRUE;
}


/* The class initialization function. Here we set the class' update(), paint()
   and is_item_at() methods. */
static void
goo_demo_item_class_init (GooDemoItemClass *klass)
{
  GooCanvasItemSimpleClass *simple_class = (GooCanvasItemSimpleClass*) klass;

  simple_class->simple_update        = goo_demo_item_update;
  simple_class->simple_paint         = goo_demo_item_paint;
  simple_class->simple_is_item_at    = goo_demo_item_is_item_at;
}


