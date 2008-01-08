/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasrect.h - rectangle item.
 */
#ifndef __GOO_CANVAS_RECT_H__
#define __GOO_CANVAS_RECT_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"

G_BEGIN_DECLS


/* This is the data used by both model and view classes. */
typedef struct _GooCanvasRectData   GooCanvasRectData;
struct _GooCanvasRectData
{
  gdouble x, y, width, height, radius_x, radius_y;
};


#define GOO_TYPE_CANVAS_RECT            (goo_canvas_rect_get_type ())
#define GOO_CANVAS_RECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_RECT, GooCanvasRect))
#define GOO_CANVAS_RECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_RECT, GooCanvasRectClass))
#define GOO_IS_CANVAS_RECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_RECT))
#define GOO_IS_CANVAS_RECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_RECT))
#define GOO_CANVAS_RECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_RECT, GooCanvasRectClass))


typedef struct _GooCanvasRect       GooCanvasRect;
typedef struct _GooCanvasRectClass  GooCanvasRectClass;

/**
 * GooCanvasRect
 *
 * The #GooCanvasRect-struct struct contains private data only.
 */
struct _GooCanvasRect
{
  GooCanvasItemSimple parent;

  GooCanvasRectData *rect_data;
};

struct _GooCanvasRectClass
{
  GooCanvasItemSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_rect_get_type  (void) G_GNUC_CONST;

GooCanvasItem*      goo_canvas_rect_new       (GooCanvasItem      *parent,
					       gdouble             x,
					       gdouble             y,
					       gdouble             width,
					       gdouble             height,
					       ...);



#define GOO_TYPE_CANVAS_RECT_MODEL            (goo_canvas_rect_model_get_type ())
#define GOO_CANVAS_RECT_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_RECT_MODEL, GooCanvasRectModel))
#define GOO_CANVAS_RECT_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_RECT_MODEL, GooCanvasRectModelClass))
#define GOO_IS_CANVAS_RECT_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_RECT_MODEL))
#define GOO_IS_CANVAS_RECT_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_RECT_MODEL))
#define GOO_CANVAS_RECT_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_RECT_MODEL, GooCanvasRectModelClass))


typedef struct _GooCanvasRectModel       GooCanvasRectModel;
typedef struct _GooCanvasRectModelClass  GooCanvasRectModelClass;

/**
 * GooCanvasRectModel
 *
 * The #GooCanvasRectModel-struct struct contains private data only.
 */
struct _GooCanvasRectModel
{
  GooCanvasItemModelSimple parent_object;

  GooCanvasRectData rect_data;
};

struct _GooCanvasRectModelClass
{
  GooCanvasItemModelSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_rect_model_get_type  (void) G_GNUC_CONST;

GooCanvasItemModel* goo_canvas_rect_model_new (GooCanvasItemModel *parent,
					       gdouble             x,
					       gdouble             y,
					       gdouble             width,
					       gdouble             height,
					       ...);


G_END_DECLS

#endif /* __GOO_CANVAS_RECT_H__ */
