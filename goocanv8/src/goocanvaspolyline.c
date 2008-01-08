/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvaspolyline.c - polyline item, with optional arrows.
 */

/**
 * SECTION:goocanvaspolyline
 * @Title: GooCanvasPolyline
 * @Short_Description: a polyline item (a series of lines with optional arrows).
 *
 * GooCanvasPolyline represents a polyline item, which is a series of one or
 * more lines, with optional arrows at either end.
 *
 * It is a subclass of #GooCanvasItemSimple and so inherits all of the style
 * properties such as "stroke-color", "fill-color" and "line-width".
 *
 * It also implements the #GooCanvasItem interface, so you can use the
 * #GooCanvasItem functions such as goo_canvas_item_raise() and
 * goo_canvas_item_rotate().
 *
 * To create a #GooCanvasPolyline use goo_canvas_polyline_new(), or
 * goo_canvas_polyline_new_line() for a simple line between two points.
 *
 * To get or set the properties of an existing #GooCanvasPolyline, use
 * g_object_get() and g_object_set().
 */
#include <config.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "goocanvaspolyline.h"
#include "goocanvas.h"


/**
 * goo_canvas_points_new:
 * @num_points: the number of points to create space for.
 * 
 * Creates a new #GooCanvasPoints struct with space for the given number of
 * points. It should be freed with goo_canvas_points_unref().
 * 
 * Returns: a new #GooCanvasPoints struct.
 **/
GooCanvasPoints*
goo_canvas_points_new   (int    num_points)
{
  GooCanvasPoints *points;

  points = g_slice_new (GooCanvasPoints);
  points->num_points = num_points;
  points->coords = g_slice_alloc (num_points * 2 * sizeof (double));
  points->ref_count = 1;

  return points;
}


/**
 * goo_canvas_points_ref:
 * @points: a #GooCanvasPoints struct.
 * 
 * Increments the reference count of the given #GooCanvasPoints struct.
 * 
 * Returns: the #GooCanvasPoints struct.
 **/
GooCanvasPoints*
goo_canvas_points_ref   (GooCanvasPoints *points)
{
  points->ref_count++;
  return points;
}


/**
 * goo_canvas_points_unref:
 * @points: a #GooCanvasPoints struct.
 * 
 * Decrements the reference count of the given #GooCanvasPoints struct,
 * freeing it if the reference count falls to zero.
 **/
void
goo_canvas_points_unref (GooCanvasPoints *points)
{
  if (--points->ref_count == 0)
    {
      g_slice_free1 (points->num_points * 2 * sizeof (double), points->coords);
      g_slice_free (GooCanvasPoints, points);
    }
}


GType
goo_canvas_points_get_type (void)
{
  static GType type_canvas_points = 0;

  if (!type_canvas_points)
    type_canvas_points = g_boxed_type_register_static
      ("GooCanvasPoints", 
       (GBoxedCopyFunc) goo_canvas_points_ref,
       (GBoxedFreeFunc) goo_canvas_points_unref);

  return type_canvas_points;
}


enum {
  PROP_0,

  PROP_POINTS,
  PROP_CLOSE_PATH,
  PROP_START_ARROW,
  PROP_END_ARROW,
  PROP_ARROW_LENGTH,
  PROP_ARROW_WIDTH,
  PROP_ARROW_TIP_LENGTH
};


static void goo_canvas_polyline_finalize     (GObject            *object);
static void canvas_item_interface_init       (GooCanvasItemIface *iface);
static void goo_canvas_polyline_get_property (GObject            *object,
					      guint               param_id,
					      GValue             *value,
					      GParamSpec         *pspec);
static void goo_canvas_polyline_set_property (GObject            *object,
					      guint               param_id,
					      const GValue       *value,
					      GParamSpec         *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasPolyline, goo_canvas_polyline,
			 GOO_TYPE_CANVAS_ITEM_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM,
						canvas_item_interface_init))


static void
goo_canvas_polyline_install_common_properties (GObjectClass *gobject_class)
{
  g_object_class_install_property (gobject_class, PROP_POINTS,
				   g_param_spec_boxed ("points",
						       _("Points"),
						       _("The array of points"),
						       GOO_TYPE_CANVAS_POINTS,
						       G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CLOSE_PATH,
				   g_param_spec_boolean ("close-path",
							 _("Close Path"),
							 _("If the last point should be connected to the first"),
							 FALSE,
							 G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_START_ARROW,
				   g_param_spec_boolean ("start-arrow",
							 _("Start Arrow"),
							 _("If an arrow should be displayed at the start of the polyline"),
							 FALSE,
							 G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_END_ARROW,
				   g_param_spec_boolean ("end-arrow",
							 _("End Arrow"),
							 _("If an arrow should be displayed at the end of the polyline"),
							 FALSE,
							 G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ARROW_LENGTH,
				   g_param_spec_double ("arrow-length",
							_("Arrow Length"),
							_("The length of the arrows, as a multiple of the line width"),
							0.0, G_MAXDOUBLE, 5.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ARROW_WIDTH,
				   g_param_spec_double ("arrow-width",
							_("Arrow Width"),
							_("The width of the arrows, as a multiple of the line width"),
							0.0, G_MAXDOUBLE, 4.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ARROW_TIP_LENGTH,
				   g_param_spec_double ("arrow-tip-length",
							_("Arrow Tip Length"),
							_("The length of the arrow tip, as a multiple of the line width"),
							0.0, G_MAXDOUBLE, 4.0,
							G_PARAM_READWRITE));
}


static void
goo_canvas_polyline_init (GooCanvasPolyline *polyline)
{
  polyline->polyline_data = g_slice_new0 (GooCanvasPolylineData);
}


static void
goo_canvas_polyline_finalize (GObject *object)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasPolyline *polyline = (GooCanvasPolyline*) object;

  /* Free our data if we didn't have a model. (If we had a model it would
     have been reset in dispose() and simple_data will be NULL.) */
  if (simple->simple_data)
    {
      g_slice_free1 (polyline->polyline_data->num_points * 2 * sizeof (gdouble),
		     polyline->polyline_data->coords);
      g_slice_free (GooCanvasPolylineArrowData, polyline->polyline_data->arrow_data);
      g_slice_free (GooCanvasPolylineData, polyline->polyline_data);
    }
  polyline->polyline_data = NULL;

  G_OBJECT_CLASS (goo_canvas_polyline_parent_class)->finalize (object);
}


static void
goo_canvas_polyline_get_common_property (GObject              *object,
					 GooCanvasPolylineData *polyline_data,
					 guint                 prop_id,
					 GValue               *value,
					 GParamSpec           *pspec)
{
  GooCanvasPoints *points;

  switch (prop_id)
    {
    case PROP_POINTS:
      if (polyline_data->num_points == 0)
	{
	  g_value_set_boxed (value, NULL);
	}
      else
	{
	  points = goo_canvas_points_new (polyline_data->num_points);
	  memcpy (points->coords, polyline_data->coords,
		  polyline_data->num_points * 2 * sizeof (double));
	  g_value_set_boxed (value, points);
	  goo_canvas_points_unref (points);
	}
      break;
    case PROP_CLOSE_PATH:
      g_value_set_boolean (value, polyline_data->close_path);
      break;
    case PROP_START_ARROW:
      g_value_set_boolean (value, polyline_data->start_arrow);
      break;
    case PROP_END_ARROW:
      g_value_set_boolean (value, polyline_data->end_arrow);
      break;
    case PROP_ARROW_LENGTH:
      g_value_set_double (value, polyline_data->arrow_data
			  ? polyline_data->arrow_data->arrow_length : 5.0);
      break;
    case PROP_ARROW_WIDTH:
      g_value_set_double (value, polyline_data->arrow_data
			  ? polyline_data->arrow_data->arrow_width : 4.0);
      break;
    case PROP_ARROW_TIP_LENGTH:
      g_value_set_double (value, polyline_data->arrow_data
			  ? polyline_data->arrow_data->arrow_tip_length : 4.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
goo_canvas_polyline_get_property (GObject              *object,
				 guint                 prop_id,
				 GValue               *value,
				 GParamSpec           *pspec)
{
  GooCanvasPolyline *polyline = (GooCanvasPolyline*) object;

  goo_canvas_polyline_get_common_property (object, polyline->polyline_data,
					   prop_id, value, pspec);
}


static void
ensure_arrow_data (GooCanvasPolylineData *polyline_data)
{
  if (!polyline_data->arrow_data)
    {
      polyline_data->arrow_data = g_slice_new (GooCanvasPolylineArrowData);

      /* These seem like reasonable defaults for the arrow dimensions.
	 They are specified in multiples of the line width so they scale OK. */
      polyline_data->arrow_data->arrow_width = 4.0;
      polyline_data->arrow_data->arrow_length = 5.0;
      polyline_data->arrow_data->arrow_tip_length = 4.0;
    }
}

#define GOO_CANVAS_EPSILON 1e-10

static void
reconfigure_arrow (GooCanvasPolylineData *polyline_data,
		   gint                   end_point,
		   gint                   prev_point,
		   gdouble                line_width,
		   gdouble               *line_coords,
		   gdouble               *arrow_coords)
{
  GooCanvasPolylineArrowData *arrow = polyline_data->arrow_data;
  double dx, dy, length, sin_theta, cos_theta;
  double half_arrow_width, arrow_length, arrow_tip_length;
  double arrow_end_center_x, arrow_end_center_y;
  double arrow_tip_center_x, arrow_tip_center_y;
  double x_offset, y_offset, half_line_width, line_trim;

  dx = polyline_data->coords[prev_point] - polyline_data->coords[end_point];
  dy = polyline_data->coords[prev_point + 1] - polyline_data->coords[end_point + 1];
  length = sqrt (dx * dx + dy * dy);

  if (length < GOO_CANVAS_EPSILON)
    {
      /* The length is too short to reliably get the angle so just guess. */
      sin_theta = 1.0;
      cos_theta = 0.0;
    }
  else
    {
      /* Calculate a unit vector moving from the arrow point along the line. */
      sin_theta = dy / length;
      cos_theta = dx / length;
    }

  /* These are all specified in multiples of the line width, so convert. */
  half_arrow_width = arrow->arrow_width * line_width / 2;
  arrow_length = arrow->arrow_length * line_width;
  arrow_tip_length = arrow->arrow_tip_length * line_width;

  /* The arrow tip is at the end point of the line. */
  arrow_coords[0] = polyline_data->coords[end_point];
  arrow_coords[1] = polyline_data->coords[end_point + 1];

  /* Calculate the end of the arrow, along the center line. */
  arrow_end_center_x = arrow_coords[0] + (arrow_length * cos_theta);
  arrow_end_center_y = arrow_coords[1] + (arrow_length * sin_theta);

  /* Now calculate the 2 end points of the arrow either side of the line. */
  x_offset = half_arrow_width * sin_theta;
  y_offset = half_arrow_width * cos_theta;

  arrow_coords[2] = arrow_end_center_x + x_offset;
  arrow_coords[3] = arrow_end_center_y - y_offset;

  arrow_coords[8] = arrow_end_center_x - x_offset;
  arrow_coords[9] = arrow_end_center_y + y_offset;

  /* Calculate the end of the arrow tip, along the center line. */
  arrow_tip_center_x = arrow_coords[0] + (arrow_tip_length * cos_theta);
  arrow_tip_center_y = arrow_coords[1] + (arrow_tip_length * sin_theta);

  /* Now calculate the 2 arrow points on either edge of the line. */
  half_line_width = line_width / 2.0;
  x_offset = half_line_width * sin_theta;
  y_offset = half_line_width * cos_theta;

  arrow_coords[4] = arrow_tip_center_x + x_offset;
  arrow_coords[5] = arrow_tip_center_y - y_offset;

  arrow_coords[6] = arrow_tip_center_x - x_offset;
  arrow_coords[7] = arrow_tip_center_y + y_offset;

  /* Calculate the new end of the line. We have to move it back slightly so
     it doesn't draw over the arrow tip. But we overlap the arrow by a small
     fraction of the line width to avoid a tiny gap. */
  line_trim = arrow_tip_length - (line_width / 10.0);
  line_coords[0] = arrow_coords[0] + (line_trim * cos_theta);
  line_coords[1] = arrow_coords[1] + (line_trim * sin_theta);
}


/* Recalculates the arrow polygons for the line */
static void
goo_canvas_polyline_reconfigure_arrows (GooCanvasPolyline *polyline)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) polyline;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;
  double line_width = 2.0;

  if (!polyline_data->reconfigure_arrows)
    return;

  polyline_data->reconfigure_arrows = FALSE;

  if (polyline_data->num_points < 2
      || (!polyline_data->start_arrow && !polyline_data->end_arrow))
    return;

  /* Determine the item's line width setting, either the default canvas line
     width or the item style's line width setting. */
  if (simple->canvas)
    line_width = goo_canvas_get_default_line_width (simple->canvas);
  if (simple_data->style)
    {
      GValue *value;

      value = goo_canvas_style_get_property (simple_data->style,
					     goo_canvas_style_line_width_id);
      if (value)
	line_width = value->data[0].v_double;
    }

  ensure_arrow_data (polyline_data);

  if (polyline_data->start_arrow)
    reconfigure_arrow (polyline_data, 0, 2, line_width,
		       polyline_data->arrow_data->line_start,
		       polyline_data->arrow_data->start_arrow_coords);

  if (polyline_data->end_arrow)
    {
      int end_point, prev_point;

      if (polyline_data->close_path)
	{
	  end_point = 0;
	  prev_point = polyline_data->num_points - 1;
	}
      else
	{
	  end_point = polyline_data->num_points - 1;
	  prev_point = polyline_data->num_points - 2;
	}

      reconfigure_arrow (polyline_data, end_point * 2, prev_point * 2,
			 line_width, polyline_data->arrow_data->line_end,
			 polyline_data->arrow_data->end_arrow_coords);
    }
}


static void
goo_canvas_polyline_set_common_property (GObject              *object,
					 GooCanvasPolylineData *polyline_data,
					 guint                 prop_id,
					 const GValue         *value,
					 GParamSpec           *pspec)
{
  GooCanvasPoints *points;

  switch (prop_id)
    {
    case PROP_POINTS:
      points = g_value_get_boxed (value);

      if (polyline_data->coords)
	{
	  g_slice_free1 (polyline_data->num_points * 2 * sizeof (double), polyline_data->coords);
	  polyline_data->coords = NULL;
	}

      if (!points)
	{
	  polyline_data->num_points = 0;
	}
      else
	{
	  polyline_data->num_points = points->num_points;
	  polyline_data->coords = g_slice_alloc (polyline_data->num_points * 2 * sizeof (double));
	  memcpy (polyline_data->coords, points->coords,
		  polyline_data->num_points * 2 * sizeof (double));
	}
      polyline_data->reconfigure_arrows = TRUE;
      break;
    case PROP_CLOSE_PATH:
      polyline_data->close_path = g_value_get_boolean (value);
      polyline_data->reconfigure_arrows = TRUE;
      break;
    case PROP_START_ARROW:
      polyline_data->start_arrow = g_value_get_boolean (value);
      polyline_data->reconfigure_arrows = TRUE;
      break;
    case PROP_END_ARROW:
      polyline_data->end_arrow = g_value_get_boolean (value);
      polyline_data->reconfigure_arrows = TRUE;
      break;
    case PROP_ARROW_LENGTH:
      ensure_arrow_data (polyline_data);
      polyline_data->arrow_data->arrow_length = g_value_get_double (value);
      polyline_data->reconfigure_arrows = TRUE;
      break;
    case PROP_ARROW_WIDTH:
      ensure_arrow_data (polyline_data);
      polyline_data->arrow_data->arrow_width = g_value_get_double (value);
      polyline_data->reconfigure_arrows = TRUE;
      break;
    case PROP_ARROW_TIP_LENGTH:
      ensure_arrow_data (polyline_data);
      polyline_data->arrow_data->arrow_tip_length = g_value_get_double (value);
      polyline_data->reconfigure_arrows = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
goo_canvas_polyline_set_property (GObject              *object,
				 guint                 prop_id,
				 const GValue         *value,
				 GParamSpec           *pspec)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasPolyline *polyline = (GooCanvasPolyline*) object;

  if (simple->model)
    {
      g_warning ("Can't set property of a canvas item with a model - set the model property instead");
      return;
    }

  goo_canvas_polyline_set_common_property (object, polyline->polyline_data,
					   prop_id, value, pspec);
  goo_canvas_item_simple_changed (simple, TRUE);
}


/**
 * goo_canvas_polyline_new:
 * @parent: the parent item, or %NULL. If a parent is specified, it will assume
 *  ownership of the item, and the item will automatically be freed when it is
 *  removed from the parent. Otherwise call g_object_unref() to free it.
 * @close_path: if the last point should be connected to the first.
 * @num_points: the number of points in the polyline.
 * @...: the pairs of coordinates for each point in the line, followed by
 *  optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new polyline item.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a filled triangle with vertices
 * at (100,100), (300,100), and (200,300).
 *
 * <informalexample><programlisting>
 *  GooCanvasItem *polyline = goo_canvas_polyline_new (mygroup, TRUE, 3,
 *                                                     100.0, 100.0,
 *                                                     300.0, 100.0,
 *                                                     200.0, 300.0,
 *                                                     "stroke-color", "red",
 *                                                     "line-width", 5.0,
 *                                                     "fill-color", "blue",
 *                                                     NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new polyline item.
 **/
GooCanvasItem*
goo_canvas_polyline_new               (GooCanvasItem *parent,
				       gboolean       close_path,
				       gint           num_points,
				       ...)
{
  GooCanvasItem *item;
  GooCanvasPolyline *polyline;
  GooCanvasPolylineData *polyline_data;
  const char *first_property;
  va_list var_args;
  gint i;

  item = g_object_new (GOO_TYPE_CANVAS_POLYLINE, NULL);
  polyline = (GooCanvasPolyline*) item;

  polyline_data = polyline->polyline_data;
  polyline_data->close_path = close_path;
  polyline_data->num_points = num_points;
  if (num_points)
    polyline_data->coords = g_slice_alloc (num_points * 2 * sizeof (gdouble));

  va_start (var_args, num_points);
  for (i = 0; i < num_points * 2; i++)
    polyline_data->coords[i] = va_arg (var_args, gdouble);

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


/**
 * goo_canvas_polyline_new_line:
 * @parent: the parent item, or %NULL.
 * @x1: the x coordinate of the start of the line.
 * @y1: the y coordinate of the start of the line.
 * @x2: the x coordinate of the end of the line.
 * @y2: the y coordinate of the end of the line.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new polyline item with a single line.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a line from (100,100) to (300,300).
 *
 * <informalexample><programlisting>
 *  GooCanvasItem *polyline = goo_canvas_polyline_new_line (mygroup,
 *                                                          100.0, 100.0,
 *                                                          300.0, 300.0,
 *                                                          "stroke-color", "red",
 *                                                          "line-width", 5.0,
 *                                                          NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new polyline item.
 **/
GooCanvasItem*
goo_canvas_polyline_new_line          (GooCanvasItem *parent,
				       gdouble        x1,
				       gdouble        y1,
				       gdouble        x2,
				       gdouble        y2,
				       ...)
{
  GooCanvasItem *item;
  GooCanvasPolyline *polyline;
  GooCanvasPolylineData *polyline_data;
  const char *first_property;
  va_list var_args;

  item = g_object_new (GOO_TYPE_CANVAS_POLYLINE, NULL);
  polyline = (GooCanvasPolyline*) item;

  polyline_data = polyline->polyline_data;
  polyline_data->close_path = FALSE;
  polyline_data->num_points = 2;
  polyline_data->coords = g_slice_alloc (4 * sizeof (gdouble));
  polyline_data->coords[0] = x1;
  polyline_data->coords[1] = y1;
  polyline_data->coords[2] = x2;
  polyline_data->coords[3] = y2;

  va_start (var_args, y2);
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
goo_canvas_polyline_create_path (GooCanvasPolyline *polyline,
				 cairo_t           *cr)
{
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;
  GooCanvasPolylineArrowData *arrow = polyline_data->arrow_data;
  gint i;

  cairo_new_path (cr);

  if (polyline_data->num_points == 0)
    return;

  /* If there is an arrow at the start of the polyline, we need to move the
     start of the line slightly to avoid drawing over the arrow tip. */
  if (polyline_data->start_arrow && polyline_data->num_points >= 2)
    cairo_move_to (cr, arrow->line_start[0], arrow->line_start[1]);
  else
    cairo_move_to (cr, polyline_data->coords[0], polyline_data->coords[1]);

  if (polyline_data->end_arrow && polyline_data->num_points >= 2)
    {
      gint last_point = polyline_data->num_points - 1;

      if (!polyline_data->close_path)
	last_point--;

      for (i = 1; i <= last_point; i++)
	cairo_line_to (cr, polyline_data->coords[i * 2],
		       polyline_data->coords[i * 2 + 1]);

      cairo_line_to (cr, arrow->line_end[0], arrow->line_end[1]);
    }
  else
    {
      for (i = 1; i < polyline_data->num_points; i++)
	cairo_line_to (cr, polyline_data->coords[i * 2],
		       polyline_data->coords[i * 2 + 1]);

      if (polyline_data->close_path)
	cairo_close_path (cr);
    }
}


static void
goo_canvas_polyline_create_start_arrow_path (GooCanvasPolyline *polyline,
					     cairo_t           *cr)
{
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;
  GooCanvasPolylineArrowData *arrow = polyline_data->arrow_data;
  gint i;

  cairo_new_path (cr);

  if (polyline_data->num_points < 2)
    return;

  cairo_move_to (cr, arrow->start_arrow_coords[0],
		 arrow->start_arrow_coords[1]);
  for (i = 1; i < NUM_ARROW_POINTS; i++)
    {
      cairo_line_to (cr, arrow->start_arrow_coords[i * 2],
		     arrow->start_arrow_coords[i * 2 + 1]);
    }
  cairo_close_path (cr);
}


static void
goo_canvas_polyline_create_end_arrow_path (GooCanvasPolyline *polyline,
					   cairo_t           *cr)
{
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;
  GooCanvasPolylineArrowData *arrow = polyline_data->arrow_data;
  gint i;

  cairo_new_path (cr);

  if (polyline_data->num_points < 2)
    return;

  cairo_move_to (cr, arrow->end_arrow_coords[0],
		 arrow->end_arrow_coords[1]);
  for (i = 1; i < NUM_ARROW_POINTS; i++)
    {
      cairo_line_to (cr, arrow->end_arrow_coords[i * 2],
		     arrow->end_arrow_coords[i * 2 + 1]);
    }
  cairo_close_path (cr);
}


static gboolean
goo_canvas_polyline_is_item_at (GooCanvasItemSimple *simple,
				gdouble              x,
				gdouble              y,
				cairo_t             *cr,
				gboolean             is_pointer_event)
{
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasPolyline *polyline = (GooCanvasPolyline*) simple;
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;
  GooCanvasPointerEvents pointer_events = GOO_CANVAS_EVENTS_ALL;
  gboolean do_stroke;

  if (polyline_data->num_points == 0)
    return FALSE;

  /* Check if the item should receive events. */
  if (is_pointer_event)
    pointer_events = simple_data->pointer_events;

  goo_canvas_polyline_create_path (polyline, cr);
  if (goo_canvas_item_simple_check_in_path (simple, x, y, cr, pointer_events))
    return TRUE;

  /* Check the arrows, if the polyline has them. */
  if ((polyline_data->start_arrow || polyline_data->end_arrow)
      && polyline_data->num_points >= 2
      && (pointer_events & GOO_CANVAS_EVENTS_STROKE_MASK))
    {
      /* We use the stroke pattern to match the style of the line. */
      do_stroke = goo_canvas_style_set_stroke_options (simple_data->style, cr);
      if (!(pointer_events & GOO_CANVAS_EVENTS_PAINTED_MASK) || do_stroke)
	{
	  if (polyline_data->start_arrow)
	    {
	      goo_canvas_polyline_create_start_arrow_path (polyline, cr);
	      if (cairo_in_fill (cr, x, y))
		return TRUE;
	    }

	  if (polyline_data->end_arrow)
	    {
	      goo_canvas_polyline_create_end_arrow_path (polyline, cr);
	      if (cairo_in_fill (cr, x, y))
		return TRUE;
	    }
	}
    }

  return FALSE;
}


static void
goo_canvas_polyline_compute_bounds (GooCanvasPolyline     *polyline,
				    cairo_t               *cr,
				    GooCanvasBounds       *bounds)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) polyline;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;
  GooCanvasBounds tmp_bounds;
  cairo_matrix_t transform;

  if (polyline_data->num_points == 0)
    {
      bounds->x1 = bounds->x2 = bounds->y1 = bounds->y2 = 0.0;
      return;
    }

  /* Use the identity matrix to get the bounds completely in user space. */
  cairo_get_matrix (cr, &transform);
  cairo_identity_matrix (cr);

  goo_canvas_polyline_create_path (polyline, cr);
  goo_canvas_item_simple_get_path_bounds (simple, cr, bounds);

  /* Add on the arrows, if required. */
  if ((polyline_data->start_arrow || polyline_data->end_arrow)
      && polyline_data->num_points >= 2)
    {
      /* We use the stroke pattern to match the style of the line. */
      goo_canvas_style_set_stroke_options (simple_data->style, cr);

      if (polyline_data->start_arrow)
	{
	  goo_canvas_polyline_create_start_arrow_path (polyline, cr);
	  cairo_fill_extents (cr, &tmp_bounds.x1, &tmp_bounds.y1,
			      &tmp_bounds.x2, &tmp_bounds.y2);
	  bounds->x1 = MIN (bounds->x1, tmp_bounds.x1);
	  bounds->y1 = MIN (bounds->y1, tmp_bounds.y1);
	  bounds->x2 = MAX (bounds->x2, tmp_bounds.x2);
	  bounds->y2 = MAX (bounds->y2, tmp_bounds.y2);
	}

      if (polyline_data->end_arrow)
	{
	  goo_canvas_polyline_create_end_arrow_path (polyline, cr);
	  cairo_fill_extents (cr, &tmp_bounds.x1, &tmp_bounds.y1,
			      &tmp_bounds.x2, &tmp_bounds.y2);
	  bounds->x1 = MIN (bounds->x1, tmp_bounds.x1);
	  bounds->y1 = MIN (bounds->y1, tmp_bounds.y1);
	  bounds->x2 = MAX (bounds->x2, tmp_bounds.x2);
	  bounds->y2 = MAX (bounds->y2, tmp_bounds.y2);
	}
    }

  cairo_set_matrix (cr, &transform);
}


static void
goo_canvas_polyline_update  (GooCanvasItemSimple *simple,
			     cairo_t             *cr)
{
  GooCanvasPolyline *polyline = (GooCanvasPolyline*) simple;
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;

  if (polyline_data->reconfigure_arrows)
    goo_canvas_polyline_reconfigure_arrows (polyline);

  /* Compute the new bounds. */
  goo_canvas_polyline_compute_bounds (polyline, cr, &simple->bounds);
}


static void
goo_canvas_polyline_paint (GooCanvasItemSimple   *simple,
			   cairo_t               *cr,
			   const GooCanvasBounds *bounds)
{
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasPolyline *polyline = (GooCanvasPolyline*) simple;
  GooCanvasPolylineData *polyline_data = polyline->polyline_data;

  if (polyline_data->num_points == 0)
    return;

  goo_canvas_polyline_create_path (polyline, cr);
  goo_canvas_item_simple_paint_path (simple, cr);

  /* Paint the arrows, if required. */
  if ((polyline_data->start_arrow || polyline_data->end_arrow)
      && polyline_data->num_points >= 2)
    {
      /* We use the stroke pattern to match the style of the line. */
      goo_canvas_style_set_stroke_options (simple_data->style, cr);

      if (polyline_data->start_arrow)
	{
	  goo_canvas_polyline_create_start_arrow_path (polyline, cr);
	  cairo_fill (cr);
	}

      if (polyline_data->end_arrow)
	{
	  goo_canvas_polyline_create_end_arrow_path (polyline, cr);
	  cairo_fill (cr);
	}
    }
}


static void
goo_canvas_polyline_set_model    (GooCanvasItem      *item,
				 GooCanvasItemModel *model)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasPolyline *polyline = (GooCanvasPolyline*) item;
  GooCanvasPolylineModel *pmodel = (GooCanvasPolylineModel*) model;

  /* If our polyline_data was allocated, free it. */
  if (!simple->model)
    g_slice_free (GooCanvasPolylineData, polyline->polyline_data);

  /* Now use the new model's polyline_data instead. */
  polyline->polyline_data = &pmodel->polyline_data;

  /* Let the parent GooCanvasItemSimple code do the rest. */
  goo_canvas_item_simple_set_model (simple, model);
}


static void
canvas_item_interface_init (GooCanvasItemIface *iface)
{
  iface->set_model   = goo_canvas_polyline_set_model;
}


static void
goo_canvas_polyline_class_init (GooCanvasPolylineClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;
  GooCanvasItemSimpleClass *simple_class = (GooCanvasItemSimpleClass*) klass;

  gobject_class->finalize  = goo_canvas_polyline_finalize;

  gobject_class->get_property = goo_canvas_polyline_get_property;
  gobject_class->set_property = goo_canvas_polyline_set_property;

  simple_class->simple_update        = goo_canvas_polyline_update;
  simple_class->simple_paint         = goo_canvas_polyline_paint;
  simple_class->simple_is_item_at    = goo_canvas_polyline_is_item_at;

  goo_canvas_polyline_install_common_properties (gobject_class);
}


/**
 * SECTION:goocanvaspolylinemodel
 * @Title: GooCanvasPolylineModel
 * @Short_Description: a model for polyline items (a series of lines with
 *  optional arrows).
 *
 * GooCanvasPolylineModel represents a model for polyline items, which are a
 * series of one or more lines, with optional arrows at either end.
 *
 * It is a subclass of #GooCanvasItemModelSimple and so inherits all of the
 * style properties such as "stroke-color", "fill-color" and "line-width".
 *
 * It also implements the #GooCanvasItemModel interface, so you can use the
 * #GooCanvasItemModel functions such as goo_canvas_item_model_raise() and
 * goo_canvas_item_model_rotate().
 *
 * To create a #GooCanvasPolylineModel use goo_canvas_polyline_model_new(), or
 * goo_canvas_polyline_model_new_line() for a simple line between two points.
 *
 * To get or set the properties of an existing #GooCanvasPolylineModel, use
 * g_object_get() and g_object_set().
 *
 * To respond to events such as mouse clicks on the polyline you must connect
 * to the signal handlers of the corresponding #GooCanvasPolyline objects.
 * (See goo_canvas_get_item() and #GooCanvas::item-created.)
 */

static void item_model_interface_init (GooCanvasItemModelIface *iface);
static void goo_canvas_polyline_model_finalize     (GObject          *object);
static void goo_canvas_polyline_model_get_property (GObject          *object,
						    guint             param_id,
						    GValue           *value,
						    GParamSpec       *pspec);
static void goo_canvas_polyline_model_set_property (GObject          *object,
						    guint             param_id,
						    const GValue     *value,
						    GParamSpec       *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasPolylineModel, goo_canvas_polyline_model,
			 GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM_MODEL,
						item_model_interface_init))


static void
goo_canvas_polyline_model_class_init (GooCanvasPolylineModelClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->finalize     = goo_canvas_polyline_model_finalize;

  gobject_class->get_property = goo_canvas_polyline_model_get_property;
  gobject_class->set_property = goo_canvas_polyline_model_set_property;

  goo_canvas_polyline_install_common_properties (gobject_class);
}


static void
goo_canvas_polyline_model_init (GooCanvasPolylineModel *pmodel)
{

}


/**
 * goo_canvas_polyline_model_new:
 * @parent: the parent model, or %NULL. If a parent is specified, it will
 *  assume ownership of the item, and the item will automatically be freed when
 *  it is removed from the parent. Otherwise call g_object_unref() to free it.
 * @close_path: if the last point should be connected to the first.
 * @num_points: the number of points in the polyline.
 * @...: the pairs of coordinates for each point in the line, followed by
 *  optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new polyline model.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a filled triangle with vertices
 * at (100,100), (300,100), and (200,300).
 *
 * <informalexample><programlisting>
 *  GooCanvasItemModel *polyline = goo_canvas_polyline_model_new (mygroup, TRUE, 3,
 *                                                                100.0, 100.0,
 *                                                                300.0, 100.0,
 *                                                                200.0, 300.0,
 *                                                                "stroke-color", "red",
 *                                                                "line-width", 5.0,
 *                                                                "fill-color", "blue",
 *                                                                NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new polyline model.
 **/
GooCanvasItemModel*
goo_canvas_polyline_model_new (GooCanvasItemModel *parent,
			       gboolean            close_path,
			       gint                num_points,
			       ...)
{
  GooCanvasItemModel *model;
  GooCanvasPolylineModel *pmodel;
  GooCanvasPolylineData *polyline_data;
  const char *first_property;
  va_list var_args;
  gint i;

  model = g_object_new (GOO_TYPE_CANVAS_POLYLINE_MODEL, NULL);
  pmodel = (GooCanvasPolylineModel*) model;

  polyline_data = &pmodel->polyline_data;
  polyline_data->close_path = close_path;
  polyline_data->num_points = num_points;
  if (num_points)
    polyline_data->coords = g_slice_alloc (num_points * 2 * sizeof (gdouble));

  va_start (var_args, num_points);
  for (i = 0; i < num_points * 2; i++)
    polyline_data->coords[i] = va_arg (var_args, gdouble);

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


/**
 * goo_canvas_polyline_model_new_line:
 * @parent: the parent model, or %NULL.
 * @x1: the x coordinate of the start of the line.
 * @y1: the y coordinate of the start of the line.
 * @x2: the x coordinate of the end of the line.
 * @y2: the y coordinate of the end of the line.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new polyline model with a single line.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a line from (100,100) to (300,300).
 *
 * <informalexample><programlisting>
 *  GooCanvasItemModel *polyline = goo_canvas_polyline_model_new_line (mygroup,
 *                                                                     100.0, 100.0,
 *                                                                     300.0, 300.0,
 *                                                                     "stroke-color", "red",
 *                                                                     "line-width", 5.0,
 *                                                                     NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new polyline model.
 **/
GooCanvasItemModel*
goo_canvas_polyline_model_new_line (GooCanvasItemModel *parent,
				    gdouble             x1,
				    gdouble             y1,
				    gdouble             x2,
				    gdouble             y2,
				    ...)
{
  GooCanvasItemModel *model;
  GooCanvasPolylineModel *pmodel;
  GooCanvasPolylineData *polyline_data;
  const char *first_property;
  va_list var_args;

  model = g_object_new (GOO_TYPE_CANVAS_POLYLINE_MODEL, NULL);
  pmodel = (GooCanvasPolylineModel*) model;

  polyline_data = &pmodel->polyline_data;
  polyline_data->close_path = FALSE;
  polyline_data->num_points = 2;
  polyline_data->coords = g_slice_alloc (4 * sizeof (gdouble));
  polyline_data->coords[0] = x1;
  polyline_data->coords[1] = y1;
  polyline_data->coords[2] = x2;
  polyline_data->coords[3] = y2;

  va_start (var_args, y2);
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
goo_canvas_polyline_model_finalize (GObject *object)
{
  GooCanvasPolylineModel *pmodel = (GooCanvasPolylineModel*) object;

  g_slice_free1 (pmodel->polyline_data.num_points * 2 * sizeof (gdouble),
		 pmodel->polyline_data.coords);
  g_slice_free (GooCanvasPolylineArrowData, pmodel->polyline_data.arrow_data);

  G_OBJECT_CLASS (goo_canvas_polyline_model_parent_class)->finalize (object);
}


static void
goo_canvas_polyline_model_get_property (GObject              *object,
					guint                 prop_id,
					GValue               *value,
					GParamSpec           *pspec)
{
  GooCanvasPolylineModel *pmodel = (GooCanvasPolylineModel*) object;

  goo_canvas_polyline_get_common_property (object, &pmodel->polyline_data,
					   prop_id, value, pspec);
}


static void
goo_canvas_polyline_model_set_property (GObject              *object,
					guint                 prop_id,
					const GValue         *value,
					GParamSpec           *pspec)
{
  GooCanvasPolylineModel *pmodel = (GooCanvasPolylineModel*) object;

  goo_canvas_polyline_set_common_property (object, &pmodel->polyline_data,
					   prop_id, value, pspec);
  g_signal_emit_by_name (pmodel, "changed", TRUE);
}


static GooCanvasItem*
goo_canvas_polyline_model_create_item (GooCanvasItemModel *model,
				       GooCanvas          *canvas)
{
  GooCanvasItem *item;

  item = g_object_new (GOO_TYPE_CANVAS_POLYLINE, NULL);
  goo_canvas_item_set_model (item, model);

  return item;
}


static void
item_model_interface_init (GooCanvasItemModelIface *iface)
{
  iface->create_item    = goo_canvas_polyline_model_create_item;
}
