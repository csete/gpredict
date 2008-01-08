/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasutils.h - utility functions.
 */
#ifndef __GOO_CANVAS_UTILS_H__
#define __GOO_CANVAS_UTILS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


/*
 * Enum types.
 */

/**
 * GooCanvasPointerEvents
 * @GOO_CANVAS_EVENTS_VISIBLE_MASK: a mask indicating that the item only
 *  receives events when it is visible.
 * @GOO_CANVAS_EVENTS_PAINTED_MASK: a mask indicating that the item only
 *  receives events when the specified parts of it are painted.
 * @GOO_CANVAS_EVENTS_FILL_MASK: a mask indicating that the filled part of
 *  the item receives events.
 * @GOO_CANVAS_EVENTS_STROKE_MASK: a mask indicating that the stroked part
 *  of the item receives events.
 * @GOO_CANVAS_EVENTS_NONE: the item doesn't receive events at all.
 * @GOO_CANVAS_EVENTS_VISIBLE_PAINTED: the item receives events in its
 *  painted areas when it is visible (the default).
 * @GOO_CANVAS_EVENTS_VISIBLE_FILL: the item's interior receives events
 *  when it is visible.
 * @GOO_CANVAS_EVENTS_VISIBLE_STROKE: the item's perimeter receives
 *  events when it is visible.
 * @GOO_CANVAS_EVENTS_VISIBLE: the item receives events when it is visible,
 *  whether it is painted or not.
 * @GOO_CANVAS_EVENTS_PAINTED: the item receives events in its painted areas,
 *  whether it is visible or not.
 * @GOO_CANVAS_EVENTS_FILL: the item's interior receives events, whether it
 *  is visible or painted or not.
 * @GOO_CANVAS_EVENTS_STROKE: the item's perimeter receives events, whether
 *  it is visible or painted or not.
 * @GOO_CANVAS_EVENTS_ALL: the item's perimeter and interior receive events,
 *  whether it is visible or painted or not.
 *
 * Specifies when an item receives pointer events such as mouse clicks.
 */
typedef enum
{
  GOO_CANVAS_EVENTS_VISIBLE_MASK	= 1 << 0,
  GOO_CANVAS_EVENTS_PAINTED_MASK	= 1 << 1,
  GOO_CANVAS_EVENTS_FILL_MASK		= 1 << 2,
  GOO_CANVAS_EVENTS_STROKE_MASK		= 1 << 3,

  GOO_CANVAS_EVENTS_NONE		= 0,
  GOO_CANVAS_EVENTS_VISIBLE_PAINTED	= GOO_CANVAS_EVENTS_VISIBLE_MASK | GOO_CANVAS_EVENTS_PAINTED_MASK | GOO_CANVAS_EVENTS_FILL_MASK | GOO_CANVAS_EVENTS_STROKE_MASK,
  GOO_CANVAS_EVENTS_VISIBLE_FILL	= GOO_CANVAS_EVENTS_VISIBLE_MASK | GOO_CANVAS_EVENTS_FILL_MASK,
  GOO_CANVAS_EVENTS_VISIBLE_STROKE	= GOO_CANVAS_EVENTS_VISIBLE_MASK | GOO_CANVAS_EVENTS_STROKE_MASK,
  GOO_CANVAS_EVENTS_VISIBLE		= GOO_CANVAS_EVENTS_VISIBLE_MASK | GOO_CANVAS_EVENTS_FILL_MASK | GOO_CANVAS_EVENTS_STROKE_MASK,
  GOO_CANVAS_EVENTS_PAINTED		= GOO_CANVAS_EVENTS_PAINTED_MASK | GOO_CANVAS_EVENTS_FILL_MASK | GOO_CANVAS_EVENTS_STROKE_MASK,
  GOO_CANVAS_EVENTS_FILL		= GOO_CANVAS_EVENTS_FILL_MASK,
  GOO_CANVAS_EVENTS_STROKE		= GOO_CANVAS_EVENTS_STROKE_MASK,
  GOO_CANVAS_EVENTS_ALL			= GOO_CANVAS_EVENTS_FILL_MASK | GOO_CANVAS_EVENTS_STROKE_MASK
} GooCanvasPointerEvents;


/**
 * GooCanvasItemVisibility
 * @GOO_CANVAS_ITEM_HIDDEN: the item is invisible, and is not allocated any
 *  space in layout container items such as #GooCanvasTable.
 * @GOO_CANVAS_ITEM_INVISIBLE: the item is invisible, but it is still allocated
 *  space in layout container items.
 * @GOO_CANVAS_ITEM_VISIBLE: the item is visible.
 * @GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD: the item is visible when the
 *  canvas scale setting is greater than or equal to the item's visibility
 *  threshold setting.
 *
 * The #GooCanvasItemVisibility enumeration is used to specify when a canvas
 * item is visible.
 */
/* NOTE: Values are important here, as we test for <= GOO_CANVAS_ITEM_INVISIBLE
   to check if an item is invisible or hidden. */
typedef enum
{
  GOO_CANVAS_ITEM_HIDDEN			= 0,
  GOO_CANVAS_ITEM_INVISIBLE			= 1,
  GOO_CANVAS_ITEM_VISIBLE			= 2,
  GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD	= 3,
} GooCanvasItemVisibility;


/**
 * GooCanvasPathCommandType
 * @GOO_CANVAS_PATH_MOVE_TO: move to the given point.
 * @GOO_CANVAS_PATH_CLOSE_PATH: close the current path, drawing a line from the
 *  current position to the start of the path.
 * @GOO_CANVAS_PATH_LINE_TO: draw a line to the given point.
 * @GOO_CANVAS_PATH_HORIZONTAL_LINE_TO: draw a horizontal line to the given
 *  x coordinate.
 * @GOO_CANVAS_PATH_VERTICAL_LINE_TO: draw a vertical line to the given y
 *  coordinate.
 * @GOO_CANVAS_PATH_CURVE_TO: draw a bezier curve using two control
 *  points to the given point.
 * @GOO_CANVAS_PATH_SMOOTH_CURVE_TO: draw a bezier curve using a reflection
 *  of the last control point of the last curve as the first control point,
 *  and one new control point, to the given point.
 * @GOO_CANVAS_PATH_QUADRATIC_CURVE_TO: draw a quadratic bezier curve using
 *  a single control point to the given point.
 * @GOO_CANVAS_PATH_SMOOTH_QUADRATIC_CURVE_TO: draw a quadratic bezier curve
 *  using a reflection of the control point from the previous curve as the
 *  control point, to the given point.
 * @GOO_CANVAS_PATH_ELLIPTICAL_ARC: draw an elliptical arc, using the given
 *  2 radii, the x axis rotation, and the 2 flags to disambiguate the arc,
 *  to the given point.
 *
 * GooCanvasPathCommandType specifies the type of each command in the path.
 * See the path element in the <ulink url="http://www.w3.org/Graphics/SVG/">
 * Scalable Vector Graphics (SVG) specification</ulink> for more details.
 */
typedef enum
{
  /* Simple commands like moveto and lineto: MmZzLlHhVv. */
  GOO_CANVAS_PATH_MOVE_TO,
  GOO_CANVAS_PATH_CLOSE_PATH,
  GOO_CANVAS_PATH_LINE_TO,
  GOO_CANVAS_PATH_HORIZONTAL_LINE_TO,
  GOO_CANVAS_PATH_VERTICAL_LINE_TO,

  /* Bezier curve commands: CcSsQqTt. */
  GOO_CANVAS_PATH_CURVE_TO,
  GOO_CANVAS_PATH_SMOOTH_CURVE_TO,
  GOO_CANVAS_PATH_QUADRATIC_CURVE_TO,
  GOO_CANVAS_PATH_SMOOTH_QUADRATIC_CURVE_TO,

  /* The elliptical arc commands: Aa. */
  GOO_CANVAS_PATH_ELLIPTICAL_ARC
} GooCanvasPathCommandType;


typedef union _GooCanvasPathCommand  GooCanvasPathCommand;

/* Note that the command type is always the first element in each struct, so
   we can always use it whatever type of command it is. */

/**
 * GooCanvasPathCommand
 *
 * GooCanvasPathCommand holds the data for each command in the path.
 *
 * The @relative flag specifies that the coordinates for the command are
 * relative to the current point. Otherwise they are assumed to be absolute
 * coordinates.
 */
union _GooCanvasPathCommand
{
  /* Simple commands like moveto and lineto: MmZzLlHhVv. */
  struct {
    guint type : 5; /* GooCanvasPathCommandType */
    guint relative : 1;
    gdouble x, y;
  } simple;

  /* Bezier curve commands: CcSsQqTt. */
  struct {
    guint type : 5; /* GooCanvasPathCommandType */
    guint relative : 1;
    gdouble x, y, x1, y1, x2, y2;
  } curve;

  /* The elliptical arc commands: Aa. */
  struct {
    guint type : 5; /* GooCanvasPathCommandType */
    guint relative : 1;
    guint large_arc_flag : 1;
    guint sweep_flag : 1;
    gdouble rx, ry, x_axis_rotation, x, y;
  } arc;
};


GArray*	goo_canvas_parse_path_data	(const gchar       *path_data);
void	goo_canvas_create_path		(GArray		   *commands,
					 cairo_t           *cr);


/*
 * Cairo utilities.
 */
typedef struct _GooCanvasLineDash GooCanvasLineDash;

/**
 * GooCanvasLineDash
 * @ref_count: the reference count of the struct.
 * @num_dashes: the number of dashes and gaps between them.
 * @dashes: the sizes of each dash and gap.
 * @dash_offset: the start offset into the dash pattern.
 *
 * #GooCanvasLineDash specifies a dash pattern to be used when drawing items.
 */
struct _GooCanvasLineDash
{
  int ref_count;
  int num_dashes;
  double *dashes;
  double dash_offset;
};

#define GOO_TYPE_CANVAS_LINE_DASH  (goo_canvas_line_dash_get_type ())
GType              goo_canvas_line_dash_get_type (void) G_GNUC_CONST;
GooCanvasLineDash* goo_canvas_line_dash_new   (gint               num_dashes,
					       ...);
GooCanvasLineDash* goo_canvas_line_dash_newv  (gint               num_dashes,
                                               double            *dashes);
GooCanvasLineDash* goo_canvas_line_dash_ref   (GooCanvasLineDash *dash);
void               goo_canvas_line_dash_unref (GooCanvasLineDash *dash);

#define GOO_TYPE_CAIRO_MATRIX	   (goo_cairo_matrix_get_type())
GType              goo_cairo_matrix_get_type  (void) G_GNUC_CONST;
cairo_matrix_t*    goo_cairo_matrix_copy      (const cairo_matrix_t *matrix);
void               goo_cairo_matrix_free      (cairo_matrix_t       *matrix);

#define GOO_TYPE_CAIRO_PATTERN	   (goo_cairo_pattern_get_type ())
GType              goo_cairo_pattern_get_type (void) G_GNUC_CONST;

#define GOO_TYPE_CAIRO_FILL_RULE   (goo_cairo_fill_rule_get_type ())
GType		   goo_cairo_fill_rule_get_type (void) G_GNUC_CONST;

#define GOO_TYPE_CAIRO_OPERATOR    (goo_cairo_operator_get_type())
GType		   goo_cairo_operator_get_type  (void) G_GNUC_CONST;

#define GOO_TYPE_CAIRO_ANTIALIAS   (goo_cairo_antialias_get_type())
GType		   goo_cairo_antialias_get_type (void) G_GNUC_CONST;

#define GOO_TYPE_CAIRO_LINE_CAP    (goo_cairo_line_cap_get_type ())
GType		   goo_cairo_line_cap_get_type  (void) G_GNUC_CONST;

#define GOO_TYPE_CAIRO_LINE_JOIN   (goo_cairo_line_join_get_type ())
GType		   goo_cairo_line_join_get_type (void) G_GNUC_CONST;

#define GOO_TYPE_CAIRO_HINT_METRICS (goo_cairo_hint_metrics_get_type ())
GType		   goo_cairo_hint_metrics_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GOO_CANVAS_UTILS_H__ */
