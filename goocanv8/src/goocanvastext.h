/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvastext.h - text item.
 */
#ifndef __GOO_CANVAS_TEXT_H__
#define __GOO_CANVAS_TEXT_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"

G_BEGIN_DECLS


/* This is the data used by both model and view classes. */
typedef struct _GooCanvasTextData   GooCanvasTextData;
struct _GooCanvasTextData
{
  gchar *text;
  gdouble x, y, width;
  guint use_markup		: 1;
  guint anchor			: 5;	/* GtkAnchorType */
  guint alignment		: 3;	/* PangoAlignment */
  guint ellipsize		: 3;	/* PangoEllipsizeMode */
};


#define GOO_TYPE_CANVAS_TEXT            (goo_canvas_text_get_type ())
#define GOO_CANVAS_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_TEXT, GooCanvasText))
#define GOO_CANVAS_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_TEXT, GooCanvasTextClass))
#define GOO_IS_CANVAS_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_TEXT))
#define GOO_IS_CANVAS_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_TEXT))
#define GOO_CANVAS_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_TEXT, GooCanvasTextClass))


typedef struct _GooCanvasText       GooCanvasText;
typedef struct _GooCanvasTextClass  GooCanvasTextClass;

/**
 * GooCanvasText
 *
 * The #GooCanvasText-struct struct contains private data only.
 */
struct _GooCanvasText
{
  GooCanvasItemSimple parent;

  GooCanvasTextData *text_data;
  gdouble layout_width;
};

struct _GooCanvasTextClass
{
  GooCanvasItemSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_text_get_type  (void) G_GNUC_CONST;

GooCanvasItem*      goo_canvas_text_new       (GooCanvasItem      *parent,
					       const char         *string,
					       gdouble             x,
					       gdouble             y,
					       gdouble             width,
					       GtkAnchorType       anchor,
					       ...);



#define GOO_TYPE_CANVAS_TEXT_MODEL            (goo_canvas_text_model_get_type ())
#define GOO_CANVAS_TEXT_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_TEXT_MODEL, GooCanvasTextModel))
#define GOO_CANVAS_TEXT_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_TEXT_MODEL, GooCanvasTextModelClass))
#define GOO_IS_CANVAS_TEXT_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_TEXT_MODEL))
#define GOO_IS_CANVAS_TEXT_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_TEXT_MODEL))
#define GOO_CANVAS_TEXT_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_TEXT_MODEL, GooCanvasTextModelClass))


typedef struct _GooCanvasTextModel       GooCanvasTextModel;
typedef struct _GooCanvasTextModelClass  GooCanvasTextModelClass;

/**
 * GooCanvasTextModel
 *
 * The #GooCanvasTextModel-struct struct contains private data only.
 */
struct _GooCanvasTextModel
{
  GooCanvasItemModelSimple parent_object;

  GooCanvasTextData text_data;
};

struct _GooCanvasTextModelClass
{
  GooCanvasItemModelSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_text_model_get_type  (void) G_GNUC_CONST;

GooCanvasItemModel* goo_canvas_text_model_new (GooCanvasItemModel *parent,
					       const char         *string,
					       gdouble             x,
					       gdouble             y,
					       gdouble             width,
					       GtkAnchorType       anchor,
					       ...);


G_END_DECLS

#endif /* __GOO_CANVAS_TEXT_H__ */
