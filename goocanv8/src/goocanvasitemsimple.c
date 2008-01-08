/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasitemsimple.c - abstract base class for canvas items.
 */

/**
 * SECTION:goocanvasitemsimple
 * @Title: GooCanvasItemSimple
 * @Short_Description: the base class for the standard canvas items.
 * @Stability_Level: 
 * @See_Also: 
 *
 * #GooCanvasItemSimple is used as a base class for all of the standard canvas
 * items. It can also be used as the base class for new custom canvas items.
 *
 * It provides default implementations for many of the #GooCanvasItem
 * methods.
 *
 * For very simple items, all that is needed is to implement the create_path()
 * method. (#GooCanvasEllipse, #GooCanvasRect and #GooCanvasPath do this.)
 *
 * More complicated items need to implement the update(), paint() and
 * is_item_at() methods. (#GooCanvasImage, #GooCanvasPolyline,
 * #GooCanvasText and #GooCanvasWidget do this.) They may also need to
 * override some of the other GooCanvasItem methods such as set_canvas(),
 * set_parent() or allocate_area() if special code is needed. (#GooCanvasWidget
 * does this to make sure the #GtkWidget is embedded in the #GooCanvas widget
 * correctly.)
 */
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "goocanvasprivate.h"
#include "goocanvasitemsimple.h"
#include "goocanvas.h"
#include "goocanvasatk.h"


enum {
  PROP_0,

  /* Basic drawing properties. */
  PROP_STROKE_PATTERN,
  PROP_FILL_PATTERN,
  PROP_FILL_RULE,
  PROP_OPERATOR,
  PROP_ANTIALIAS,

  /* Line style & width properties. */
  PROP_LINE_WIDTH,
  PROP_LINE_CAP,
  PROP_LINE_JOIN,
  PROP_LINE_JOIN_MITER_LIMIT,
  PROP_LINE_DASH,

  /* Font properties. */
  PROP_FONT,
  PROP_FONT_DESC,
  PROP_HINT_METRICS,

  /* Convenience properties. */
  PROP_STROKE_COLOR,
  PROP_STROKE_COLOR_RGBA,
  PROP_STROKE_PIXBUF,
  PROP_FILL_COLOR,
  PROP_FILL_COLOR_RGBA,
  PROP_FILL_PIXBUF,

  /* Other properties. Note that the order here is important PROP_TRANSFORM
     must be the first non-style property. see set_property(). */
  PROP_TRANSFORM,
  PROP_PARENT,
  PROP_VISIBILITY,
  PROP_VISIBILITY_THRESHOLD,
  PROP_POINTER_EVENTS,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_CAN_FOCUS,
  PROP_CLIP_PATH,
  PROP_CLIP_FILL_RULE
};


static void canvas_item_interface_init          (GooCanvasItemIface   *iface);
static void goo_canvas_item_simple_dispose      (GObject              *object);
static void goo_canvas_item_simple_finalize     (GObject              *object);
static void goo_canvas_item_simple_get_property (GObject              *object,
						 guint                 prop_id,
						 GValue               *value,
						 GParamSpec           *pspec);
static void goo_canvas_item_simple_set_property (GObject              *object,
						 guint                 prop_id,
						 const GValue         *value,
						 GParamSpec           *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasItemSimple, goo_canvas_item_simple,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM,
						canvas_item_interface_init))


static void
goo_canvas_item_simple_install_common_properties (GObjectClass *gobject_class)
{
  /* Basic drawing properties. */
  g_object_class_install_property (gobject_class, PROP_STROKE_PATTERN,
                                   g_param_spec_boxed ("stroke-pattern",
						       _("Stroke Pattern"),
						       _("The pattern to use to paint the perimeter of the item, or NULL disable painting"),
						       GOO_TYPE_CAIRO_PATTERN,
						       G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FILL_PATTERN,
                                   g_param_spec_boxed ("fill-pattern",
						       _("Fill Pattern"),
						       _("The pattern to use to paint the interior of the item, or NULL to disable painting"),
						       GOO_TYPE_CAIRO_PATTERN,
						       G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FILL_RULE,
				   g_param_spec_enum ("fill-rule",
						      _("Fill Rule"),
						      _("The fill rule used to determine which parts of the item are filled"),
						      GOO_TYPE_CAIRO_FILL_RULE,
						      CAIRO_FILL_RULE_WINDING,
						      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_OPERATOR,
				   g_param_spec_enum ("operator",
						      _("Operator"),
						      _("The compositing operator to use"),
						      GOO_TYPE_CAIRO_OPERATOR,
						      CAIRO_OPERATOR_OVER,
						      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ANTIALIAS,
				   g_param_spec_enum ("antialias",
						      _("Antialias"),
						      _("The antialiasing mode to use"),
						      GOO_TYPE_CAIRO_ANTIALIAS,
						      CAIRO_ANTIALIAS_GRAY,
						      G_PARAM_READWRITE));

  /* Line style & width properties. */
  g_object_class_install_property (gobject_class, PROP_LINE_WIDTH,
				   g_param_spec_double ("line-width",
							_("Line Width"),
							_("The line width to use for the item's perimeter"),
							0.0, G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LINE_CAP,
				   g_param_spec_enum ("line-cap",
						      _("Line Cap"),
						      _("The line cap style to use"),
						      GOO_TYPE_CAIRO_LINE_CAP,
						      CAIRO_LINE_CAP_BUTT,
						      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LINE_JOIN,
				   g_param_spec_enum ("line-join",
						      _("Line Join"),
						      _("The line join style to use"),
						      GOO_TYPE_CAIRO_LINE_JOIN,
						      CAIRO_LINE_JOIN_MITER,
						      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LINE_JOIN_MITER_LIMIT,
				   g_param_spec_double ("line-join-miter-limit",
							_("Miter Limit"),
							_("The smallest angle to use with miter joins, in degrees. Bevel joins will be used below this limit"),
							0.0, G_MAXDOUBLE, 10.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LINE_DASH,
				   g_param_spec_boxed ("line-dash",
						       _("Line Dash"),
						       _("The dash pattern to use"),
						       GOO_TYPE_CANVAS_LINE_DASH,
						       G_PARAM_READWRITE));

  /* Font properties. */
  g_object_class_install_property (gobject_class, PROP_FONT,
				   g_param_spec_string ("font",
							_("Font"),
							_("The base font to use for the text"),
							NULL,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FONT_DESC,
				   g_param_spec_boxed ("font-desc",
						       _("Font Description"),
						       _("The attributes specifying which font to use"),
						       PANGO_TYPE_FONT_DESCRIPTION,
						       G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HINT_METRICS,
				   g_param_spec_enum ("hint-metrics",
						      _("Hint Metrics"),
						      _("The hinting to be used for font metrics"),
						      GOO_TYPE_CAIRO_HINT_METRICS,
						      CAIRO_HINT_METRICS_OFF,
						      G_PARAM_READWRITE));

  /* Convenience properties - writable only. */
  g_object_class_install_property (gobject_class, PROP_STROKE_COLOR,
				   g_param_spec_string ("stroke-color",
							_("Stroke Color"),
							_("The color to use for the item's perimeter. To disable painting set the 'stroke-pattern' property to NULL"),
							NULL,
							G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_STROKE_COLOR_RGBA,
				   g_param_spec_uint ("stroke-color-rgba",
						      _("Stroke Color RGBA"),
						      _("The color to use for the item's perimeter, specified as a 32-bit integer value. To disable painting set the 'stroke-pattern' property to NULL"),
						      0, G_MAXUINT, 0,
						      G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_STROKE_PIXBUF,
                                   g_param_spec_object ("stroke-pixbuf",
							_("Stroke Pixbuf"),
							_("The pixbuf to use to draw the item's perimeter. To disable painting set the 'stroke-pattern' property to NULL"),
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_FILL_COLOR,
				   g_param_spec_string ("fill-color",
							_("Fill Color"),
							_("The color to use to paint the interior of the item. To disable painting set the 'fill-pattern' property to NULL"),
							NULL,
							G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_FILL_COLOR_RGBA,
				   g_param_spec_uint ("fill-color-rgba",
						      _("Fill Color RGBA"),
						      _("The color to use to paint the interior of the item, specified as a 32-bit integer value. To disable painting set the 'fill-pattern' property to NULL"),
						      0, G_MAXUINT, 0,
						      G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_FILL_PIXBUF,
                                   g_param_spec_object ("fill-pixbuf",
							_("Fill Pixbuf"),
							_("The pixbuf to use to paint the interior of the item. To disable painting set the 'fill-pattern' property to NULL"),
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_WRITABLE));

  /* Other properties. */
  g_object_class_override_property (gobject_class, PROP_PARENT,
				    "parent");

  g_object_class_override_property (gobject_class, PROP_VISIBILITY,
				    "visibility");

  g_object_class_override_property (gobject_class, PROP_VISIBILITY_THRESHOLD,
				    "visibility-threshold");

  g_object_class_override_property (gobject_class, PROP_TRANSFORM,
				    "transform");

  g_object_class_override_property (gobject_class, PROP_POINTER_EVENTS,
				    "pointer-events");

  g_object_class_override_property (gobject_class, PROP_TITLE,
				    "title");

  g_object_class_override_property (gobject_class, PROP_DESCRIPTION,
				    "description");

  g_object_class_override_property (gobject_class, PROP_CAN_FOCUS,
				    "can-focus");

  /**
   * GooCanvasItemSimple:clip-path
   *
   * The sequence of commands describing the clip path of the item, specified
   * as a string using the same syntax
   * as in the <ulink url="http://www.w3.org/Graphics/SVG/">Scalable Vector
   * Graphics (SVG)</ulink> path element.
   */
  /**
   * GooCanvasItemModelSimple:clip-path
   *
   * The sequence of commands describing the clip path of the item, specified
   * as a string using the same syntax
   * as in the <ulink url="http://www.w3.org/Graphics/SVG/">Scalable Vector
   * Graphics (SVG)</ulink> path element.
   */
  g_object_class_install_property (gobject_class, PROP_CLIP_PATH,
				   g_param_spec_string ("clip-path",
							_("Clip Path"),
							_("The sequence of path commands specifying the clip path"),
							NULL,
							G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_CLIP_FILL_RULE,
				   g_param_spec_enum ("clip-fill-rule",
						      _("Clip Fill Rule"),
						      _("The fill rule used to determine which parts of the item are clipped"),
						      GOO_TYPE_CAIRO_FILL_RULE,
						      CAIRO_FILL_RULE_WINDING,
						      G_PARAM_READWRITE));

}


static void
goo_canvas_item_simple_class_init (GooCanvasItemSimpleClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->dispose  = goo_canvas_item_simple_dispose;
  gobject_class->finalize = goo_canvas_item_simple_finalize;

  gobject_class->get_property = goo_canvas_item_simple_get_property;
  gobject_class->set_property = goo_canvas_item_simple_set_property;

  /* Register our accessible factory, but only if accessibility is enabled. */
  if (!ATK_IS_NO_OP_OBJECT_FACTORY (atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_WIDGET)))
    {
      atk_registry_set_factory_type (atk_get_default_registry (),
				     GOO_TYPE_CANVAS_ITEM_SIMPLE,
				     goo_canvas_item_accessible_factory_get_type ());
    }

  goo_canvas_item_simple_install_common_properties (gobject_class);
}


static void
goo_canvas_item_simple_init (GooCanvasItemSimple *item)
{
  GooCanvasBounds *bounds = &item->bounds;

  bounds->x1 = bounds->y1 = bounds->x2 = bounds->y2 = 0.0;
  item->simple_data = g_slice_new0 (GooCanvasItemSimpleData);
  item->simple_data->visibility = GOO_CANVAS_ITEM_VISIBLE;
  item->simple_data->pointer_events = GOO_CANVAS_EVENTS_VISIBLE_PAINTED;
  item->simple_data->clip_fill_rule = CAIRO_FILL_RULE_WINDING;
  item->need_update = TRUE;
  item->need_entire_subtree_update = TRUE;
}


static void
goo_canvas_item_simple_reset_model (GooCanvasItemSimple *simple)
{
  if (simple->model)
    {
      g_signal_handlers_disconnect_matched (simple->model, G_SIGNAL_MATCH_DATA,
					    0, 0, NULL, NULL, simple);
      g_object_unref (simple->model);
      simple->model = NULL;
      simple->simple_data = NULL;
    }
}


/* Frees the contents of the GooCanvasItemSimpleData, but not the struct. */
static void
goo_canvas_item_simple_free_data (GooCanvasItemSimpleData *simple_data)
{
  if (simple_data)
    {
      if (simple_data->style)
	{
	  g_object_unref (simple_data->style);
	  simple_data->style = NULL;
	}

      if (simple_data->clip_path_commands)
	{
	  g_array_free (simple_data->clip_path_commands, TRUE);
	  simple_data->clip_path_commands = NULL;
	}

      g_slice_free (cairo_matrix_t, simple_data->transform);
      simple_data->transform = NULL;
    }
}


static void
goo_canvas_item_simple_dispose (GObject *object)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;

  /* Remove the view from the GooCanvas hash table. */
  if (simple->canvas && simple->model)
    goo_canvas_unregister_item (simple->canvas,
				(GooCanvasItemModel*) simple->model);

  goo_canvas_item_simple_reset_model (simple);
  goo_canvas_item_simple_free_data (simple->simple_data);

  G_OBJECT_CLASS (goo_canvas_item_simple_parent_class)->dispose (object);
}


static void
goo_canvas_item_simple_finalize (GObject *object)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;

  g_slice_free (GooCanvasItemSimpleData, simple->simple_data);
  simple->simple_data = NULL;

  G_OBJECT_CLASS (goo_canvas_item_simple_parent_class)->finalize (object);
}


static void
goo_canvas_item_simple_get_common_property (GObject                 *object,
					    GooCanvasItemSimpleData *simple_data,
					    guint                    prop_id,
					    GValue                  *value,
					    GParamSpec              *pspec)
{
  GooCanvasStyle *style = simple_data->style;
  GValue *svalue;
  gchar *font = NULL;

  switch (prop_id)
    {
      /* Basic drawing properties. */
    case PROP_STROKE_PATTERN:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_stroke_pattern_id);
      g_value_set_boxed (value, svalue ? svalue->data[0].v_pointer : NULL);
      break;
    case PROP_FILL_PATTERN:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_fill_pattern_id);
      g_value_set_boxed (value, svalue ? svalue->data[0].v_pointer : NULL);
      break;
    case PROP_FILL_RULE:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_fill_rule_id);
      g_value_set_enum (value, svalue ? svalue->data[0].v_long : CAIRO_FILL_RULE_WINDING);
      break;
    case PROP_OPERATOR:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_operator_id);
      g_value_set_enum (value, svalue ? svalue->data[0].v_long : CAIRO_OPERATOR_OVER);
      break;
    case PROP_ANTIALIAS:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_antialias_id);
      g_value_set_enum (value, svalue ? svalue->data[0].v_long : CAIRO_ANTIALIAS_GRAY);
      break;

      /* Line style & width properties. */
    case PROP_LINE_WIDTH:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_line_width_id);
      g_value_set_double (value, svalue ? svalue->data[0].v_double : 2.0);
      break;
    case PROP_LINE_CAP:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_line_cap_id);
      g_value_set_enum (value, svalue ? svalue->data[0].v_long : CAIRO_LINE_CAP_BUTT);
      break;
    case PROP_LINE_JOIN:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_line_join_id);
      g_value_set_enum (value, svalue ? svalue->data[0].v_long : CAIRO_LINE_JOIN_MITER);
      break;
    case PROP_LINE_JOIN_MITER_LIMIT:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_line_join_miter_limit_id);
      g_value_set_double (value, svalue ? svalue->data[0].v_double : 10.0);
      break;
    case PROP_LINE_DASH:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_line_dash_id);
      g_value_set_boxed (value, svalue ? svalue->data[0].v_pointer : NULL);
      break;

      /* Font properties. */
    case PROP_FONT:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_font_desc_id);
      if (svalue)
	font = pango_font_description_to_string (svalue->data[0].v_pointer);
      g_value_set_string (value, font);
      g_free (font);
      break;
    case PROP_FONT_DESC:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_font_desc_id);
      g_value_set_boxed (value, svalue ? svalue->data[0].v_pointer : NULL);
      break;
    case PROP_HINT_METRICS:
      svalue = goo_canvas_style_get_property (style, goo_canvas_style_hint_metrics_id);
      g_value_set_enum (value, svalue ? svalue->data[0].v_long : CAIRO_HINT_METRICS_OFF);
      break;

      /* Other properties. */
    case PROP_TRANSFORM:
      g_value_set_boxed (value, simple_data->transform);
      break;
    case PROP_VISIBILITY:
      g_value_set_enum (value, simple_data->visibility);
      break;
    case PROP_VISIBILITY_THRESHOLD:
      g_value_set_double (value, simple_data->visibility_threshold);
      break;
    case PROP_POINTER_EVENTS:
      g_value_set_flags (value, simple_data->pointer_events);
      break;
    case PROP_CAN_FOCUS:
      g_value_set_boolean (value, simple_data->can_focus);
      break;
    case PROP_CLIP_FILL_RULE:
      g_value_set_enum (value, simple_data->clip_fill_rule);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
goo_canvas_item_simple_get_property (GObject              *object,
				     guint                 prop_id,
				     GValue               *value,
				     GParamSpec           *pspec)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  AtkObject *accessible;

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, simple->parent);
      break;
    case PROP_TITLE:
      accessible = atk_gobject_accessible_for_object (object);
      g_value_set_string (value, atk_object_get_name (accessible));
      break;
    case PROP_DESCRIPTION:
      accessible = atk_gobject_accessible_for_object (object);
      g_value_set_string (value, atk_object_get_description (accessible));
      break;
    default:
      goo_canvas_item_simple_get_common_property (object, simple_data, prop_id,
						  value, pspec);
      break;
    }
}


static gboolean
goo_canvas_item_simple_set_common_property (GObject                 *object,
					    GooCanvasItemSimpleData *simple_data,
					    guint                    prop_id,
					    const GValue            *value,
					    GParamSpec              *pspec)
{
  GooCanvasStyle *style;
  GdkColor color = { 0, 0, 0, 0, };
  guint rgba, red, green, blue, alpha;
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  gboolean recompute_bounds = FALSE;
  cairo_matrix_t *transform;
  GValue tmpval = { 0 };
  const char *font_name;
  PangoFontDescription *font_desc = NULL;

  /* See if we need to create our own style. */
  if (prop_id < PROP_TRANSFORM)
    {
      if (!simple_data->style)
	{
	  simple_data->style = goo_canvas_style_new ();
	}
      else if (!simple_data->own_style)
	{
	  g_object_unref (simple_data->style);
	  simple_data->style = goo_canvas_style_new ();
	}
      simple_data->own_style = TRUE;
    }

  style = simple_data->style;

  switch (prop_id)
    {
      /* Basic drawing properties. */
    case PROP_STROKE_PATTERN:
      goo_canvas_style_set_property (style, goo_canvas_style_stroke_pattern_id, value);
      break;
    case PROP_FILL_PATTERN:
      goo_canvas_style_set_property (style, goo_canvas_style_fill_pattern_id, value);
      break;
    case PROP_FILL_RULE:
      goo_canvas_style_set_property (style, goo_canvas_style_fill_rule_id, value);
      break;
    case PROP_OPERATOR:
      goo_canvas_style_set_property (style, goo_canvas_style_operator_id, value);
      break;
    case PROP_ANTIALIAS:
      goo_canvas_style_set_property (style, goo_canvas_style_antialias_id, value);
      break;

      /* Line style & width properties. */
    case PROP_LINE_WIDTH:
      goo_canvas_style_set_property (style, goo_canvas_style_line_width_id, value);
      recompute_bounds = TRUE;
      break;
    case PROP_LINE_CAP:
      goo_canvas_style_set_property (style, goo_canvas_style_line_cap_id, value);
      recompute_bounds = TRUE;
      break;
    case PROP_LINE_JOIN:
      goo_canvas_style_set_property (style, goo_canvas_style_line_join_id, value);
      recompute_bounds = TRUE;
      break;
    case PROP_LINE_JOIN_MITER_LIMIT:
      goo_canvas_style_set_property (style, goo_canvas_style_line_join_miter_limit_id,
				     value);
      recompute_bounds = TRUE;
      break;
    case PROP_LINE_DASH:
      goo_canvas_style_set_property (style, goo_canvas_style_line_dash_id, value);
      recompute_bounds = TRUE;
      break;

      /* Font properties. */
    case PROP_FONT:
      font_name = g_value_get_string (value);
      if (font_name)
	font_desc = pango_font_description_from_string (font_name);
      g_value_init (&tmpval, PANGO_TYPE_FONT_DESCRIPTION);
      g_value_take_boxed (&tmpval, font_desc);
      goo_canvas_style_set_property (style, goo_canvas_style_font_desc_id, &tmpval);
      g_value_unset (&tmpval);
      recompute_bounds = TRUE;
      break;
    case PROP_FONT_DESC:
      goo_canvas_style_set_property (style, goo_canvas_style_font_desc_id, value);
      recompute_bounds = TRUE;
      break;
    case PROP_HINT_METRICS:
      goo_canvas_style_set_property (style, goo_canvas_style_hint_metrics_id, value);
      recompute_bounds = TRUE;
      break;

      /* Convenience properties. */
    case PROP_STROKE_COLOR:
      if (g_value_get_string (value))
	gdk_color_parse (g_value_get_string (value), &color);
      pattern = cairo_pattern_create_rgb (color.red / 65535.0,
					  color.green / 65535.0,
					  color.blue / 65535.0);
      g_value_init (&tmpval, GOO_TYPE_CAIRO_PATTERN);
      g_value_take_boxed (&tmpval, pattern);
      goo_canvas_style_set_property (style, goo_canvas_style_stroke_pattern_id, &tmpval);
      g_value_unset (&tmpval);
      break;
    case PROP_STROKE_COLOR_RGBA:
      rgba = g_value_get_uint (value);
      red   = (rgba >> 24) & 0xFF;
      green = (rgba >> 16) & 0xFF;
      blue  = (rgba >> 8)  & 0xFF;
      alpha = (rgba)       & 0xFF;
      pattern = cairo_pattern_create_rgba (red / 255.0, green / 255.0,
					   blue / 255.0, alpha / 255.0);
      g_value_init (&tmpval, GOO_TYPE_CAIRO_PATTERN);
      g_value_take_boxed (&tmpval, pattern);
      goo_canvas_style_set_property (style, goo_canvas_style_stroke_pattern_id, &tmpval);
      g_value_unset (&tmpval);
      break;
    case PROP_STROKE_PIXBUF:
      pixbuf = g_value_get_object (value);
      surface = goo_canvas_cairo_surface_from_pixbuf (pixbuf);
      pattern = cairo_pattern_create_for_surface (surface);
      cairo_surface_destroy (surface);
      cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
      g_value_init (&tmpval, GOO_TYPE_CAIRO_PATTERN);
      g_value_take_boxed (&tmpval, pattern);
      goo_canvas_style_set_property (style, goo_canvas_style_stroke_pattern_id, &tmpval);
      g_value_unset (&tmpval);
      break;
    case PROP_FILL_COLOR:
      if (g_value_get_string (value))
	gdk_color_parse (g_value_get_string (value), &color);
      pattern = cairo_pattern_create_rgb (color.red / 65535.0,
					  color.green / 65535.0,
					  color.blue / 65535.0);
      g_value_init (&tmpval, GOO_TYPE_CAIRO_PATTERN);
      g_value_take_boxed (&tmpval, pattern);
      goo_canvas_style_set_property (style, goo_canvas_style_fill_pattern_id, &tmpval);
      g_value_unset (&tmpval);
      break;
    case PROP_FILL_COLOR_RGBA:
      rgba = g_value_get_uint (value);
      red   = (rgba >> 24) & 0xFF;
      green = (rgba >> 16) & 0xFF;
      blue  = (rgba >> 8)  & 0xFF;
      alpha = (rgba)       & 0xFF;
      pattern = cairo_pattern_create_rgba (red / 255.0, green / 255.0,
					   blue / 255.0, alpha / 255.0);
      g_value_init (&tmpval, GOO_TYPE_CAIRO_PATTERN);
      g_value_take_boxed (&tmpval, pattern);
      goo_canvas_style_set_property (style, goo_canvas_style_fill_pattern_id, &tmpval);
      g_value_unset (&tmpval);
      break;
    case PROP_FILL_PIXBUF:
      pixbuf = g_value_get_object (value);
      surface = goo_canvas_cairo_surface_from_pixbuf (pixbuf);
      pattern = cairo_pattern_create_for_surface (surface);
      cairo_surface_destroy (surface);
      cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
      g_value_init (&tmpval, GOO_TYPE_CAIRO_PATTERN);
      g_value_take_boxed (&tmpval, pattern);
      goo_canvas_style_set_property (style, goo_canvas_style_fill_pattern_id, &tmpval);
      g_value_unset (&tmpval);
      break;

      /* Other properties. */
    case PROP_TRANSFORM:
      g_slice_free (cairo_matrix_t, simple_data->transform);
      transform = g_value_get_boxed (value);
      simple_data->transform = goo_cairo_matrix_copy (transform);
      recompute_bounds = TRUE;
      break;
    case PROP_VISIBILITY:
      simple_data->visibility = g_value_get_enum (value);
      break;
    case PROP_VISIBILITY_THRESHOLD:
      simple_data->visibility_threshold = g_value_get_double (value);
      break;
    case PROP_POINTER_EVENTS:
      simple_data->pointer_events = g_value_get_flags (value);
      break;
    case PROP_CAN_FOCUS:
      simple_data->can_focus = g_value_get_boolean (value);
      break;
    case PROP_CLIP_PATH:
      if (simple_data->clip_path_commands)
	g_array_free (simple_data->clip_path_commands, TRUE);
      simple_data->clip_path_commands = goo_canvas_parse_path_data (g_value_get_string (value));
      recompute_bounds = TRUE;
      break;
    case PROP_CLIP_FILL_RULE:
      simple_data->clip_fill_rule = g_value_get_enum (value);
      recompute_bounds = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  return recompute_bounds;
}


static void
goo_canvas_item_simple_set_property (GObject              *object,
				     guint                 prop_id,
				     const GValue         *value,
				     GParamSpec           *pspec)
{
  GooCanvasItem *item = (GooCanvasItem*) object;
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasItem *parent;
  AtkObject *accessible;
  gboolean recompute_bounds;
  gint child_num;

  if (simple->model)
    {
      g_warning ("Can't set property of a canvas item with a model - set the model property instead");
      return;
    }

  switch (prop_id)
    {
    case PROP_PARENT:
      parent = g_value_get_object (value);
      if (simple->parent)
	{
	  child_num = goo_canvas_item_find_child (simple->parent, item);
	  goo_canvas_item_remove_child (simple->parent, child_num);
	}
      goo_canvas_item_add_child (parent, item, -1);
      break;
    case PROP_TITLE:
      accessible = atk_gobject_accessible_for_object (object);
      atk_object_set_name (accessible, g_value_get_string (value));
      break;
    case PROP_DESCRIPTION:
      accessible = atk_gobject_accessible_for_object (object);
      atk_object_set_description (accessible, g_value_get_string (value));
      break;
    default:
      recompute_bounds = goo_canvas_item_simple_set_common_property (object,
								     simple_data,
								     prop_id,
								     value,
								     pspec);
      goo_canvas_item_simple_changed (simple, recompute_bounds);
      break;
    }
}


static GooCanvas*
goo_canvas_item_simple_get_canvas  (GooCanvasItem *item)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  return simple->canvas;
}


static void
goo_canvas_item_simple_set_canvas  (GooCanvasItem *item,
				    GooCanvas     *canvas)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  simple->canvas = canvas;
}


static GooCanvasItem*
goo_canvas_item_simple_get_parent (GooCanvasItem   *item)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  return simple->parent;
}


static void
goo_canvas_item_simple_set_parent (GooCanvasItem  *item,
				   GooCanvasItem  *parent)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  simple->parent = parent;
  simple->canvas = parent ? goo_canvas_item_get_canvas (parent) : NULL;
  simple->need_update = TRUE;
  simple->need_entire_subtree_update = TRUE;
}


/**
 * goo_canvas_item_simple_changed:
 * @item: a #GooCanvasItemSimple.
 * @recompute_bounds: if the item's bounds need to be recomputed.
 * 
 * This function is intended to be used by subclasses of #GooCanvasItemSimple.
 *
 * It is used as a callback for the "changed" signal of the item models.
 * It requests an update or redraw of the item as appropriate.
 **/
void
goo_canvas_item_simple_changed    (GooCanvasItemSimple *item,
				   gboolean             recompute_bounds)
{
  if (recompute_bounds)
    {
      item->need_entire_subtree_update = TRUE;
      if (!item->need_update)
	{
	  goo_canvas_item_request_update ((GooCanvasItem*) item);

	  /* Do this after requesting an update, since GooCanvasGroup will
	     ignore the update request if we do this first. */
	  item->need_update = TRUE;
	}
    }
  else
    {
      if (item->canvas)
	goo_canvas_request_redraw (item->canvas, &item->bounds);
    }
}


static gboolean
goo_canvas_item_simple_get_transform (GooCanvasItem       *item,
				      cairo_matrix_t      *matrix)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;

  if (!simple_data->transform)
    return FALSE;

  *matrix = *simple_data->transform;
  return TRUE;
}


static void
goo_canvas_item_simple_set_transform (GooCanvasItem        *item,
				      const cairo_matrix_t *transform)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;

  if (transform)
    {
      if (!simple_data->transform)
	simple_data->transform = g_slice_new (cairo_matrix_t);

      *simple_data->transform = *transform;
    }
  else
    {
      g_slice_free (cairo_matrix_t, simple_data->transform);
      simple_data->transform = NULL;
    }

  goo_canvas_item_simple_changed (simple, TRUE);
}


static GooCanvasStyle*
goo_canvas_item_simple_get_style (GooCanvasItem       *item)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;

  return simple->simple_data->style;
}


static void
goo_canvas_item_simple_set_style (GooCanvasItem  *item,
				  GooCanvasStyle *style)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;

  if (simple_data->style)
    g_object_unref (simple_data->style);

  if (style)
    {
      simple_data->style = goo_canvas_style_copy (style);
      simple_data->own_style = TRUE;
    }
  else
    {
      simple_data->style = NULL;
      simple_data->own_style = FALSE;
    }

  goo_canvas_item_simple_changed (simple, TRUE);
}


static void
goo_canvas_item_simple_get_bounds  (GooCanvasItem   *item,
				    GooCanvasBounds *bounds)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;

  if (simple->need_update)
    goo_canvas_item_ensure_updated (item);
    
  *bounds = simple->bounds;
}


static GList*
goo_canvas_item_simple_get_items_at (GooCanvasItem  *item,
				     gdouble         x,
				     gdouble         y,
				     cairo_t        *cr,
				     gboolean        is_pointer_event,
				     gboolean        parent_visible,
				     GList          *found_items)
{
  GooCanvasItemSimpleClass *class = GOO_CANVAS_ITEM_SIMPLE_GET_CLASS (item);
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  double user_x = x, user_y = y;
  GooCanvasPointerEvents pointer_events = GOO_CANVAS_EVENTS_ALL;
  cairo_matrix_t matrix;
  gboolean add_item = FALSE;

  if (simple->need_update)
    goo_canvas_item_ensure_updated (item);

  /* Skip the item if the point isn't in the item's bounds. */
  if (simple->bounds.x1 > x || simple->bounds.x2 < x
      || simple->bounds.y1 > y || simple->bounds.y2 < y)
    return found_items;

  /* Check if the item should receive events. */
  if (is_pointer_event)
    {
      if (simple_data->pointer_events == GOO_CANVAS_EVENTS_NONE)
	return found_items;
      if (simple_data->pointer_events & GOO_CANVAS_EVENTS_VISIBLE_MASK
	  && (!parent_visible
	      || simple_data->visibility <= GOO_CANVAS_ITEM_INVISIBLE
	      || (simple_data->visibility == GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD
		  && simple->canvas->scale < simple_data->visibility_threshold)))
	return found_items;

      pointer_events = simple_data->pointer_events;
    }

  cairo_save (cr);
  if (simple_data->transform)
    cairo_transform (cr, simple_data->transform);

  cairo_device_to_user (cr, &user_x, &user_y);

  /* Remove any current translation, to avoid the 16-bit cairo limit. */
  cairo_get_matrix (cr, &matrix);
  matrix.x0 = matrix.y0 = 0.0;
  cairo_set_matrix (cr, &matrix);

  /* If the item has a clip path, check if the point is inside it. */
  if (simple_data->clip_path_commands)
    {
      goo_canvas_create_path (simple_data->clip_path_commands, cr);
      cairo_set_fill_rule (cr, simple_data->clip_fill_rule);
      if (!cairo_in_fill (cr, user_x, user_y))
	{
	  cairo_restore (cr);
	  return found_items;
	}
    }

  if (class->simple_is_item_at)
    {
      add_item = class->simple_is_item_at (simple, user_x, user_y, cr,
					   is_pointer_event);
    }
  else
    {
      /* Use the virtual method subclasses define to create the path. */
      class->simple_create_path (simple, cr);

      if (goo_canvas_item_simple_check_in_path (simple, user_x, user_y, cr,
						pointer_events))
	add_item = TRUE;
    }

  cairo_restore (cr);

  if (add_item)
    return g_list_prepend (found_items, item);
  else
    return found_items;
}


static gboolean
goo_canvas_item_simple_is_visible  (GooCanvasItem   *item)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;

  if (simple_data->visibility <= GOO_CANVAS_ITEM_INVISIBLE
      || (simple_data->visibility == GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD
	  && simple->canvas->scale < simple_data->visibility_threshold))
    return FALSE;

  if (simple->parent)
    return goo_canvas_item_is_visible (simple->parent);

  return TRUE;
}


/**
 * goo_canvas_item_simple_check_style:
 * @item: a #GooCanvasItemSimple.
 * 
 * This function is intended to be used by subclasses of #GooCanvasItemSimple,
 * typically in their update() or get_requested_area() methods.
 *
 * It ensures that the item's style is setup correctly. If the item has its
 * own #GooCanvasStyle it makes sure the parent is set correctly. If it
 * doesn't have its own style it uses the parent item's style.
 **/
void
goo_canvas_item_simple_check_style (GooCanvasItemSimple *item)
{
  GooCanvasItemSimpleData *simple_data = item->simple_data;
  GooCanvasStyle *parent_style = NULL;

  if (item->parent)
    parent_style = goo_canvas_item_get_style (item->parent);

  if (simple_data->own_style)
    {
      goo_canvas_style_set_parent (simple_data->style, parent_style);
    }
  else if (simple_data->style != parent_style)
    {
      /* The item doesn't have its own style so we use the parent's (though
	 that may also be NULL). */
      if (simple_data->style)
	g_object_unref (simple_data->style);

      simple_data->style = parent_style;

      if (parent_style)
	g_object_ref (parent_style);
    }
}


static void
goo_canvas_item_simple_update_internal  (GooCanvasItemSimple *simple,
					 cairo_t             *cr)
{
  GooCanvasItemSimpleClass *class = GOO_CANVAS_ITEM_SIMPLE_GET_CLASS (simple);
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasBounds tmp_bounds;
  cairo_matrix_t transform;

  simple->need_update = FALSE;

  goo_canvas_item_simple_check_style (simple);

  cairo_get_matrix (cr, &transform);

  if (class->simple_update)
    {
      class->simple_update (simple, cr);
    }
  else
    {
      /* Use the identity matrix to get the bounds completely in user space. */
      cairo_identity_matrix (cr);
      class->simple_create_path (simple, cr);
      goo_canvas_item_simple_get_path_bounds (simple, cr, &simple->bounds);
    }

  /* Modify the extents by the item's clip path. */
  if (simple_data->clip_path_commands)
    {
      cairo_identity_matrix (cr);
      goo_canvas_create_path (simple_data->clip_path_commands, cr);
      cairo_set_fill_rule (cr, simple_data->clip_fill_rule);
      cairo_fill_extents (cr, &tmp_bounds.x1, &tmp_bounds.y1,
			  &tmp_bounds.x2, &tmp_bounds.y2);
      simple->bounds.x1 = MAX (simple->bounds.x1, tmp_bounds.x1);
      simple->bounds.y1 = MAX (simple->bounds.y1, tmp_bounds.y1);
      simple->bounds.x2 = MIN (simple->bounds.x2, tmp_bounds.x2);
      simple->bounds.y2 = MIN (simple->bounds.y2, tmp_bounds.y2);

      if (simple->bounds.x1 > simple->bounds.x2)
	simple->bounds.x2 = simple->bounds.x1;
      if (simple->bounds.y1 > simple->bounds.y2)
	simple->bounds.y2 = simple->bounds.y1;
    }

  cairo_set_matrix (cr, &transform);
}


static void
goo_canvas_item_simple_update  (GooCanvasItem   *item,
				gboolean         entire_tree,
				cairo_t         *cr,
				GooCanvasBounds *bounds)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  cairo_matrix_t matrix;
  double x_offset, y_offset;

  if (entire_tree || simple->need_update)
    {
      /* Request a redraw of the existing bounds. */
      goo_canvas_request_redraw (simple->canvas, &simple->bounds);

      cairo_save (cr);
      if (simple_data->transform)
	cairo_transform (cr, simple_data->transform);

      /* Remove any current translation, to avoid the 16-bit cairo limit. */
      cairo_get_matrix (cr, &matrix);
      x_offset = matrix.x0;
      y_offset = matrix.y0;
      matrix.x0 = matrix.y0 = 0.0;
      cairo_set_matrix (cr, &matrix);

      goo_canvas_item_simple_update_internal (simple, cr);

      goo_canvas_item_simple_user_bounds_to_device (simple, cr,
						    &simple->bounds);

      /* Add the translation back to the bounds. */
      simple->bounds.x1 += x_offset;
      simple->bounds.y1 += y_offset;
      simple->bounds.x2 += x_offset;
      simple->bounds.y2 += y_offset;

      cairo_restore (cr);

      /* Request a redraw of the new bounds. */
      goo_canvas_request_redraw (simple->canvas, &simple->bounds);
    }

  *bounds = simple->bounds;
}


static gboolean
goo_canvas_item_simple_get_requested_area (GooCanvasItem    *item,
					   cairo_t          *cr,
					   GooCanvasBounds  *requested_area)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  cairo_matrix_t matrix;
  double x_offset, y_offset;

  cairo_save (cr);
  if (simple_data->transform)
    cairo_transform (cr, simple_data->transform);

  /* Remove any current translation, to avoid the 16-bit cairo limit. */
  cairo_get_matrix (cr, &matrix);
  x_offset = matrix.x0;
  y_offset = matrix.y0;
  matrix.x0 = matrix.y0 = 0.0;
  cairo_set_matrix (cr, &matrix);

  goo_canvas_item_simple_update_internal (simple, cr);

  if (simple->simple_data->visibility == GOO_CANVAS_ITEM_HIDDEN)
    {
      simple->bounds.x1 = simple->bounds.x2 = 0.0;
      simple->bounds.y1 = simple->bounds.y2 = 0.0;
      cairo_restore (cr);
      return FALSE;
    }

  /* FIXME: Maybe optimize by just converting the offsets to user space
     and adding them? */

  /* Convert to device space. */
  cairo_user_to_device (cr, &simple->bounds.x1, &simple->bounds.y1);
  cairo_user_to_device (cr, &simple->bounds.x2, &simple->bounds.y2);

  /* Add the translation back to the bounds. */
  simple->bounds.x1 += x_offset;
  simple->bounds.y1 += y_offset;
  simple->bounds.x2 += x_offset;
  simple->bounds.y2 += y_offset;

  /* Restore the item's proper transformation matrix. */
  matrix.x0 = x_offset;
  matrix.y0 = y_offset;
  cairo_set_matrix (cr, &matrix);

  /* Convert back to user space. */
  cairo_device_to_user (cr, &simple->bounds.x1, &simple->bounds.y1);
  cairo_device_to_user (cr, &simple->bounds.x2, &simple->bounds.y2);


  /* Copy the user bounds to the requested area. */
  *requested_area = simple->bounds;

  /* Convert to the parent's coordinate space. */
  goo_canvas_item_simple_user_bounds_to_parent (simple, cr, requested_area);

  /* Convert the item's bounds to device space. */
  goo_canvas_item_simple_user_bounds_to_device (simple, cr, &simple->bounds);

  cairo_restore (cr);

  return TRUE;
}


static void
goo_canvas_item_simple_allocate_area      (GooCanvasItem         *item,
					   cairo_t               *cr,
					   const GooCanvasBounds *requested_area,
					   const GooCanvasBounds *allocated_area,
					   gdouble                x_offset,
					   gdouble                y_offset)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;

  /* Simple items can't resize at all, so we just adjust the bounds x & y
     positions here, and let the item be clipped if necessary. */
  simple->bounds.x1 += x_offset;
  simple->bounds.y1 += y_offset;
  simple->bounds.x2 += x_offset;
  simple->bounds.y2 += y_offset;
}


static void
goo_canvas_item_simple_paint (GooCanvasItem         *item,
			      cairo_t               *cr,
			      const GooCanvasBounds *bounds,
			      gdouble                scale)
{
  GooCanvasItemSimpleClass *class = GOO_CANVAS_ITEM_SIMPLE_GET_CLASS (item);
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;

  /* Skip the item if the bounds don't intersect the expose rectangle. */
  if (simple->bounds.x1 > bounds->x2 || simple->bounds.x2 < bounds->x1
      || simple->bounds.y1 > bounds->y2 || simple->bounds.y2 < bounds->y1)
    return;

  /* Check if the item should be visible. */
  if (simple_data->visibility <= GOO_CANVAS_ITEM_INVISIBLE
      || (simple_data->visibility == GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD
	  && scale < simple_data->visibility_threshold))
    return;

  cairo_save (cr);
  if (simple_data->transform)
    cairo_transform (cr, simple_data->transform);

  /* Clip with the item's clip path, if it is set. */
  if (simple_data->clip_path_commands)
    {
      goo_canvas_create_path (simple_data->clip_path_commands, cr);
      cairo_set_fill_rule (cr, simple_data->clip_fill_rule);
      cairo_clip (cr);
    }

  if (class->simple_paint)
    {
      class->simple_paint (simple, cr, bounds);
    }
  else
    {
      class->simple_create_path (simple, cr);

      goo_canvas_item_simple_paint_path (simple, cr);
    }

  cairo_restore (cr);
}



static void
goo_canvas_item_simple_title_changed (GooCanvasItemModelSimple *smodel,
				      GParamSpec               *pspec,
				      GooCanvasItemSimple      *item)
{
  AtkObject *accessible;

  accessible = atk_gobject_accessible_for_object (G_OBJECT (item));
  atk_object_set_name (accessible, smodel->title);
}


static void
goo_canvas_item_simple_description_changed (GooCanvasItemModelSimple *smodel,
					    GParamSpec               *pspec,
					    GooCanvasItemSimple      *item)
{
  AtkObject *accessible;

  accessible = atk_gobject_accessible_for_object (G_OBJECT (item));
  atk_object_set_description (accessible, smodel->description);
}


static void
goo_canvas_item_simple_setup_accessibility (GooCanvasItemSimple      *item)
{
  GooCanvasItemModelSimple *smodel = item->model;
  AtkObject *accessible;

  accessible = atk_gobject_accessible_for_object (G_OBJECT (item));
  if (!ATK_IS_NO_OP_OBJECT (accessible))
    {
      if (smodel->title)
	atk_object_set_name (accessible, smodel->title);
      if (smodel->description)
	atk_object_set_description (accessible, smodel->description);

      g_signal_connect (smodel, "notify::title",
			G_CALLBACK (goo_canvas_item_simple_title_changed),
			item);
      g_signal_connect (smodel, "notify::description",
			G_CALLBACK (goo_canvas_item_simple_description_changed),
			item);
    }
}


static void
goo_canvas_item_model_simple_changed (GooCanvasItemModelSimple *smodel,
				      gboolean                  recompute_bounds,
				      GooCanvasItemSimple      *simple)
{
  goo_canvas_item_simple_changed (simple, recompute_bounds);
}


static GooCanvasItemModel*
goo_canvas_item_simple_get_model    (GooCanvasItem      *item)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  return (GooCanvasItemModel*) simple->model;
}


/**
 * goo_canvas_item_simple_set_model:
 * @item: a #GooCanvasItemSimple.
 * @model: the model that @item will view.
 * 
 * This function should be called by subclasses of #GooCanvasItemSimple
 * in their set_model() method.
 **/
void
goo_canvas_item_simple_set_model (GooCanvasItemSimple  *item,
				  GooCanvasItemModel   *model)
{
  g_return_if_fail (model != NULL);

  goo_canvas_item_simple_reset_model (item);
  goo_canvas_item_simple_free_data (item->simple_data);
  g_slice_free (GooCanvasItemSimpleData, item->simple_data);

  item->model = g_object_ref (model);
  item->simple_data = &item->model->simple_data;

  goo_canvas_item_simple_setup_accessibility (item);

  g_signal_connect (model, "changed",
		    G_CALLBACK (goo_canvas_item_model_simple_changed),
		    item);
}


static void
goo_canvas_item_simple_set_model_internal    (GooCanvasItem      *item,
					      GooCanvasItemModel *model)
{
  goo_canvas_item_simple_set_model ((GooCanvasItemSimple*) item, model);
}


static void
canvas_item_interface_init (GooCanvasItemIface *iface)
{
  iface->get_canvas         = goo_canvas_item_simple_get_canvas;
  iface->set_canvas         = goo_canvas_item_simple_set_canvas;

  iface->get_parent	    = goo_canvas_item_simple_get_parent;
  iface->set_parent	    = goo_canvas_item_simple_set_parent;
  iface->get_bounds         = goo_canvas_item_simple_get_bounds;
  iface->get_items_at	    = goo_canvas_item_simple_get_items_at;
  iface->update             = goo_canvas_item_simple_update;
  iface->get_requested_area = goo_canvas_item_simple_get_requested_area;
  iface->allocate_area      = goo_canvas_item_simple_allocate_area;
  iface->paint              = goo_canvas_item_simple_paint;

  iface->get_transform      = goo_canvas_item_simple_get_transform;
  iface->set_transform      = goo_canvas_item_simple_set_transform;
  iface->get_style          = goo_canvas_item_simple_get_style;
  iface->set_style          = goo_canvas_item_simple_set_style;
  iface->is_visible         = goo_canvas_item_simple_is_visible;

  iface->get_model          = goo_canvas_item_simple_get_model;
  iface->set_model          = goo_canvas_item_simple_set_model_internal;
}


/**
 * goo_canvas_item_simple_paint_path:
 * @item: a #GooCanvasItemSimple.
 * @cr: a cairo context.
 * 
 * This function is intended to be used by subclasses of #GooCanvasItemSimple.
 *
 * It paints the current path, using the item's style settings.
 **/
void
goo_canvas_item_simple_paint_path (GooCanvasItemSimple *item,
				   cairo_t             *cr)
{
  GooCanvasStyle *style = item->simple_data->style;

  if (goo_canvas_style_set_fill_options (style, cr))
    cairo_fill_preserve (cr);

  if (goo_canvas_style_set_stroke_options (style, cr))
    cairo_stroke (cr);

  cairo_new_path (cr);
}


/* Returns the bounds of the path, using the item's stroke and fill options,
   in device coords. Note that the bounds include both the stroke and the
   fill extents, even if they will not be painted. (We need this to handle
   the "pointer-events" property.) */
/**
 * goo_canvas_item_simple_get_path_bounds:
 * @item: a #GooCanvasItemSimple.
 * @cr: a cairo context.
 * @bounds: the #GooCanvasBounds struct to store the resulting bounding box.
 * 
 * This function is intended to be used by subclasses of #GooCanvasItemSimple,
 * typically in their update() or get_requested_area() methods.
 *
 * It calculates the bounds of the current path, using the item's style
 * settings, and stores the results in the given #GooCanvasBounds struct.
 *
 * The returned bounds contains the bounding box of the path in device space,
 * converted to user space coordinates. To calculate the bounds completely in
 * user space, use cairo_identity_matrix() to temporarily reset the current
 * transformation matrix to the identity matrix.
 **/
void
goo_canvas_item_simple_get_path_bounds (GooCanvasItemSimple *item,
					cairo_t             *cr,
					GooCanvasBounds     *bounds)
{
  GooCanvasStyle *style = item->simple_data->style;
  GooCanvasBounds tmp_bounds, tmp_bounds2;

  /* Calculate the filled extents. */
  goo_canvas_style_set_fill_options (style, cr);
  cairo_fill_extents (cr, &tmp_bounds.x1, &tmp_bounds.y1,
		      &tmp_bounds.x2, &tmp_bounds.y2);

  /* Check the stroke. */
  goo_canvas_style_set_stroke_options (style, cr);
  cairo_stroke_extents (cr, &tmp_bounds2.x1, &tmp_bounds2.y1,
			&tmp_bounds2.x2, &tmp_bounds2.y2);

  /* FIXME: Handle whatever cairo returns for NULL bounds. Cairo starts with
     INT16_MAX << 16 in cairo-traps.c, but this may get converted to user
     space so could be anything? cairo 1.4 handles this better, I think. */

  bounds->x1 = MIN (tmp_bounds.x1, tmp_bounds.x2);
  bounds->x1 = MIN (bounds->x1, tmp_bounds2.x1);
  bounds->x1 = MIN (bounds->x1, tmp_bounds2.x2);

  bounds->x2 = MAX (tmp_bounds.x1, tmp_bounds.x2);
  bounds->x2 = MAX (bounds->x2, tmp_bounds2.x1);
  bounds->x2 = MAX (bounds->x2, tmp_bounds2.x2);

  bounds->y1 = MIN (tmp_bounds.y1, tmp_bounds.y2);
  bounds->y1 = MIN (bounds->y1, tmp_bounds2.y1);
  bounds->y1 = MIN (bounds->y1, tmp_bounds2.y2);

  bounds->y2 = MAX (tmp_bounds.y1, tmp_bounds.y2);
  bounds->y2 = MAX (bounds->y2, tmp_bounds2.y1);
  bounds->y2 = MAX (bounds->y2, tmp_bounds2.y2);
}


/**
 * goo_canvas_item_simple_user_bounds_to_device:
 * @item: a #GooCanvasItemSimple.
 * @cr: a cairo context.
 * @bounds: the bounds of the item, in the item's coordinate space.
 * 
 * This function is intended to be used by subclasses of #GooCanvasItemSimple,
 * typically in their update() or get_requested_area() methods.
 *
 * It converts the item's bounds to a bounding box in device space.
 **/
void
goo_canvas_item_simple_user_bounds_to_device (GooCanvasItemSimple *item,
					      cairo_t             *cr,
					      GooCanvasBounds     *bounds)
{
  GooCanvasBounds tmp_bounds = *bounds, tmp_bounds2 = *bounds;

  /* Convert the top-left and bottom-right corners to device coords. */
  cairo_user_to_device (cr, &tmp_bounds.x1, &tmp_bounds.y1);
  cairo_user_to_device (cr, &tmp_bounds.x2, &tmp_bounds.y2);

  /* Now convert the top-right and bottom-left corners. */
  cairo_user_to_device (cr, &tmp_bounds2.x1, &tmp_bounds2.y2);
  cairo_user_to_device (cr, &tmp_bounds2.x2, &tmp_bounds2.y1);

  /* Calculate the minimum x coordinate seen and put in x1. */
  bounds->x1 = MIN (tmp_bounds.x1, tmp_bounds.x2);
  bounds->x1 = MIN (bounds->x1, tmp_bounds2.x1);
  bounds->x1 = MIN (bounds->x1, tmp_bounds2.x2);

  /* Calculate the maximum x coordinate seen and put in x2. */
  bounds->x2 = MAX (tmp_bounds.x1, tmp_bounds.x2);
  bounds->x2 = MAX (bounds->x2, tmp_bounds2.x1);
  bounds->x2 = MAX (bounds->x2, tmp_bounds2.x2);

  /* Calculate the minimum y coordinate seen and put in y1. */
  bounds->y1 = MIN (tmp_bounds.y1, tmp_bounds.y2);
  bounds->y1 = MIN (bounds->y1, tmp_bounds2.y1);
  bounds->y1 = MIN (bounds->y1, tmp_bounds2.y2);

  /* Calculate the maximum y coordinate seen and put in y2. */
  bounds->y2 = MAX (tmp_bounds.y1, tmp_bounds.y2);
  bounds->y2 = MAX (bounds->y2, tmp_bounds2.y1);
  bounds->y2 = MAX (bounds->y2, tmp_bounds2.y2);
}


/**
 * goo_canvas_item_simple_user_bounds_to_parent:
 * @item: a #GooCanvasItemSimple.
 * @cr: a cairo context.
 * @bounds: the bounds of the item, in the item's coordinate space.
 * 
 * This function is intended to be used by subclasses of #GooCanvasItemSimple,
 * typically in their get_requested_area() method.
 *
 * It converts the item's bounds to a bounding box in its parent's coordinate
 * space. If the item has no transformation matrix set then no conversion is
 * needed.
 **/
void
goo_canvas_item_simple_user_bounds_to_parent (GooCanvasItemSimple *item,
					      cairo_t             *cr,
					      GooCanvasBounds     *bounds)
{
  GooCanvasItemSimpleData *simple_data = item->simple_data;
  cairo_matrix_t *transform = simple_data->transform;
  GooCanvasBounds tmp_bounds, tmp_bounds2;

  if (!transform)
    return;

  tmp_bounds = tmp_bounds2 = *bounds;

  /* Convert the top-left and bottom-right corners to parent coords. */
  cairo_matrix_transform_point (transform, &tmp_bounds.x1, &tmp_bounds.y1);
  cairo_matrix_transform_point (transform, &tmp_bounds.x2, &tmp_bounds.y2);

  /* Now convert the top-right and bottom-left corners. */
  cairo_matrix_transform_point (transform, &tmp_bounds2.x1, &tmp_bounds2.y2);
  cairo_matrix_transform_point (transform, &tmp_bounds2.x2, &tmp_bounds2.y1);

  /* Calculate the minimum x coordinate seen and put in x1. */
  bounds->x1 = MIN (tmp_bounds.x1, tmp_bounds.x2);
  bounds->x1 = MIN (bounds->x1, tmp_bounds2.x1);
  bounds->x1 = MIN (bounds->x1, tmp_bounds2.x2);

  /* Calculate the maximum x coordinate seen and put in x2. */
  bounds->x2 = MAX (tmp_bounds.x1, tmp_bounds.x2);
  bounds->x2 = MAX (bounds->x2, tmp_bounds2.x1);
  bounds->x2 = MAX (bounds->x2, tmp_bounds2.x2);

  /* Calculate the minimum y coordinate seen and put in y1. */
  bounds->y1 = MIN (tmp_bounds.y1, tmp_bounds.y2);
  bounds->y1 = MIN (bounds->y1, tmp_bounds2.y1);
  bounds->y1 = MIN (bounds->y1, tmp_bounds2.y2);

  /* Calculate the maximum y coordinate seen and put in y2. */
  bounds->y2 = MAX (tmp_bounds.y1, tmp_bounds.y2);
  bounds->y2 = MAX (bounds->y2, tmp_bounds2.y1);
  bounds->y2 = MAX (bounds->y2, tmp_bounds2.y2);
}


/**
 * goo_canvas_item_simple_check_in_path:
 * @item: a #GooCanvasItemSimple.
 * @x: the x coordinate of the point.
 * @y: the y coordinate of the point.
 * @cr: a cairo context.
 * @pointer_events: specifies which parts of the path to check.
 * 
 * This function is intended to be used by subclasses of #GooCanvasItemSimple.
 *
 * It checks if the given point is in the current path, using the item's
 * style settings.
 * 
 * Returns: %TRUE if the given point is in the current path.
 **/
gboolean
goo_canvas_item_simple_check_in_path (GooCanvasItemSimple   *item,
				      gdouble                x,
				      gdouble                y,
				      cairo_t               *cr,
				      GooCanvasPointerEvents pointer_events)
{
  GooCanvasStyle *style = item->simple_data->style;
  gboolean do_fill, do_stroke;

  /* Check the filled path, if required. */
  if (pointer_events & GOO_CANVAS_EVENTS_FILL_MASK)
    {
      do_fill = goo_canvas_style_set_fill_options (style, cr);
      if (!(pointer_events & GOO_CANVAS_EVENTS_PAINTED_MASK) || do_fill)
	{
	  if (cairo_in_fill (cr, x, y))
	    return TRUE;
	}
    }

  /* Check the stroke, if required. */
  if (pointer_events & GOO_CANVAS_EVENTS_STROKE_MASK)
    {
      do_stroke = goo_canvas_style_set_stroke_options (style, cr);
      if (!(pointer_events & GOO_CANVAS_EVENTS_PAINTED_MASK) || do_stroke)
	{
	  if (cairo_in_stroke (cr, x, y))
	    return TRUE;
	}
    }

  return FALSE;
}



/**
 * SECTION:goocanvasitemmodelsimple
 * @Title: GooCanvasItemModelSimple
 * @Short_Description: the base class for the standard canvas item models.
 * @Stability_Level: 
 * @See_Also: 
 *
 * #GooCanvasItemModelSimple is used as a base class for the standard canvas
 * item models. It can also be used as the base class for new custom canvas
 * item models.
 *
 * It provides default implementations for many of the #GooCanvasItemModel
 * methods.
 *
 * Subclasses of #GooCanvasItemModelSimple only need to implement the
 * create_item() method of the #GooCanvasItemModel interface, to create
 * the default canvas item to view the item model.
 */


static void item_model_interface_init  (GooCanvasItemModelIface *iface);
static void goo_canvas_item_model_simple_dispose  (GObject *object);
static void goo_canvas_item_model_simple_finalize (GObject *object);
static void goo_canvas_item_model_simple_get_property (GObject              *object,
						       guint                 prop_id,
						       GValue               *value,
						       GParamSpec           *pspec);
static void goo_canvas_item_model_simple_set_property (GObject              *object,
						       guint                 prop_id,
						       const GValue         *value,
						       GParamSpec           *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasItemModelSimple,
			 goo_canvas_item_model_simple, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM_MODEL,
						item_model_interface_init))


static void
goo_canvas_item_model_simple_class_init (GooCanvasItemModelSimpleClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->dispose  = goo_canvas_item_model_simple_dispose;
  gobject_class->finalize = goo_canvas_item_model_simple_finalize;

  gobject_class->get_property = goo_canvas_item_model_simple_get_property;
  gobject_class->set_property = goo_canvas_item_model_simple_set_property;

  goo_canvas_item_simple_install_common_properties (gobject_class);
}


static void
goo_canvas_item_model_simple_init (GooCanvasItemModelSimple *smodel)
{
  smodel->simple_data.visibility = GOO_CANVAS_ITEM_VISIBLE;
  smodel->simple_data.pointer_events = GOO_CANVAS_EVENTS_VISIBLE_PAINTED;
  smodel->simple_data.clip_fill_rule = CAIRO_FILL_RULE_WINDING;
}


static void
goo_canvas_item_model_simple_dispose (GObject *object)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) object;

  goo_canvas_item_simple_free_data (&smodel->simple_data);

  G_OBJECT_CLASS (goo_canvas_item_model_simple_parent_class)->dispose (object);
}


static void
goo_canvas_item_model_simple_finalize (GObject *object)
{
  /*GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) object;*/

  G_OBJECT_CLASS (goo_canvas_item_model_simple_parent_class)->finalize (object);
}


static void
goo_canvas_item_model_simple_get_property (GObject              *object,
					   guint                 prop_id,
					   GValue               *value,
					   GParamSpec           *pspec)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) object;
  GooCanvasItemSimpleData *simple_data = &smodel->simple_data;

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, smodel->parent);
      break;
    case PROP_TITLE:
      g_value_set_string (value, smodel->title);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, smodel->description);
      break;
    default:
      goo_canvas_item_simple_get_common_property (object, simple_data, prop_id,
						  value, pspec);
      break;
    }
}


static void
goo_canvas_item_model_simple_set_property (GObject              *object,
					   guint                 prop_id,
					   const GValue         *value,
					   GParamSpec           *pspec)
{
  GooCanvasItemModel *model = (GooCanvasItemModel*) object;
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) object;
  GooCanvasItemSimpleData *simple_data = &smodel->simple_data;
  GooCanvasItemModel *parent;
  gboolean recompute_bounds;
  gint child_num;

  switch (prop_id)
    {
    case PROP_PARENT:
      parent = g_value_get_object (value);
      if (smodel->parent)
	{
	  child_num = goo_canvas_item_model_find_child (smodel->parent, model);
	  goo_canvas_item_model_remove_child (smodel->parent, child_num);
	}
      goo_canvas_item_model_add_child (parent, model, -1);
      break;
    case PROP_TITLE:
      g_free (smodel->title);
      smodel->title = g_value_dup_string (value);
      break;
    case PROP_DESCRIPTION:
      g_free (smodel->description);
      smodel->description = g_value_dup_string (value);
      break;
    default:
      recompute_bounds = goo_canvas_item_simple_set_common_property (object,
								     simple_data,
								     prop_id,
								     value,
								     pspec);
      g_signal_emit_by_name (smodel, "changed", recompute_bounds);
      break;
    }
}


static GooCanvasItemModel*
goo_canvas_item_model_simple_get_parent (GooCanvasItemModel   *model)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) model;
  return smodel->parent;
}


static void
goo_canvas_item_model_simple_set_parent (GooCanvasItemModel   *model,
					 GooCanvasItemModel   *parent)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) model;
  smodel->parent = parent;
}


static gboolean
goo_canvas_item_model_simple_get_transform (GooCanvasItemModel  *model,
					    cairo_matrix_t      *matrix)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) model;
  GooCanvasItemSimpleData *simple_data = &smodel->simple_data;

  if (!simple_data->transform)
    return FALSE;

  *matrix = *simple_data->transform;
  return TRUE;
}


static void
goo_canvas_item_model_simple_set_transform (GooCanvasItemModel   *model,
					    const cairo_matrix_t *transform)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) model;
  GooCanvasItemSimpleData *simple_data = &smodel->simple_data;

  if (transform)
    {
      if (!simple_data->transform)
	simple_data->transform = g_slice_new (cairo_matrix_t);

      *simple_data->transform = *transform;
    }
  else
    {
      g_slice_free (cairo_matrix_t, simple_data->transform);
      simple_data->transform = NULL;
    }

  g_signal_emit_by_name (smodel, "changed", TRUE);
}


static GooCanvasStyle*
goo_canvas_item_model_simple_get_style (GooCanvasItemModel  *model)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) model;

  return smodel->simple_data.style;
}


static void
goo_canvas_item_model_simple_set_style (GooCanvasItemModel *model,
					GooCanvasStyle     *style)
{
  GooCanvasItemModelSimple *smodel = (GooCanvasItemModelSimple*) model;
  GooCanvasItemSimpleData *simple_data = &smodel->simple_data;

  if (simple_data->style)
    g_object_unref (simple_data->style);

  if (style)
    {
      simple_data->style = goo_canvas_style_copy (style);
      simple_data->own_style = TRUE;
    }
  else
    {
      simple_data->style = NULL;
      simple_data->own_style = FALSE;
    }

  g_signal_emit_by_name (smodel, "changed", TRUE);
}


static void
item_model_interface_init  (GooCanvasItemModelIface *iface)
{
  iface->get_parent     = goo_canvas_item_model_simple_get_parent;
  iface->set_parent     = goo_canvas_item_model_simple_set_parent;
  iface->get_transform  = goo_canvas_item_model_simple_get_transform;
  iface->set_transform  = goo_canvas_item_model_simple_set_transform;
  iface->get_style      = goo_canvas_item_model_simple_get_style;
  iface->set_style      = goo_canvas_item_model_simple_set_style;
}
