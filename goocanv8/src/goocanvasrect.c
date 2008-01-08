/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasrect.c - rectangle item.
 */

/**
 * SECTION:goocanvasrect
 * @Title: GooCanvasRect
 * @Short_Description: a rectangle item.
 *
 * GooCanvasRect represents a rectangle item.
 *
 * It is a subclass of #GooCanvasItemSimple and so inherits all of the style
 * properties such as "stroke-color", "fill-color" and "line-width".
 *
 * It also implements the #GooCanvasItem interface, so you can use the
 * #GooCanvasItem functions such as goo_canvas_item_raise() and
 * goo_canvas_item_rotate().
 *
 * To create a #GooCanvasRect use goo_canvas_rect_new().
 *
 * To get or set the properties of an existing #GooCanvasRect, use
 * g_object_get() and g_object_set().
 */
#include <config.h>
#include <math.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "goocanvasrect.h"


enum {
  PROP_0,

  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_RADIUS_X,
  PROP_RADIUS_Y
};


static void canvas_item_interface_init   (GooCanvasItemIface *iface);
static void goo_canvas_rect_finalize     (GObject            *object);
static void goo_canvas_rect_get_property (GObject            *object,
					  guint               param_id,
					  GValue             *value,
					  GParamSpec         *pspec);
static void goo_canvas_rect_set_property (GObject            *object,
					  guint               param_id,
					  const GValue       *value,
					  GParamSpec         *pspec);
static void goo_canvas_rect_create_path (GooCanvasItemSimple *simple,
					 cairo_t             *cr);

G_DEFINE_TYPE_WITH_CODE (GooCanvasRect, goo_canvas_rect,
			 GOO_TYPE_CANVAS_ITEM_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM,
						canvas_item_interface_init))


static void
goo_canvas_rect_install_common_properties (GObjectClass *gobject_class)
{
  g_object_class_install_property (gobject_class, PROP_X,
				   g_param_spec_double ("x",
							"X",
							_("The x coordinate of the rectangle"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y,
				   g_param_spec_double ("y",
							"Y",
							_("The y coordinate of the rectangle"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
				   g_param_spec_double ("width",
							_("Width"),
							_("The width of the rectangle"),
							0.0, G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
				   g_param_spec_double ("height",
							_("Height"),
							_("The height of the rectangle"),
							0.0, G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RADIUS_X,
				   g_param_spec_double ("radius_x",
							_("Radius X"),
							_("The horizontal radius to use for rounded corners"),
							0.0, G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RADIUS_Y,
				   g_param_spec_double ("radius_y",
							_("Radius Y"),
							_("The vertical radius to use for rounded corners"),
							0.0, G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));
}


static void
goo_canvas_rect_class_init (GooCanvasRectClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;
  GooCanvasItemSimpleClass *simple_class = (GooCanvasItemSimpleClass*) klass;

  gobject_class->finalize     = goo_canvas_rect_finalize;

  gobject_class->get_property = goo_canvas_rect_get_property;
  gobject_class->set_property = goo_canvas_rect_set_property;

  simple_class->simple_create_path = goo_canvas_rect_create_path;

  goo_canvas_rect_install_common_properties (gobject_class);
}


static void
goo_canvas_rect_init (GooCanvasRect *rect)
{
  rect->rect_data = g_slice_new0 (GooCanvasRectData);
}


/**
 * goo_canvas_rect_new:
 * @parent: the parent item, or %NULL. If a parent is specified, it will assume
 *  ownership of the item, and the item will automatically be freed when it is
 *  removed from the parent. Otherwise call g_object_unref() to free it.
 * @x: the x coordinate of the left of the rectangle.
 * @y: the y coordinate of the top of the rectangle.
 * @width: the width of the rectangle.
 * @height: the height of the rectangle.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new rectangle item.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a rectangle at (100,100) with a
 * width of 200 and a height of 100.
 *
 * <informalexample><programlisting>
 *  GooCanvasItem *rect = goo_canvas_rect_new (mygroup, 100.0, 100.0, 200.0, 100.0,
 *                                             "stroke-color", "red",
 *                                             "line-width", 5.0,
 *                                             "fill-color", "blue",
 *                                             NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new rectangle item.
 **/
GooCanvasItem*
goo_canvas_rect_new (GooCanvasItem *parent,
		     gdouble        x,
		     gdouble        y,
		     gdouble        width,
		     gdouble        height,
		     ...)
{
  GooCanvasItem *item;
  GooCanvasRect *rect;
  GooCanvasRectData *rect_data;
  const char *first_property;
  va_list var_args;

  item = g_object_new (GOO_TYPE_CANVAS_RECT, NULL);
  rect = (GooCanvasRect*) item;

  rect_data = rect->rect_data;
  rect_data->x = x;
  rect_data->y = y;
  rect_data->width = width;
  rect_data->height = height;
  rect_data->radius_x = 0;
  rect_data->radius_y = 0;

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
goo_canvas_rect_finalize (GObject *object)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasRect *rect = (GooCanvasRect*) object;

  /* Free our data if we didn't have a model. (If we had a model it would
     have been reset in dispose() and simple_data will be NULL.) */
  if (simple->simple_data)
    g_slice_free (GooCanvasRectData, rect->rect_data);
  rect->rect_data = NULL;

  G_OBJECT_CLASS (goo_canvas_rect_parent_class)->finalize (object);
}


static void
goo_canvas_rect_get_common_property (GObject              *object,
				     GooCanvasRectData    *rect_data,
				     guint                 prop_id,
				     GValue               *value,
				     GParamSpec           *pspec)
{
  switch (prop_id)
    {
    case PROP_X:
      g_value_set_double (value, rect_data->x);
      break;
    case PROP_Y:
      g_value_set_double (value, rect_data->y);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, rect_data->width);
      break;
    case PROP_HEIGHT:
      g_value_set_double (value, rect_data->height);
      break;
    case PROP_RADIUS_X:
      g_value_set_double (value, rect_data->radius_x);
      break;
    case PROP_RADIUS_Y:
      g_value_set_double (value, rect_data->radius_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
goo_canvas_rect_get_property (GObject              *object,
			      guint                 prop_id,
			      GValue               *value,
			      GParamSpec           *pspec)
{
  GooCanvasRect *rect = (GooCanvasRect*) object;

  goo_canvas_rect_get_common_property (object, rect->rect_data, prop_id,
				       value, pspec);
}


static void
goo_canvas_rect_set_common_property (GObject              *object,
				     GooCanvasRectData    *rect_data,
				     guint                 prop_id,
				     const GValue         *value,
				     GParamSpec           *pspec)
{
  switch (prop_id)
    {
    case PROP_X:
      rect_data->x = g_value_get_double (value);
      break;
    case PROP_Y:
      rect_data->y = g_value_get_double (value);
      break;
    case PROP_WIDTH:
      rect_data->width = g_value_get_double (value);
      break;
    case PROP_HEIGHT:
      rect_data->height = g_value_get_double (value);
      break;
    case PROP_RADIUS_X:
      rect_data->radius_x = g_value_get_double (value);
      break;
    case PROP_RADIUS_Y:
      rect_data->radius_y = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
goo_canvas_rect_set_property (GObject              *object,
			      guint                 prop_id,
			      const GValue         *value,
			      GParamSpec           *pspec)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasRect *rect = (GooCanvasRect*) object;

  if (simple->model)
    {
      g_warning ("Can't set property of a canvas item with a model - set the model property instead");
      return;
    }

  goo_canvas_rect_set_common_property (object, rect->rect_data, prop_id,
				       value, pspec);
  goo_canvas_item_simple_changed (simple, TRUE);
}


static void
goo_canvas_rect_create_path (GooCanvasItemSimple *simple,
			     cairo_t             *cr)
{
  GooCanvasRect *rect = (GooCanvasRect*) simple;
  GooCanvasRectData *rect_data = rect->rect_data;

  cairo_new_path (cr);

  /* Check if we need to draw rounded corners. */
  if (rect_data->radius_x > 0 && rect_data->radius_y > 0)
    {
      /* The radii can't be more than half the size of the rect. */
      double rx = MIN (rect_data->radius_x, rect_data->width / 2);
      double ry = MIN (rect_data->radius_y, rect_data->height / 2);

      /* Draw the top-right arc. */
      cairo_save (cr);
      cairo_translate (cr, rect_data->x + rect_data->width - rx,
		       rect_data->y + ry);
      cairo_scale (cr, rx, ry);
      cairo_arc (cr, 0.0, 0.0, 1.0, 1.5 * M_PI, 2.0 * M_PI);
      cairo_restore (cr);

      /* Draw the line down the right side. */
      cairo_line_to (cr, rect_data->x + rect_data->width,
		     rect_data->y + rect_data->height - ry);

      /* Draw the bottom-right arc. */
      cairo_save (cr);
      cairo_translate (cr, rect_data->x + rect_data->width - rx,
		       rect_data->y + rect_data->height - ry);
      cairo_scale (cr, rx, ry);
      cairo_arc (cr, 0.0, 0.0, 1.0, 0.0, 0.5 * M_PI);
      cairo_restore (cr);

      /* Draw the line left across the bottom. */
      cairo_line_to (cr, rect_data->x + rx, rect_data->y + rect_data->height);

      /* Draw the bottom-left arc. */
      cairo_save (cr);
      cairo_translate (cr, rect_data->x + rx,
		       rect_data->y + rect_data->height - ry);
      cairo_scale (cr, rx, ry);
      cairo_arc (cr, 0.0, 0.0, 1.0, 0.5 * M_PI, M_PI);
      cairo_restore (cr);

      /* Draw the line up the left side. */
      cairo_line_to (cr, rect_data->x, rect_data->y + ry);

      /* Draw the top-left arc. */
      cairo_save (cr);
      cairo_translate (cr, rect_data->x + rx, rect_data->y + ry);
      cairo_scale (cr, rx, ry);
      cairo_arc (cr, 0.0, 0.0, 1.0, M_PI, 1.5 * M_PI);
      cairo_restore (cr);

      /* Close the path across the top. */
      cairo_close_path (cr);
    }
  else
    {
      /* Draw the plain rectangle. */
      cairo_rectangle (cr, rect_data->x, rect_data->y,
		       rect_data->width, rect_data->height);
      cairo_close_path (cr);
    }
}



static void
goo_canvas_rect_set_model    (GooCanvasItem      *item,
			      GooCanvasItemModel *model)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasRect *rect = (GooCanvasRect*) item;
  GooCanvasRectModel *rmodel = (GooCanvasRectModel*) model;

  /* If our rect_data was allocated, free it. */
  if (!simple->model)
    g_slice_free (GooCanvasRectData, rect->rect_data);

  /* Now use the new model's rect_data instead. */
  rect->rect_data = &rmodel->rect_data;

  /* Let the parent GooCanvasItemSimple code do the rest. */
  goo_canvas_item_simple_set_model (simple, model);
}


static void
canvas_item_interface_init (GooCanvasItemIface *iface)
{
  iface->set_model      = goo_canvas_rect_set_model;
}


/**
 * SECTION:goocanvasrectmodel
 * @Title: GooCanvasRectModel
 * @Short_Description: a model for rectangle items.
 *
 * GooCanvasRectModel represents a model for rectangle items.
 *
 * It is a subclass of #GooCanvasItemModelSimple and so inherits all of the
 * style properties such as "stroke-color", "fill-color" and "line-width".
 *
 * It also implements the #GooCanvasItemModel interface, so you can use the
 * #GooCanvasItemModel functions such as goo_canvas_item_model_raise() and
 * goo_canvas_item_model_rotate().
 *
 * To create a #GooCanvasRectModel use goo_canvas_rect_model_new().
 *
 * To get or set the properties of an existing #GooCanvasRectModel, use
 * g_object_get() and g_object_set().
 *
 * To respond to events such as mouse clicks on the rectangle you must connect
 * to the signal handlers of the corresponding #GooCanvasRect objects.
 * (See goo_canvas_get_item() and #GooCanvas::item-created.)
 */

static void item_model_interface_init (GooCanvasItemModelIface *iface);
static void goo_canvas_rect_model_get_property (GObject            *object,
						guint               param_id,
						GValue             *value,
						GParamSpec         *pspec);
static void goo_canvas_rect_model_set_property (GObject            *object,
						guint               param_id,
						const GValue       *value,
						GParamSpec         *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasRectModel, goo_canvas_rect_model,
			 GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM_MODEL,
						item_model_interface_init))


static void
goo_canvas_rect_model_class_init (GooCanvasRectModelClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->get_property = goo_canvas_rect_model_get_property;
  gobject_class->set_property = goo_canvas_rect_model_set_property;

  goo_canvas_rect_install_common_properties (gobject_class);
}


static void
goo_canvas_rect_model_init (GooCanvasRectModel *rmodel)
{

}


/**
 * goo_canvas_rect_model_new:
 * @parent: the parent model, or %NULL. If a parent is specified, it will
 *  assume ownership of the item, and the item will automatically be freed when
 *  it is removed from the parent. Otherwise call g_object_unref() to free it.
 * @x: the x coordinate of the left of the rectangle.
 * @y: the y coordinate of the top of the rectangle.
 * @width: the width of the rectangle.
 * @height: the height of the rectangle.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new rectangle item.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a rectangle at (100,100) with a
 * width of 200 and a height of 100.
 *
 * <informalexample><programlisting>
 *  GooCanvasItemModel *rect = goo_canvas_rect_model_new (mygroup, 100.0, 100.0, 200.0, 100.0,
 *                                                        "stroke-color", "red",
 *                                                        "line-width", 5.0,
 *                                                        "fill-color", "blue",
 *                                                        NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new rectangle model.
 **/
GooCanvasItemModel*
goo_canvas_rect_model_new (GooCanvasItemModel *parent,
			   gdouble             x,
			   gdouble             y,
			   gdouble             width,
			   gdouble             height,
			   ...)
{
  GooCanvasItemModel *model;
  GooCanvasRectModel *rmodel;
  GooCanvasRectData *rect_data;
  const char *first_property;
  va_list var_args;

  model = g_object_new (GOO_TYPE_CANVAS_RECT_MODEL, NULL);
  rmodel = (GooCanvasRectModel*) model;

  rect_data = &rmodel->rect_data;
  rect_data->x = x;
  rect_data->y = y;
  rect_data->width = width;
  rect_data->height = height;
  rect_data->radius_x = 0;
  rect_data->radius_y = 0;

  va_start (var_args, height);
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
goo_canvas_rect_model_get_property (GObject              *object,
				    guint                 prop_id,
				    GValue               *value,
				    GParamSpec           *pspec)
{
  GooCanvasRectModel *rmodel = (GooCanvasRectModel*) object;

  goo_canvas_rect_get_common_property (object, &rmodel->rect_data, prop_id,
				       value, pspec);
}


static void
goo_canvas_rect_model_set_property (GObject              *object,
				    guint                 prop_id,
				    const GValue         *value,
				    GParamSpec           *pspec)
{
  GooCanvasRectModel *rmodel = (GooCanvasRectModel*) object;

  goo_canvas_rect_set_common_property (object, &rmodel->rect_data, prop_id,
				       value, pspec);
  g_signal_emit_by_name (rmodel, "changed", TRUE);
}


static GooCanvasItem*
goo_canvas_rect_model_create_item (GooCanvasItemModel *model,
				   GooCanvas          *canvas)
{
  GooCanvasItem *item;

  item = g_object_new (GOO_TYPE_CANVAS_RECT, NULL);
  goo_canvas_item_set_model (item, model);

  return item;
}


static void
item_model_interface_init (GooCanvasItemModelIface *iface)
{
  iface->create_item    = goo_canvas_rect_model_create_item;
}
