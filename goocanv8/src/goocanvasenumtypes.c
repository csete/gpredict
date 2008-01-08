
/* Generated data (by glib-mkenums) */

#include <goocanvas.h>
#include <glib-object.h>

/* Enumerations from "goocanvasitem.h" */
GType
goo_canvas_animate_type_get_type (void)
{
  static GType etype = 0;
 if( etype == 0 ) 
  {
    static const GEnumValue values[] = {
      { GOO_CANVAS_ANIMATE_FREEZE, "GOO_CANVAS_ANIMATE_FREEZE", "freeze" },
      { GOO_CANVAS_ANIMATE_RESET, "GOO_CANVAS_ANIMATE_RESET", "reset" },
      { GOO_CANVAS_ANIMATE_RESTART, "GOO_CANVAS_ANIMATE_RESTART", "restart" },
      { GOO_CANVAS_ANIMATE_BOUNCE, "GOO_CANVAS_ANIMATE_BOUNCE", "bounce" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCanvasAnimateType", values );
  }
  return etype;
}

/* Enumerations from "goocanvasutils.h" */
GType
goo_canvas_pointer_events_get_type (void)
{
  static GType etype = 0;
 if( etype == 0 ) 
  {
    static const GFlagsValue values[] = {
      { GOO_CANVAS_EVENTS_VISIBLE_MASK, "GOO_CANVAS_EVENTS_VISIBLE_MASK", "visible-mask" },
      { GOO_CANVAS_EVENTS_PAINTED_MASK, "GOO_CANVAS_EVENTS_PAINTED_MASK", "painted-mask" },
      { GOO_CANVAS_EVENTS_FILL_MASK, "GOO_CANVAS_EVENTS_FILL_MASK", "fill-mask" },
      { GOO_CANVAS_EVENTS_STROKE_MASK, "GOO_CANVAS_EVENTS_STROKE_MASK", "stroke-mask" },
      { GOO_CANVAS_EVENTS_NONE, "GOO_CANVAS_EVENTS_NONE", "none" },
      { GOO_CANVAS_EVENTS_VISIBLE_PAINTED, "GOO_CANVAS_EVENTS_VISIBLE_PAINTED", "visible-painted" },
      { GOO_CANVAS_EVENTS_VISIBLE_FILL, "GOO_CANVAS_EVENTS_VISIBLE_FILL", "visible-fill" },
      { GOO_CANVAS_EVENTS_VISIBLE_STROKE, "GOO_CANVAS_EVENTS_VISIBLE_STROKE", "visible-stroke" },
      { GOO_CANVAS_EVENTS_VISIBLE, "GOO_CANVAS_EVENTS_VISIBLE", "visible" },
      { GOO_CANVAS_EVENTS_PAINTED, "GOO_CANVAS_EVENTS_PAINTED", "painted" },
      { GOO_CANVAS_EVENTS_FILL, "GOO_CANVAS_EVENTS_FILL", "fill" },
      { GOO_CANVAS_EVENTS_STROKE, "GOO_CANVAS_EVENTS_STROKE", "stroke" },
      { GOO_CANVAS_EVENTS_ALL, "GOO_CANVAS_EVENTS_ALL", "all" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("GooCanvasPointerEvents", values );
  }
  return etype;
}
GType
goo_canvas_item_visibility_get_type (void)
{
  static GType etype = 0;
 if( etype == 0 ) 
  {
    static const GEnumValue values[] = {
      { GOO_CANVAS_ITEM_HIDDEN, "GOO_CANVAS_ITEM_HIDDEN", "hidden" },
      { GOO_CANVAS_ITEM_INVISIBLE, "GOO_CANVAS_ITEM_INVISIBLE", "invisible" },
      { GOO_CANVAS_ITEM_VISIBLE, "GOO_CANVAS_ITEM_VISIBLE", "visible" },
      { GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD, "GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD", "visible-above-threshold" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCanvasItemVisibility", values );
  }
  return etype;
}
GType
goo_canvas_path_command_type_get_type (void)
{
  static GType etype = 0;
 if( etype == 0 ) 
  {
    static const GEnumValue values[] = {
      { GOO_CANVAS_PATH_MOVE_TO, "GOO_CANVAS_PATH_MOVE_TO", "move-to" },
      { GOO_CANVAS_PATH_CLOSE_PATH, "GOO_CANVAS_PATH_CLOSE_PATH", "close-path" },
      { GOO_CANVAS_PATH_LINE_TO, "GOO_CANVAS_PATH_LINE_TO", "line-to" },
      { GOO_CANVAS_PATH_HORIZONTAL_LINE_TO, "GOO_CANVAS_PATH_HORIZONTAL_LINE_TO", "horizontal-line-to" },
      { GOO_CANVAS_PATH_VERTICAL_LINE_TO, "GOO_CANVAS_PATH_VERTICAL_LINE_TO", "vertical-line-to" },
      { GOO_CANVAS_PATH_CURVE_TO, "GOO_CANVAS_PATH_CURVE_TO", "curve-to" },
      { GOO_CANVAS_PATH_SMOOTH_CURVE_TO, "GOO_CANVAS_PATH_SMOOTH_CURVE_TO", "smooth-curve-to" },
      { GOO_CANVAS_PATH_QUADRATIC_CURVE_TO, "GOO_CANVAS_PATH_QUADRATIC_CURVE_TO", "quadratic-curve-to" },
      { GOO_CANVAS_PATH_SMOOTH_QUADRATIC_CURVE_TO, "GOO_CANVAS_PATH_SMOOTH_QUADRATIC_CURVE_TO", "smooth-quadratic-curve-to" },
      { GOO_CANVAS_PATH_ELLIPTICAL_ARC, "GOO_CANVAS_PATH_ELLIPTICAL_ARC", "elliptical-arc" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GooCanvasPathCommandType", values );
  }
  return etype;
}

/* Generated data ends here */

