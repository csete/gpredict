/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvaswidget.c - wrapper item for an embedded GtkWidget.
 */

/**
 * SECTION:goocanvaswidget
 * @Title: GooCanvasWidget
 * @Short_Description: an embedded widget item.
 *
 * GooCanvasWidget provides support for placing any GtkWidget in the canvas.
 *
 * Note that there are a number of limitations in the use of #GooCanvasWidget:
 *
 * <itemizedlist><listitem><para>
 * It doesn't support any transformation besides simple translation.
 * This means you can't scale a canvas with a #GooCanvasWidget in it.
 * </para></listitem><listitem><para>
 * It doesn't support layering, so you can't place other items beneath
 * or above the #GooCanvasWidget.
 * </para></listitem><listitem><para>
 * It doesn't support rendering of widgets to a given cairo_t, which
 * means you can't output the widget to a pdf or postscript file.
 * </para></listitem><listitem><para>
 * It doesn't have a model/view variant like the other standard items,
 * so it can only be used in a simple canvas without a model.
 * </para></listitem></itemizedlist>
 */
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "goocanvas.h"
#include "goocanvasatk.h"

enum {
  PROP_0,

  PROP_WIDGET,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_ANCHOR,
  PROP_VISIBILITY
};


static void canvas_item_interface_init      (GooCanvasItemIface  *iface);
static void goo_canvas_widget_dispose       (GObject             *object);
static void goo_canvas_widget_get_property  (GObject             *object,
					     guint                param_id,
					     GValue              *value,
					     GParamSpec          *pspec);
static void goo_canvas_widget_set_property  (GObject             *object,
					     guint                param_id,
					     const GValue        *value,
					     GParamSpec          *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasWidget, goo_canvas_widget,
			 GOO_TYPE_CANVAS_ITEM_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM,
						canvas_item_interface_init))


static void
goo_canvas_widget_init (GooCanvasWidget *witem)
{
  /* By default we place the widget at the top-left of the canvas at its
     requested size. */
  witem->x = 0.0;
  witem->y = 0.0;
  witem->width = -1.0;
  witem->height = -1.0;
  witem->anchor = GTK_ANCHOR_NW;
}


/**
 * goo_canvas_widget_new:
 * @parent: the parent item, or %NULL. If a parent is specified, it will assume
 *  ownership of the item, and the item will automatically be freed when it is
 *  removed from the parent. Otherwise call g_object_unref() to free it.
 * @widget: the widget.
 * @x: the x coordinate of the item.
 * @y: the y coordinate of the item.
 * @width: the width of the item, or -1 to use the widget's requested width.
 * @height: the height of the item, or -1 to use the widget's requested height.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 *
 * Creates a new widget item.
 *
 * <!--PARAMETERS-->
 * 
 * Here's an example showing how to create an entry widget centered at (100.0,
 * 100.0):
 *
 * <informalexample><programlisting>
 *  GtkWidget *entry = gtk_entry_new ();
 *  GooCanvasItem *witem = goo_canvas_widget_new (mygroup, entry,
 *                                                100, 100, -1, -1,
 *                                                "anchor", GTK_ANCHOR_CENTER,
 *                                                NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new widget item.
 **/
GooCanvasItem*
goo_canvas_widget_new               (GooCanvasItem    *parent,
				     GtkWidget        *widget,
				     gdouble           x,
				     gdouble           y,
				     gdouble           width,
				     gdouble           height,
				     ...)
{
  GooCanvasItem *item;
  GooCanvasWidget *witem;
  const char *first_property;
  va_list var_args;

  item = g_object_new (GOO_TYPE_CANVAS_WIDGET, NULL);
  witem = (GooCanvasWidget*) item;

  witem->widget = widget;
  g_object_ref (witem->widget);
  g_object_set_data (G_OBJECT (witem->widget), "goo-canvas-item", witem);

  witem->x = x;
  witem->y = y;
  witem->width = width;
  witem->height = height;

  /* The widget defaults to being visible, like the canvas item, but this
     can be overridden by the object property below. */
  if (widget)
    gtk_widget_show (widget);

  va_start (var_args, height);
  first_property = va_arg (var_args, char*);
  if (first_property)
    g_object_set_valist ((GObject*) item, first_property, var_args);
  va_end (var_args);

  if (parent)
    {
      goo_canvas_item_add_child (parent, item, -1);
      g_object_unref (item);
    }

  return item;
}


static void
goo_canvas_widget_set_widget (GooCanvasWidget *witem,
			      GtkWidget       *widget)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) witem;

  if (witem->widget)
    {
      g_object_set_data (G_OBJECT (witem->widget), "goo-canvas-item", NULL);
      gtk_widget_unparent (witem->widget);
      g_object_unref (witem->widget);
      witem->widget = NULL;
    }

  if (widget)
    {
      witem->widget = widget;
      g_object_ref (witem->widget);
      g_object_set_data (G_OBJECT (witem->widget), "goo-canvas-item", witem);

      if (simple->simple_data->visibility <= GOO_CANVAS_ITEM_INVISIBLE)
	gtk_widget_hide (widget);
      else
	gtk_widget_show (widget);

      if (simple->canvas)
	{
	  if (GTK_WIDGET_REALIZED (simple->canvas))
	    gtk_widget_set_parent_window (widget,
					  simple->canvas->canvas_window);

	  gtk_widget_set_parent (widget, GTK_WIDGET (simple->canvas));
	}
    }
}


static void
goo_canvas_widget_dispose (GObject *object)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasWidget *witem = (GooCanvasWidget*) object;

  if (simple->canvas)
    goo_canvas_unregister_widget_item (simple->canvas, witem);

  goo_canvas_widget_set_widget (witem, NULL);

  G_OBJECT_CLASS (goo_canvas_widget_parent_class)->dispose (object);
}


static void
goo_canvas_widget_get_property  (GObject             *object,
				 guint                prop_id,
				 GValue              *value,
				 GParamSpec          *pspec)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasWidget *witem = (GooCanvasWidget*) object;

  switch (prop_id)
    {
    case PROP_WIDGET:
      g_value_set_object (value, witem->widget);
      break;
    case PROP_X:
      g_value_set_double (value, witem->x);
      break;
    case PROP_Y:
      g_value_set_double (value, witem->y);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, witem->width);
      break;
    case PROP_HEIGHT:
      g_value_set_double (value, witem->height);
      break;
    case PROP_ANCHOR:
      g_value_set_enum (value, witem->anchor);
      break;
    case PROP_VISIBILITY:
      g_value_set_enum (value, simple->simple_data->visibility);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
goo_canvas_widget_set_property  (GObject             *object,
				 guint                prop_id,
				 const GValue        *value,
				 GParamSpec          *pspec)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasWidget *witem = (GooCanvasWidget*) object;

  switch (prop_id)
    {
    case PROP_WIDGET:
      goo_canvas_widget_set_widget (witem, g_value_get_object (value));
      break;
    case PROP_X:
      witem->x = g_value_get_double (value);
      break;
    case PROP_Y:
      witem->y = g_value_get_double (value);
      break;
    case PROP_WIDTH:
      witem->width = g_value_get_double (value);
      break;
    case PROP_HEIGHT:
      witem->height = g_value_get_double (value);
      break;
    case PROP_ANCHOR:
      witem->anchor = g_value_get_enum (value);
      break;
    case PROP_VISIBILITY:
      simple->simple_data->visibility = g_value_get_enum (value);
      if (simple->simple_data->visibility <= GOO_CANVAS_ITEM_INVISIBLE)
	gtk_widget_hide (witem->widget);
      else
	gtk_widget_show (witem->widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  goo_canvas_item_simple_changed (simple, TRUE);
}


static void
goo_canvas_widget_set_canvas  (GooCanvasItem *item,
			       GooCanvas     *canvas)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasWidget *witem = (GooCanvasWidget*) item;

  if (simple->canvas != canvas)
    {
      if (simple->canvas)
	goo_canvas_unregister_widget_item (simple->canvas, witem);

      simple->canvas = canvas;

      if (simple->canvas)
	{
	  goo_canvas_register_widget_item (simple->canvas, witem);

	  if (witem->widget)
	    {
	      if (GTK_WIDGET_REALIZED (simple->canvas))
		gtk_widget_set_parent_window (witem->widget,
					      simple->canvas->canvas_window);

	      gtk_widget_set_parent (witem->widget,
				     GTK_WIDGET (simple->canvas));
	    }
	}
      else
	{
	  if (witem->widget)
	    gtk_widget_unparent (witem->widget);
	}
    }
}


static void
goo_canvas_widget_set_parent (GooCanvasItem  *item,
			      GooCanvasItem  *parent)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvas *canvas;

  simple->parent = parent;
  simple->need_update = TRUE;
  simple->need_entire_subtree_update = TRUE;

  canvas = parent ? goo_canvas_item_get_canvas (parent) : NULL;
  goo_canvas_widget_set_canvas (item, canvas);
}


static void
goo_canvas_widget_update  (GooCanvasItemSimple *simple,
			   cairo_t             *cr)
{
  GooCanvasWidget *witem = (GooCanvasWidget*) simple;
  GtkRequisition requisition;
  gdouble width, height;

  if (witem->widget)
    {
      /* Compute the new bounds. */
      if (witem->width < 0 || witem->height < 0)
	{
	  gtk_widget_size_request (witem->widget, &requisition);
	}

      simple->bounds.x1 = witem->x;
      simple->bounds.y1 = witem->y;
      width = witem->width < 0 ? requisition.width : witem->width;
      height = witem->height < 0 ? requisition.height : witem->height;

      switch (witem->anchor)
	{
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
	  simple->bounds.x1 -= width / 2.0;
	  break;
	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
	  simple->bounds.x1 -= width;
	  break;
	default:
	  break;
	}

      switch (witem->anchor)
	{
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
	  simple->bounds.y1 -= height / 2.0;
	  break;
	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
	  simple->bounds.y1 -= height;
	  break;
	default:
	  break;
	}

      simple->bounds.x2 = simple->bounds.x1 + width;
      simple->bounds.y2 = simple->bounds.y1 + height;

      /* Queue a resize of the widget so it gets moved. Note that the widget
	 is moved by goo_canvas_size_allocate(). */
      gtk_widget_queue_resize (witem->widget);
    }
  else
    {
      simple->bounds.x1 = simple->bounds.y1 = 0.0;
      simple->bounds.x2 = simple->bounds.y2 = 0.0;
    }
}


static void
goo_canvas_widget_allocate_area      (GooCanvasItem         *item,
				      cairo_t               *cr,
				      const GooCanvasBounds *requested_area,
				      const GooCanvasBounds *allocated_area,
				      gdouble                x_offset,
				      gdouble                y_offset)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasWidget *witem = (GooCanvasWidget*) item;
  gdouble requested_width, requested_height, allocated_width, allocated_height;
  gdouble width_proportion, height_proportion;
  gdouble width, height;

  width = simple->bounds.x2 - simple->bounds.x1;
  height = simple->bounds.y2 - simple->bounds.y1;

  simple->bounds.x1 += x_offset;
  simple->bounds.y1 += y_offset;

  requested_width = requested_area->x2 - requested_area->x1;
  requested_height = requested_area->y2 - requested_area->y1;
  allocated_width = allocated_area->x2 - allocated_area->x1;
  allocated_height = allocated_area->y2 - allocated_area->y1;

  width_proportion = allocated_width / requested_width;
  height_proportion = allocated_height / requested_height;

  width *= width_proportion;
  height *= height_proportion;

  simple->bounds.x2 = simple->bounds.x1 + width;
  simple->bounds.y2 = simple->bounds.y1 + height;

  /* Queue a resize of the widget so it gets moved. Note that the widget
     is moved by goo_canvas_size_allocate(). */
  gtk_widget_queue_resize (witem->widget);
}


static void
goo_canvas_widget_paint (GooCanvasItemSimple   *simple,
			 cairo_t               *cr,
			 const GooCanvasBounds *bounds)
{
  /* Do nothing for now. Maybe render for printing in future. */
}


static gboolean
goo_canvas_widget_is_item_at (GooCanvasItemSimple *simple,
			      gdouble              x,
			      gdouble              y,
			      cairo_t             *cr,
			      gboolean             is_pointer_event)
{
  /* For now we just assume that the widget covers its entire bounds so we just
     return TRUE. In future if widget items support transforms we'll need to
     modify this. */
  return TRUE;
}


static void
canvas_item_interface_init (GooCanvasItemIface *iface)
{
  iface->set_canvas     = goo_canvas_widget_set_canvas;
  iface->set_parent	= goo_canvas_widget_set_parent;
  iface->allocate_area  = goo_canvas_widget_allocate_area;
}


static void
goo_canvas_widget_class_init (GooCanvasWidgetClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;
  GooCanvasItemSimpleClass *simple_class = (GooCanvasItemSimpleClass*) klass;

  gobject_class->dispose = goo_canvas_widget_dispose;

  gobject_class->get_property = goo_canvas_widget_get_property;
  gobject_class->set_property = goo_canvas_widget_set_property;

  simple_class->simple_update        = goo_canvas_widget_update;
  simple_class->simple_paint         = goo_canvas_widget_paint;
  simple_class->simple_is_item_at    = goo_canvas_widget_is_item_at;

  /* Register our accessible factory, but only if accessibility is enabled. */
  if (!ATK_IS_NO_OP_OBJECT_FACTORY (atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_WIDGET)))
    {
      atk_registry_set_factory_type (atk_get_default_registry (),
				     GOO_TYPE_CANVAS_WIDGET,
				     goo_canvas_widget_accessible_factory_get_type ());
    }

  g_object_class_install_property (gobject_class, PROP_WIDGET,
				   g_param_spec_object ("widget",
							_("Widget"),
							_("The widget to place in the canvas"),
							GTK_TYPE_WIDGET,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_X,
				   g_param_spec_double ("x",
							"X",
							_("The x coordinate of the widget"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y,
				   g_param_spec_double ("y",
							"Y",
							_("The y coordinate of the widget"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
				   g_param_spec_double ("width",
							_("Width"),
							_("The width of the widget, or -1 to use its requested width"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, -1.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
				   g_param_spec_double ("height",
							_("Height"),
							_("The height of the widget, or -1 to use its requested height"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, -1.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ANCHOR,
				   g_param_spec_enum ("anchor",
						      _("Anchor"),
						      _("How to position the widget relative to the item's x and y coordinate settings"),
						      GTK_TYPE_ANCHOR_TYPE,
						      GTK_ANCHOR_NW,
						      G_PARAM_READWRITE));

  g_object_class_override_property (gobject_class, PROP_VISIBILITY,
				    "visibility");
}


