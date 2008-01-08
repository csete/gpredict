/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasitem.c - interface for canvas items & groups.
 */

/**
 * SECTION:goocanvasitem
 * @Title: GooCanvasItem
 * @Short_Description: the interface for canvas items.
 *
 * #GooCanvasItem defines the interface that canvas items must implement,
 * and contains methods for operating on canvas items.
 */
#include <config.h>
#include <math.h>
#include <glib/gi18n-lib.h>
#include <gobject/gobjectnotifyqueue.c>
#include <gobject/gvaluecollector.h>
#include <gtk/gtk.h>
#include "goocanvasprivate.h"
#include "goocanvasitem.h"
#include "goocanvas.h"
#include "goocanvasutils.h"
#include "goocanvasmarshal.h"


static GParamSpecPool       *_goo_canvas_item_child_property_pool = NULL;
static GObjectNotifyContext *_goo_canvas_item_child_property_notify_context = NULL;
static const char *animation_key = "GooCanvasItemAnimation";

enum {
  /* Mouse events. */
  ENTER_NOTIFY_EVENT,
  LEAVE_NOTIFY_EVENT,
  MOTION_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,

  /* Keyboard events. */
  FOCUS_IN_EVENT,
  FOCUS_OUT_EVENT,
  KEY_PRESS_EVENT,
  KEY_RELEASE_EVENT,

  /* Miscellaneous signals. */
  GRAB_BROKEN_EVENT,
  CHILD_NOTIFY,

  LAST_SIGNAL
};

static guint canvas_item_signals[LAST_SIGNAL] = { 0 };

static void goo_canvas_item_base_init (gpointer g_class);
extern void _goo_canvas_style_init (void);


GType
goo_canvas_item_get_type (void)
{
  static GType canvas_item_type = 0;

  if (!canvas_item_type)
    {
      static const GTypeInfo canvas_item_info =
      {
        sizeof (GooCanvasItemIface), /* class_size */
	goo_canvas_item_base_init,   /* base_init */
	NULL,			     /* base_finalize */
      };

      canvas_item_type = g_type_register_static (G_TYPE_INTERFACE,
						 "GooCanvasItem",
						 &canvas_item_info, 0);

      g_type_interface_add_prerequisite (canvas_item_type, G_TYPE_OBJECT);
    }

  return canvas_item_type;
}


static void
child_property_notify_dispatcher (GObject     *object,
				  guint        n_pspecs,
				  GParamSpec **pspecs)
{
  guint i;

  for (i = 0; i < n_pspecs; i++)
    g_signal_emit (object, canvas_item_signals[CHILD_NOTIFY],
		   g_quark_from_string (pspecs[i]->name), pspecs[i]);
}


static void
goo_canvas_item_base_init (gpointer g_iface)
{
  static GObjectNotifyContext cpn_context = { 0, NULL, NULL };
  static gboolean initialized = FALSE;
  
  if (!initialized)
    {
      GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);

      _goo_canvas_item_child_property_pool = g_param_spec_pool_new (TRUE);

      cpn_context.quark_notify_queue = g_quark_from_static_string ("GooCanvasItem-child-property-notify-queue");
      cpn_context.dispatcher = child_property_notify_dispatcher;
      _goo_canvas_item_child_property_notify_context = &cpn_context;

      /* Mouse events. */

      /**
       * GooCanvasItem::enter-notify-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data, with coordinates translated to canvas
       *  coordinates.
       *
       * Emitted when the mouse enters an item.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[ENTER_NOTIFY_EVENT] =
	g_signal_new ("enter_notify_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       enter_notify_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::leave-notify-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data, with coordinates translated to canvas
       *  coordinates.
       *
       * Emitted when the mouse leaves an item.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[LEAVE_NOTIFY_EVENT] =
	g_signal_new ("leave_notify_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       leave_notify_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::motion-notify-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data, with coordinates translated to canvas
       *  coordinates.
       *
       * Emitted when the mouse moves within an item.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[MOTION_NOTIFY_EVENT] =
	g_signal_new ("motion_notify_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       motion_notify_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::button-press-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data, with coordinates translated to canvas
       *  coordinates.
       *
       * Emitted when a mouse button is pressed in an item.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[BUTTON_PRESS_EVENT] =
	g_signal_new ("button_press_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       button_press_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::button-release-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data, with coordinates translated to canvas
       *  coordinates.
       *
       * Emitted when a mouse button is released in an item.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[BUTTON_RELEASE_EVENT] =
	g_signal_new ("button_release_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       button_release_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);


      /* Keyboard events. */

      /**
       * GooCanvasItem::focus-in-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data.
       *
       * Emitted when the item receives the keyboard focus.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[FOCUS_IN_EVENT] =
	g_signal_new ("focus_in_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       focus_in_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::focus-out-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data.
       *
       * Emitted when the item loses the keyboard focus.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[FOCUS_OUT_EVENT] =
	g_signal_new ("focus_out_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       focus_out_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::key-press-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data.
       *
       * Emitted when a key is pressed and the item has the keyboard
       * focus.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[KEY_PRESS_EVENT] =
	g_signal_new ("key_press_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       key_press_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::key-release-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data.
       *
       * Emitted when a key is released and the item has the keyboard
       * focus.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[KEY_RELEASE_EVENT] =
	g_signal_new ("key_release_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       key_release_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);


      /**
       * GooCanvasItem::grab-broken-event
       * @item: the item that received the signal.
       * @target_item: the target of the event.
       * @event: the event data.
       *
       * Emitted when the item's keyboard or pointer grab was lost
       * unexpectedly.
       *
       * Returns: %TRUE to stop the signal emission, or %FALSE to let it
       *  continue.
       */
      canvas_item_signals[GRAB_BROKEN_EVENT] =
	g_signal_new ("grab_broken_event",
		      iface_type,
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GooCanvasItemIface,
				       grab_broken_event),
		      goo_canvas_boolean_handled_accumulator, NULL,
		      goo_canvas_marshal_BOOLEAN__OBJECT_BOXED,
		      G_TYPE_BOOLEAN, 2,
		      GOO_TYPE_CANVAS_ITEM,
		      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

      /**
       * GooCanvasItem::child-notify
       * @item: the item that received the signal.
       * @pspec: the #GParamSpec of the changed child property.
       *
       * Emitted for each child property that has changed.
       * The signal's detail holds the property name. 
       */
      canvas_item_signals[CHILD_NOTIFY] =
	g_signal_new ("child_notify",
		      iface_type,
		      G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS,
		      G_STRUCT_OFFSET (GooCanvasItemIface, child_notify),
		      NULL, NULL,
		      g_cclosure_marshal_VOID__PARAM,
		      G_TYPE_NONE, 1,
		      G_TYPE_PARAM);


      g_object_interface_install_property (g_iface,
					   g_param_spec_object ("parent",
								_("Parent"),
								_("The parent item"),
								GOO_TYPE_CANVAS_ITEM,
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
 * goo_canvas_item_get_canvas:
 * @item: a #GooCanvasItem.
 * 
 * Returns the #GooCanvas containing the given #GooCanvasItem.
 * 
 * Returns: the #GooCanvas.
 **/
GooCanvas*
goo_canvas_item_get_canvas (GooCanvasItem *item)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (iface->get_canvas)
    {
      return iface->get_canvas (item);
    }
  else
    {
      GooCanvasItem *parent = iface->get_parent (item);

      if (parent)
	return goo_canvas_item_get_canvas (parent);
      return NULL;
    }
}


/**
 * goo_canvas_item_set_canvas:
 * @item: a #GooCanvasItem.
 * @canvas: a #GooCanvas
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically container items such as #GooCanvasGroup.
 *
 * It sets the canvas of the item.
 **/
void
goo_canvas_item_set_canvas     (GooCanvasItem   *item,
				GooCanvas       *canvas)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (iface->set_canvas)
    iface->set_canvas (item, canvas);
}


/**
 * goo_canvas_item_add_child:
 * @item: the container to add the item to.
 * @child: the item to add.
 * @position: the position of the item, or -1 to place it last (at the top of
 *  the stacking order).
 * 
 * Adds a child item to a container item at the given stack position.
 **/
void
goo_canvas_item_add_child      (GooCanvasItem       *item,
				GooCanvasItem       *child,
				gint                 position)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  g_return_if_fail (iface->add_child != NULL);

  iface->add_child (item, child, position);
}


/**
 * goo_canvas_item_move_child:
 * @item: a container item.
 * @old_position: the current position of the child item.
 * @new_position: the new position of the child item.
 * 
 * Moves a child item to a new stack position within the container.
 **/
void
goo_canvas_item_move_child     (GooCanvasItem       *item,
				gint                 old_position,
				gint                 new_position)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  g_return_if_fail (iface->move_child != NULL);

  iface->move_child (item, old_position, new_position);
}


/**
 * goo_canvas_item_remove_child:
 * @item: a container item.
 * @child_num: the position of the child item to remove.
 * 
 * Removes the child item at the given position.
 **/
void
goo_canvas_item_remove_child   (GooCanvasItem       *item,
				gint                 child_num)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  g_return_if_fail (iface->remove_child != NULL);

  iface->remove_child (item, child_num);
}


/**
 * goo_canvas_item_find_child:
 * @item: a container item.
 * @child: the child item to find.
 * 
 * Attempts to find the given child item with the container's stack.
 * 
 * Returns: the position of the given @child item, or -1 if it isn't found.
 **/
gint
goo_canvas_item_find_child     (GooCanvasItem *item,
				GooCanvasItem *child)
{
  GooCanvasItem *tmp;
  int n_children, i;

  /* Find the current position of item and above. */
  n_children = goo_canvas_item_get_n_children (item);
  for (i = 0; i < n_children; i++)
    {
      tmp = goo_canvas_item_get_child (item, i);
      if (child == tmp)
	return i;
    }
  return -1;
}


/**
 * goo_canvas_item_is_container:
 * @item: an item.
 * 
 * Tests to see if the given item is a container.
 * 
 * Returns: %TRUE if the item is a container.
 **/
gboolean
goo_canvas_item_is_container (GooCanvasItem       *item)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  return iface->get_n_children ? TRUE : FALSE;
}


/**
 * goo_canvas_item_get_n_children:
 * @item: a container item.
 * 
 * Gets the number of children of the container.
 * 
 * Returns: the number of children.
 **/
gint
goo_canvas_item_get_n_children (GooCanvasItem       *item)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  return iface->get_n_children ? iface->get_n_children (item) : 0;
}


/**
 * goo_canvas_item_get_child:
 * @item: a container item.
 * @child_num: the position of a child in the container's stack.
 * 
 * Gets the child item at the given stack position.
 * 
 * Returns: the child item at the given stack position.
 **/
GooCanvasItem*
goo_canvas_item_get_child (GooCanvasItem       *item,
			   gint                 child_num)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  return iface->get_child ? iface->get_child (item, child_num) : NULL;
}


/**
 * goo_canvas_item_get_parent:
 * @item: an item.
 * 
 * Gets the parent of the given item.
 * 
 * Returns: the parent item, or %NULL if the item has no parent.
 **/
GooCanvasItem*
goo_canvas_item_get_parent  (GooCanvasItem *item)
{
  g_return_val_if_fail (GOO_IS_CANVAS_ITEM (item), NULL);

  return GOO_CANVAS_ITEM_GET_IFACE (item)->get_parent (item);
}


/**
 * goo_canvas_item_set_parent:
 * @item: an item.
 * @parent: the new parent item.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically container items such as #GooCanvasGroup.
 *
 * It sets the parent of the item.
 **/
void
goo_canvas_item_set_parent (GooCanvasItem *item,
			    GooCanvasItem *parent)
{
  GOO_CANVAS_ITEM_GET_IFACE (item)->set_parent (item, parent);
}


/**
 * goo_canvas_item_remove:
 * @item: an item.
 * 
 * Removes an item from its parent. If the item is in a canvas it will be
 * removed.
 *
 * This would normally also result in the item being freed.
 **/
void
goo_canvas_item_remove (GooCanvasItem *item)
{
  GooCanvasItem *parent;
  gint child_num;

  parent = goo_canvas_item_get_parent (item);
  if (!parent)
    return;

  child_num = goo_canvas_item_find_child (parent, item);
  if (child_num == -1)
    return;

  goo_canvas_item_remove_child (parent, child_num);
}


/**
 * goo_canvas_item_raise:
 * @item: an item.
 * @above: the item to raise @item above, or %NULL to raise @item to the top
 *  of the stack.
 * 
 * Raises an item in the stacking order.
 **/
void
goo_canvas_item_raise          (GooCanvasItem *item,
				GooCanvasItem *above)
{
  GooCanvasItem *parent, *child;
  int n_children, i, item_pos = -1, above_pos = -1;

  parent = goo_canvas_item_get_parent (item);
  if (!parent || item == above)
    return;

  /* Find the current position of item and above. */
  n_children = goo_canvas_item_get_n_children (parent);
  for (i = 0; i < n_children; i++)
    {
      child = goo_canvas_item_get_child (parent, i);
      if (child == item)
	item_pos = i;
      if (child == above)
	above_pos = i;
    }

  /* If above is NULL we raise the item to the top of the stack. */
  if (!above)
    above_pos = n_children - 1;

  g_return_if_fail (item_pos != -1);
  g_return_if_fail (above_pos != -1);

  /* Only move the item if the new position is higher in the stack. */
  if (above_pos > item_pos)
    goo_canvas_item_move_child (parent, item_pos, above_pos);
}


/**
 * goo_canvas_item_lower:
 * @item: an item.
 * @below: the item to lower @item below, or %NULL to lower @item to the
 *  bottom of the stack.
 * 
 * Lowers an item in the stacking order.
 **/
void
goo_canvas_item_lower          (GooCanvasItem *item,
				GooCanvasItem *below)
{
  GooCanvasItem *parent, *child;
  int n_children, i, item_pos = -1, below_pos = -1;

  parent = goo_canvas_item_get_parent (item);
  if (!parent || item == below)
    return;

  /* Find the current position of item and below. */
  n_children = goo_canvas_item_get_n_children (parent);
  for (i = 0; i < n_children; i++)
    {
      child = goo_canvas_item_get_child (parent, i);
      if (child == item)
	item_pos = i;
      if (child == below)
	below_pos = i;
    }

  /* If below is NULL we lower the item to the bottom of the stack. */
  if (!below)
    below_pos = 0;

  g_return_if_fail (item_pos != -1);
  g_return_if_fail (below_pos != -1);

  /* Only move the item if the new position is lower in the stack. */
  if (below_pos < item_pos)
    goo_canvas_item_move_child (parent, item_pos, below_pos);
}


/**
 * goo_canvas_item_get_transform:
 * @item: an item.
 * @transform: the place to store the transform.
 * 
 * Gets the transformation matrix of an item.
 * 
 * Returns: %TRUE if a transform is set.
 **/
gboolean
goo_canvas_item_get_transform  (GooCanvasItem   *item,
				cairo_matrix_t  *transform)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  return iface->get_transform ? iface->get_transform (item, transform) : FALSE;
}


/**
 * goo_canvas_item_get_transform_for_child:
 * @item: an item.
 * @child: a child of @item.
 * @transform: the place to store the transform.
 * 
 * Gets the transformation matrix of an item combined with any special
 * transform needed for the given child. These special transforms are used
 * by layout items such as #GooCanvasTable.
 * 
 * Returns: %TRUE if a transform is set.
 **/
gboolean
goo_canvas_item_get_transform_for_child  (GooCanvasItem  *item,
					  GooCanvasItem  *child,
					  cairo_matrix_t *transform)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (child && iface->get_transform_for_child)
    return iface->get_transform_for_child (item, child, transform);

  /* We fallback to the standard get_transform method. */
  if (iface->get_transform)
    return iface->get_transform (item, transform);

  return FALSE;
}


/**
 * goo_canvas_item_set_transform:
 * @item: an item.
 * @transform: the new transformation matrix, or %NULL to reset the
 *  transformation to the identity matrix.
 * 
 * Sets the transformation matrix of an item.
 **/
void
goo_canvas_item_set_transform  (GooCanvasItem        *item,
				const cairo_matrix_t *transform)
{
  GOO_CANVAS_ITEM_GET_IFACE (item)->set_transform (item, transform);
}


/**
 * goo_canvas_item_set_simple_transform:
 * @item: an item.
 * @x: the x coordinate of the origin of the item's coordinate space.
 * @y: the y coordinate of the origin of the item's coordinate space.
 * @scale: the scale of the item.
 * @rotation: the clockwise rotation of the item, in degrees.
 * 
 * A convenience function to set the item's transformation matrix.
 **/
void
goo_canvas_item_set_simple_transform (GooCanvasItem   *item,
				      gdouble          x,
				      gdouble          y,
				      gdouble          scale,
				      gdouble          rotation)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };

  cairo_matrix_translate (&new_matrix, x, y);
  cairo_matrix_scale (&new_matrix, scale, scale);
  cairo_matrix_rotate (&new_matrix, rotation * (M_PI  / 180));
  iface->set_transform (item, &new_matrix);
}


/**
 * goo_canvas_item_translate:
 * @item: an item.
 * @tx: the amount to move the origin in the horizontal direction.
 * @ty: the amount to move the origin in the vertical direction.
 * 
 * Translates the origin of the item's coordinate system by the given amounts.
 **/
void
goo_canvas_item_translate      (GooCanvasItem *item,
				gdouble        tx,
				gdouble        ty)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };

  iface->get_transform (item, &new_matrix);
  cairo_matrix_translate (&new_matrix, tx, ty);
  iface->set_transform (item, &new_matrix);
}


/**
 * goo_canvas_item_scale:
 * @item: an item.
 * @sx: the amount to scale the horizontal axis.
 * @sy: the amount to scale the vertical axis.
 * 
 * Scales the item's coordinate system by the given amounts.
 **/
void
goo_canvas_item_scale          (GooCanvasItem *item,
				gdouble        sx,
				gdouble        sy)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };

  iface->get_transform (item, &new_matrix);
  cairo_matrix_scale (&new_matrix, sx, sy);
  iface->set_transform (item, &new_matrix);
}


/**
 * goo_canvas_item_rotate:
 * @item: an item.
 * @degrees: the clockwise angle of rotation.
 * @cx: the x coordinate of the origin of the rotation.
 * @cy: the y coordinate of the origin of the rotation.
 * 
 * Rotates the item's coordinate system by the given amount, about the given
 * origin.
 **/
void
goo_canvas_item_rotate         (GooCanvasItem *item,
				gdouble        degrees,
				gdouble        cx,
				gdouble        cy)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);
  cairo_matrix_t new_matrix = { 1, 0, 0, 1, 0, 0 };
  double radians = degrees * (M_PI / 180);

  iface->get_transform (item, &new_matrix);
  cairo_matrix_translate (&new_matrix, cx, cy);
  cairo_matrix_rotate (&new_matrix, radians);
  cairo_matrix_translate (&new_matrix, -cx, -cy);
  iface->set_transform (item, &new_matrix);
}


/**
 * goo_canvas_item_skew_x:
 * @item: an item.
 * @degrees: the skew angle.
 * @cx: the x coordinate of the origin of the skew transform.
 * @cy: the y coordinate of the origin of the skew transform.
 * 
 * Skews the item's coordinate system along the x axis by the given amount,
 * about the given origin.
 **/
void
goo_canvas_item_skew_x         (GooCanvasItem *item,
				gdouble        degrees,
				gdouble        cx,
				gdouble        cy)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);
  cairo_matrix_t tmp, new_matrix = { 1, 0, 0, 1, 0, 0 };
  double radians = degrees * (M_PI / 180);

  iface->get_transform (item, &new_matrix);
  cairo_matrix_translate (&new_matrix, cx, cy);
  cairo_matrix_init (&tmp, 1, 0, tan (radians), 1, 0, 0);
  cairo_matrix_multiply (&new_matrix, &tmp, &new_matrix);
  cairo_matrix_translate (&new_matrix, -cx, -cy);
  iface->set_transform (item, &new_matrix);
}


/**
 * goo_canvas_item_skew_y:
 * @item: an item.
 * @degrees: the skew angle.
 * @cx: the x coordinate of the origin of the skew transform.
 * @cy: the y coordinate of the origin of the skew transform.
 * 
 * Skews the item's coordinate system along the y axis by the given amount,
 * about the given origin.
 **/
void
goo_canvas_item_skew_y         (GooCanvasItem *item,
				gdouble        degrees,
				gdouble        cx,
				gdouble        cy)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);
  cairo_matrix_t tmp, new_matrix = { 1, 0, 0, 1, 0, 0 };
  double radians = degrees * (M_PI / 180);

  iface->get_transform (item, &new_matrix);
  cairo_matrix_translate (&new_matrix, cx, cy);
  cairo_matrix_init (&tmp, 1, tan (radians), 0, 1, 0, 0);
  cairo_matrix_multiply (&new_matrix, &tmp, &new_matrix);
  cairo_matrix_translate (&new_matrix, -cx, -cy);
  iface->set_transform (item, &new_matrix);
}


/**
 * goo_canvas_item_get_style:
 * @item: an item.
 * 
 * Gets the item's style. If the item doesn't have its own style it will return
 * its parent's style.
 * 
 * Returns: the item's style.
 **/
GooCanvasStyle*
goo_canvas_item_get_style      (GooCanvasItem   *item)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  return iface->get_style ? iface->get_style (item) : NULL;
}


/**
 * goo_canvas_item_set_style:
 * @item: an item.
 * @style: a style.
 * 
 * Sets the item's style, by copying the properties from the given style.
 **/
void
goo_canvas_item_set_style      (GooCanvasItem   *item,
				GooCanvasStyle  *style)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (iface->set_style)
    iface->set_style (item, style);
}


typedef struct _GooCanvasItemAnimation  GooCanvasItemAnimation;
struct _GooCanvasItemAnimation
{
  GooCanvasAnimateType type;
  GooCanvasItem *item;
  GooCanvasItemModel *model;
  int step, total_steps;
  cairo_matrix_t start;
  gdouble x_start, y_start, scale_start, radians_start;
  gdouble x_step, y_step, scale_step, radians_step;
  gboolean absolute;
  gboolean forward;
  guint timeout_id;
};


static void
goo_canvas_item_free_animation (GooCanvasItemAnimation *anim)
{
  if (anim->timeout_id)
    {
      g_source_remove (anim->timeout_id);
      anim->timeout_id = 0;
    }

  g_free (anim);
}


static gboolean
goo_canvas_item_animate_cb (GooCanvasItemAnimation *anim)
{
  GooCanvasItemIface *iface;
  GooCanvasItemModelIface *model_iface;
  cairo_matrix_t new_matrix;
  gboolean keep_source = TRUE;
  gdouble scale;
  gint step;

  GDK_THREADS_ENTER ();

  if (anim->model)
    model_iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (anim->model);
  else
    iface = GOO_CANVAS_ITEM_GET_IFACE (anim->item);

  if (++anim->step > anim->total_steps)
    {
      switch (anim->type)
	{
	case GOO_CANVAS_ANIMATE_RESET:
	  /* Reset the transform to the initial value. */
	  if (anim->model)
	    model_iface->set_transform (anim->model, &anim->start);
	  else
	    iface->set_transform (anim->item, &anim->start);

	  /* Fall through.. */
	case GOO_CANVAS_ANIMATE_FREEZE:
	  keep_source = FALSE;
	  anim->timeout_id = 0;
	  /* This will result in a call to goo_canvas_item_free_animation()
	     above. We've set the timeout_id to 0 so it isn't removed twice. */
	  if (anim->model)
	    g_object_set_data (G_OBJECT (anim->model), animation_key, NULL);
	  else
	    g_object_set_data (G_OBJECT (anim->item), animation_key, NULL);
	  break;

	case GOO_CANVAS_ANIMATE_RESTART:
	  anim->step = 0;
	  break;

	case GOO_CANVAS_ANIMATE_BOUNCE:
	  anim->forward = !anim->forward;
	  anim->step = 1;
	  break;
	}
    }

  step = anim->forward ? anim->step : anim->total_steps - anim->step;

  if (anim->absolute)
    {
      cairo_matrix_init_identity (&new_matrix);
      scale = anim->scale_start + anim->scale_step * step;
      cairo_matrix_translate (&new_matrix,
			      anim->x_start + anim->x_step * step,
			      anim->y_start + anim->y_step * step);
      cairo_matrix_scale (&new_matrix, scale, scale);
      cairo_matrix_rotate (&new_matrix,
			   anim->radians_start + anim->radians_step * step);
    }
  else
    {
      new_matrix = anim->start;
      scale = 1 + anim->scale_step * step;
      cairo_matrix_translate (&new_matrix, anim->x_step * step,
			      anim->y_step * step);
      cairo_matrix_scale (&new_matrix, scale, scale);
      cairo_matrix_rotate (&new_matrix, anim->radians_step * step);
    }

  if (anim->model)
    model_iface->set_transform (anim->model, &new_matrix);
  else
    iface->set_transform (anim->item, &new_matrix);


  GDK_THREADS_LEAVE ();

  /* Return FALSE to remove the timeout handler when we are finished. */
  return keep_source;
}


void
_goo_canvas_item_animate_internal (GooCanvasItem       *item,
				   GooCanvasItemModel  *model,
				   gdouble              x,
				   gdouble              y,
				   gdouble              scale,
				   gdouble              degrees,
				   gboolean             absolute,
				   gint                 duration,
				   gint                 step_time,
				   GooCanvasAnimateType type)
{
  GObject *object;
  cairo_matrix_t matrix = { 1, 0, 0, 1, 0, 0 };
  GooCanvasItemAnimation *anim;

  if (item)
    {
      GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);
      iface->get_transform (item, &matrix);
      object = (GObject*) item;
    }
  else
    {
      GooCanvasItemModelIface *iface = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model);
      iface->get_transform (model, &matrix);
      object = (GObject*) model;
    }

  anim = g_new (GooCanvasItemAnimation, 1);
  anim->type = type;
  anim->item = item;
  anim->model = model;
  anim->step = 0;
  anim->total_steps = duration / step_time;
  anim->start = matrix;
  anim->absolute = absolute;
  anim->forward = TRUE;

  /* For absolute animation we have to try to calculate the current position,
     scale and rotation. */
  if (absolute)
    {
      cairo_matrix_t tmp_matrix = anim->start;
      double x1 = 1.0, y1 = 0.0;

      anim->x_start = tmp_matrix.x0;
      anim->y_start = tmp_matrix.y0;

      tmp_matrix.x0 = 0.0;
      tmp_matrix.y0 = 0.0;

      cairo_matrix_transform_point (&tmp_matrix, &x1, &y1);
      anim->scale_start = sqrt (x1 * x1 + y1 * y1);
      anim->radians_start = atan2 (y1, x1);

      anim->x_step = (x - anim->x_start) / anim->total_steps;
      anim->y_step = (y - anim->y_start) / anim->total_steps;
      anim->scale_step = (scale - anim->scale_start) / anim->total_steps;
      anim->radians_step = (degrees * (M_PI / 180) - anim->radians_start) / anim->total_steps;
    }
  else
    {
      anim->x_step = x / anim->total_steps;
      anim->y_step = y / anim->total_steps;
      anim->scale_step = (scale - 1.0) / anim->total_steps;
      anim->radians_step = (degrees * (M_PI / 180)) / anim->total_steps;
    }


  /* Store a pointer to the new animation in the item. This will automatically
     stop any current animation and free it. */
  g_object_set_data_full (object, animation_key, anim,
			  (GDestroyNotify) goo_canvas_item_free_animation);

  anim->timeout_id = g_timeout_add (step_time,
				    (GSourceFunc) goo_canvas_item_animate_cb,
				    anim);
}


/**
 * goo_canvas_item_animate:
 * @item: an item.
 * @x: the final x coordinate.
 * @y: the final y coordinate.
 * @scale: the final scale.
 * @degrees: the final rotation. This can be negative to rotate anticlockwise,
 *  and can also be greater than 360 to rotate a number of times.
 * @absolute: if the @x, @y, @scale and @degrees values are absolute, or
 *  relative to the current transform. Note that absolute animations only work
 *  if the item currently has a simple transform. If the item has a shear or
 *  some other complicated transform it may result in strange animations.
 * @duration: the duration of the animation, in milliseconds (1/1000ths of a
 *  second).
 * @step_time: the time between each animation step, in milliseconds.
 * @type: specifies what happens when the animation finishes.
 * 
 * Animates an item from its current position to the given offsets, scale
 * and rotation.
 **/
void
goo_canvas_item_animate        (GooCanvasItem *item,
				gdouble        x,
				gdouble        y,
				gdouble        scale,
				gdouble        degrees,
				gboolean       absolute,
				gint           duration,
				gint           step_time,
				GooCanvasAnimateType type)
{
  _goo_canvas_item_animate_internal (item, NULL, x, y, scale, degrees,
				     absolute, duration, step_time, type);
}


/**
 * goo_canvas_item_stop_animation:
 * @item: an item.
 * 
 * Stops any current animation for the given item, leaving it at its current
 * position.
 **/
void
goo_canvas_item_stop_animation (GooCanvasItem *item)
{
  /* This will result in a call to goo_canvas_item_free_animation() above. */
  g_object_set_data (G_OBJECT (item), animation_key, NULL);
}




/**
 * goo_canvas_item_request_update:
 * @item: a #GooCanvasItem.
 * 
 * This function is only intended to be used when implementing new canvas
 * items.
 *
 * It requests that an update of the item is scheduled. It will be performed
 * as soon as the application is idle, and before the canvas is redrawn.
 **/
void
goo_canvas_item_request_update  (GooCanvasItem *item)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (iface->request_update)
    return iface->request_update (item);
  else
    return goo_canvas_item_request_update (iface->get_parent (item));
}


/**
 * goo_canvas_item_get_bounds:
 * @item: a #GooCanvasItem.
 * @bounds: a #GooCanvasBounds to return the bounds in.
 * 
 * Gets the bounds of the item.
 *
 * Note that the bounds includes the entire fill and stroke extents of the
 * item, whether they are painted or not.
 **/
void
goo_canvas_item_get_bounds  (GooCanvasItem   *item,
			     GooCanvasBounds *bounds)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  iface->get_bounds (item, bounds);
}


/**
 * goo_canvas_item_get_items_at:
 * @item: a #GooCanvasItem.
 * @x: the x coordinate of the point.
 * @y: the y coordinate of the point.
 * @cr: a cairo contect.
 * @is_pointer_event: %TRUE if the "pointer-events" properties of items should
 *  be used to determine which parts of the item are tested.
 * @parent_is_visible: %TRUE if the parent item is visible (which
 *  implies that all ancestors are also visible).
 * @found_items: the list of items found so far.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically container items such as #GooCanvasGroup.
 *
 * It gets the items at the given point.
 * 
 * Returns: the @found_items list, with any more found items added onto
 *  the start of the list, leaving the top item first.
 **/
GList*
goo_canvas_item_get_items_at (GooCanvasItem  *item,
			      gdouble         x,
			      gdouble         y,
			      cairo_t        *cr,
			      gboolean        is_pointer_event,
			      gboolean        parent_is_visible,
			      GList          *found_items)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (iface->get_items_at)
    return iface->get_items_at (item, x, y, cr, is_pointer_event,
				parent_is_visible, found_items);
  else
    return found_items;
}


/**
 * goo_canvas_item_is_visible:
 * @item: a #GooCanvasItem.
 * 
 * Checks if the item is visible.
 *
 * This entails checking the item's own visibility setting, as well as those
 * of its ancestors.
 *
 * Note that the item may be scrolled off the screen and so may not
 * be actually visible to the user.
 * 
 * Returns: %TRUE if the item is visible.
 **/
gboolean
goo_canvas_item_is_visible  (GooCanvasItem   *item)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  /* If the item doesn't implement this method assume it is visible. */
  return iface->is_visible ? iface->is_visible (item) : TRUE;
}


/**
 * goo_canvas_item_get_model:
 * @item: a #GooCanvasItem.
 * 
 * Gets the model of the given canvas item.
 * 
 * Returns: the item's model, or %NULL if it has no model.
 **/
GooCanvasItemModel*
goo_canvas_item_get_model	  (GooCanvasItem   *item)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  return iface->get_model ? iface->get_model (item) : NULL;
}


/**
 * goo_canvas_item_set_model:
 * @item: a #GooCanvasItem.
 * @model: a #GooCanvasItemModel.
 * 
 * Sets the model of the given canvas item.
 **/
void
goo_canvas_item_set_model	  (GooCanvasItem      *item,
				   GooCanvasItemModel *model)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (iface->set_model)
    iface->set_model (item, model);
}


/**
 * goo_canvas_item_ensure_updated:
 * @item: a #GooCanvasItem.
 * 
 * This function is only intended to be used when implementing new canvas
 * items.
 *
 * It updates the canvas immediately, if an update is scheduled.
 * This ensures that all item bounds are up-to-date.
 **/
void
goo_canvas_item_ensure_updated (GooCanvasItem *item)
{
  GooCanvas *canvas;

  canvas = goo_canvas_item_get_canvas (item);
  if (canvas)
    goo_canvas_update (canvas);
}


/**
 * goo_canvas_item_update:
 * @item: a #GooCanvasItem.
 * @entire_tree: if the entire subtree should be updated.
 * @cr: a cairo context.
 * @bounds: a #GooCanvasBounds to return the new bounds in.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically container items such as #GooCanvasGroup.
 *
 * Updates the item, if needed, and any children.
 **/
void
goo_canvas_item_update      (GooCanvasItem   *item,
			     gboolean         entire_tree,
			     cairo_t         *cr,
			     GooCanvasBounds *bounds)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  iface->update (item, entire_tree, cr, bounds);
}


/**
 * goo_canvas_item_paint:
 * @item: a #GooCanvasItem.
 * @cr: a cairo context.
 * @bounds: the bounds that need to be repainted.
 * @scale: the scale to use to determine whether an item should be painted.
 *  See #GooCanvasItem:visibility-threshold.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically container items such as #GooCanvasGroup.
 *
 * It paints the item and all children if they intersect the given bounds.
 *
 * Note that the @scale argument may be different to the current scale in the
 * #GooCanvasItem, e.g. when the canvas is being printed.
 **/
void
goo_canvas_item_paint (GooCanvasItem         *item,
		       cairo_t               *cr,
		       const GooCanvasBounds *bounds,
		       gdouble                scale)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  iface->paint (item, cr, bounds, scale);
}


/**
 * goo_canvas_item_get_requested_area:
 * @item: a #GooCanvasItem.
 * @cr: a cairo context.
 * @requested_area: a #GooCanvasBounds to return the requested area in, in the
 *  parent's coordinate space.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout items such as #GooCanvasTable.
 *
 * It gets the requested area of a child item.
 * 
 * Returns: %TRUE if the item should be allocated space.
 **/
gboolean
goo_canvas_item_get_requested_area (GooCanvasItem    *item,
				    cairo_t          *cr,
				    GooCanvasBounds  *requested_area)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  return iface->get_requested_area (item, cr, requested_area);
}


/**
 * goo_canvas_item_get_requested_height:
 * @item: a #GooCanvasItem.
 * @cr: a cairo context.
 * @width: the width that the item may be allocated.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout items such as #GooCanvasTable.
 *
 * It gets the requested height of a child item, assuming it is allocated the
 * given width. This is useful for text items whose requested height may change
 * depending on the allocated width.
 * 
 * Returns: the requested height of the item, given the allocated width,
 *  or %-1 if the item doesn't support this method or its height doesn't
 *  change when allocated different widths.
 **/
gdouble
goo_canvas_item_get_requested_height (GooCanvasItem       *item,
				      cairo_t		  *cr,
				      gdouble              width)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  if (iface->get_requested_height)
    return iface->get_requested_height (item, cr, width);
  else
    return -1;
}


/**
 * goo_canvas_item_allocate_area:
 * @item: a #GooCanvasItem.
 * @cr: a cairo context.
 * @requested_area: the area that the item originally requested, in the
 *  parent's coordinate space.
 * @allocated_area: the area that the item has been allocated, in the parent's
 *  coordinate space.
 * @x_offset: the x offset of the allocated area from the requested area in
 *  the device coordinate space.
 * @y_offset: the y offset of the allocated area from the requested area in
 *  the device coordinate space.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout items such as #GooCanvasTable.
 *
 * It allocates an area to a child #GooCanvasItem.
 *
 * Note that the parent layout item will use a transform to move each of its
 * children for the layout, so there is no need for the child item to
 * reposition itself. It only needs to recalculate its device bounds.
 *
 * To help recalculate the item's device bounds, the @x_offset and @y_offset
 * of the child item's allocated position from its requested position are
 * provided. Simple items can just add these to their bounds.
 **/
void
goo_canvas_item_allocate_area      (GooCanvasItem         *item,
				    cairo_t               *cr,
				    const GooCanvasBounds *requested_area,
				    const GooCanvasBounds *allocated_area,
				    gdouble                x_offset,
				    gdouble                y_offset)
{
  GooCanvasItemIface *iface = GOO_CANVAS_ITEM_GET_IFACE (item);

  iface->allocate_area (item, cr, requested_area, allocated_area,
			x_offset, y_offset);
}


/*
 * Child Properties.
 */
void
_goo_canvas_item_get_child_properties_internal (GObject              *object,
						GObject              *child,
						va_list	              var_args,
						GParamSpecPool       *property_pool,
						GObjectNotifyContext *notify_context,
						gboolean              is_model)
{
  g_object_ref (object);
  g_object_ref (child);

  for (;;)
    {
      GObjectClass *class;
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *name, *error = NULL;

      name = va_arg (var_args, gchar*);
      if (!name)
	break;

      pspec = g_param_spec_pool_lookup (property_pool, name,
					G_OBJECT_TYPE (object), TRUE);
      if (!pspec)
	{
	  g_warning ("%s: class `%s' has no child property named `%s'",
		     G_STRLOC, G_OBJECT_TYPE_NAME (object), name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: child property `%s' of class `%s' is not readable",
		     G_STRLOC, pspec->name, G_OBJECT_TYPE_NAME (object));
	  break;
	}
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

      class = g_type_class_peek (pspec->owner_type);

      if (is_model)
	{
	  GooCanvasItemModelIface *iface;

	  iface = g_type_interface_peek (class, GOO_TYPE_CANVAS_ITEM_MODEL);
	  iface->get_child_property ((GooCanvasItemModel*) object,
				     (GooCanvasItemModel*) child,
				     pspec->param_id, &value, pspec);
	}
      else
	{
	  GooCanvasItemIface *iface;

	  iface = g_type_interface_peek (class, GOO_TYPE_CANVAS_ITEM);
	  iface->get_child_property ((GooCanvasItem*) object,
				     (GooCanvasItem*) child,
				     pspec->param_id, &value, pspec);
	}

      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  g_value_unset (&value);
	  break;
	}
      g_value_unset (&value);
    }

  g_object_unref (child);
  g_object_unref (object);
}


static inline void
canvas_item_set_child_property (GObject            *object,
				GObject            *child,
				GParamSpec         *pspec,
				const GValue       *value,
				GObjectNotifyQueue *nqueue,
				gboolean            is_model)
{
  GValue tmp_value = { 0, };

  /* provide a copy to work from, convert (if necessary) and validate */
  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (!g_value_transform (value, &tmp_value))
    g_warning ("unable to set child property `%s' of type `%s' from value of type `%s'",
	       pspec->name,
	       g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
	       G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
    {
      gchar *contents = g_strdup_value_contents (value);

      g_warning ("value \"%s\" of type `%s' is invalid for property `%s' of type `%s'",
		 contents,
		 G_VALUE_TYPE_NAME (value),
		 pspec->name,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      g_free (contents);
    }
  else
    {
      GObjectClass *class = g_type_class_peek (pspec->owner_type);

      if (is_model)
	{
	  GooCanvasItemModelIface *iface;

	  iface = g_type_interface_peek (class, GOO_TYPE_CANVAS_ITEM_MODEL);
	  iface->set_child_property ((GooCanvasItemModel*) object,
				     (GooCanvasItemModel*) child,
				     pspec->param_id, &tmp_value, pspec);
	}
      else
	{
	  GooCanvasItemIface *iface;

	  iface = g_type_interface_peek (class, GOO_TYPE_CANVAS_ITEM);
	  iface->set_child_property ((GooCanvasItem*) object,
				     (GooCanvasItem*) child,
				     pspec->param_id, &tmp_value, pspec);
	}

      g_object_notify_queue_add (G_OBJECT (child), nqueue, pspec);
    }
  g_value_unset (&tmp_value);
}


void
_goo_canvas_item_set_child_properties_internal (GObject              *object,
						GObject              *child,
						va_list	              var_args,
						GParamSpecPool       *property_pool,
						GObjectNotifyContext *notify_context,
						gboolean              is_model)
{
  GObjectNotifyQueue *nqueue;

  g_object_ref (object);
  g_object_ref (child);

  nqueue = g_object_notify_queue_freeze (child, notify_context);

  for (;;)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *name, *error = NULL;

      name = va_arg (var_args, gchar*);
      if (!name)
	break;

      pspec = g_param_spec_pool_lookup (property_pool, name,
					G_OBJECT_TYPE (object), TRUE);
      if (!pspec)
	{
	  g_warning ("%s: class `%s' has no child property named `%s'",
		     G_STRLOC, G_OBJECT_TYPE_NAME (object), name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: child property `%s' of class `%s' is not writable",
		     G_STRLOC, pspec->name, G_OBJECT_TYPE_NAME (object));
	  break;
	}
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      canvas_item_set_child_property (object, child, pspec, &value, nqueue,
				      is_model);
      g_value_unset (&value);
    }
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);

  g_object_unref (object);
  g_object_unref (child);
}


/**
 * goo_canvas_item_get_child_properties_valist:
 * @item: a #GooCanvasItem.
 * @child: a child #GooCanvasItem.
 * @var_args: pairs of property names and value pointers, and a terminating
 *  %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout container items such as #GooCanvasTable.
 *
 * It gets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_get_child_properties_valist (GooCanvasItem   *item,
					     GooCanvasItem   *child,
					     va_list	      var_args)
{
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (GOO_IS_CANVAS_ITEM (child));

  _goo_canvas_item_get_child_properties_internal ((GObject*) item, (GObject*) child, var_args, _goo_canvas_item_child_property_pool, _goo_canvas_item_child_property_notify_context, FALSE);
}


/**
 * goo_canvas_item_set_child_properties_valist:
 * @item: a #GooCanvasItem.
 * @child: a child #GooCanvasItem.
 * @var_args: pairs of property names and values, and a terminating %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout container items such as #GooCanvasTable.
 *
 * It sets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_set_child_properties_valist (GooCanvasItem   *item,
					     GooCanvasItem   *child,
					     va_list	      var_args)
{
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (GOO_IS_CANVAS_ITEM (child));

  _goo_canvas_item_set_child_properties_internal ((GObject*) item, (GObject*) child, var_args, _goo_canvas_item_child_property_pool, _goo_canvas_item_child_property_notify_context, FALSE);
}


/**
 * goo_canvas_item_get_child_properties:
 * @item: a #GooCanvasItem.
 * @child: a child #GooCanvasItem.
 * @...: pairs of property names and value pointers, and a terminating %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout container items such as #GooCanvasTable.
 *
 * It gets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_get_child_properties        (GooCanvasItem   *item,
					     GooCanvasItem   *child,
					     ...)
{
  va_list var_args;
  
  va_start (var_args, child);
  goo_canvas_item_get_child_properties_valist (item, child, var_args);
  va_end (var_args);
}


/**
 * goo_canvas_item_set_child_properties:
 * @item: a #GooCanvasItem.
 * @child: a child #GooCanvasItem.
 * @...: pairs of property names and values, and a terminating %NULL.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout container items such as #GooCanvasTable.
 *
 * It sets the values of one or more child properties of @child.
 **/
void
goo_canvas_item_set_child_properties        (GooCanvasItem   *item,
					     GooCanvasItem   *child,
					     ...)
{
  va_list var_args;
  
  va_start (var_args, child);
  goo_canvas_item_set_child_properties_valist (item, child, var_args);
  va_end (var_args);
}



/**
 * goo_canvas_item_class_install_child_property:
 * @iclass: a #GObjectClass
 * @property_id: the id for the property
 * @pspec: the #GParamSpec for the property
 * 
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout container items such as #GooCanvasTable.
 *
 * It installs a child property on a canvas item class. 
 **/
void
goo_canvas_item_class_install_child_property (GObjectClass *iclass,
					      guint         property_id,
					      GParamSpec   *pspec)
{
  g_return_if_fail (G_IS_OBJECT_CLASS (iclass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (property_id > 0);

  if (g_param_spec_pool_lookup (_goo_canvas_item_child_property_pool,
				pspec->name, G_OBJECT_CLASS_TYPE (iclass),
				FALSE))
    {
      g_warning (G_STRLOC ": class `%s' already contains a child property named `%s'",
		 G_OBJECT_CLASS_NAME (iclass), pspec->name);
      return;
    }
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  pspec->param_id = property_id;
  g_param_spec_pool_insert (_goo_canvas_item_child_property_pool, pspec,
			    G_OBJECT_CLASS_TYPE (iclass));
}

/**
 * goo_canvas_item_class_find_child_property:
 * @iclass: a #GObjectClass
 * @property_name: the name of the child property to find
 * @returns: the #GParamSpec of the child property or %NULL if @class has no
 *   child property with that name.
 *
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout container items such as #GooCanvasTable.
 *
 * It finds a child property of a canvas item class by name.
 */
GParamSpec*
goo_canvas_item_class_find_child_property (GObjectClass *iclass,
					   const gchar  *property_name)
{
  g_return_val_if_fail (G_IS_OBJECT_CLASS (iclass), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (_goo_canvas_item_child_property_pool,
				   property_name, G_OBJECT_CLASS_TYPE (iclass),
				   TRUE);
}

/**
 * goo_canvas_item_class_list_child_properties:
 * @iclass: a #GObjectClass
 * @n_properties: location to return the number of child properties found
 * @returns: a newly allocated array of #GParamSpec*. The array must be 
 *           freed with g_free().
 *
 * This function is only intended to be used when implementing new canvas
 * items, specifically layout container items such as #GooCanvasTable.
 *
 * It returns all child properties of a canvas item class.
 */
GParamSpec**
goo_canvas_item_class_list_child_properties (GObjectClass *iclass,
					     guint        *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (G_IS_OBJECT_CLASS (iclass), NULL);

  pspecs = g_param_spec_pool_list (_goo_canvas_item_child_property_pool,
				   G_OBJECT_CLASS_TYPE (iclass), &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}
