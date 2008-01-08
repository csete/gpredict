/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasitemsimple.h - abstract base class for simple items.
 */
#ifndef __GOO_CANVAS_ITEM_SIMPLE_H__
#define __GOO_CANVAS_ITEM_SIMPLE_H__

#include <gtk/gtk.h>
#include "goocanvasitem.h"
#include "goocanvasitemmodel.h"
#include "goocanvasstyle.h"
#include "goocanvasutils.h"

G_BEGIN_DECLS


/**
 * GooCanvasItemSimpleData
 * @style: the style to draw with.
 * @transform: the transformation matrix of the item, or %NULL.
 * @clip_path_commands: an array of #GooCanvasPathCommand specifying the clip
 *  path of the item, or %NULL.
 * @visibility_threshold: the threshold scale setting at which to show the item
 *  (if the @visibility setting is set to %VISIBLE_ABOVE_THRESHOLD).
 * @visibility: the #GooCanvasItemVisibility setting specifying whether the
 *  item is visible, invisible, or visible above a given canvas scale setting.
 * @pointer_events: the #GooCanvasPointerEvents setting specifying the events
 *  the item should receive.
 * @can_focus: if the item can take the keyboard focus.
 * @own_style: if the item has its own style, rather than using its parent's.
 * @clip_fill_rule: the #cairo_fill_rule_t setting specifying the fill rule
 *  used for the clip path.
 *
 * This is the data common to both the model and view classes.
 */
typedef struct _GooCanvasItemSimpleData   GooCanvasItemSimpleData;
struct _GooCanvasItemSimpleData
{
  GooCanvasStyle *style;
  cairo_matrix_t *transform;
  GArray *clip_path_commands;

  /*< private >*/
  /* We will store tooltips here in future. */
  gchar *tooltip;

  /*< public >*/
  gdouble visibility_threshold;
  guint visibility			: 2;
  guint pointer_events			: 4;
  guint can_focus                       : 1;
  guint own_style                       : 1;
  guint clip_fill_rule			: 4;

  /*< private >*/
  /* We might use this in future for a cache setting - never/always/visible. */
  guint cache_setting			: 3;
  /* We might need this for tooltips in future. */
  guint has_tooltip			: 1;
};


#define GOO_TYPE_CANVAS_ITEM_SIMPLE            (goo_canvas_item_simple_get_type ())
#define GOO_CANVAS_ITEM_SIMPLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_ITEM_SIMPLE, GooCanvasItemSimple))
#define GOO_CANVAS_ITEM_SIMPLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_ITEM_SIMPLE, GooCanvasItemSimpleClass))
#define GOO_IS_CANVAS_ITEM_SIMPLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_ITEM_SIMPLE))
#define GOO_IS_CANVAS_ITEM_SIMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_ITEM_SIMPLE))
#define GOO_CANVAS_ITEM_SIMPLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_ITEM_SIMPLE, GooCanvasItemSimpleClass))


typedef struct _GooCanvasItemSimple       GooCanvasItemSimple;
typedef struct _GooCanvasItemSimpleClass  GooCanvasItemSimpleClass;

typedef struct _GooCanvasItemModelSimple       GooCanvasItemModelSimple;

/**
 * GooCanvasItemSimple
 * @canvas: the canvas.
 * @parent: the parent item.
 * @model: the item's model, if it has one.
 * @simple_data: data that is common to both the model and view classes. If
 *  the canvas item has a model, this will point to the model's
 *  #GooCanvasItemSimpleData, otherwise the canvas item will have its own
 *  #GooCanvasItemSimpleData.
 * @bounds: the bounds of the item, in device space.
 * @need_update: if the item needs to recompute its bounds and redraw.
 * @need_entire_subtree_update: if all descendants need to be updated.
 *
 * The #GooCanvasItemSimple-struct struct contains the basic data needed to
 * implement canvas items.
 */
struct _GooCanvasItemSimple
{
  /* <private> */
  GObject parent_object;

  /* <public> */
  GooCanvas *canvas;
  GooCanvasItem *parent;
  GooCanvasItemModelSimple *model;
  GooCanvasItemSimpleData *simple_data;
  GooCanvasBounds bounds;
  guint	need_update			: 1;
  guint need_entire_subtree_update      : 1;

  /* <private> */
  /* We might use this in future for things like a cache. */
  gpointer priv;
};

/**
 * GooCanvasItemSimpleClass
 * @simple_create_path: simple subclasses that draw basic shapes and paths only
 *  need to override this one method. It creates the path for the item.
 *  All updating, painting and hit-testing is provided automatically by the
 *  #GooCanvasItemSimple class. (This method is used by the builtin
 *  #GooCanvasEllipse, #GooCanvasRect and #GooCanvasPath items.)
 * @simple_update: subclasses should override this to calculate their new
 *  bounds, in user space.
 * @simple_paint: subclasses should override this to paint their item.
 * @simple_is_item_at: subclasses should override this to do hit-testing.
 *
 * The #GooCanvasItemSimpleClass-struct struct contains several methods that
 * subclasses can override.
 *
 * Simple items need only implement the create_path() method. More complex
 * items must override the update(), paint() and is_item_at() methods.
 */
struct _GooCanvasItemSimpleClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  void		 (* simple_create_path)	(GooCanvasItemSimple   *simple,
					 cairo_t               *cr);

  void           (* simple_update)	(GooCanvasItemSimple   *simple,
					 cairo_t               *cr);
  void           (* simple_paint)	(GooCanvasItemSimple   *simple,
					 cairo_t               *cr,
					 const GooCanvasBounds *bounds);
  gboolean       (* simple_is_item_at)  (GooCanvasItemSimple   *simple,
					 gdouble                x,
					 gdouble                y,
					 cairo_t               *cr,
					 gboolean               is_pointer_event);

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType    goo_canvas_item_simple_get_type		(void) G_GNUC_CONST;


void     goo_canvas_item_simple_get_path_bounds		(GooCanvasItemSimple	*item,
							 cairo_t		*cr,
							 GooCanvasBounds	*bounds);
void     goo_canvas_item_simple_user_bounds_to_device	(GooCanvasItemSimple	*item,
							 cairo_t		*cr,
							 GooCanvasBounds	*bounds);
void     goo_canvas_item_simple_user_bounds_to_parent	(GooCanvasItemSimple	*item,
							 cairo_t		*cr,
							 GooCanvasBounds	*bounds);
gboolean goo_canvas_item_simple_check_in_path		(GooCanvasItemSimple	*item,
							 gdouble		 x,
							 gdouble		 y,
							 cairo_t		*cr,
							 GooCanvasPointerEvents  pointer_events);
void     goo_canvas_item_simple_paint_path		(GooCanvasItemSimple	*item,
							 cairo_t		*cr);

void     goo_canvas_item_simple_changed			(GooCanvasItemSimple	*item,
							 gboolean		 recompute_bounds);
void     goo_canvas_item_simple_check_style		(GooCanvasItemSimple	*item);
void	 goo_canvas_item_simple_set_model		(GooCanvasItemSimple	*item,
							 GooCanvasItemModel	*model);





#define GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE            (goo_canvas_item_model_simple_get_type ())
#define GOO_CANVAS_ITEM_MODEL_SIMPLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE, GooCanvasItemModelSimple))
#define GOO_CANVAS_ITEM_MODEL_SIMPLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE, GooCanvasItemModelSimpleClass))
#define GOO_IS_CANVAS_ITEM_MODEL_SIMPLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE))
#define GOO_IS_CANVAS_ITEM_MODEL_SIMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE))
#define GOO_CANVAS_ITEM_MODEL_SIMPLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE, GooCanvasItemModelSimpleClass))


typedef struct _GooCanvasItemModelSimpleClass  GooCanvasItemModelSimpleClass;

/**
 * GooCanvasItemModelSimple
 * @parent: the parent model.
 * @simple_data: data used by the canvas item for viewing the model.
 *
 * The #GooCanvasItemModelSimple-struct struct contains the basic data needed
 * to implement canvas item models.
 */
struct _GooCanvasItemModelSimple
{
  GObject parent_object;

  /*< public >*/
  GooCanvasItemModel *parent;
  GooCanvasItemSimpleData simple_data;

  /*< private >*/

  /* The title and description of the item for accessibility. */
  gchar *title;
  gchar *description;
};


struct _GooCanvasItemModelSimpleClass
{
  GObjectClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType    goo_canvas_item_model_simple_get_type  (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GOO_CANVAS_ITEM_SIMPLE_H__ */
