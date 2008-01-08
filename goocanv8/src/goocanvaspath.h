/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvaspath.h - a path item, very similar to the SVG path element.
 */
#ifndef __GOO_CANVAS_PATH_H__
#define __GOO_CANVAS_PATH_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"

G_BEGIN_DECLS


#define GOO_TYPE_CANVAS_PATH            (goo_canvas_path_get_type ())
#define GOO_CANVAS_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_PATH, GooCanvasPath))
#define GOO_CANVAS_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_PATH, GooCanvasPathClass))
#define GOO_IS_CANVAS_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_PATH))
#define GOO_IS_CANVAS_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_PATH))
#define GOO_CANVAS_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_PATH, GooCanvasPathClass))


typedef struct _GooCanvasPath       GooCanvasPath;
typedef struct _GooCanvasPathClass  GooCanvasPathClass;

/**
 * GooCanvasPath
 *
 * The #GooCanvasPath-struct struct contains private data only.
 */
struct _GooCanvasPath
{
  GooCanvasItemSimple parent;

  /* An array of GooCanvasPathCommand. */
  GArray *path_commands;
};

struct _GooCanvasPathClass
{
  GooCanvasItemSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_path_get_type  (void) G_GNUC_CONST;

GooCanvasItem*      goo_canvas_path_new       (GooCanvasItem      *parent,
					       const gchar        *path_data,
					       ...);



#define GOO_TYPE_CANVAS_PATH_MODEL            (goo_canvas_path_model_get_type ())
#define GOO_CANVAS_PATH_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_PATH_MODEL, GooCanvasPathModel))
#define GOO_CANVAS_PATH_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_PATH_MODEL, GooCanvasPathModelClass))
#define GOO_IS_CANVAS_PATH_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_PATH_MODEL))
#define GOO_IS_CANVAS_PATH_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_PATH_MODEL))
#define GOO_CANVAS_PATH_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_PATH_MODEL, GooCanvasPathModelClass))


typedef struct _GooCanvasPathModel       GooCanvasPathModel;
typedef struct _GooCanvasPathModelClass  GooCanvasPathModelClass;

/**
 * GooCanvasPathModel
 *
 * The #GooCanvasPathModel-struct struct contains private data only.
 */
struct _GooCanvasPathModel
{
  GooCanvasItemModelSimple parent_object;

  /* An array of GooCanvasPathCommand. */
  GArray *path_commands;
};

struct _GooCanvasPathModelClass
{
  GooCanvasItemModelSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_path_model_get_type  (void) G_GNUC_CONST;

GooCanvasItemModel* goo_canvas_path_model_new (GooCanvasItemModel *parent,
					       const gchar        *path_data,
					       ...);


G_END_DECLS

#endif /* __GOO_CANVAS_PATH_H__ */
