/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasitem.c - interface for canvas items & groups.
 */

/**
 * SECTION:goocanvasitemmodel
 * @Title: GooCanvasItemModel
 * @Short_Description: the interface for canvas item models.
 *
 * #GooCanvasItemModel defines the interface that models for canvas items must
 * implement, and contains methods for operating on canvas item models.
 */
#include <config.h>
#include <math.h>
#include <glib/gi18n-lib.h>
#include <gobject/gobjectnotifyqueue.c>
#include <gobject/gvaluecollector.h>
#include <gtk/gtk.h>
#include "goocanvasprivate.h"
#include <goocanvasenumtypes.h>
#include "goocanvasitemmodel.h"
#include "goocanvasutils.h"
#include "goocanvasmarshal.h"


static GParamSpecPool       *_goo_canvas_item_model_child_property_pool = NULL;
static GObjectNotifyContext *_goo_canvas_item_model_child_property_notify_context = NULL;
static const char *animation_key = "GooCanvasItemAnimation";

enum {
  CHILD_ADDED,
  CHILD_MOVED,
  CHILD_REMOVED,
  CHANGED,

  CHILD_NOTIFY,

  LAST_SIGNAL
};

static guint item_model_signals[LAST_SIGNAL] = { 0 };

static void goo_canvas_item_model_base_init (gpointer g_class);
extern void _goo_canvas_style_init (void);


GType
goo_canvas_item_model_get_type (void)
{
  static GType item_model_type = 0;

  if (!item_model_type)
    {
      static const GTypeInfo item_model_info =
      {
        sizeof (GooCanvasItemModelIface),  /* class_size */
	goo_canvas_item_model_base_init,   /* base_init */
	NULL,			           /* base_finalize */
      };

      item_model_type = g_type_register_static (G_TYPE_INTERFACE,
						"GooCanvasItemModel",
						&item_model_info, 0);

      g_type_interface_add_prerequisite (item_model_type, G_TYPE_OBJECT);
    }

  return item_model_type;
}



static void
child_property_notify_dispatcher (GObject     *object,
				  guint        n_pspecs,
				  GParamSpec **pspecs)
{
  guint i;

  for (i = 0; i < n_pspecs; i++)
    g_signal_emit (object, item_model_signals[CHILD_NOTIFY],
		   g_quark_from_string (pspecs[i]->name), pspecs[i]);
}


static void
goo_canvas_item_model_base_init (gpointer g_iface)
{
  static GObjectNotifyContext cpn_context = { 0, NULL, NULL };
  static gboolean initialized = FALSE;
  
  if (!initialized)
    {
      GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);

      _goo_canvas_item_model_child_property_pool = g_param_spec_pool_new (TRUE);

      cpn_context.quark_notify_queue = g_quark_from_static_string ("GooCanvasItemModel-child-property-notify-queue");
      cpn_context.dispatcher = child_property_notify_dispatcher;
      _goo_canvas_item_model_child_property_notify_context = &cpn_context;

      /**
       * GooCanvasItemModel::child-added
       * @model: the item model that received the signal.
       * @child_num: the index of the new child.
       *
       * Emitted when a child has been added.
       */
      item_model_signals[CHILD_ADDED] =
	g_signal_new ("child-added",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemModelIface, child_added),
		      NULL, NULL,
		      goo_canvas_marshal_VOID__INT,
		      G_TYPE_NONE, 1,
		      G_TYPE_INT);

      /**
       * GooCanvasItemModel::child-moved
       * @model: the item model that received the signal.
       * @old_child_num: the old index of the child.
       * @new_child_num: the new index of the child.
       *
       * Emitted when a child has been moved in the stacking order.
       */
      item_model_signals[CHILD_MOVED] =
	g_signal_new ("child-moved",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemModelIface, child_moved),
		      NULL, NULL,
		      goo_canvas_marshal_VOID__INT_INT,
		      G_TYPE_NONE, 2,
		      G_TYPE_INT, G_TYPE_INT);

      /**
       * GooCanvasItemModel::child-removed
       * @model: the item model that received the signal.
       * @child_num: the index of the child that was removed.
       *
       * Emitted when a child has been removed.
       */
      item_model_signals[CHILD_REMOVED] =
	g_signal_new ("child-removed",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemModelIface, child_removed),
		      NULL, NULL,
		      goo_canvas_marshal_VOID__INT,
		      G_TYPE_NONE, 1,
		      G_TYPE_INT);

      /**
       * GooCanvasItemModel::changed
       * @model: the item model that received the signal.
       * @recompute_bounds: if the bounds of the item need to be recomputed.
       *
       * Emitted when the item model has been changed.
       */
      item_model_signals[CHANGED] =
	g_signal_new ("changed",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemModelIface, changed),
		      NULL, NULL,
		      goo_canvas_marshal_VOID__BOOLEAN,
		      G_TYPE_NONE, 1,
		      G_TYPE_BOOLEAN);

      /**
       * GooCanvasItemModel::child-notify
       * @item: the item model that received the signal.
       * @pspec: the #GParamSpec of the changed child property.
       *
       * Emitted for each child property that has changed.
       * The signal's detail holds the property name. 
       */
      item_model_signals[CHILD_NOTIFY] =
	g_signal_new ("child_notify",
		      iface_type,
		      G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS,
		      G_STRUCT_OFFSET (GooCanvasItemModelIface, child_notify),
		      NULL, NULL,
		      g_cclosure_marshal_VOID__PARAM,
		      G_TYPE_NONE, 1,
		      G_TYPE_PARAM);


      g_object_interface_install_property (g_iface,
					   g_param_spec_object ("parent",
								_("Parent"),
								_("The parent item model"),
								GOO_TYPE_CANVAS_ITEM_MODEL,
								G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
					   g_param_spec_enum ("visibility",
							      _("Visibility"),
							      _("When the canvas item is visible"),
							      GOO_TYPE_CANVAS_ITEM_VISIBILITY,
							      GOO_CANVAS_ITEM_VISIBLE,
							      G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
					   g_param_spec_double ("visibility-threshold",
								_("Visibility Threshold"),
								_("The scale threshold at which the item becomes visible"),
								0.0,
								G_MAXDOUBLE,
								0.0,
								G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
					   g_param_spec_boxed ("transform",
							       _("Transform"),
							       _("The transformation matrix of the item"),
							       GOO_TYPE_CAIRO_MATRIX,
							       G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
					   g_param_spec_flags ("pointer-events",
							       _("Pointer Events"),
							       _("Specifies when the item receives pointer events"),
							       GOO_TYPE_CANVAS_POINTER_EVENTS,
							       GOO_CANVAS_EVENTS_VISIBLE_PAINTED,
							       G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
					   g_param_spec_string ("title",
								_("Title"),
								_("A short context-rich description of the item for use by assistive technologies"),
								NULL,
								G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
					   g_param_spec_string ("description",
								_("Description"),
								_("A description of the item for use by assistive technologies"),
								NULL,
								G_PARAM_READWRITE));

      g_object_interface_install_property (g_iface,
					   g_param_spec_boolean ("can-focus",
								 _("Can Focus"),
								 _("If the item can take the keyboard focus"),
								 FALSE,
								 G_PARAM_READWRITE));

      _goo_canvas_style_init ();

      initialized = TRUE;
    }
}


/**
 * goo_canvas_item_model_add_child:
 * @model: an item model.
 * @child: the child to add.
 * @position: the position of the child, or -1 to place it last (at the top of
 *  the stacking order).
 * 
 * Adds a child at the given stack position.
 **/
void
goo_canvas_item_model_add_child      (GooCanvasItemModel  *model,
				      GooCanvasItemModel  *child,
				      gint                 position)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  g_return_if_fail (iface->add_child != NULL);

  iface->add_child (model, child, position);
}


/**
 * goo_canvas_item_model_move_child:
 * @model: an item model.
 * @old_position: the current position of the child.
 * @new_position: the new position of the child.
 * 
 * Moves a child to a new stack position.
 **/
void
goo_canvas_item_model_move_child     (GooCanvasItemModel  *model,
				      gint                 old_position,
				      gint                 new_position)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  g_return_if_fail (iface->move_child != NULL);

  iface->move_child (model, old_position, new_position);
}


/**
 * goo_canvas_item_model_remove_child:
 * @model: an item model.
 * @child_num: the position of the child to remove.
 * 
 * Removes the child at the given position.
 **/
void
goo_canvas_item_model_remove_child   (GooCanvasItemModel  *model,
				      gint                 child_num)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  g_return_if_fail (iface->remove_child != NULL);

  iface->remove_child (model, child_num);
}


/**
 * goo_canvas_item_model_find_child:
 * @model: an item model.
 * @child: the child to find.
 * 
 * Attempts to find the given child with the container's stack.
 * 
 * Returns: the position of the given @child, or -1 if it isn't found.
 **/
gint
goo_canvas_item_model_find_child     (GooCanvasItemModel *model,
				      GooCanvasItemModel *child)
{
  GooCanvasItemModel *item;
  int n_children, i;

  /* Find the current position of item and above. */
  n_children = goo_canvas_item_model_get_n_children (model);
  for (i = 0; i < n_children; i++)
    {
      item = goo_canvas_item_model_get_child (model, i);
      if (child == item)
	return i;
    }
  return -1;
}


/**
 * goo_canvas_item_model_is_container:
 * @model: an item model.
 * 
 * Tests to see if the given item model is a container.
 * 
 * Returns: %TRUE if the item model is a container.
 **/
gboolean
goo_canvas_item_model_is_container (GooCanvasItemModel       *model)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  return iface->get_n_children ? TRUE : FALSE;
}


/**
 * goo_canvas_item_model_get_n_children:
 * @model: an item model.
 * 
 * Gets the number of children of the container.
 * 
 * Returns: the number of children.
 **/
gint
goo_canvas_item_model_get_n_children (GooCanvasItemModel       *model)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  return iface->get_n_children ? iface->get_n_children (model) : 0;
}


/**
 * goo_canvas_item_model_get_child:
 * @model: an item model.
 * @child_num: the position of a child in the container's stack.
 * 
 * Gets the child at the given stack position.
 * 
 * Returns: the child at the given stack position.
 **/
GooCanvasItemModel*
goo_canvas_item_model_get_child (GooCanvasItemModel  *model,
				 gint                 child_num)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  return iface->get_child ? iface->get_child (model, child_num) : NULL;
}


/**
 * goo_canvas_item_model_get_parent:
 * @model: an item model.
 * 
 * Gets the parent of the given model.
 * 
 * Returns: the parent model, or %NULL if the model has no parent.
 **/
GooCanvasItemModel*
goo_canvas_item_model_get_parent  (GooCanvasItemModel *model)
{
  return GOO_CANVAS_ITEM_MODEL_GET_IFACE (model)->get_parent (model);
}


/**
 * goo_canvas_item_model_set_parent:
 * @model: an item model.
 * @parent: the new parent item model.
 * 
 * Sets the parent of the model.
 **/
void
goo_canvas_item_model_set_parent (GooCanvasItemModel *model,
				  GooCanvasItemModel *parent)
{
  GOO_CANVAS_ITEM_MODEL_GET_IFACE (model)->set_parent (model, parent);
}


/**
 * goo_canvas_item_model_remove:
 * @model: an item model.
 * 
 * Removes a model from its parent. If the model is in a canvas it will be
 * removed.
 *
 * This would normally also result in the model being freed.
 **/
void
goo_canvas_item_model_remove         (GooCanvasItemModel *model)
{
  GooCanvasItemModel *parent;
  gint child_num;

  parent = goo_canvas_item_model_get_parent (model);
  if (!parent)
    return;

  child_num = goo_canvas_item_model_find_child (parent, model);
  if (child_num == -1)
    return;

  goo_canvas_item_model_remove_child (parent, child_num);
}


/**
 * goo_canvas_item_model_raise:
 * @model: an item model.
 * @above: the item model to raise @model above, or %NULL to raise @model to the top
 *  of the stack.
 * 
 * Raises a model in the stacking order.
 **/
void
goo_canvas_item_model_raise          (GooCanvasItemModel *model,
				      GooCanvasItemModel *above)
{
  GooCanvasItemModel *parent, *child;
  int n_children, i, model_pos = -1, above_pos = -1;

  parent = goo_canvas_item_model_get_parent (model);
  if (!parent || model == above)
    return;

  /* Find the current position of model and above. */
  n_children = goo_canvas_item_model_get_n_children (parent);
  for (i = 0; i < n_children; i++)
    {
      child = goo_canvas_item_model_get_child (parent, i);
      if (child == model)
	model_pos = i;
      if (child == above)
	above_pos = i;
    }

  /* If above is NULL we raise the model to the top of the stack. */
  if (!above)
    above_pos = n_children - 1;

  g_return_if_fail (model_pos != -1);
  g_return_if_fail (above_pos != -1);

  /* Only move the model if the new position is higher in the stack. */
  if (above_pos > model_pos)
    goo_canvas_item_model_move_child (parent, model_pos, above_pos);
}


/**
 * goo_canvas_item_model_lower:
 * @model: an item model.
 * @below: the item model to lower @model below, or %NULL to lower @model to the
 *  bottom of the stack.
 * 
 * Lowers a model in the stacking order.
 **/
void
goo_canvas_item_model_lower          (GooCanvasItemModel *model,
				      GooCanvasItemModel *below)
{
  GooCanvasItemModel *parent, *child;
  int n_children, i, model_pos = -1, below_pos = -1;

  parent = goo_canvas_item_model_get_parent (model);
  if (!parent || model == below)
    return;

  /* Find the current position of model and below. */
  n_children = goo_canvas_item_model_get_n_children (parent);
  for (i = 0; i < n_children; i++)
    {
      child = goo_canvas_item_model_get_child (parent, i);
      if (child == model)
	model_pos = i;
      if (child == below)
	below_pos = i;
    }

  /* If below is NULL we lower the model to the bottom of the stack. */
  if (!below)
    below_pos = 0;

  g_return_if_fail (model_pos != -1);
  g_return_if_fail (below_pos != -1);

  /* Only move the model if the new position is lower in the stack. */
  if (below_pos < model_pos)
    goo_canvas_item_model_move_child (parent, model_pos, below_pos);
}


/**
 * goo_canvas_item_model_get_transform:
 * @model: an item model.
 * @transform: the place to store the transform.
 * 
 * Gets the transformation matrix of an item model.
 * 
 * Returns: %TRUE if a transform is set.
 **/
gboolean
goo_canvas_item_model_get_transform  (GooCanvasItemModel *model,
				      cairo_matrix_t     *transform)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  return iface->get_transform ? iface->get_transform (model, transform) : FALSE;
}


/**
 * goo_canvas_item_model_set_transform:
 * @model: an item model.
 * @transform: the new transformation matrix, or %NULL to reset the
 *  transformation to the identity matrix.
 * 
 * Sets the transformation matrix of an item model.
 **/
void
goo_canvas_item_model_set_transform  (GooCanvasItemModel   *model,
				      const cairo_matrix_t *transform)
{
  GOO_CANVAS_ITEM_MODEL_GET_IFACE (model)->set_transform (model, transform);
}


/**
 * goo_canvas_item_model_set_simple_transform:
 * @model: an item model.
 * @x: the x coordinate of the origin of the model's coordinate space.
 * @y: the y coordinate of the origin of the model's coordinate space.
 * @scale: the scale of the model.
 * @rotation: the clockwise rotation of the model, in degrees.
 * 
 * A convenience function to set the item model's transformation matrix.
 **/
void
goo_canvas_item_model_set_simple_transform (GooCanvasItemModel *model,
					    gdouble             x,
					    gdouble             y,
					    gdouble             scale,
					    gdouble             rotation)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };

  cairo_matrix_translate (&new_matrix, x, y);
  cairo_matrix_scale (&new_matrix, scale, scale);
  cairo_matrix_rotate (&new_matrix, rotation * (M_PI  / 180));
  iface->set_transform (model, &new_matrix);
}


/**
 * goo_canvas_item_model_translate:
 * @model: an item model.
 * @tx: the amount to move the origin in the horizontal direction.
 * @ty: the amount to move the origin in the vertical direction.
 * 
 * Translates the origin of the model's coordinate system by the given amounts.
 **/
void
goo_canvas_item_model_translate      (GooCanvasItemModel *model,
				      gdouble             tx,
				      gdouble             ty)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };

  iface->get_transform (model, &new_matrix);
  cairo_matrix_translate (&new_matrix, tx, ty);
  iface->set_transform (model, &new_matrix);
}


/**
 * goo_canvas_item_model_scale:
 * @model: an item model.
 * @sx: the amount to scale the horizontal axis.
 * @sy: the amount to scale the vertical axis.
 * 
 * Scales the model's coordinate system by the given amounts.
 **/
void
goo_canvas_item_model_scale          (GooCanvasItemModel *model,
				      gdouble             sx,
				      gdouble             sy)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };

  iface->get_transform (model, &new_matrix);
  cairo_matrix_scale (&new_matrix, sx, sy);
  iface->set_transform (model, &new_matrix);
}


/**
 * goo_canvas_item_model_rotate:
 * @model: an item model.
 * @degrees: the clockwise angle of rotation.
 * @cx: the x coordinate of the origin of the rotation.
 * @cy: the y coordinate of the origin of the rotation.
 * 
 * Rotates the model's coordinate system by the given amount, about the given
 * origin.
 **/
void
goo_canvas_item_model_rotate         (GooCanvasItemModel *model,
				      gdouble             degrees,
				      gdouble             cx,
				      gdouble             cy)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };
  double radians = degrees * (M_PI / 180);

  iface->get_transform (model, &new_matrix);
  cairo_matrix_translate (&new_matrix, cx, cy);
  cairo_matrix_rotate (&new_matrix, radians);
  cairo_matrix_translate (&new_matrix, -cx, -cy);
  iface->set_transform (model, &new_matrix);
}


/**
 * goo_canvas_item_model_skew_x:
 * @model: an item model.
 * @degrees: the skew angle.
 * @cx: the x coordinate of the origin of the skew transform.
 * @cy: the y coordinate of the origin of the skew transform.
 * 
 * Skews the model's coordinate system along the x axis by the given amount,
 * about the given origin.
 **/
void
goo_canvas_item_model_skew_x         (GooCanvasItemModel *model,
				      gdouble             degrees,
				      gdouble             cx,
				      gdouble             cy)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);
  cairo_matrix_t tmp, new_matrix = { 1, 0, 0, 1, 0, 0 };
  double radians = degrees * (M_PI / 180);

  iface->get_transform (model, &new_matrix);
  cairo_matrix_translate (&new_matrix, cx, cy);
  cairo_matrix_init (&tmp, 1, 0, tan (radians), 1, 0, 0);
  cairo_matrix_multiply (&new_matrix, &tmp, &new_matrix);
  cairo_matrix_translate (&new_matrix, -cx, -cy);
  iface->set_transform (model, &new_matrix);
}


/**
 * goo_canvas_item_model_skew_y:
 * @model: an item model.
 * @degrees: the skew angle.
 * @cx: the x coordinate of the origin of the skew transform.
 * @cy: the y coordinate of the origin of the skew transform.
 * 
 * Skews the model's coordinate system along the y axis by the given amount,
 * about the given origin.
 **/
void
goo_canvas_item_model_skew_y         (GooCanvasItemModel *model,
				      gdouble             degrees,
				      gdouble             cx,
				      gdouble             cy)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);
  cairo_matrix_t tmp, new_matrix = { 1, 0, 0, 1, 0, 0 };
  double radians = degrees * (M_PI / 180);

  iface->get_transform (model, &new_matrix);
  cairo_matrix_translate (&new_matrix, cx, cy);
  cairo_matrix_init (&tmp, 1, tan (radians), 0, 1, 0, 0);
  cairo_matrix_multiply (&new_matrix, &tmp, &new_matrix);
  cairo_matrix_translate (&new_matrix, -cx, -cy);
  iface->set_transform (model, &new_matrix);
}


/**
 * goo_canvas_item_model_get_style:
 * @model: an item model.
 * 
 * Gets the model's style. If the model doesn't have its own style it will
 * return its parent's style.
 * 
 * Returns: the model's style.
 **/
GooCanvasStyle*
goo_canvas_item_model_get_style      (GooCanvasItemModel   *model)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  return iface->get_style ? iface->get_style (model) : NULL;
}


/**
 * goo_canvas_item_model_set_style:
 * @model: an item model.
 * @style: a style.
 * 
 * Sets the model's style, by copying the properties from the given style.
 **/
void
goo_canvas_item_model_set_style      (GooCanvasItemModel *model,
				      GooCanvasStyle     *style)
{
  GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);

  if (iface->set_style)
    iface->set_style (model, style);
}


extern void _goo_canvas_item_animate_internal (GooCanvasItem       *item,
					       GooCanvasItemModel  *model,
					       gdouble              x,
					       gdouble              y,
					       gdouble              scale,
					       gdouble              degrees,
					       gboolean             absolute,
					       gint                 duration,
					       gint                 step_time,
					       GooCanvasAnimateType type);

/**
 * goo_canvas_item_model_animate:
 * @model: an item model.
 * @x: the final x coordinate.
 * @y: the final y coordinate.
 * @scale: the final scale.
 * @degrees: the final rotation. This can be negative to rotate anticlockwise,
 *  and can also be greater than 360 to rotate a number of times.
 * @absolute: if the @x, @y, @scale and @degrees values are absolute, or
 *  relative to the current transform. Note that absolute animations only work
 *  if the model currently has a simple transform. If the model has a shear or
 *  some other complicated transform it may result in strange animations.
 * @duration: the duration of the animation, in milliseconds (1/1000ths of a
 *  second).
 * @step_time: the time between each animation step, in milliseconds.
 * @type: specifies what happens when the animation finishes.
 * 
 * Animates a model from its current position to the given offsets, scale
 * and rotation.
 **/
void
goo_canvas_item_model_animate        (GooCanvasItemModel  *model,
				      gdouble              x,
				      gdouble              y,
				      gdouble              scale,
				      gdouble              degrees,
				      gboolean             absolute,
				      gint                 duration,
				      gint                 step_time,
				      GooCanvasAnimateType type)
{
  _goo_canvas_item_animate_internal (NULL, model, x, y, scale, degrees,
				     absolute, duration, step_time, type);
}


/**
 * goo_canvas_item_model_stop_animation:
 * @model: an item model.
 * 
 * Stops any current animation for the given model, leaving it at its current
 * position.
 **/
void
goo_canvas_item_model_stop_animation (GooCanvasItemModel *model)
{
  /* This will result in a call to goo_canvas_item_free_animation() above. */
  g_object_set_data (G_OBJECT (model), animation_key, NULL);
}



/*
 * Child Properties.
 */
extern void _goo_canvas_item_get_child_properties_internal (GObject *object, GObject *child, va_list var_args, GParamSpecPool *property_pool, GObjectNotifyContext *notify_context, gboolean is_model);

extern void _goo_canvas_item_set_child_properties_internal (GObject *object, GObject *child, va_list var_args, GParamSpecPool *property_pool, GObjectNotifyContext *notify_context, gboolean is_model);


/**
 * goo_canvas_item_model_get_child_properties_valist:
 * @model: a #GooCanvasItemModel.
 * @child: a child #GooCanvasItemModel.
 * @var_args: pairs of property names and value pointers, and a terminating
 *  %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * item models, specifically layout container item models such as
 * #GooCanvasTableModel.
 *
 * It gets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_model_get_child_properties_valist (GooCanvasItemModel *model,
						   GooCanvasItemModel *child,
						   va_list	       var_args)
{
  g_return_if_fail (GOO_IS_CANVAS_ITEM_MODEL (model));
  g_return_if_fail (GOO_IS_CANVAS_ITEM_MODEL (child));

  _goo_canvas_item_get_child_properties_internal ((GObject*) model, (GObject*) child, var_args, _goo_canvas_item_model_child_property_pool, _goo_canvas_item_model_child_property_notify_context, TRUE);
}


/**
 * goo_canvas_item_model_set_child_properties_valist:
 * @model: a #GooCanvasItemModel.
 * @child: a child #GooCanvasItemModel.
 * @var_args: pairs of property names and values, and a terminating %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * item models, specifically layout container item models such as
 * #GooCanvasTableModel.
 *
 * It sets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_model_set_child_properties_valist (GooCanvasItemModel *model,
						   GooCanvasItemModel *child,
						   va_list	       var_args)
{
  g_return_if_fail (GOO_IS_CANVAS_ITEM_MODEL (model));
  g_return_if_fail (GOO_IS_CANVAS_ITEM_MODEL (child));

  _goo_canvas_item_set_child_properties_internal ((GObject*) model, (GObject*) child, var_args, _goo_canvas_item_model_child_property_pool, _goo_canvas_item_model_child_property_notify_context, TRUE);
}


/**
 * goo_canvas_item_model_get_child_properties:
 * @model: a #GooCanvasItemModel.
 * @child: a child #GooCanvasItemModel.
 * @...: pairs of property names and value pointers, and a terminating %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * item models, specifically layout container item models such as
 * #GooCanvasTableModel.
 *
 * It gets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_model_get_child_properties  (GooCanvasItemModel   *model,
					     GooCanvasItemModel   *child,
					     ...)
{
  va_list var_args;
  
  va_start (var_args, child);
  goo_canvas_item_model_get_child_properties_valist (model, child, var_args);
  va_end (var_args);
}


/**
 * goo_canvas_item_model_set_child_properties:
 * @model: a #GooCanvasItemModel.
 * @child: a child #GooCanvasItemModel.
 * @...: pairs of property names and values, and a terminating %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * item models, specifically layout container item models such as
 * #GooCanvasTableModel.
 *
 * It sets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_model_set_child_properties  (GooCanvasItemModel   *model,
					     GooCanvasItemModel   *child,
					     ...)
{
  va_list var_args;
  
  va_start (var_args, child);
  goo_canvas_item_model_set_child_properties_valist (model, child, var_args);
  va_end (var_args);
}



/**
 * goo_canvas_item_model_class_install_child_property:
 * @mclass: a #GObjectClass
 * @property_id: the id for the property
 * @pspec: the #GParamSpec for the property
 * 
 * This function is only intended to be used when implementing new canvas
 * item models, specifically layout container item models such as
 * #GooCanvasTableModel.
 *
 * It installs a child property on a canvas item class. 
 **/
void
goo_canvas_item_model_class_install_child_property (GObjectClass *mclass,
						    guint         property_id,
						    GParamSpec   *pspec)
{
  g_return_if_fail (G_IS_OBJECT_CLASS (mclass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (property_id > 0);

  if (g_param_spec_pool_lookup (_goo_canvas_item_model_child_property_pool,
				pspec->name, G_OBJECT_CLASS_TYPE (mclass),
				FALSE))
    {
      g_warning (G_STRLOC ": class `%s' already contains a child property named `%s'",
		 G_OBJECT_CLASS_NAME (mclass), pspec->name);
      return;
    }
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  pspec->param_id = property_id;
  g_param_spec_pool_insert (_goo_canvas_item_model_child_property_pool, pspec,
			    G_OBJECT_CLASS_TYPE (mclass));
}

/**
 * goo_canvas_item_model_class_find_child_property:
 * @mclass: a #GObjectClass
 * @property_name: the name of the child property to find
 * @returns: the #GParamSpec of the child property or %NULL if @class has no
 *   child property with that name.
 *
 * This function is only intended to be used when implementing new canvas
 * item models, specifically layout container item models such as
 * #GooCanvasTableModel.
 *
 * It finds a child property of a canvas item class by name.
 */
GParamSpec*
goo_canvas_item_model_class_find_child_property (GObjectClass *mclass,
						 const gchar  *property_name)
{
  g_return_val_if_fail (G_IS_OBJECT_CLASS (mclass), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (_goo_canvas_item_model_child_property_pool,
				   property_name, G_OBJECT_CLASS_TYPE (mclass),
				   TRUE);
}

/**
 * goo_canvas_item_model_class_list_child_properties:
 * @mclass: a #GObjectClass
 * @n_properties: location to return the number of child properties found
 * @returns: a newly allocated array of #GParamSpec*. The array must be 
 *           freed with g_free().
 *
 * This function is only intended to be used when implementing new canvas
 * item models, specifically layout container item models such as
 * #GooCanvasTableModel.
 *
 * It returns all child properties of a canvas item class.
 */
GParamSpec**
goo_canvas_item_model_class_list_child_properties (GObjectClass *mclass,
						   guint        *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (G_IS_OBJECT_CLASS (mclass), NULL);

  pspecs = g_param_spec_pool_list (_goo_canvas_item_model_child_property_pool,
				   G_OBJECT_CLASS_TYPE (mclass), &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}
