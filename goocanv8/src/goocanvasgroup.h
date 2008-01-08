/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasgroup.h - group item.
 */
#ifndef __GOO_CANVAS_GROUP_H__
#define __GOO_CANVAS_GROUP_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"
#include "goocanvasutils.h"

G_BEGIN_DECLS


#define GOO_TYPE_CANVAS_GROUP            (goo_canvas_group_get_type ())
#define GOO_CANVAS_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_GROUP, GooCanvasGroup))
#define GOO_CANVAS_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_GROUP, GooCanvasGroupClass))
#define GOO_IS_CANVAS_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_GROUP))
#define GOO_IS_CANVAS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_GROUP))
#define GOO_CANVAS_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_GROUP, GooCanvasGroupClass))


typedef struct _GooCanvasGroup            GooCanvasGroup;
typedef struct _GooCanvasGroupClass       GooCanvasGroupClass;

typedef struct _GooCanvasGroupModel       GooCanvasGroupModel;
typedef struct _GooCanvasGroupModelClass  GooCanvasGroupModelClass;

/**
 * GooCanvasGroup
 *
 * The #GooCanvasGroup-struct struct contains private data only.
 */
struct _GooCanvasGroup
{
  GooCanvasItemSimple parent_object;

  /* An array of pointers to GooCanvasItems. The first element is at the
     bottom of the display stack and the last element is at the top. */
  GPtrArray *items;
};

struct _GooCanvasGroupClass
{
  GooCanvasItemSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType          goo_canvas_group_get_type    (void) G_GNUC_CONST;
GooCanvasItem* goo_canvas_group_new         (GooCanvasItem  *parent,
					     ...);



#define GOO_TYPE_CANVAS_GROUP_MODEL            (goo_canvas_group_model_get_type ())
#define GOO_CANVAS_GROUP_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_GROUP_MODEL, GooCanvasGroupModel))
#define GOO_CANVAS_GROUP_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_GROUP_MODEL, GooCanvasGroupModelClass))
#define GOO_IS_CANVAS_GROUP_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_GROUP_MODEL))
#define GOO_IS_CANVAS_GROUP_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_GROUP_MODEL))
#define GOO_CANVAS_GROUP_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_GROUP_MODEL, GooCanvasGroupModelClass))



/**
 * GooCanvasGroupModel
 *
 * The #GooCanvasGroupModel-struct struct contains private data only.
 */
struct _GooCanvasGroupModel
{
  GooCanvasItemModelSimple parent_object;

  /* An array of pointers to GooCanvasItemModels. The first element is at the
     bottom of the display stack and the last element is at the top. */
  GPtrArray *children;
};

struct _GooCanvasGroupModelClass
{
  GooCanvasItemModelSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_group_model_get_type (void) G_GNUC_CONST;
GooCanvasItemModel* goo_canvas_group_model_new      (GooCanvasItemModel  *parent,
						     ...);


G_END_DECLS

#endif /* __GOO_CANVAS_GROUP_H__ */
