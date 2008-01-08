/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasitemmodel.h - interface for canvas item models.
 */
#ifndef __GOO_CANVAS_ITEM_MODEL_H__
#define __GOO_CANVAS_ITEM_MODEL_H__

#include <gtk/gtk.h>
#include "goocanvasitem.h"

G_BEGIN_DECLS


#define GOO_TYPE_CANVAS_ITEM_MODEL            (goo_canvas_item_model_get_type ())
#define GOO_CANVAS_ITEM_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_ITEM_MODEL, GooCanvasItemModel))
#define GOO_CANVAS_ITEM_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_ITEM_MODEL, GooCanvasItemModelClass))
#define GOO_IS_CANVAS_ITEM_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_ITEM_MODEL))
#define GOO_CANVAS_ITEM_MODEL_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GOO_TYPE_CANVAS_ITEM_MODEL, GooCanvasItemModelIface))


/**
 * GooCanvasItemModel
 *
 * #GooCanvasItemModel is a typedef used for objects that implement the
 * #GooCanvasItemModel interface.
 *
 * (There is no actual #GooCanvasItemModel struct, since it is only an interface.
 * But using '#GooCanvasItemModel' is more helpful than using '#GObject'.)
 */
/* The typedef is in goocanvasitem.h */
/*typedef struct _GooCanvasItemModel       GooCanvasItemModel;*/


/**
 * GooCanvasItemModelIface
 * @get_n_children: returns the number of children of the model.
 * @get_child: returns the child at the given index.
 * @add_child: adds a child.
 * @move_child: moves a child up or down the stacking order.
 * @remove_child: removes a child.
 * @get_child_property: gets a child property of a given child model,
 *  e.g. the "row" or "column" property of a model in a #GooCanvasTableModel.
 * @set_child_property: sets a child property for a given child model.
 * @get_parent: gets the model's parent.
 * @set_parent: sets the model's parent.
 * @create_item: creates a default canvas item to view the model.
 * @get_transform: gets the model's transformation matrix.
 * @set_transform: sets the model's transformation matrix.
 * @get_style: gets the model's style.
 * @set_style: sets the model's style.
 * @child_added: signal emitted when a child is added.
 * @child_moved: signal emitted when a child is moved in the stacking order.
 * @child_removed: signal emitted when a child is removed.
 * @changed: signal emitted when the model has changed.
 * @child_notify: signal emitted when a child property has changed.
 *
 * #GooCanvasItemModelIFace holds the virtual methods that make up the
 * #GooCanvasItemModel interface.
 *
 * Simple item models only need to implement the get_parent(), set_parent()
 * and create_item() methods.
 *
 * Items that support transforms should also implement get_transform() and
 * set_transform(). Items that support styles should implement get_style()
 * and set_style().
 *
 * Container items must implement get_n_children() and get_child().
 * Containers that support dynamic changes to their children should implement
 * add_child(), move_child() and remove_child().
 * Layout containers like #GooCanvasTable may implement
 * get_child_property() and set_child_property().
 */
typedef struct _GooCanvasItemModelIface  GooCanvasItemModelIface;

struct _GooCanvasItemModelIface
{
  /*< private >*/
  GTypeInterface base_iface;

  /*< public >*/
  /* Virtual methods that group models must implement. */
  gint		       (* get_n_children)		(GooCanvasItemModel	*model);
  GooCanvasItemModel*  (* get_child)			(GooCanvasItemModel	*model,
							 gint			 child_num);

  /* Virtual methods that group models may implement. */
  void                 (* add_child)			(GooCanvasItemModel	*model,
							 GooCanvasItemModel	*child,
							 gint			 position);
  void                 (* move_child)			(GooCanvasItemModel	*model,
							 gint			 old_position,
							 gint			 new_position);
  void                 (* remove_child)			(GooCanvasItemModel	*model,
							 gint			 child_num);
  void                 (* get_child_property)		(GooCanvasItemModel	*model,
							 GooCanvasItemModel	*child,
							 guint			 property_id,
							 GValue			*value,
							 GParamSpec		*pspec);
  void                 (* set_child_property)		(GooCanvasItemModel	*item,
							 GooCanvasItemModel	*child,
							 guint			 property_id,
							 const GValue		*value,
							 GParamSpec		*pspec);

  /* Virtual methods that all item models must implement. */
  GooCanvasItemModel*  (* get_parent)			(GooCanvasItemModel	*model);
  void                 (* set_parent)			(GooCanvasItemModel	*model,
							 GooCanvasItemModel	*parent);

  GooCanvasItem*       (* create_item)			(GooCanvasItemModel	*model,
							 GooCanvas		*canvas);

  /* Virtual methods that all item models may implement. */
  gboolean             (* get_transform)		(GooCanvasItemModel	*model,
							 cairo_matrix_t         *transform);
  void                 (* set_transform)		(GooCanvasItemModel	*model,
							 const cairo_matrix_t	*transform);
  GooCanvasStyle*      (* get_style)			(GooCanvasItemModel	*model);
  void                 (* set_style)			(GooCanvasItemModel	*model,
							 GooCanvasStyle		*style);

  /* Signals. */
  void                 (* child_added)			(GooCanvasItemModel	*model,
							 gint			 child_num);
  void                 (* child_moved)			(GooCanvasItemModel	*model,
							 gint			 old_child_num,
							 gint			 new_child_num);
  void                 (* child_removed)		(GooCanvasItemModel	*model,
							 gint			 child_num);
  void                 (* changed)			(GooCanvasItemModel	*model,
							 gboolean		 recompute_bounds);
  void                 (* child_notify)			(GooCanvasItemModel	*model,
							 GParamSpec		*pspec);

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
  void (*_goo_canvas_reserved5) (void);
  void (*_goo_canvas_reserved6) (void);
  void (*_goo_canvas_reserved7) (void);
  void (*_goo_canvas_reserved8) (void);
};


GType               goo_canvas_item_model_get_type       (void) G_GNUC_CONST;


/*
 * Group functions - these should only be called on container models.
 */
gint                goo_canvas_item_model_get_n_children (GooCanvasItemModel *model);
GooCanvasItemModel* goo_canvas_item_model_get_child      (GooCanvasItemModel *model,
							  gint                child_num);
void                goo_canvas_item_model_add_child      (GooCanvasItemModel *model,
							  GooCanvasItemModel *child,
							  gint                position);
void                goo_canvas_item_model_move_child     (GooCanvasItemModel *model,
							  gint                old_position,
							  gint                new_position);
void                goo_canvas_item_model_remove_child   (GooCanvasItemModel *model,
							  gint                child_num);
gint                goo_canvas_item_model_find_child     (GooCanvasItemModel *model,
							  GooCanvasItemModel *child);

void     goo_canvas_item_model_get_child_properties	 (GooCanvasItemModel   *model,
							  GooCanvasItemModel   *child,
							  ...) G_GNUC_NULL_TERMINATED;
void     goo_canvas_item_model_set_child_properties	 (GooCanvasItemModel   *model,
							  GooCanvasItemModel   *child,
							  ...) G_GNUC_NULL_TERMINATED;
void     goo_canvas_item_model_get_child_properties_valist (GooCanvasItemModel   *model,
							    GooCanvasItemModel   *child,
							    va_list	         var_args);
void     goo_canvas_item_model_set_child_properties_valist (GooCanvasItemModel   *model,
							    GooCanvasItemModel   *child,
							    va_list	         var_args);


/*
 * Model functions - these are safe to call on all models.
 */
GooCanvasItemModel* goo_canvas_item_model_get_parent     (GooCanvasItemModel *model);
void                goo_canvas_item_model_set_parent	 (GooCanvasItemModel *model,
							  GooCanvasItemModel *parent);
void                goo_canvas_item_model_remove         (GooCanvasItemModel *model);
gboolean            goo_canvas_item_model_is_container   (GooCanvasItemModel *model);

void                goo_canvas_item_model_raise          (GooCanvasItemModel *model,
							  GooCanvasItemModel *above);
void                goo_canvas_item_model_lower          (GooCanvasItemModel *model,
							  GooCanvasItemModel *below);

gboolean            goo_canvas_item_model_get_transform  (GooCanvasItemModel *model,
							  cairo_matrix_t     *transform);
void                goo_canvas_item_model_set_transform  (GooCanvasItemModel   *model,
							  const cairo_matrix_t *transform);
void                goo_canvas_item_model_set_simple_transform (GooCanvasItemModel *model,
								gdouble             x,
								gdouble             y,
								gdouble             scale,
								gdouble             rotation);

void                goo_canvas_item_model_translate      (GooCanvasItemModel *model,
							  gdouble             tx,
							  gdouble             ty);
void                goo_canvas_item_model_scale          (GooCanvasItemModel *model,
							  gdouble             sx,
							  gdouble             sy);
void                goo_canvas_item_model_rotate         (GooCanvasItemModel *model,
							  gdouble             degrees,
							  gdouble             cx,
							  gdouble             cy);
void                goo_canvas_item_model_skew_x         (GooCanvasItemModel *model,
							  gdouble             degrees,
							  gdouble             cx,
							  gdouble             cy);
void                goo_canvas_item_model_skew_y         (GooCanvasItemModel *model,
							  gdouble             degrees,
							  gdouble             cx,
							  gdouble             cy);

GooCanvasStyle*     goo_canvas_item_model_get_style      (GooCanvasItemModel *model);
void                goo_canvas_item_model_set_style      (GooCanvasItemModel *model,
							  GooCanvasStyle  *style);

void                goo_canvas_item_model_animate        (GooCanvasItemModel *model,
							  gdouble             x,
							  gdouble             y,
							  gdouble             scale,
							  gdouble             degrees,
							  gboolean            absolute,
							  gint                duration,
							  gint                step_time,
							  GooCanvasAnimateType type);
void                goo_canvas_item_model_stop_animation (GooCanvasItemModel *model);


/*
 * Functions to support child properties when implementing new canvas items.
 */
void         goo_canvas_item_model_class_install_child_property (GObjectClass *mclass,
								 guint	 property_id,
								 GParamSpec	*pspec);
GParamSpec*  goo_canvas_item_model_class_find_child_property	(GObjectClass	*mclass,
								 const gchar	*property_name);
GParamSpec** goo_canvas_item_model_class_list_child_properties  (GObjectClass	*mclass,
								 guint	*n_properties);



G_END_DECLS

#endif /* __GOO_CANVAS_ITEM_MODEL_H__ */
