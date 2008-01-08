/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasimage.h - image item.
 */
#ifndef __GOO_CANVAS_IMAGE_H__
#define __GOO_CANVAS_IMAGE_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"

G_BEGIN_DECLS


/* This is the data used by both model and view classes. */
typedef struct _GooCanvasImageData   GooCanvasImageData;
struct _GooCanvasImageData
{
  cairo_pattern_t *pattern;

  gdouble x, y, width, height;
};


#define GOO_TYPE_CANVAS_IMAGE            (goo_canvas_image_get_type ())
#define GOO_CANVAS_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_IMAGE, GooCanvasImage))
#define GOO_CANVAS_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_IMAGE, GooCanvasImageClass))
#define GOO_IS_CANVAS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_IMAGE))
#define GOO_IS_CANVAS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_IMAGE))
#define GOO_CANVAS_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_IMAGE, GooCanvasImageClass))


typedef struct _GooCanvasImage       GooCanvasImage;
typedef struct _GooCanvasImageClass  GooCanvasImageClass;

/**
 * GooCanvasImage
 *
 * The #GooCanvasImage-struct struct contains private data only.
 */
struct _GooCanvasImage
{
  GooCanvasItemSimple parent_object;

  GooCanvasImageData *image_data;
};

struct _GooCanvasImageClass
{
  GooCanvasItemSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_image_get_type  (void) G_GNUC_CONST;

GooCanvasItem*      goo_canvas_image_new       (GooCanvasItem      *parent,
						GdkPixbuf          *pixbuf,
						gdouble             x,
						gdouble             y,
						...);



#define GOO_TYPE_CANVAS_IMAGE_MODEL            (goo_canvas_image_model_get_type ())
#define GOO_CANVAS_IMAGE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_IMAGE_MODEL, GooCanvasImageModel))
#define GOO_CANVAS_IMAGE_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_IMAGE_MODEL, GooCanvasImageModelClass))
#define GOO_IS_CANVAS_IMAGE_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_IMAGE_MODEL))
#define GOO_IS_CANVAS_IMAGE_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_IMAGE_MODEL))
#define GOO_CANVAS_IMAGE_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_IMAGE_MODEL, GooCanvasImageModelClass))


typedef struct _GooCanvasImageModel       GooCanvasImageModel;
typedef struct _GooCanvasImageModelClass  GooCanvasImageModelClass;

/**
 * GooCanvasImageModel
 *
 * The #GooCanvasImageModel-struct struct contains private data only.
 */
struct _GooCanvasImageModel
{
  GooCanvasItemModelSimple parent_object;

  GooCanvasImageData image_data;
};

struct _GooCanvasImageModelClass
{
  GooCanvasItemModelSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_image_model_get_type  (void) G_GNUC_CONST;

GooCanvasItemModel* goo_canvas_image_model_new (GooCanvasItemModel *parent,
						GdkPixbuf          *pixbuf,
						gdouble             x,
						gdouble             y,
						...);


G_END_DECLS

#endif /* __GOO_CANVAS_IMAGE_H__ */
