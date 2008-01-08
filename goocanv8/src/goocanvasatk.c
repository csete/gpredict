/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Copyright 2001 Sun Microsystems Inc.
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasatk.c - the accessibility code. Most of this has been ported from
 *                  the gail/libgnomecanvas & foocanvas code.
 */
#include <config.h>
#include <math.h>
#include <gtk/gtk.h>
#include "goocanvas.h"


/*
 * GooCanvasItemAccessible.
 */

typedef AtkGObjectAccessible      GooCanvasItemAccessible;
typedef AtkGObjectAccessibleClass GooCanvasItemAccessibleClass;

#define GOO_IS_CANVAS_ITEM_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), goo_canvas_item_accessible_get_type ()))

static void goo_canvas_item_accessible_component_interface_init (AtkComponentIface *iface);
static gint goo_canvas_item_accessible_get_index_in_parent (AtkObject *accessible);

G_DEFINE_TYPE_WITH_CODE (GooCanvasItemAccessible,
			 goo_canvas_item_accessible,
			 ATK_TYPE_GOBJECT_ACCESSIBLE,
			 G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, goo_canvas_item_accessible_component_interface_init))


/* Returns the extents of the item, in pixels relative to the main
   canvas window. */
static void
goo_canvas_item_accessible_get_item_extents (GooCanvasItem *item,
					     GdkRectangle  *rect)
{
  GooCanvas *canvas;
  GooCanvasBounds bounds;

  canvas = goo_canvas_item_get_canvas (item);
  if (!canvas)
    {
      rect->x = rect->y = rect->width = rect->height = 0;
      return;
    }

  /* Get the bounds in device units. */
  goo_canvas_item_get_bounds (item, &bounds);

  /* Convert to pixels within the entire canvas. */
  goo_canvas_convert_to_pixels (canvas, &bounds.x1, &bounds.y1);
  goo_canvas_convert_to_pixels (canvas, &bounds.x2, &bounds.y2);

  /* Convert to pixels within the visible window. */
  bounds.x1 -= canvas->hadjustment->value;
  bounds.y1 -= canvas->vadjustment->value;
  bounds.x2 -= canvas->hadjustment->value;
  bounds.y2 -= canvas->vadjustment->value;

  /* Round up or down to integers. */
  rect->x = floor (bounds.x1);
  rect->y = floor (bounds.y1);
  rect->width = ceil (bounds.x1) - rect->x;
  rect->height = ceil (bounds.y1) - rect->y;
}


/* This returns TRUE if the given rectangle intersects the canvas window.
   The rectangle should be in pixels relative to the main canvas window. */
static gboolean
goo_canvas_item_accessible_is_item_in_window (GooCanvasItem *item,
					      GdkRectangle  *rect)
{
  GtkWidget *widget;

  widget = (GtkWidget*) goo_canvas_item_get_canvas (item);
  if (!widget)
    return FALSE;

  if (rect->x + rect->width < 0 || rect->x > widget->allocation.width
      || rect->y + rect->height < 0 || rect->y > widget->allocation.height)
    return FALSE;

  return TRUE;
}


static gboolean
goo_canvas_item_accessible_is_item_on_screen (GooCanvasItem *item)
{
  GdkRectangle rect;

  goo_canvas_item_accessible_get_item_extents (item, &rect);
  return goo_canvas_item_accessible_is_item_in_window (item, &rect);
}



static void
goo_canvas_item_accessible_get_extents (AtkComponent *component,
					gint         *x,
					gint         *y,
					gint         *width,
					gint         *height,
					AtkCoordType  coord_type)
{
  GooCanvasItem *item;
  GooCanvas *canvas;
  GObject *object;
  gint window_x, window_y;
  gint toplevel_x, toplevel_y;
  GdkRectangle rect;
  GdkWindow *window;

  g_return_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (component));

  *x = *y = G_MININT;

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (component));
  if (!object)
    return;

  item = GOO_CANVAS_ITEM (object);

  canvas = goo_canvas_item_get_canvas (item);
  if (!canvas || !GTK_WIDGET (canvas)->window)
    return;

  goo_canvas_item_accessible_get_item_extents (item, &rect);
  *width = rect.width;
  *height = rect.height;

  if (!goo_canvas_item_accessible_is_item_in_window (item, &rect))
    return;

  gdk_window_get_origin (GTK_WIDGET (canvas)->window,
			 &window_x, &window_y);
  *x = rect.x + window_x;
  *y = rect.y + window_y;

  if (coord_type == ATK_XY_WINDOW)
    {
      window = gdk_window_get_toplevel (GTK_WIDGET (canvas)->window);
      gdk_window_get_origin (window, &toplevel_x, &toplevel_y);
      *x -= toplevel_x;
      *y -= toplevel_y;
    }
}


static gint
goo_canvas_item_accessible_get_mdi_zorder (AtkComponent *component)
{
  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (component), -1);

  return goo_canvas_item_accessible_get_index_in_parent (ATK_OBJECT (component));
}


static guint
goo_canvas_item_accessible_add_focus_handler (AtkComponent    *component,
					      AtkFocusHandler  handler)
{
  GSignalMatchType match_type;
  GClosure *closure;
  guint signal_id;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (component), 0);

  match_type = G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC;
  signal_id = g_signal_lookup ("focus-event", ATK_TYPE_OBJECT);

  /* If the handler has already been added just return. */
  if (g_signal_handler_find (component, match_type, signal_id, 0, NULL,
			     (gpointer) handler, NULL))
    return 0;

  closure = g_cclosure_new (G_CALLBACK (handler), NULL, (GClosureNotify) NULL);
  return g_signal_connect_closure_by_id (component, signal_id, 0, closure,
					 FALSE);
}


static void
goo_canvas_item_accessible_remove_focus_handler (AtkComponent *component,
						 guint         handler_id)
{
  g_return_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (component));

  g_signal_handler_disconnect (ATK_OBJECT (component), handler_id);
}


static gboolean
goo_canvas_item_accessible_grab_focus (AtkComponent    *component)
{
  GooCanvasItem *item;
  GooCanvas *canvas;
  GtkWidget *toplevel;
  GObject *object;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (component), FALSE);

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (component));
  if (!object)
    return FALSE;

  item = GOO_CANVAS_ITEM (object);

  canvas = goo_canvas_item_get_canvas (item);
  if (!canvas)
    return FALSE;

  goo_canvas_grab_focus (canvas, item);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (canvas));
  if (GTK_WIDGET_TOPLEVEL (toplevel))
    gtk_window_present (GTK_WINDOW (toplevel));

  return TRUE;
}


static void
goo_canvas_item_accessible_component_interface_init (AtkComponentIface *iface)
{
  iface->get_extents          = goo_canvas_item_accessible_get_extents;
  iface->get_mdi_zorder       = goo_canvas_item_accessible_get_mdi_zorder;
  iface->add_focus_handler    = goo_canvas_item_accessible_add_focus_handler;
  iface->remove_focus_handler = goo_canvas_item_accessible_remove_focus_handler;
  iface->grab_focus           = goo_canvas_item_accessible_grab_focus;
}


static void
goo_canvas_item_accessible_initialize (AtkObject *object,
				       gpointer   data)
{
  if (ATK_OBJECT_CLASS (goo_canvas_item_accessible_parent_class)->initialize)
    ATK_OBJECT_CLASS (goo_canvas_item_accessible_parent_class)->initialize (object, data);

  object->role = ATK_ROLE_UNKNOWN;

  /* FIXME: Maybe this should be ATK_LAYER_CANVAS. */
  g_object_set_data (G_OBJECT (object), "atk-component-layer",
		     GINT_TO_POINTER (ATK_LAYER_MDI));
}


static AtkObject*
goo_canvas_item_accessible_get_parent (AtkObject *accessible)
{
  GooCanvasItem *item, *parent;
  GooCanvas *canvas;
  GObject *object;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (accessible), NULL);

  if (accessible->accessible_parent)
    return accessible->accessible_parent;

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (accessible));
  if (object == NULL)
    return NULL;

  item = GOO_CANVAS_ITEM (object);
  parent = goo_canvas_item_get_parent (item);

  if (parent)
    return atk_gobject_accessible_for_object (G_OBJECT (parent));

  canvas = goo_canvas_item_get_canvas (item);
  if (canvas)
    return gtk_widget_get_accessible (GTK_WIDGET (canvas));

  return NULL;
}


static gint
goo_canvas_item_accessible_get_index_in_parent (AtkObject *accessible)
{
  GooCanvasItem *item, *parent;
  GooCanvas *canvas;
  GObject *object;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (accessible), -1);

  if (accessible->accessible_parent)
    {
      gint n_children, i;
      gboolean found = FALSE;

      n_children = atk_object_get_n_accessible_children (accessible->accessible_parent);
      for (i = 0; i < n_children; i++)
        {
          AtkObject *child;

          child = atk_object_ref_accessible_child (accessible->accessible_parent, i);
          if (child == accessible)
            found = TRUE;

          g_object_unref (child);
          if (found)
            return i;
        }
      return -1;
    }

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (accessible));
  if (object == NULL)
    return -1;

  item = GOO_CANVAS_ITEM (object);
  parent = goo_canvas_item_get_parent (item);

  if (parent)
    return goo_canvas_item_find_child (parent, item);

  canvas = goo_canvas_item_get_canvas (item);
  if (canvas)
    return 0;

  return -1;
}


static gint
goo_canvas_item_accessible_get_n_children (AtkObject *accessible)
{
  GooCanvasItem *item;
  GObject *object;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (accessible), 0);

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (accessible));
  if (object == NULL)
    return 0;

  item = GOO_CANVAS_ITEM (object);

  return goo_canvas_item_get_n_children (item);
}


static AtkObject*
goo_canvas_item_accessible_ref_child (AtkObject *accessible,
				      gint       child_num)
{
  GooCanvasItem *item, *child;
  AtkObject *atk_object;
  GObject *object;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (accessible), NULL);

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (accessible));
  if (object == NULL)
    return NULL;

  item = GOO_CANVAS_ITEM (object);

  child = goo_canvas_item_get_child (item, child_num);
  if (!child)
    return NULL;

  atk_object = atk_gobject_accessible_for_object (G_OBJECT (child));
  g_object_ref (atk_object);

  return atk_object;
}


static AtkStateSet*
goo_canvas_item_accessible_ref_state_set (AtkObject *accessible)
{
  GooCanvasItem *item;
  GooCanvas *canvas;
  AtkStateSet *state_set;
  GObject *object;
  gboolean can_focus = FALSE;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_ACCESSIBLE (accessible), NULL);

  state_set = ATK_OBJECT_CLASS (goo_canvas_item_accessible_parent_class)->ref_state_set (accessible);

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (accessible));
  if (!object)
    {
      atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);
      return state_set;
    }

  item = GOO_CANVAS_ITEM (object);

  canvas = goo_canvas_item_get_canvas (item);
  if (!canvas)
    return state_set;

  if (goo_canvas_item_is_visible (item))
    {
      atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);

      if (goo_canvas_item_accessible_is_item_on_screen (item))
	atk_state_set_add_state (state_set, ATK_STATE_SHOWING);
    }

  g_object_get (item, "can-focus", &can_focus, NULL);

  if (GTK_WIDGET_CAN_FOCUS (GTK_WIDGET (canvas)) && can_focus)
    {
      atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);

      if (GTK_WIDGET_HAS_FOCUS (canvas)
	  && canvas->focused_item == item)
	atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);
    }

  return state_set;
}


static void
goo_canvas_item_accessible_class_init (GooCanvasItemAccessibleClass *klass)
{
  AtkObjectClass *aklass = (AtkObjectClass*) klass;

  aklass->initialize          = goo_canvas_item_accessible_initialize;
  aklass->get_parent          = goo_canvas_item_accessible_get_parent;
  aklass->get_index_in_parent = goo_canvas_item_accessible_get_index_in_parent;
  aklass->get_n_children      = goo_canvas_item_accessible_get_n_children;
  aklass->ref_child           = goo_canvas_item_accessible_ref_child;
  aklass->ref_state_set       = goo_canvas_item_accessible_ref_state_set;
}


static void
goo_canvas_item_accessible_init (GooCanvasItemAccessible *accessible)
{
}


static AtkObject *
goo_canvas_item_accessible_new (GObject *object)
{
  AtkObject *accessible;

  g_return_val_if_fail (GOO_IS_CANVAS_ITEM (object), NULL);

  accessible = g_object_new (goo_canvas_item_accessible_get_type (), NULL);
  atk_object_initialize (accessible, object);

  return accessible;
}


/*
 * GooCanvasItemAccessibleFactory.
 */

typedef AtkObjectFactory      GooCanvasItemAccessibleFactory;
typedef AtkObjectFactoryClass GooCanvasItemAccessibleFactoryClass;

G_DEFINE_TYPE (GooCanvasItemAccessibleFactory,
	       goo_canvas_item_accessible_factory,
	       ATK_TYPE_OBJECT_FACTORY)

static void
goo_canvas_item_accessible_factory_class_init (GooCanvasItemAccessibleFactoryClass *klass)
{
  klass->create_accessible   = goo_canvas_item_accessible_new;
  klass->get_accessible_type = goo_canvas_item_accessible_get_type;
}

static void
goo_canvas_item_accessible_factory_init (GooCanvasItemAccessibleFactory *factory)
{
}



/*
 * GooCanvasWidgetAccessible.
 */

typedef AtkGObjectAccessible      GooCanvasWidgetAccessible;
typedef AtkGObjectAccessibleClass GooCanvasWidgetAccessibleClass;

#define GOO_IS_CANVAS_WIDGET_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), goo_canvas_widget_accessible_get_type ()))

G_DEFINE_TYPE (GooCanvasWidgetAccessible, goo_canvas_widget_accessible,
	       GOO_TYPE_CANVAS_ITEM)


static void
goo_canvas_widget_accessible_initialize (AtkObject *object,
					 gpointer   data)
{
  if (ATK_OBJECT_CLASS (goo_canvas_widget_accessible_parent_class)->initialize)
    ATK_OBJECT_CLASS (goo_canvas_widget_accessible_parent_class)->initialize (object, data);

  object->role = ATK_ROLE_PANEL;
}


static gint 
goo_canvas_widget_accessible_get_n_children (AtkObject *accessible)
{
  GooCanvasWidget *witem;
  GObject *object;

  g_return_val_if_fail (GOO_IS_CANVAS_WIDGET_ACCESSIBLE (accessible), 0);

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (accessible));
  if (object == NULL)
    return 0;

  witem = GOO_CANVAS_WIDGET (object);

  return witem->widget ? 1 : 0;
}

static AtkObject *
goo_canvas_widget_accessible_ref_child (AtkObject *accessible,
					gint       child_num)
{
  GooCanvasWidget *witem;
  AtkObject *atk_object;
  GObject *object;

  g_return_val_if_fail (GOO_IS_CANVAS_WIDGET_ACCESSIBLE (accessible), NULL);

  if (child_num != 0)
    return NULL;

  object = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (accessible));
  if (object == NULL)
    return NULL;

  g_return_val_if_fail (GOO_IS_CANVAS_WIDGET (object), NULL);

  witem = GOO_CANVAS_WIDGET (object);

  if (!witem->widget)
    return NULL;

  atk_object = gtk_widget_get_accessible (witem->widget);
  g_object_ref (atk_object);

  return atk_object;
}


static void
goo_canvas_widget_accessible_class_init (GooCanvasWidgetAccessibleClass *klass)
{
  AtkObjectClass *aklass = (AtkObjectClass*) klass;

  aklass->initialize     = goo_canvas_widget_accessible_initialize;
  aklass->get_n_children = goo_canvas_widget_accessible_get_n_children;
  aklass->ref_child      = goo_canvas_widget_accessible_ref_child;
}


static void
goo_canvas_widget_accessible_init (GooCanvasWidgetAccessible *accessible)
{
}


AtkObject*
goo_canvas_widget_accessible_new (GObject *object)
{
  AtkObject *accessible;

  g_return_val_if_fail (GOO_IS_CANVAS_WIDGET (object), NULL);

  accessible = g_object_new (goo_canvas_widget_accessible_get_type (), NULL);
  atk_object_initialize (accessible, object);

  return accessible;
}


/*
 * GooCanvasWidgetAccessibleFactory.
 */

typedef AtkObjectFactory      GooCanvasWidgetAccessibleFactory;
typedef AtkObjectFactoryClass GooCanvasWidgetAccessibleFactoryClass;

G_DEFINE_TYPE (GooCanvasWidgetAccessibleFactory,
	       goo_canvas_widget_accessible_factory,
	       ATK_TYPE_OBJECT_FACTORY)

static void
goo_canvas_widget_accessible_factory_class_init (GooCanvasWidgetAccessibleFactoryClass *klass)
{
  klass->create_accessible   = goo_canvas_widget_accessible_new;
  klass->get_accessible_type = goo_canvas_widget_accessible_get_type;
}

static void
goo_canvas_widget_accessible_factory_init (GooCanvasWidgetAccessibleFactory *factory)
{
}


/*
 * GooCanvasAccessible.
 */

static gpointer accessible_parent_class = NULL;


static void
goo_canvas_accessible_initialize (AtkObject *object, 
				  gpointer   data)
{
  if (ATK_OBJECT_CLASS (accessible_parent_class)->initialize) 
    ATK_OBJECT_CLASS (accessible_parent_class)->initialize (object, data);

  /* FIXME: Maybe this should be ATK_ROLE_CANVAS. */
  object->role = ATK_ROLE_LAYERED_PANE;
}


static gint
goo_canvas_accessible_get_n_children (AtkObject *object)
{
  GtkAccessible *accessible;
  GtkWidget *widget;

  accessible = GTK_ACCESSIBLE (object);
  widget = accessible->widget;

  /* Check if widget still exists. */
  if (widget == NULL)
    return 0;

  g_return_val_if_fail (GOO_IS_CANVAS (widget), 0);

  if (goo_canvas_get_root_item (GOO_CANVAS (widget)))
    return 1;

  return 0;
}

static AtkObject*
goo_canvas_accessible_ref_child (AtkObject *object,
				 gint       child_num)
{
  GtkAccessible *accessible;
  GtkWidget *widget;
  GooCanvasItem *root;
  AtkObject *atk_object;

  /* Canvas only has one child, so return NULL if index is non zero */
  if (child_num != 0)
    return NULL;

  accessible = GTK_ACCESSIBLE (object);
  widget = accessible->widget;

  /* Check if widget still exists. */
  if (widget == NULL)
    return NULL;

  root = goo_canvas_get_root_item (GOO_CANVAS (widget));
  if (!root)
    return NULL;

  atk_object = atk_gobject_accessible_for_object (G_OBJECT (root));
  g_object_ref (atk_object);

  return atk_object;
}



static void
goo_canvas_accessible_class_init (AtkObjectClass *klass)
{
  accessible_parent_class = g_type_class_peek_parent (klass);

  klass->initialize     = goo_canvas_accessible_initialize;
  klass->get_n_children = goo_canvas_accessible_get_n_children;
  klass->ref_child      = goo_canvas_accessible_ref_child;
}


static GType
goo_canvas_accessible_get_type (void)
{
  static GType g_define_type_id = 0;

  if (G_UNLIKELY (g_define_type_id == 0))
    {
      AtkObjectFactory *factory;
      GType parent_atk_type;
      GTypeQuery query;
      GTypeInfo tinfo = { 0 };

      /* Gail doesn't declare its classes publicly, so we have to do a query
	 to find the size of the class and instances. */
      factory = atk_registry_get_factory (atk_get_default_registry (),
					  GTK_TYPE_WIDGET);
      if (!factory)
	return G_TYPE_INVALID;

      parent_atk_type = atk_object_factory_get_accessible_type (factory);
      if (!parent_atk_type)
	return G_TYPE_INVALID;

      g_type_query (parent_atk_type, &query);
      tinfo.class_init = (GClassInitFunc) goo_canvas_accessible_class_init;
      tinfo.class_size = query.class_size;
      tinfo.instance_size = query.instance_size;
      g_define_type_id = g_type_register_static (parent_atk_type,
						 "GooCanvasAccessible",
						 &tinfo, 0);
    }

  return g_define_type_id;
}


static AtkObject *
goo_canvas_accessible_new (GObject *object)
{
  AtkObject *accessible;

  g_return_val_if_fail (GOO_IS_CANVAS (object), NULL);

  accessible = g_object_new (goo_canvas_accessible_get_type (), NULL);
  atk_object_initialize (accessible, object);

  return accessible;
}


/*
 * GooCanvasAccessibleFactory.
 */

typedef AtkObjectFactory      GooCanvasAccessibleFactory;
typedef AtkObjectFactoryClass GooCanvasAccessibleFactoryClass;

G_DEFINE_TYPE (GooCanvasAccessibleFactory,
	       goo_canvas_accessible_factory,
	       ATK_TYPE_OBJECT_FACTORY)

static void
goo_canvas_accessible_factory_class_init (GooCanvasAccessibleFactoryClass *klass)
{
  klass->create_accessible   = goo_canvas_accessible_new;
  klass->get_accessible_type = goo_canvas_accessible_get_type;
}

static void
goo_canvas_accessible_factory_init (GooCanvasAccessibleFactory *factory)
{
}
