/*
 * GooCanvas Demo. Copyright (C) 2006 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * demo-item.c - a simple demo item.
 */
#ifndef __GOO_DEMO_ITEM_H__
#define __GOO_DEMO_ITEM_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"

G_BEGIN_DECLS


#define GOO_TYPE_DEMO_ITEM            (goo_demo_item_get_type ())
#define GOO_DEMO_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_DEMO_ITEM, GooDemoItem))
#define GOO_DEMO_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_DEMO_ITEM, GooDemoItemClass))
#define GOO_IS_DEMO_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_DEMO_ITEM))
#define GOO_IS_DEMO_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_DEMO_ITEM))
#define GOO_DEMO_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_DEMO_ITEM, GooDemoItemClass))


typedef struct _GooDemoItem       GooDemoItem;
typedef struct _GooDemoItemClass  GooDemoItemClass;

struct _GooDemoItem
{
  GooCanvasItemSimple parent_object;

  gdouble x, y, width, height;
};

struct _GooDemoItemClass
{
  GooCanvasItemSimpleClass parent_class;
};


GType               goo_demo_item_get_type  (void) G_GNUC_CONST;

GooCanvasItem*      goo_demo_item_new       (GooCanvasItem      *parent,
					     gdouble             x,
					     gdouble             y,
					     gdouble             width,
					     gdouble             height,
					     ...);


G_END_DECLS

#endif /* __GOO_DEMO_ITEM_H__ */
