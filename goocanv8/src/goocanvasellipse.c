/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasellipse.c - ellipse item.
 */

/**
 * SECTION:goocanvasellipse
 * @Title: GooCanvasEllipse
 * @Short_Description: an ellipse item.
 *
 * GooCanvasEllipse represents an ellipse item.
 *
 * It is a subclass of #GooCanvasItemSimple and so inherits all of the style
 * properties such as "stroke-color", "fill-color" and "line-width".
 *
 * It also implements the #GooCanvasItem interface, so you can use the
 * #GooCanvasItem functions such as goo_canvas_item_raise() and
 * goo_canvas_item_rotate().
 *
 * To create a #GooCanvasEllipse use goo_canvas_ellipse_new().
 *
 * To get or set the properties of an existing #GooCanvasEllipse, use
 * g_object_get() and g_object_set().
 */
#include <config.h>
#include <math.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "goocanvasellipse.h"


enum {
  PROP_0,

  PROP_CENTER_X,
  PROP_CENTER_Y,
  PROP_RADIUS_X,
  PROP_RADIUS_Y
};


static void canvas_item_interface_init      (GooCanvasItemIface  *iface);
static void goo_canvas_ellipse_finalize     (GObject             *object);
static void goo_canvas_ellipse_get_property (GObject             *object,
					     guint                param_id,
					     GValue              *value,
					     GParamSpec          *pspec);
static void goo_canvas_ellipse_set_property (GObject             *object,
					     guint                param_id,
					     const GValue        *value,
					     GParamSpec          *pspec);
static void goo_canvas_ellipse_create_path  (GooCanvasItemSimple *simple,
					     cairo_t             *cr);

G_DEFINE_TYPE_WITH_CODE (GooCanvasEllipse, goo_canvas_ellipse,
			 GOO_TYPE_CANVAS_ITEM_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM,
						canvas_item_interface_init))


static void
goo_canvas_ellipse_install_common_properties (GObjectClass *gobject_class)
{
  g_object_class_install_property (gobject_class, PROP_CENTER_X,
				   g_param_spec_double ("center-x",
							_("Center X"),
							_("The x coordinate of the center of the ellipse"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CENTER_Y,
				   g_param_spec_double ("center-y",
							_("Center Y"),
							_("The y coordinate of the center of the ellipse"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RADIUS_X,
				   g_param_spec_double ("radius-x",
							_("Radius X"),
							_("The horizontal radius of the ellipse"),
							0.0, G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RADIUS_Y,
				   g_param_spec_double ("radius-y",
							_("Radius Y"),
							_("The vertical radius of the ellipse"),
							0.0, G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));
}


static void
goo_canvas_ellipse_class_init (GooCanvasEllipseClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;
  GooCanvasItemSimpleClass *simple_class = (GooCanvasItemSimpleClass*) klass;

  gobject_class->finalize     = goo_canvas_ellipse_finalize;

  gobject_class->get_property = goo_canvas_ellipse_get_property;
  gobject_class->set_property = goo_canvas_ellipse_set_property;

  simple_class->simple_create_path = goo_canvas_ellipse_create_path;

  goo_canvas_ellipse_install_common_properties (gobject_class);
}


static void
goo_canvas_ellipse_init (GooCanvasEllipse *ellipse)
{
  ellipse->ellipse_data = g_slice_new0 (GooCanvasEllipseData);
}


/**
 * goo_canvas_ellipse_new:
 * @parent: the parent item, or %NULL. If a parent is specified, it will assume
 *  ownership of the item, and the item will automatically be freed when it is
 *  removed from the parent. Otherwise call g_object_unref() to free it.
 * @center_x: the x coordinate of the center of the ellipse.
 * @center_y: the y coordinate of the center of the ellipse.
 * @radius_x: the horizontal radius of the ellipse.
 * @radius_y: the vertical radius of the ellipse.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new ellipse item.
 *
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create an ellipse centered at (100.0,
 * 100.0), with a horizontal radius of 50.0 and a vertical radius of 30.0.
 * It is drawn with a red outline with a width of 5.0 and filled with blue:
 *
 * <informalexample><programlisting>
 *  GooCanvasItem *ellipse = goo_canvas_ellipse_new (mygroup, 100.0, 100.0, 50.0, 30.0,
 *                                                   "stroke-color", "red",
 *                                                   "line-width", 5.0,
 *                                                   "fill-color", "blue",
 *                                                   NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new ellipse item.
 **/
GooCanvasItem*
goo_canvas_ellipse_new (GooCanvasItem *parent,
			gdouble        center_x,
			gdouble        center_y,
			gdouble        radius_x,
			gdouble        radius_y,
			...)
{
  GooCanvasItem *item;
  GooCanvasEllipse *ellipse;
  GooCanvasEllipseData *ellipse_data;
  const char *first_property;
  va_list var_args;

  item = g_object_new (GOO_TYPE_CANVAS_ELLIPSE, NULL);
  ellipse = (GooCanvasEllipse*) item;

  ellipse_data = ellipse->ellipse_data;
  ellipse_data->center_x = center_x;
  ellipse_data->center_y = center_y;
  ellipse_data->radius_x = radius_x;
  ellipse_data->radius_y = radius_y;

  va_start (var_args, radius_y);
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
goo_canvas_ellipse_finalize (GObject *object)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasEllipse *ellipse = (GooCanvasEllipse*) object;

  /* Free our data if we didn't have a model. (If we had a model it would
     have been reset in dispose() and simple_data will be NULL.) */
  if (simple->simple_data)
    g_slice_free (GooCanvasEllipseData, ellipse->ellipse_data);
  ellipse->ellipse_data = NULL;

  G_OBJECT_CLASS (goo_canvas_ellipse_parent_class)->finalize (object);
}


static void
goo_canvas_ellipse_get_common_property (GObject              *object,
					GooCanvasEllipseData *ellipse_data,
					guint                 prop_id,
					GValue               *value,
					GParamSpec           *pspec)
{
  switch (prop_id)
    {
    case PROP_CENTER_X:
      g_value_set_double (value, ellipse_data->center_x);
      break;
    case PROP_CENTER_Y:
      g_value_set_double (value, ellipse_data->center_y);
      break;
    case PROP_RADIUS_X:
      g_value_set_double (value, ellipse_data->radius_x);
      break;
    case PROP_RADIUS_Y:
      g_value_set_double (value, ellipse_data->radius_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
goo_canvas_ellipse_get_property (GObject              *object,
				 guint                 prop_id,
				 GValue               *value,
				 GParamSpec           *pspec)
{
  GooCanvasEllipse *ellipse = (GooCanvasEllipse*) object;

  goo_canvas_ellipse_get_common_property (object, ellipse->ellipse_data,
					  prop_id, value, pspec);
}


static void
goo_canvas_ellipse_set_common_property (GObject              *object,
					GooCanvasEllipseData *ellipse_data,
					guint                 prop_id,
					const GValue         *value,
					GParamSpec           *pspec)
{
  switch (prop_id)
    {
    case PROP_CENTER_X:
      ellipse_data->center_x = g_value_get_double (value);
      break;
    case PROP_CENTER_Y:
      ellipse_data->center_y = g_value_get_double (value);
      break;
    case PROP_RADIUS_X:
      ellipse_data->radius_x = g_value_get_double (value);
      break;
    case PROP_RADIUS_Y:
      ellipse_data->radius_y = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
goo_canvas_ellipse_set_property (GObject              *object,
				 guint                 prop_id,
				 const GValue         *value,
				 GParamSpec           *pspec)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasEllipse *ellipse = (GooCanvasEllipse*) object;

  if (simple->model)
    {
      g_warning ("Can't set property of a canvas item with a model - set the model property instead");
      return;
    }

  goo_canvas_ellipse_set_common_property (object, ellipse->ellipse_data,
					  prop_id, value, pspec);
  goo_canvas_item_simple_changed (simple, TRUE);
}


static void
goo_canvas_ellipse_create_path (GooCanvasItemSimple *simple,
				cairo_t             *cr)
{
  GooCanvasEllipse *ellipse = (GooCanvasEllipse*) simple;
  GooCanvasEllipseData *ellipse_data = ellipse->ellipse_data;

  cairo_new_path (cr);
  cairo_save (cr);
  cairo_translate (cr, ellipse_data->center_x, ellipse_data->center_y);
  cairo_scale (cr, ellipse_data->radius_x, ellipse_data->radius_y);
  cairo_arc (cr, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
  cairo_restore (cr);
}



static void
goo_canvas_ellipse_set_model    (GooCanvasItem      *item,
				 GooCanvasItemModel *model)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasEllipse *ellipse = (GooCanvasEllipse*) item;
  GooCanvasEllipseModel *emodel = (GooCanvasEllipseModel*) model;

  /* If our ellipse_data was allocated, free it. */
  if (!simple->model)
    g_slice_free (GooCanvasEllipseData, ellipse->ellipse_data);

  /* Now use the new model's ellipse_data instead. */
  ellipse->ellipse_data = &emodel->ellipse_data;

  /* Let the parent GooCanvasItemSimple code do the rest. */
  goo_canvas_item_simple_set_model (simple, model);
}


static void
canvas_item_interface_init (GooCanvasItemIface *iface)
{
  iface->set_model      = goo_canvas_ellipse_set_model;
}


/**
 * SECTION:goocanvasellipsemodel
 * @Title: GooCanvasEllipseModel
 * @Short_Description: a model for ellipse items.
 *
 * GooCanvasEllipseModel represents a model for ellipse items.
 *
 * It is a subclass of #GooCanvasItemModelSimple and so inherits all of the
 * style properties such as "stroke-color", "fill-color" and "line-width".
 *
 * It also implements the #GooCanvasItemModel interface, so you can use the
 * #GooCanvasItemModel functions such as goo_canvas_item_model_raise() and
 * goo_canvas_item_model_rotate().
 *
 * To create a #GooCanvasEllipseModel use goo_canvas_ellipse_model_new().
 *
 * To get or set the properties of an existing #GooCanvasEllipseModel, use
 * g_object_get() and g_object_set().
 *
 * To respond to events such as mouse clicks on the ellipse you must connect
 * to the signal handlers of the corresponding #GooCanvasEllipse objects.
 * (See goo_canvas_get_item() and #GooCanvas::item-created.)
 */

static void item_model_interface_init (GooCanvasItemModelIface *iface);
static void goo_canvas_ellipse_model_finalize     (GObject            *object);
static void goo_canvas_ellipse_model_get_property (GObject            *object,
						   guint               param_id,
						   GValue             *value,
						   GParamSpec         *pspec);
static void goo_canvas_ellipse_model_set_property (GObject            *object,
						   guint               param_id,
						   const GValue       *value,
						   GParamSpec         *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasEllipseModel, goo_canvas_ellipse_model,
			 GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM_MODEL,
						item_model_interface_init))


static void
goo_canvas_ellipse_model_class_init (GooCanvasEllipseModelClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->finalize     = goo_canvas_ellipse_model_finalize;

  gobject_class->get_property = goo_canvas_ellipse_model_get_property;
  gobject_class->set_property = goo_canvas_ellipse_model_set_property;

  goo_canvas_ellipse_install_common_properties (gobject_class);
}


static void
goo_canvas_ellipse_model_init (GooCanvasEllipseModel *emodel)
{

}


/**
 * goo_canvas_ellipse_model_new:
 * @parent: the parent model, or %NULL. If a parent is specified, it will
 *  assume ownership of the item, and the item will automatically be freed when
 *  it is removed from the parent. Otherwise call g_object_unref() to free it.
 * @center_x: the x coordinate of the center of the ellipse.
 * @center_y: the y coordinate of the center of the ellipse.
 * @radius_x: the horizontal radius of the ellipse.
 * @radius_y: the vertical radius of the ellipse.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new ellipse model.
 *
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create an ellipse centered at (100.0,
 * 100.0), with a horizontal radius of 50.0 and a vertical radius of 30.0.
 * It is drawn with a red outline with a width of 5.0 and filled with blue:
 *
 * <informalexample><programlisting>
 *  GooCanvasItemModel *ellipse = goo_canvas_ellipse_model_new (mygroup, 100.0, 100.0, 50.0, 30.0,
 *                                                              "stroke-color", "red",
 *                                                              "line-width", 5.0,
 *                                                              "fill-color", "blue",
 *                                                              NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new ellipse model.
 **/
GooCanvasItemModel*
goo_canvas_ellipse_model_new (GooCanvasItemModel *parent,
			      gdouble             center_x,
			      gdouble             center_y,
			      gdouble             radius_x,
			      gdouble             radius_y,
			      ...)
{
  GooCanvasItemModel *model;
  GooCanvasEllipseModel *emodel;
  GooCanvasEllipseData *ellipse_data;
  const char *first_property;
  va_list var_args;

  model = g_object_new (GOO_TYPE_CANVAS_ELLIPSE_MODEL, NULL);
  emodel = (GooCanvasEllipseModel*) model;

  ellipse_data = &emodel->ellipse_data;
  ellipse_data->center_x = center_x;
  ellipse_data->center_y = center_y;
  ellipse_data->radius_x = radius_x;
  ellipse_data->radius_y = radius_y;

  va_start (var_args, radius_y);
  first_property = va_arg (var_args, char*);
  if (first_property)
    g_object_set_valist ((GObject*) model, first_property, var_args);
  va_end (var_args);

  if (parent)
    {
      goo_canvas_item_model_add_child (parent, model, -1);
      g_object_unref (model);
    }

  return model;
}


static void
goo_canvas_ellipse_model_finalize (GObject *object)
{
  /*GooCanvasEllipseModel *emodel = (GooCanvasEllipseModel*) object;*/

  G_OBJECT_CLASS (goo_canvas_ellipse_model_parent_class)->finalize (object);
}


static void
goo_canvas_ellipse_model_get_property (GObject              *object,
				       guint                 prop_id,
				       GValue               *value,
				       GParamSpec           *pspec)
{
  GooCanvasEllipseModel *emodel = (GooCanvasEllipseModel*) object;

  goo_canvas_ellipse_get_common_property (object, &emodel->ellipse_data,
					  prop_id, value, pspec);
}


static void
goo_canvas_ellipse_model_set_property (GObject              *object,
				       guint                 prop_id,
				       const GValue         *value,
				       GParamSpec           *pspec)
{
  GooCanvasEllipseModel *emodel = (GooCanvasEllipseModel*) object;

  goo_canvas_ellipse_set_common_property (object, &emodel->ellipse_data,
					  prop_id, value, pspec);
  g_signal_emit_by_name (emodel, "changed", TRUE);
}


static GooCanvasItem*
goo_canvas_ellipse_model_create_item (GooCanvasItemModel *model,
				      GooCanvas          *canvas)
{
  GooCanvasItem *item;

  item = g_object_new (GOO_TYPE_CANVAS_ELLIPSE, NULL);
  goo_canvas_item_set_model (item, model);

  return item;
}


static void
item_model_interface_init (GooCanvasItemModelIface *iface)
{
  iface->create_item    = goo_canvas_ellipse_model_create_item;
}
