/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasutils.c - utility functions.
 */

/**
 * SECTION:goocanvasutils
 * @Title: GooCanvas Types
 * @Short_Description: types used in GooCanvas.
 *
 * This section describes the types used throughout GooCanvas.
 */
#include <config.h>
#include <math.h>
#include <gtk/gtk.h>
#include "goocanvas.h"


/* Glib doesn't provide a g_ptr_array_index() so we need our own one. */
void
goo_canvas_util_ptr_array_insert (GPtrArray *ptr_array,
				  gpointer   data,
				  gint       index)
{
  gint i;

  /* Add the pointer at the end so there is enough room. */
  g_ptr_array_add (ptr_array, data);

  /* If index is -1, we are done. */
  if (index == -1)
    return;

  /* Move the other pointers to make room for the new one. */
  for (i = ptr_array->len - 1; i > index; i--)
    ptr_array->pdata[i] = ptr_array->pdata[i - 1];

  /* Put the new element in its proper place. */
  ptr_array->pdata[index] = data;
}


/* Glib doesn't provide a g_ptr_array_move() so we need our own one. */
void
goo_canvas_util_ptr_array_move (GPtrArray *ptr_array,
				gint       old_index,
				gint       new_index)
{
  gpointer data;
  gint i;

  data = ptr_array->pdata[old_index];

  if (new_index > old_index)
    {
      /* Move the pointers down one place. */
      for (i = old_index; i < new_index; i++)
	ptr_array->pdata[i] = ptr_array->pdata[i + 1];
    }
  else
    {
      /* Move the pointers up one place. */
      for (i = old_index; i > new_index; i--)
	ptr_array->pdata[i] = ptr_array->pdata[i - 1];
    }

  ptr_array->pdata[new_index] = data;
}


/* Glib doesn't provide a g_ptr_array_move() so we need our own one. */
gint
goo_canvas_util_ptr_array_find_index (GPtrArray *ptr_array,
				      gpointer   data)
{
  gint i;

  for (i = 0; i < ptr_array->len; i++)
    {
      if (ptr_array->pdata[i] == data)
	return i;
    }

  return -1;
}


/*
 * Cairo utilities.
 */

cairo_surface_t*
goo_canvas_cairo_surface_from_pixbuf (GdkPixbuf *pixbuf)
{
  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);
  guchar *gdk_pixels = gdk_pixbuf_get_pixels (pixbuf);
  int gdk_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  int n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  guchar *cairo_pixels;
  cairo_format_t format;
  cairo_surface_t *surface;
  static const cairo_user_data_key_t key;
  int j;

  if (n_channels == 3)
    format = CAIRO_FORMAT_RGB24;
  else
    format = CAIRO_FORMAT_ARGB32;

  cairo_pixels = g_malloc (4 * width * height);
  surface = cairo_image_surface_create_for_data ((unsigned char *)cairo_pixels,
						 format,
						 width, height, 4 * width);
  cairo_surface_set_user_data (surface, &key,
			       cairo_pixels, (cairo_destroy_func_t)g_free);

  for (j = height; j; j--)
    {
      guchar *p = gdk_pixels;
      guchar *q = cairo_pixels;

      if (n_channels == 3)
	{
	  guchar *end = p + 3 * width;
	  
	  while (p < end)
	    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      q[0] = p[2];
	      q[1] = p[1];
	      q[2] = p[0];
#else	  
	      q[1] = p[0];
	      q[2] = p[1];
	      q[3] = p[2];
#endif
	      p += 3;
	      q += 4;
	    }
	}
      else
	{
	  guchar *end = p + 4 * width;
	  guint t1,t2,t3;
	    
#define MULT(d,c,a,t) G_STMT_START { t = c * a; d = ((t >> 8) + t) >> 8; } G_STMT_END

	  while (p < end)
	    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      MULT(q[0], p[2], p[3], t1);
	      MULT(q[1], p[1], p[3], t2);
	      MULT(q[2], p[0], p[3], t3);
	      q[3] = p[3];
#else	  
	      q[0] = p[3];
	      MULT(q[1], p[0], p[3], t1);
	      MULT(q[2], p[1], p[3], t2);
	      MULT(q[3], p[2], p[3], t3);
#endif
	      
	      p += 4;
	      q += 4;
	    }
	  
#undef MULT
	}

      gdk_pixels += gdk_rowstride;
      cairo_pixels += 4 * width;
    }

  return surface;
}


cairo_pattern_t*
goo_canvas_cairo_pattern_from_pixbuf (GdkPixbuf *pixbuf)
{
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;

  surface = goo_canvas_cairo_surface_from_pixbuf (pixbuf);
  pattern = cairo_pattern_create_for_surface (surface);
  cairo_surface_destroy (surface);

  return pattern;
}


/*
 * Cairo types.
 */
GType
goo_cairo_pattern_get_type (void)
{
  static GType cairo_pattern_type = 0;
  
  if (cairo_pattern_type == 0)
    cairo_pattern_type = g_boxed_type_register_static
      ("GooCairoPattern",
       (GBoxedCopyFunc) cairo_pattern_reference,
       (GBoxedFreeFunc) cairo_pattern_destroy);

  return cairo_pattern_type;
}


GType
goo_cairo_fill_rule_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { CAIRO_FILL_RULE_WINDING, "CAIRO_FILL_RULE_WINDING", "winding" },
      { CAIRO_FILL_RULE_EVEN_ODD, "CAIRO_FILL_RULE_EVEN_ODD", "even-odd" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCairoFillRule", values);
  }
  return etype;
}


GType
goo_cairo_operator_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { CAIRO_OPERATOR_CLEAR, "CAIRO_OPERATOR_CLEAR", "clear" },

      { CAIRO_OPERATOR_SOURCE, "CAIRO_OPERATOR_SOURCE", "source" },
      { CAIRO_OPERATOR_OVER, "CAIRO_OPERATOR_OVER", "over" },
      { CAIRO_OPERATOR_IN, "CAIRO_OPERATOR_IN", "in" },
      { CAIRO_OPERATOR_OUT, "CAIRO_OPERATOR_OUT", "out" },
      { CAIRO_OPERATOR_ATOP, "CAIRO_OPERATOR_ATOP", "atop" },

      { CAIRO_OPERATOR_DEST, "CAIRO_OPERATOR_DEST", "dest" },
      { CAIRO_OPERATOR_DEST_OVER, "CAIRO_OPERATOR_DEST_OVER", "dest-over" },
      { CAIRO_OPERATOR_DEST_IN, "CAIRO_OPERATOR_DEST_IN", "dest-in" },
      { CAIRO_OPERATOR_DEST_OUT, "CAIRO_OPERATOR_DEST_OUT", "dest-out" },
      { CAIRO_OPERATOR_DEST_ATOP, "CAIRO_OPERATOR_DEST_ATOP", "dest-atop" },

      { CAIRO_OPERATOR_XOR, "CAIRO_OPERATOR_XOR", "xor" },
      { CAIRO_OPERATOR_ADD, "CAIRO_OPERATOR_ADD", "add" },
      { CAIRO_OPERATOR_SATURATE, "CAIRO_OPERATOR_SATURATE", "saturate" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCairoOperator", values);
  }
  return etype;
}


GType
goo_cairo_antialias_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { CAIRO_ANTIALIAS_DEFAULT, "CAIRO_ANTIALIAS_DEFAULT", "default" },
      { CAIRO_ANTIALIAS_NONE, "CAIRO_ANTIALIAS_NONE", "none" },
      { CAIRO_ANTIALIAS_GRAY, "CAIRO_ANTIALIAS_GRAY", "gray" },
      { CAIRO_ANTIALIAS_SUBPIXEL, "CAIRO_ANTIALIAS_SUBPIXEL", "subpixel" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCairoAntialias", values);
  }
  return etype;
}


GType
goo_cairo_line_cap_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { CAIRO_LINE_CAP_BUTT, "CAIRO_LINE_CAP_BUTT", "butt" },
      { CAIRO_LINE_CAP_ROUND, "CAIRO_LINE_CAP_ROUND", "round" },
      { CAIRO_LINE_CAP_SQUARE, "CAIRO_LINE_CAP_SQUARE", "square" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCairoLineCap", values);
  }
  return etype;
}


GType
goo_cairo_line_join_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { CAIRO_LINE_JOIN_MITER, "CAIRO_LINE_JOIN_MITER", "miter" },
      { CAIRO_LINE_JOIN_ROUND, "CAIRO_LINE_JOIN_ROUND", "round" },
      { CAIRO_LINE_JOIN_BEVEL, "CAIRO_LINE_JOIN_BEVEL", "bevel" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCairoLineJoin", values);
  }
  return etype;
}


GType
goo_cairo_hint_metrics_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { CAIRO_HINT_METRICS_DEFAULT, "CAIRO_HINT_METRICS_DEFAULT", "default" },
      { CAIRO_HINT_METRICS_OFF, "CAIRO_HINT_METRICS_OFF", "off" },
      { CAIRO_HINT_METRICS_ON, "CAIRO_HINT_METRICS_ON", "on" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCairoHintMetrics", values);
  }
  return etype;
}


/**
 * goo_canvas_line_dash_ref:
 * @dash: a #GooCanvasLineDash.
 * 
 * Increments the reference count of the dash pattern.
 * 
 * Returns: the dash pattern.
 **/
GooCanvasLineDash*
goo_canvas_line_dash_ref   (GooCanvasLineDash *dash)
{
  if (dash)
    dash->ref_count++;
  return dash;
}


/**
 * goo_canvas_line_dash_unref:
 * @dash: a #GooCanvasLineDash.
 * 
 * Decrements the reference count of the dash pattern. If it falls to 0
 * it is freed.
 **/
void
goo_canvas_line_dash_unref (GooCanvasLineDash *dash)
{
  if (dash && --dash->ref_count == 0) {
    g_free (dash->dashes);
    g_free (dash);
  }
}


GType
goo_canvas_line_dash_get_type (void)
{
  static GType cairo_line_dash_type = 0;
  
  if (cairo_line_dash_type == 0)
    cairo_line_dash_type = g_boxed_type_register_static
      ("GooCairoLineDash",
       (GBoxedCopyFunc) goo_canvas_line_dash_ref,
       (GBoxedFreeFunc) goo_canvas_line_dash_unref);

  return cairo_line_dash_type;
}


/**
 * goo_canvas_line_dash_new:
 * @num_dashes: the number of dashes and gaps in the pattern.
 * @...: the length of each dash and gap.
 * 
 * Creates a new dash pattern.
 * 
 * Returns: a new dash pattern.
 **/
GooCanvasLineDash*
goo_canvas_line_dash_new (gint num_dashes,
			  ...)
{
  GooCanvasLineDash *dash;
  va_list var_args;
  gint i;

  dash = g_new (GooCanvasLineDash, 1);
  dash->ref_count = 1;
  dash->num_dashes = num_dashes;
  dash->dashes = g_new (double, num_dashes);
  dash->dash_offset = 0.0;

  va_start (var_args, num_dashes);

  for (i = 0; i < num_dashes; i++)
    {
      dash->dashes[i] = va_arg (var_args, double);
    }

  va_end (var_args);

  return dash;
}

/**
 * goo_canvas_line_dash_newv:
 * @num_dashes: the number of dashes and gaps in the pattern.
 * @dashes: a g_new-allocated vector of doubles, the length of each
 * dash and gap.
 * 
 * Creates a new dash pattern.  Takes ownership of the @dashes vector.
 * 
 * Returns: a new dash pattern.
 **/
GooCanvasLineDash*
goo_canvas_line_dash_newv (gint    num_dashes,
                           double *dashes)
{
  GooCanvasLineDash *dash;

  dash = g_new (GooCanvasLineDash, 1);
  dash->ref_count = 1;
  dash->num_dashes = num_dashes;
  dash->dashes = dashes;
  dash->dash_offset = 0.0;

  return dash;
}

cairo_matrix_t*
goo_cairo_matrix_copy   (const cairo_matrix_t *matrix)
{
  cairo_matrix_t *matrix_copy;

  if (!matrix)
    return NULL;

  matrix_copy = g_slice_new (cairo_matrix_t);
  *matrix_copy = *matrix;

  return matrix_copy;
}


void
goo_cairo_matrix_free   (cairo_matrix_t *matrix)
{
  g_slice_free (cairo_matrix_t, matrix);
}


GType
goo_cairo_matrix_get_type (void)
{
  static GType type_cairo_matrix = 0;

  if (!type_cairo_matrix)
    type_cairo_matrix = g_boxed_type_register_static
      ("GooCairoMatrix", 
       (GBoxedCopyFunc) goo_cairo_matrix_copy,
       (GBoxedFreeFunc) goo_cairo_matrix_free);

  return type_cairo_matrix;
}


/* This is a semi-private function to help gtk-doc find child properties. */
GParamSpec**
goo_canvas_query_child_properties (gpointer  class,
				   guint     *n_properties)
{
  if (!G_TYPE_IS_CLASSED (G_TYPE_FROM_CLASS (class)))
    return NULL;

  if (g_type_interface_peek (class, GOO_TYPE_CANVAS_ITEM))
    return goo_canvas_item_class_list_child_properties (class,
							n_properties);

  if (g_type_interface_peek (class, GOO_TYPE_CANVAS_ITEM_MODEL))
    return goo_canvas_item_model_class_list_child_properties (class,
							      n_properties);

  return NULL;
}


static gdouble
parse_double (gchar    **pos,
	      gboolean  *error)
{
  gdouble result;
  gchar *p;

  /* If an error has already occurred, just return. */
  if (*error)
    return 0;

  /* Skip whitespace and commas. */
  p = *pos;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',')
    p++;

  /* Parse the double, and set pos to the first char after it. */
  result = g_ascii_strtod (p, pos);

  /* If no characters were parsed, set the error flag. */
  if (p == *pos)
    *error = TRUE;

  return result;
}


static gint
parse_flag (gchar    **pos,
	    gboolean  *error)
{
  gint result = 0;
  gchar *p;

  /* If an error has already occurred, just return. */
  if (*error)
    return 0;

  /* Skip whitespace and commas. */
  p = *pos;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',')
    p++;

  /* The flag must be a '0' or a '1'. */
  if (*p == '0')
    result = 0;
  else if (*p == '1')
    result = 1;
  else
    {
      *error = TRUE;
      return 0;
    }

  *pos = p + 1;

  return result;
}


/**
 * goo_canvas_parse_path_data:
 * @path_data: the sequence of path commands, specified as a string using the
 *  same syntax as in the <ulink url="http://www.w3.org/Graphics/SVG/">Scalable
 *  Vector Graphics (SVG)</ulink> path element.
 * 
 * Parses the given SVG path specification string.
 * 
 * Returns: a #GArray of #GooCanvasPathCommand elements.
 **/
GArray*
goo_canvas_parse_path_data (const gchar       *path_data)
{
  GArray *commands;
  GooCanvasPathCommand cmd;
  gchar *pos, command = 0, next_command;
  gboolean error = FALSE;

  commands = g_array_new (0, 0, sizeof (GooCanvasPathCommand));

  if (!path_data)
    return commands;

  pos = (gchar*) path_data;
  for (;;)
    {
      while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n')
	pos++;
      if (!*pos)
	break;

      next_command = *pos;

      /* If there is no command letter, we use the same command as the last
	 one, except for the first command, and 'moveto' (which becomes
	 'lineto'). */
      if ((next_command < 'a' || next_command > 'z')
	  && (next_command < 'A' || next_command > 'Z'))
	{
	  /* If this is the first command, then set the error flag and assume
	     a simple close-path command. */
	  if (!command)
	    {
	      error = TRUE;
	      command = 'Z';
	    }
	  /* moveto commands change to lineto. */
	  else if (command == 'm')
	    command = 'l';
	  else if (command == 'M')
	    command = 'L';
	}
      else
	{
	  command = next_command;
	  pos++;
	}

      cmd.simple.relative = 0;
      switch (command)
	{
	  /* Simple commands like moveto and lineto: MmZzLlHhVv. */
	case 'm':
	  cmd.simple.relative = 1;
	case 'M':
	  cmd.simple.type = GOO_CANVAS_PATH_MOVE_TO;
	  cmd.simple.x = parse_double (&pos, &error);
	  cmd.simple.y = parse_double (&pos, &error);
	  break;

	case 'Z':
	case 'z':
	  cmd.simple.type = GOO_CANVAS_PATH_CLOSE_PATH;
	  break;

	case 'l':
	  cmd.simple.relative = 1;
	case 'L':
	  cmd.simple.type = GOO_CANVAS_PATH_LINE_TO;
	  cmd.simple.x = parse_double (&pos, &error);
	  cmd.simple.y = parse_double (&pos, &error);
	  break;

	case 'h':
	  cmd.simple.relative = 1;
	case 'H':
	  cmd.simple.type = GOO_CANVAS_PATH_HORIZONTAL_LINE_TO;
	  cmd.simple.x = parse_double (&pos, &error);
	  break;

	case 'v':
	  cmd.simple.relative = 1;
	case 'V':
	  cmd.simple.type = GOO_CANVAS_PATH_VERTICAL_LINE_TO;
	  cmd.simple.y = parse_double (&pos, &error);
	  break;

	  /* Bezier curve commands: CcSsQqTt. */
	case 'c':
	  cmd.curve.relative = 1;
	case 'C':
	  cmd.curve.type = GOO_CANVAS_PATH_CURVE_TO;
	  cmd.curve.x1 = parse_double (&pos, &error);
	  cmd.curve.y1 = parse_double (&pos, &error);
	  cmd.curve.x2 = parse_double (&pos, &error);
	  cmd.curve.y2 = parse_double (&pos, &error);
	  cmd.curve.x = parse_double (&pos, &error);
	  cmd.curve.y = parse_double (&pos, &error);
	  break;

	case 's':
	  cmd.curve.relative = 1;
	case 'S':
	  cmd.curve.type = GOO_CANVAS_PATH_SMOOTH_CURVE_TO;
	  cmd.curve.x2 = parse_double (&pos, &error);
	  cmd.curve.y2 = parse_double (&pos, &error);
	  cmd.curve.x = parse_double (&pos, &error);
	  cmd.curve.y = parse_double (&pos, &error);
	  break;

	case 'q':
	  cmd.curve.relative = 1;
	case 'Q':
	  cmd.curve.type = GOO_CANVAS_PATH_QUADRATIC_CURVE_TO;
	  cmd.curve.x1 = parse_double (&pos, &error);
	  cmd.curve.y1 = parse_double (&pos, &error);
	  cmd.curve.x = parse_double (&pos, &error);
	  cmd.curve.y = parse_double (&pos, &error);
	  break;

	case 't':
	  cmd.curve.relative = 1;
	case 'T':
	  cmd.curve.type = GOO_CANVAS_PATH_SMOOTH_QUADRATIC_CURVE_TO;
	  cmd.curve.x = parse_double (&pos, &error);
	  cmd.curve.y = parse_double (&pos, &error);
	  break;

	  /* The elliptical arc commands: Aa. */
	case 'a':
	  cmd.arc.relative = 1;
	case 'A':
	  cmd.arc.type = GOO_CANVAS_PATH_ELLIPTICAL_ARC;
	  cmd.arc.rx = parse_double (&pos, &error);
	  cmd.arc.ry = parse_double (&pos, &error);
	  cmd.arc.x_axis_rotation = parse_double (&pos, &error);
	  cmd.arc.large_arc_flag = parse_flag (&pos, &error);
	  cmd.arc.sweep_flag = parse_flag (&pos, &error);
	  cmd.arc.x = parse_double (&pos, &error);
	  cmd.arc.y = parse_double (&pos, &error);
	  break;

	default:
	  error = TRUE;
	  break;
	}

      /* If an error has occurred, return without adding the new command.
	 Thus we include everything in the path up to the error, like SVG. */
      if (error)
	return commands;

      g_array_append_val (commands, cmd);
    }

  return commands;
}


static void
do_curve_to (GooCanvasPathCommand *cmd,
	     cairo_t              *cr,
	     gdouble              *x,
	     gdouble              *y,
	     gdouble              *last_control_point_x,
	     gdouble              *last_control_point_y)
{
  if (cmd->curve.relative)
    {
      cairo_curve_to (cr, *x + cmd->curve.x1, *y + cmd->curve.y1,
		      *x + cmd->curve.x2, *y + cmd->curve.y2,
		      *x + cmd->curve.x, *y + cmd->curve.y);
      *last_control_point_x = *x + cmd->curve.x2;
      *last_control_point_y = *y + cmd->curve.y2;
      *x += cmd->curve.x;
      *y += cmd->curve.y;
    }
  else
    {
      cairo_curve_to (cr, cmd->curve.x1, cmd->curve.y1,
		      cmd->curve.x2, cmd->curve.y2,
		      cmd->curve.x, cmd->curve.y);
      *last_control_point_x = cmd->curve.x2;
      *last_control_point_y = cmd->curve.y2;
      *x = cmd->curve.x;
      *y = cmd->curve.y;
    }
}


static void
do_smooth_curve_to (GooCanvasPathCommand    *cmd,
		    GooCanvasPathCommandType prev_cmd_type,
		    cairo_t                 *cr,
		    gdouble                 *x,
		    gdouble                 *y,
		    gdouble                 *last_control_point_x,
		    gdouble                 *last_control_point_y)
{
  gdouble x1, y1;

  /* If the last command was a curveto or smooth curveto, we use the
     reflection of the last control point about the current point as
     the first control point of this curve. Otherwise we use the
     current point as the first control point. */
  if (prev_cmd_type == GOO_CANVAS_PATH_CURVE_TO
      || prev_cmd_type == GOO_CANVAS_PATH_SMOOTH_CURVE_TO)
    {
      x1 = *x + (*x - *last_control_point_x);
      y1 = *y + (*y - *last_control_point_y);
    }
  else
    {
      x1 = *x;
      y1 = *y;
    }

  if (cmd->curve.relative)
    {
      cairo_curve_to (cr, x1, y1, *x + cmd->curve.x2, *y + cmd->curve.y2,
		      *x + cmd->curve.x, *y + cmd->curve.y);
      *last_control_point_x = *x + cmd->curve.x2;
      *last_control_point_y = *y + cmd->curve.y2;
      *x += cmd->curve.x;
      *y += cmd->curve.y;
    }
  else
    {
      cairo_curve_to (cr, x1, y1, cmd->curve.x2, cmd->curve.y2,
		      cmd->curve.x, cmd->curve.y);
      *last_control_point_x = cmd->curve.x2;
      *last_control_point_y = cmd->curve.y2;
      *x = cmd->curve.x;
      *y = cmd->curve.y;
    }
}


static void
do_quadratic_curve_to (GooCanvasPathCommand *cmd,
		       cairo_t              *cr,
		       gdouble              *x,
		       gdouble              *y,
		       gdouble              *last_control_point_x,
		       gdouble              *last_control_point_y)
{
  gdouble qx1, qy1, qx2, qy2, x1, y1, x2, y2;

  if (cmd->curve.relative)
    {
      qx1 = *x + cmd->curve.x1;
      qy1 = *y + cmd->curve.y1;
      qx2 = *x + cmd->curve.x;
      qy2 = *y + cmd->curve.y;
    }
  else
    {
      qx1 = cmd->curve.x1;
      qy1 = cmd->curve.y1;
      qx2 = cmd->curve.x;
      qy2 = cmd->curve.y;
    }

  /* We need to convert the quadratic into a cubic bezier. */
  x1 = *x + (qx1 - *x) * 2.0 / 3.0;
  y1 = *y + (qy1 - *y) * 2.0 / 3.0;

  x2 = x1 + (qx2 - *x) / 3.0;
  y2 = y1 + (qy2 - *y) / 3.0;

  cairo_curve_to (cr, x1, y1, x2, y2, qx2, qy2);

  *x = qx2;
  *y = qy2;
  *last_control_point_x = qx1;
  *last_control_point_y = qy1;
}


static void
do_smooth_quadratic_curve_to (GooCanvasPathCommand    *cmd,
			      GooCanvasPathCommandType prev_cmd_type,
			      cairo_t                 *cr,
			      gdouble                 *x,
			      gdouble                 *y,
			      gdouble                 *last_control_point_x,
			      gdouble                 *last_control_point_y)
{
  gdouble qx1, qy1, qx2, qy2, x1, y1, x2, y2;

  /* If the last command was a quadratic or smooth quadratic, we use
     the reflection of the last control point about the current point
     as the first control point of this curve. Otherwise we use the
     current point as the first control point. */
  if (prev_cmd_type == GOO_CANVAS_PATH_QUADRATIC_CURVE_TO
      || prev_cmd_type == GOO_CANVAS_PATH_SMOOTH_QUADRATIC_CURVE_TO)
    {
      qx1 = *x + (*x - *last_control_point_x);
      qy1 = *y + (*y - *last_control_point_y);
    }
  else
    {
      qx1 = *x;
      qy1 = *y;
    }

  if (cmd->curve.relative)
    {
      qx2 = *x + cmd->curve.x;
      qy2 = *y + cmd->curve.y;
    }
  else
    {
      qx2 = cmd->curve.x;
      qy2 = cmd->curve.y;
    }

  /* We need to convert the quadratic into a cubic bezier. */
  x1 = *x + (qx1 - *x) * 2.0 / 3.0;
  y1 = *y + (qy1 - *y) * 2.0 / 3.0;

  x2 = x1 + (qx2 - *x) / 3.0;
  y2 = y1 + (qy2 - *y) / 3.0;

  cairo_curve_to (cr, x1, y1, x2, y2, qx2, qy2);

  *x = qx2;
  *y = qy2;
  *last_control_point_x = qx1;
  *last_control_point_y = qy1;
}


static gdouble
calc_angle (gdouble ux, gdouble uy, gdouble vx, gdouble vy)
{
  gdouble top, u_magnitude, v_magnitude, angle_cos, angle;

  top = ux * vx + uy * vy;
  u_magnitude = sqrt (ux * ux + uy * uy);
  v_magnitude = sqrt (vx * vx + vy * vy);
  angle_cos = top / (u_magnitude * v_magnitude);

  /* We check if the cosine is slightly out-of-bounds. */
  if (angle_cos >= 1.0)
    angle = 0.0;
  if (angle_cos <= -1.0)
    angle = M_PI;
  else
    angle = acos (angle_cos);

  if (ux * vy - uy * vx < 0)
    angle = - angle;

  return angle;
}


/* FIXME: Maybe we should do these calculations once when the path data is
   parsed, and keep the cairo parameters we need in the command instead. */
static void
do_elliptical_arc (GooCanvasPathCommand    *cmd,
		   cairo_t                 *cr,
		   gdouble                 *x,
		   gdouble                 *y)
{
  gdouble x1 = *x, y1 = *y, x2, y2, rx, ry, lambda;
  gdouble v1, v2, angle, angle_sin, angle_cos, x11, y11;
  gdouble rx_squared, ry_squared, x11_squared, y11_squared, top, bottom;
  gdouble c, cx1, cy1, cx, cy, start_angle, angle_delta;

  /* Calculate the end point of the arc - x2,y2. */
  if (cmd->arc.relative)
    {
      x2 = x1 + cmd->arc.x;
      y2 = y1 + cmd->arc.y;
    }
  else
    {
      x2 = cmd->arc.x;
      y2 = cmd->arc.y;
    }

  *x = x2;
  *y = y2;

  /* If the endpoints are exactly the same, just return (see SVG spec). */
  if (x1 == x2 && y1 == y2)
    return;

  /* If either rx or ry is 0, do a simple lineto (see SVG spec). */
  if (cmd->arc.rx == 0.0 || cmd->arc.ry == 0.0)
    {
      cairo_line_to (cr, x2, y2);
      return;
    }

  /* Calculate x1' and y1' (as per SVG implementation notes). */
  v1 = (x1 - x2) / 2.0;
  v2 = (y1 - y2) / 2.0;

  angle = cmd->arc.x_axis_rotation * (M_PI / 180.0);
  angle_sin = sin (angle);
  angle_cos = cos (angle);

  x11 = (angle_cos * v1) + (angle_sin * v2);
  y11 = - (angle_sin * v1) + (angle_cos * v2);

  /* Ensure rx and ry are positive and large enough. */
  rx = cmd->arc.rx > 0.0 ? cmd->arc.rx : - cmd->arc.rx;
  ry = cmd->arc.ry > 0.0 ? cmd->arc.ry : - cmd->arc.ry;
  lambda = (x11 * x11) / (rx * rx) + (y11 * y11) / (ry * ry);
  if (lambda > 1.0)
    {
      gdouble square_root = sqrt (lambda);
      rx *= square_root;
      ry *= square_root;
    }

  /* Calculate cx' and cy'. */
  rx_squared = rx * rx;
  ry_squared = ry * ry;
  x11_squared = x11 * x11;
  y11_squared = y11 * y11;

  top = (rx_squared * ry_squared) - (rx_squared * y11_squared)
    - (ry_squared * x11_squared);
  if (top < 0.0)
    {
      c = 0.0;
    }
  else
    {
      bottom = (rx_squared * y11_squared) + (ry_squared * x11_squared);
      c = sqrt (top / bottom);
    }

  if (cmd->arc.large_arc_flag == cmd->arc.sweep_flag)
    c = - c;

  cx1 = c * ((rx * y11) / ry);
  cy1 = c * (- (ry * x11) / rx);

  /* Calculate cx and cy. */
  cx = (angle_cos * cx1) - (angle_sin * cy1) + (x1 + x2) / 2;
  cy = (angle_sin * cx1) + (angle_cos * cy1) + (y1 + y2) / 2;

  /* Calculate the start and end angles. */
  v1 = (x11 - cx1) / rx;
  v2 = (y11 - cy1) / ry;

  start_angle = calc_angle (1, 0, v1, v2);
  angle_delta = calc_angle (v1, v2, (-x11 - cx1) / rx, (-y11 - cy1) / ry);

  if (cmd->arc.sweep_flag == 0 && angle_delta > 0.0)
    angle_delta -= 2 * M_PI;
  else if (cmd->arc.sweep_flag == 1 && angle_delta < 0.0)
    angle_delta += 2 * M_PI;

  /* Now draw the arc. */
  cairo_save (cr);
  cairo_translate (cr, cx, cy);
  cairo_rotate (cr, angle);
  cairo_scale (cr, rx, ry);

  if (angle_delta > 0.0)
    cairo_arc (cr, 0.0, 0.0, 1.0,
	       start_angle, start_angle + angle_delta);
  else
    cairo_arc_negative (cr, 0.0, 0.0, 1.0,
			start_angle, start_angle + angle_delta);

  cairo_restore (cr);
}


/**
 * goo_canvas_create_path:
 * @commands: an array of #GooCanvasPathCommand.
 * @cr: a cairo context.
 * 
 * Creates the path specified by the given #GooCanvasPathCommand array.
 **/
void
goo_canvas_create_path (GArray              *commands,
			cairo_t             *cr)
{
  GooCanvasPathCommand *cmd;
  GooCanvasPathCommandType prev_cmd_type = GOO_CANVAS_PATH_CLOSE_PATH;
  gdouble x = 0, y = 0, path_start_x = 0, path_start_y = 0;
  gdouble last_control_point_x = 0.0, last_control_point_y = 0.0;
  gint i;

  cairo_new_path (cr);

  if (!commands || commands->len == 0)
    return;

  for (i = 0; i < commands->len; i++)
    {
      cmd = &g_array_index (commands, GooCanvasPathCommand, i);
      switch (cmd->simple.type)
	{
	  /* Simple commands like moveto and lineto: MmZzLlHhVv. */
	case GOO_CANVAS_PATH_MOVE_TO:
	  if (cmd->simple.relative)
	    {
	      x += cmd->simple.x;
	      y += cmd->simple.y;
	    }
	  else
	    {
	      x = cmd->simple.x;
	      y = cmd->simple.y;
	    }
	  path_start_x = x;
	  path_start_y = y;
	  cairo_move_to (cr, x, y);
	  break;

	case GOO_CANVAS_PATH_CLOSE_PATH:
	  x = path_start_x;
	  y = path_start_y;
	  cairo_close_path (cr);
	  break;

	case GOO_CANVAS_PATH_LINE_TO:
	  if (cmd->simple.relative)
	    {
	      x += cmd->simple.x;
	      y += cmd->simple.y;
	    }
	  else
	    {
	      x = cmd->simple.x;
	      y = cmd->simple.y;
	    }
	  cairo_line_to (cr, x, y);
	  break;

	case GOO_CANVAS_PATH_HORIZONTAL_LINE_TO:
	  if (cmd->simple.relative)
	    x += cmd->simple.x;
	  else
	    x = cmd->simple.x;
	  cairo_line_to (cr, x, y);
	  break;

	case GOO_CANVAS_PATH_VERTICAL_LINE_TO:
	  if (cmd->simple.relative)
	    y += cmd->simple.y;
	  else
	    y = cmd->simple.y;
	  cairo_line_to (cr, x, y);
	  break;

	  /* Bezier curve commands: CcSsQqTt. */
	case GOO_CANVAS_PATH_CURVE_TO:
	  do_curve_to (cmd, cr, &x, &y,
		       &last_control_point_x, &last_control_point_y);
	  break;

	case GOO_CANVAS_PATH_SMOOTH_CURVE_TO:
	  do_smooth_curve_to (cmd, prev_cmd_type, cr, &x, &y,
			      &last_control_point_x, &last_control_point_y);
	  break;

	case GOO_CANVAS_PATH_QUADRATIC_CURVE_TO:
	  do_quadratic_curve_to (cmd, cr, &x, &y,
				 &last_control_point_x, &last_control_point_y);
	  break;

	case GOO_CANVAS_PATH_SMOOTH_QUADRATIC_CURVE_TO:
	  do_smooth_quadratic_curve_to (cmd, prev_cmd_type, cr, &x, &y,
					&last_control_point_x,
					&last_control_point_y);
	  break;

	  /* The elliptical arc commands: Aa. */
	case GOO_CANVAS_PATH_ELLIPTICAL_ARC:
	  do_elliptical_arc (cmd, cr, &x, &y);
	  break;
	}

      prev_cmd_type = cmd->simple.type;
    }
}


/* This is a copy of _gtk_boolean_handled_accumulator. */
gboolean
goo_canvas_boolean_handled_accumulator (GSignalInvocationHint *ihint,
					GValue                *return_accu,
					const GValue          *handler_return,
					gpointer               dummy)
{
  gboolean continue_emission;
  gboolean signal_handled;
  
  signal_handled = g_value_get_boolean (handler_return);
  g_value_set_boolean (return_accu, signal_handled);
  continue_emission = !signal_handled;
  
  return continue_emission;
}



