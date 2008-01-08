/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasstyle.c - cascading styles.
 */

/**
 * SECTION:goocanvasstyle
 * @Title: GooCanvasStyle
 * @Short_Description: support for cascading style properties for canvas items.
 *
 * #GooCanvasStyle provides support for cascading style properties for canvas
 * items. It is intended to be used when implementing new canvas items.
 *
 * Style properties are identified by a unique #GQuark, and contain
 * arbitrary data stored in a #GValue.
 *
 * #GooCanvasStyle also provides a few convenience functions such as
 * goo_canvas_style_set_stroke_options() and
 * goo_canvas_style_set_fill_options() which efficiently apply an item's
 * standard style properties to the given cairo_t.
 */
#include <config.h>
#include <gtk/gtk.h>
#include "goocanvasstyle.h"
#include "goocanvasutils.h"

/* GQuarks for the basic properties. */

/**
 * goo_canvas_style_stroke_pattern_id:
 * 
 * Unique #GQuark identifier used for the standard stroke pattern property.
 **/
GQuark goo_canvas_style_stroke_pattern_id;

/**
 * goo_canvas_style_fill_pattern_id:
 * 
 * Unique #GQuark identifier used for the standard fill pattern property.
 **/
GQuark goo_canvas_style_fill_pattern_id;

/**
 * goo_canvas_style_fill_rule_id:
 * 
 * Unique #GQuark identifier used for the standard fill rule property.
 **/
GQuark goo_canvas_style_fill_rule_id;

/**
 * goo_canvas_style_operator_id:
 * 
 * Unique #GQuark identifier used for the standard operator property.
 **/
GQuark goo_canvas_style_operator_id;

/**
 * goo_canvas_style_antialias_id:
 * 
 * Unique #GQuark identifier used for the standard antialias property.
 **/
GQuark goo_canvas_style_antialias_id;

/**
 * goo_canvas_style_line_width_id:
 * 
 * Unique #GQuark identifier used for the standard line width property.
 **/
GQuark goo_canvas_style_line_width_id;

/**
 * goo_canvas_style_line_cap_id:
 * 
 * Unique #GQuark identifier used for the standard line cap property.
 **/
GQuark goo_canvas_style_line_cap_id;

/**
 * goo_canvas_style_line_join_id:
 * 
 * Unique #GQuark identifier used for the standard line join property.
 **/
GQuark goo_canvas_style_line_join_id;

/**
 * goo_canvas_style_line_join_miter_limit_id:
 * 
 * Unique #GQuark identifier used for the standard miter limit property.
 **/
GQuark goo_canvas_style_line_join_miter_limit_id;

/**
 * goo_canvas_style_line_dash_id:
 * 
 * Unique #GQuark identifier used for the standard line dash property.
 **/
GQuark goo_canvas_style_line_dash_id;

/**
 * goo_canvas_style_font_desc_id:
 * 
 * Unique #GQuark identifier used for the standard font description property.
 **/
GQuark goo_canvas_style_font_desc_id;

/**
 * goo_canvas_style_hint_metrics_id:
 * 
 * Unique #GQuark identifier used for the standard hint metrics property.
 **/
GQuark goo_canvas_style_hint_metrics_id;

static void goo_canvas_style_dispose  (GObject *object);
static void goo_canvas_style_finalize (GObject *object);

G_DEFINE_TYPE (GooCanvasStyle, goo_canvas_style, G_TYPE_OBJECT)


/* Create GQuarks for the basic properties. This is called by
   goo_canvas_style_class_init(), goo_canvas_item_base_init() and
   goo_canvas_item_model_base_init() to try to ensure the GQuarks are
   initialized before they are needed. */
void
_goo_canvas_style_init (void)
{
  static gboolean initialized = FALSE;
  
  if (!initialized)
    {
      goo_canvas_style_stroke_pattern_id = g_quark_from_static_string ("GooCanvasStyle:stroke_pattern");
      goo_canvas_style_fill_pattern_id = g_quark_from_static_string ("GooCanvasStyle:fill_pattern");
      goo_canvas_style_fill_rule_id = g_quark_from_static_string ("GooCanvasStyle:fill_rule");
      goo_canvas_style_operator_id = g_quark_from_static_string ("GooCanvasStyle:operator");
      goo_canvas_style_antialias_id = g_quark_from_static_string ("GooCanvasStyle:antialias");
      goo_canvas_style_line_width_id = g_quark_from_static_string ("GooCanvasStyle:line_width");
      goo_canvas_style_line_cap_id = g_quark_from_static_string ("GooCanvasStyle:line_cap");
      goo_canvas_style_line_join_id = g_quark_from_static_string ("GooCanvasStyle:line_join");
      goo_canvas_style_line_join_miter_limit_id = g_quark_from_static_string ("GooCanvasStyle:line_join_miter_limit");
      goo_canvas_style_line_dash_id = g_quark_from_static_string ("GooCanvasStyle:line_dash");
      goo_canvas_style_font_desc_id = g_quark_from_static_string ("GooCanvasStyle:font_desc");
      goo_canvas_style_hint_metrics_id = g_quark_from_static_string ("GooCanvasStyle:hint_metrics");

      initialized = TRUE;
    }
}


static void
goo_canvas_style_class_init (GooCanvasStyleClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->dispose  = goo_canvas_style_dispose;
  gobject_class->finalize = goo_canvas_style_finalize;

  _goo_canvas_style_init ();
}


static void
goo_canvas_style_init (GooCanvasStyle *style)
{
  style->properties = g_array_new (0, 0, sizeof (GooCanvasStyleProperty));
}


/**
 * goo_canvas_style_new:
 * 
 * Creates a new #GooCanvasStyle.
 * 
 * Returns: a new #GooCanvasStyle.
 **/
GooCanvasStyle*
goo_canvas_style_new (void)
{
  return GOO_CANVAS_STYLE (g_object_new (GOO_TYPE_CANVAS_STYLE, NULL));
}


static void
goo_canvas_style_dispose (GObject *object)
{
  GooCanvasStyle *style = (GooCanvasStyle*) object;
  GooCanvasStyleProperty *property;
  gint i;

  if (style->parent)
    {
      g_object_unref (style->parent);
      style->parent = NULL;
    }

  /* Free the property GValues. */
  for (i = 0; i < style->properties->len; i++)
    {
      property = &g_array_index (style->properties, GooCanvasStyleProperty, i);
      g_value_unset (&property->value);
    }
  g_array_set_size (style->properties, 0);

  G_OBJECT_CLASS (goo_canvas_style_parent_class)->dispose (object);
}


static void
goo_canvas_style_finalize (GObject *object)
{
  GooCanvasStyle *style = (GooCanvasStyle*) object;

  g_array_free (style->properties, TRUE);

  G_OBJECT_CLASS (goo_canvas_style_parent_class)->finalize (object);
}


/**
 * goo_canvas_style_copy:
 * @style: a #GooCanvasStyle.
 * 
 * Copies the given #GooCanvasStyle, by copying all of its properties.
 * Though the parent of the new style is left unset.
 * 
 * Returns: a copy of the given #GooCanvasStyle.
 **/
GooCanvasStyle*
goo_canvas_style_copy               (GooCanvasStyle *style)
{
  GooCanvasStyle *copy;
  GooCanvasStyleProperty *property;
  gint i;

  copy = goo_canvas_style_new ();

  for (i = 0; i < style->properties->len; i++)
    {
      property = &g_array_index (style->properties, GooCanvasStyleProperty, i);
      goo_canvas_style_set_property (copy, property->id, &property->value);
    }

  return copy;
}


/**
 * goo_canvas_style_get_parent:
 * @style: a style.
 * 
 * Gets the parent of the style.
 * 
 * Returns: the parent of the given style, or %NULL.
 **/
GooCanvasStyle*
goo_canvas_style_get_parent         (GooCanvasStyle *style)
{
  return style->parent;
}


/**
 * goo_canvas_style_set_parent:
 * @style: a style.
 * @parent: the new parent.
 * 
 * Sets the parent of the style.
 **/
void
goo_canvas_style_set_parent         (GooCanvasStyle *style,
				     GooCanvasStyle *parent)
{
  if (style->parent == parent)
    return;

  if (style->parent)
    g_object_unref (style->parent);

  style->parent = parent;

  if (style->parent)
    g_object_ref (style->parent);
}


/**
 * goo_canvas_style_get_property:
 * @style: a style.
 * @property_id: the property identifier.
 * 
 * Gets the value of a property.
 *
 * This searches though all the #GooCanvasStyle's own list of property settings
 * and also all ancestor #GooCanvasStyle objects.
 *
 * Note that it returns a pointer to the internal #GValue setting, which should
 * not be changed.
 * 
 * Returns: the property value, or %NULL if it isn't set.
 **/
GValue*
goo_canvas_style_get_property       (GooCanvasStyle *style,
				     GQuark          property_id)
{
  GooCanvasStyleProperty *property;
  gint i;

  /* Step up the hierarchy of styles until we find the property. Note that
     if style is passed in as NULL we simply return NULL. */
  while (style)
    {
      for (i = 0; i < style->properties->len; i++)
	{
	  property = &g_array_index (style->properties, GooCanvasStyleProperty,
				     i);
	  if (property->id == property_id)
	    return &property->value;
	}

      style = style->parent;
    }

  return NULL;
}


/**
 * goo_canvas_style_set_property:
 * @style: a style.
 * @property_id: the property identifier.
 * @value: the value of the property.
 * 
 * Sets a property in the style, replacing any current setting.
 *
 * Note that this will override the property setting in ancestor
 * #GooCanvasStyle objects.
 **/
void
goo_canvas_style_set_property	    (GooCanvasStyle *style,
				     GQuark          property_id,
				     const GValue   *value)
{
  GooCanvasStyleProperty *property, new_property = { 0 };
  gint i;

  /* See if the property is already set. */
  for (i = 0; i < style->properties->len; i++)
    {
      property = &g_array_index (style->properties, GooCanvasStyleProperty, i);
      if (property->id == property_id)
	{
	  /* If the new value is NULL, remove the property setting, otherwise
	     update the property value. */
	  if (value)
	    {
	      g_value_copy (value, &property->value);
	    }
	  else
	    {
	      g_value_unset (&property->value);
	      g_array_remove_index_fast (style->properties, i);
	    }

	  return;
	}
    }

  /* The property isn't set, so append a new property. */
  if (value)
    {
      new_property.id = property_id;
      g_value_init (&new_property.value, G_VALUE_TYPE (value));
      g_value_copy (value, &new_property.value);
      g_array_append_val (style->properties, new_property);
    }
}


/**
 * goo_canvas_style_set_stroke_options:
 * @style: a style.
 * @cr: a cairo context.
 * 
 * Sets the standard cairo stroke options using the given style.
 * 
 * Returns: %TRUE if a paint source is set, or %FALSE if the stroke should
 * be skipped.
 **/
gboolean
goo_canvas_style_set_stroke_options (GooCanvasStyle *style,
				     cairo_t        *cr)
{
  GooCanvasStyleProperty *property;
  gboolean operator_set = FALSE, antialias_set = FALSE;
  gboolean stroke_pattern_set = FALSE, line_width_set = FALSE;
  gboolean line_cap_set = FALSE, line_join_set = FALSE;
  gboolean miter_limit_set = FALSE, line_dash_set = FALSE;
  gboolean source_set = FALSE, need_stroke = TRUE;
  gint i;

  if (!style)
    return TRUE;

  /* Step up the hierarchy of styles looking for the properties. */
  while (style)
    {
      for (i = 0; i < style->properties->len; i++)
	{
	  property = &g_array_index (style->properties, GooCanvasStyleProperty,
				     i);

	  if (property->id == goo_canvas_style_operator_id && !operator_set)
	    {
	      cairo_set_operator (cr, property->value.data[0].v_long);
	      operator_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_antialias_id && !antialias_set)
	    {
	      cairo_set_antialias (cr, property->value.data[0].v_long);
	      antialias_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_stroke_pattern_id && !stroke_pattern_set)
	    {
	      if (property->value.data[0].v_pointer)
		{
		  cairo_set_source (cr, property->value.data[0].v_pointer);
		  source_set = TRUE;
		}
	      else
		{
		  /* If the stroke pattern has been explicitly set to NULL,
		     then we don't need to do the stroke. */
		  need_stroke = FALSE;
		}
	      stroke_pattern_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_line_width_id && !line_width_set)
	    {
	      cairo_set_line_width (cr, property->value.data[0].v_double);
	      line_width_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_line_cap_id && !line_cap_set)
	    {
	      cairo_set_line_cap (cr, property->value.data[0].v_long);
	      line_cap_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_line_join_id && !line_join_set)
	    {
	      cairo_set_line_join (cr, property->value.data[0].v_long);
	      line_join_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_line_join_miter_limit_id && !miter_limit_set)
	    {
	      cairo_set_miter_limit (cr, property->value.data[0].v_double);
	      miter_limit_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_line_dash_id && !line_dash_set)
	    {
	      GooCanvasLineDash *dash = property->value.data[0].v_pointer;
	      cairo_set_dash (cr, dash->dashes, dash->num_dashes,
			      dash->dash_offset);
	      line_dash_set = TRUE;
	    }
	}

      style = style->parent;
    }

  /* If a stroke pattern hasn't been set in the style we reset the source to
     black, just in case a fill pattern was used for the item. */
  if (!source_set)
    cairo_set_source_rgb (cr, 0, 0, 0);

  return need_stroke;
}


/**
 * goo_canvas_style_set_fill_options:
 * @style: a style.
 * @cr: a cairo context.
 * 
 * Sets the standard cairo fill options using the given style.
 * 
 * Returns: %TRUE if a paint source is set, or %FALSE if the fill should
 * be skipped.
 **/
gboolean
goo_canvas_style_set_fill_options   (GooCanvasStyle *style,
				     cairo_t        *cr)
{
  GooCanvasStyleProperty *property;
  gboolean operator_set = FALSE, antialias_set = FALSE;
  gboolean fill_rule_set = FALSE, fill_pattern_set = FALSE;
  gboolean need_fill = FALSE;
  gint i;

  if (!style)
    return FALSE;

  /* Step up the hierarchy of styles looking for the properties. */
  while (style)
    {
      for (i = 0; i < style->properties->len; i++)
	{
	  property = &g_array_index (style->properties, GooCanvasStyleProperty,
				     i);

	  if (property->id == goo_canvas_style_operator_id && !operator_set)
	    {
	      cairo_set_operator (cr, property->value.data[0].v_long);
	      operator_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_antialias_id && !antialias_set)
	    {
	      cairo_set_antialias (cr, property->value.data[0].v_long);
	      antialias_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_fill_rule_id && !fill_rule_set)
	    {
	      cairo_set_fill_rule (cr, property->value.data[0].v_long);
	      fill_rule_set = TRUE;
	    }
	  else if (property->id == goo_canvas_style_fill_pattern_id && !fill_pattern_set)
	    {
	      if (property->value.data[0].v_pointer)
		{
		  cairo_set_source (cr, property->value.data[0].v_pointer);
		  need_fill = TRUE;
		}
	      fill_pattern_set = TRUE;
	    }
	}

      style = style->parent;
    }

  return need_fill;
}
