/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvastext.c - text item.
 */

/**
 * SECTION:goocanvastext
 * @Title: GooCanvasText
 * @Short_Description: a text item.
 *
 * GooCanvasText represents a text item.
 *
 * It is a subclass of #GooCanvasItemSimple and so inherits all of the style
 * properties such as "fill-color".
 *
 * It also implements the #GooCanvasItem interface, so you can use the
 * #GooCanvasItem functions such as goo_canvas_item_raise() and
 * goo_canvas_item_rotate().
 *
 * To create a #GooCanvasText use goo_canvas_text_new().
 *
 * To get or set the properties of an existing #GooCanvasText, use
 * g_object_get() and g_object_set().
 */
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "goocanvastext.h"
#include "goocanvas.h"


enum {
  PROP_0,

  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_TEXT,
  PROP_USE_MARKUP,
  PROP_ANCHOR,
  PROP_ALIGN,
  PROP_ELLIPSIZE
};

static void goo_canvas_text_finalize     (GObject            *object);
static void canvas_item_interface_init   (GooCanvasItemIface *iface);
static void goo_canvas_text_get_property (GObject            *object,
					  guint               param_id,
					  GValue             *value,
					  GParamSpec         *pspec);
static void goo_canvas_text_set_property (GObject            *object,
					  guint               param_id,
					  const GValue       *value,
					  GParamSpec         *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasText, goo_canvas_text,
			 GOO_TYPE_CANVAS_ITEM_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM,
						canvas_item_interface_init))


static void
goo_canvas_text_install_common_properties (GObjectClass *gobject_class)
{
  /* Text */
  g_object_class_install_property (gobject_class, PROP_TEXT,
				   g_param_spec_string ("text",
							_("Text"),
							_("The text to display"),
							NULL,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_USE_MARKUP,
				   g_param_spec_boolean ("use-markup",
							 _("Use Markup"),
							 _("Whether to parse PangoMarkup in the text, to support different styles"),
							 FALSE,
							 G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      _("Ellipsize"),
                                                      _("The preferred place to ellipsize the string, if the label does not have enough room to display the entire string"),
						      PANGO_TYPE_ELLIPSIZE_MODE,
						      PANGO_ELLIPSIZE_NONE,
                                                      G_PARAM_READWRITE));

  /* Position */
  g_object_class_install_property (gobject_class, PROP_X,
				   g_param_spec_double ("x",
							"X",
							_("The x coordinate of the text"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y,
				   g_param_spec_double ("y",
							"Y",
							_("The y coordinate of the text"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
				   g_param_spec_double ("width",
							_("Width"),
							_("The width to use to layout the text"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, -1.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ANCHOR,
				   g_param_spec_enum ("anchor",
						      _("Anchor"),
						      _("How to position the text relative to the given x and y coordinates"),
						      GTK_TYPE_ANCHOR_TYPE,
						      GTK_ANCHOR_NW,
						      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ALIGN,
				   g_param_spec_enum ("alignment",
						      _("Alignment"),
						      _("How to align the text"),
						      PANGO_TYPE_ALIGNMENT,
						      PANGO_ALIGN_LEFT,
						      G_PARAM_READWRITE));
}


static void
goo_canvas_text_init (GooCanvasText *text)
{
  text->text_data = g_slice_new0 (GooCanvasTextData);
  text->text_data->width = -1.0;
  text->text_data->anchor = GTK_ANCHOR_NW;
  text->text_data->ellipsize = PANGO_ELLIPSIZE_NONE;

  text->layout_width = -1.0;
}


/**
 * goo_canvas_text_new:
 * @parent: the parent item, or %NULL. If a parent is specified, it will assume
 *  ownership of the item, and the item will automatically be freed when it is
 *  removed from the parent. Otherwise call g_object_unref() to free it.
 * @string: the text to display.
 * @x: the x coordinate of the text.
 * @y: the y coordinate of the text.
 * @width: the width of the text item, or -1 for unlimited width.
 * @anchor: the position of the text relative to the given @x and @y
 *  coordinates. For example an anchor of %GDK_ANCHOR_NW will result in the
 *  top-left of the text being placed at the given @x and @y coordinates.
 *  An anchor of %GDK_ANCHOR_CENTER will result in the center of the text being
 *  placed at the @x and @y coordinates.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new text item.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a text item with the bottom right
 * of the text box placed at (500,500):
 *
 * <informalexample><programlisting>
 *  GooCanvasItem *text = goo_canvas_text_new (mygroup, "Hello World", 500.0, 500.0, 200.0, GTK_ANCHOR_SE,
 *                                             "fill-color", "blue",
 *                                             NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new text item.
 **/
GooCanvasItem*
goo_canvas_text_new (GooCanvasItem *parent,
		     const char    *string,
		     gdouble        x,
		     gdouble        y,
		     gdouble        width,
		     GtkAnchorType  anchor,
		     ...)
{
  GooCanvasItem *item;
  GooCanvasText *text;
  GooCanvasTextData *text_data;
  const char *first_property;
  va_list var_args;

  item = g_object_new (GOO_TYPE_CANVAS_TEXT, NULL);
  text = (GooCanvasText*) item;

  text_data = text->text_data;
  text_data->text = g_strdup (string);
  text_data->x = x;
  text_data->y = y;
  text_data->width = width;
  text_data->anchor = anchor;

  va_start (var_args, anchor);
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
goo_canvas_text_finalize (GObject *object)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasText *text = (GooCanvasText*) object;

  /* Free our data if we didn't have a model. (If we had a model it would
     have been reset in dispose() and simple_data will be NULL.) */
  if (simple->simple_data)
    {
      g_free (text->text_data->text);
      g_slice_free (GooCanvasTextData, text->text_data);
    }
  text->text_data = NULL;

  G_OBJECT_CLASS (goo_canvas_text_parent_class)->finalize (object);
}


static void
goo_canvas_text_get_common_property (GObject                 *object,
				     GooCanvasTextData       *text_data,
				     guint                    prop_id,
				     GValue                  *value,
				     GParamSpec              *pspec)
{
  switch (prop_id)
    {
    case PROP_X:
      g_value_set_double (value, text_data->x);
      break;
    case PROP_Y:
      g_value_set_double (value, text_data->y);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, text_data->width);
      break;
    case PROP_TEXT:
      g_value_set_string (value, text_data->text);
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, text_data->use_markup);
      break;
    case PROP_ELLIPSIZE:
      g_value_set_enum (value, text_data->ellipsize);
      break;
    case PROP_ANCHOR:
      g_value_set_enum (value, text_data->anchor);
      break;
    case PROP_ALIGN:
      g_value_set_enum (value, text_data->alignment);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
goo_canvas_text_get_property (GObject              *object,
			      guint                 prop_id,
			      GValue               *value,
			      GParamSpec           *pspec)
{
  GooCanvasText *text = (GooCanvasText*) object;

  goo_canvas_text_get_common_property (object, text->text_data, prop_id,
				       value, pspec);
}


static void
goo_canvas_text_set_common_property (GObject                 *object,
				     GooCanvasTextData       *text_data,
				     guint                    prop_id,
				     const GValue            *value,
				     GParamSpec              *pspec)
{
  switch (prop_id)
    {
    case PROP_X:
      text_data->x = g_value_get_double (value);
      break;
    case PROP_Y:
      text_data->y = g_value_get_double (value);
      break;
    case PROP_WIDTH:
      text_data->width = g_value_get_double (value);
      break;
    case PROP_TEXT:
      g_free (text_data->text);
      text_data->text = g_value_dup_string (value);
      break;
    case PROP_USE_MARKUP:
      text_data->use_markup = g_value_get_boolean (value);
      break;
    case PROP_ELLIPSIZE:
      text_data->ellipsize = g_value_get_enum (value);
      break;
    case PROP_ANCHOR:
      text_data->anchor = g_value_get_enum (value);
      break;
    case PROP_ALIGN:
      text_data->alignment = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
goo_canvas_text_set_property (GObject              *object,
			      guint                 prop_id,
			      const GValue         *value,
			      GParamSpec           *pspec)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) object;
  GooCanvasText *text = (GooCanvasText*) object;

  if (simple->model)
    {
      g_warning ("Can't set property of a canvas item with a model - set the model property instead");
      return;
    }

  goo_canvas_text_set_common_property (object, text->text_data, prop_id,
				       value, pspec);
  goo_canvas_item_simple_changed (simple, TRUE);
}


static PangoLayout*
goo_canvas_text_create_layout (GooCanvasText     *text,
			       cairo_t           *cr,
			       GooCanvasBounds   *bounds,
			       gdouble	         *origin_x_return,
			       gdouble	         *origin_y_return)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) text;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasTextData *text_data = text->text_data;
  GooCanvasStyle *style = simple_data->style;
  GValue *svalue;
  PangoLayout *layout;
  PangoContext *context;
  PangoRectangle ink_rect, logical_rect;
  double logical_width, logical_height, align_width, origin_x, origin_y;
  gchar *string;
  double x1_extension, x2_extension, y1_extension, y2_extension;
  cairo_font_options_t *font_options;
  cairo_hint_metrics_t hint_metrics = CAIRO_HINT_METRICS_OFF;

  string = text_data->text ? text_data->text : "";

  layout = pango_cairo_create_layout (cr);
  context = pango_layout_get_context (layout);

  if (text->layout_width > 0)
    pango_layout_set_width (layout, (double) text->layout_width * PANGO_SCALE);

  if (text_data->use_markup)
    pango_layout_set_markup (layout, string, -1);
  else
    pango_layout_set_text (layout, string, -1);

  svalue = goo_canvas_style_get_property (style,
					  goo_canvas_style_font_desc_id);
  if (svalue)
    pango_layout_set_font_description (layout, svalue->data[0].v_pointer);

  svalue = goo_canvas_style_get_property (style,
					  goo_canvas_style_hint_metrics_id);
  if (svalue)
    hint_metrics = svalue->data[0].v_long;

  font_options = cairo_font_options_create ();
  cairo_font_options_set_hint_metrics (font_options, hint_metrics);
  cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);
  pango_cairo_context_set_font_options (context, font_options);
  cairo_font_options_destroy (font_options);

  if (text_data->alignment != PANGO_ALIGN_LEFT)
    pango_layout_set_alignment (layout, text_data->alignment);

  pango_layout_set_ellipsize (layout, text_data->ellipsize);

  if (bounds)
    {
      /* Get size of the text, so we can position it according to anchor. */
      pango_layout_get_extents (layout, &ink_rect, &logical_rect);

      logical_width = (double) logical_rect.width / PANGO_SCALE;
      logical_height = (double) logical_rect.height / PANGO_SCALE;

      /* If the text width has been set, that width is used to do the alignment
	 positioning. Otherwise the actual width is used. */
      if (text_data->width > 0)
	align_width = text_data->width;
      else
	align_width = logical_width;

      /* Now calculate the origin of the text, i.e. where we will tell Pango
	 to draw it. */
      origin_x = text_data->x;
      origin_y = text_data->y;

      switch (text_data->anchor)
	{
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
	  origin_x -= align_width / 2.0;
	break;
	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
	  origin_x -= align_width;
	  break;
	default:
	  break;
	}

      switch (text_data->anchor)
	{
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
	  origin_y -= logical_height / 2.0;
	  break;
	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
	  origin_y -= logical_height;
	  break;
	default:
	  break;
	}

      /* Return the origin of the text if required. */
      if (origin_x_return)
	*origin_x_return = origin_x;
      if (origin_y_return)
	*origin_y_return = origin_y;

      /* Now calculate the logical bounds. */
      bounds->x1 = origin_x;
      bounds->y1 = origin_y;

      if (text_data->width > 0)
	{
	  /* If the text width has been set, and the alignment isn't
	     PANGO_ALIGN_LEFT, we need to adjust for the difference between
	     the actual width of the text and the width that was used for
	     alignment. */
	  switch (text_data->alignment)
	    {
	    case PANGO_ALIGN_CENTER:
	      bounds->x1 += (align_width - logical_width) / 2.0;
	      break;
	    case PANGO_ALIGN_RIGHT:
	      bounds->x1 += align_width - logical_width;
	      break;
	    default:
	      break;
	    }
	}

      bounds->x2 = origin_x + logical_width;
      bounds->y2 = origin_y + logical_height;

      /* Now adjust it to take into account the ink bounds. Calculate how far
	 the ink rect extends outside each edge of the logical rect and adjust
	 the bounds as necessary. */
      x1_extension = logical_rect.x - ink_rect.x;
      if (x1_extension > 0)
	bounds->x1 -= x1_extension / PANGO_SCALE;

      x2_extension = (ink_rect.x + ink_rect.width)
	- (logical_rect.x + logical_rect.width);
      if (x2_extension > 0)
	bounds->x2 += x2_extension / PANGO_SCALE;

      y1_extension = logical_rect.y - ink_rect.y;
      if (y1_extension > 0)
	bounds->y1 -= y1_extension / PANGO_SCALE;

      y2_extension = (ink_rect.y + ink_rect.height)
	- (logical_rect.y + logical_rect.height);
      if (y2_extension > 0)
	bounds->y2 += y2_extension / PANGO_SCALE;
    }

  return layout;
}


static void
goo_canvas_text_update  (GooCanvasItemSimple *simple,
			 cairo_t             *cr)
{
  GooCanvasText *text = (GooCanvasText*) simple;
  PangoLayout *layout;

  /* Initialize the layout width to the text item's specified width property.
     It may get changed later in get_requested_height() according to the
     layout container and settings. */
  text->layout_width = text->text_data->width;

  /* Compute the new bounds. */
  layout = goo_canvas_text_create_layout (text, cr, &simple->bounds,
					  NULL, NULL);
  g_object_unref (layout);
}


static gboolean
goo_canvas_text_is_unpainted (GooCanvasStyle *style)
{
  GValue *value;

  value = goo_canvas_style_get_property (style,
					 goo_canvas_style_fill_pattern_id);

  /* We only return TRUE if the value is explicitly set to NULL. */
  if (value && !value->data[0].v_pointer)
    return TRUE;
  return FALSE;
}


static gboolean
goo_canvas_text_is_item_at (GooCanvasItemSimple *simple,
			    gdouble              x,
			    gdouble              y,
			    cairo_t             *cr,
			    gboolean             is_pointer_event)
{
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasText *text = (GooCanvasText*) simple;
  PangoLayout *layout;
  GooCanvasBounds bounds;
  PangoLayoutIter *iter;
  PangoRectangle ink_rect, log_rect;
  int px, py, x1, y1, x2, y2;
  int log_x2, ink_x2, log_y2, ink_y2;
  gdouble origin_x, origin_y;
  gboolean in_item = FALSE;

  /* If there is no text just return. */
  if (!text->text_data->text || !text->text_data->text[0])
    return FALSE;

  if (is_pointer_event
      && simple_data->pointer_events & GOO_CANVAS_EVENTS_PAINTED_MASK
      && goo_canvas_text_is_unpainted (simple_data->style))
    return FALSE;

  layout = goo_canvas_text_create_layout (text, cr, &bounds,
					  &origin_x, &origin_y);

  /* Convert the coordinates into Pango units. */
  px = (x - origin_x) * PANGO_SCALE;
  py = (y - origin_y) * PANGO_SCALE;

  /* We use line extents here. Note that SVG uses character cells to determine
     hits so we have slightly different behavior. */
  iter = pango_layout_get_iter (layout);
  do
    {
      pango_layout_iter_get_line_extents (iter, &ink_rect, &log_rect);

      /* We use a union of the ink rect and the logical rect, as we want to
	 let the user click on any part of the ink, even if it extends outside
	 the character cell (i.e. the ink rect), or click on the space in the
	 character cell (i.e. the logical rect). */
      x1 = MIN (log_rect.x, ink_rect.x);
      y1 = MIN (log_rect.y, ink_rect.y);

      log_x2 = log_rect.x + log_rect.width;
      ink_x2 = ink_rect.x + ink_rect.width;
      x2 = MAX (log_x2, ink_x2);

      log_y2 = log_rect.y + log_rect.height;
      ink_y2 = ink_rect.y + ink_rect.height;
      y2 = MAX (log_y2, ink_y2);

      if (px >= x1 && px < x2 && py >= y1 && py < y2)
	{
	  in_item = TRUE;
	  break;
	}

    } while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);

  g_object_unref (layout);

  return in_item;
}


static void
goo_canvas_text_paint (GooCanvasItemSimple   *simple,
		       cairo_t               *cr,
		       const GooCanvasBounds *bounds)
{
  GooCanvasText *text = (GooCanvasText*) simple;
  PangoLayout *layout;
  GooCanvasBounds layout_bounds;
  gdouble origin_x, origin_y;

  /* If there is no text just return. */
  if (!text->text_data->text || !text->text_data->text[0])
    return;

  goo_canvas_style_set_fill_options (simple->simple_data->style, cr);

  cairo_new_path (cr);
  layout = goo_canvas_text_create_layout (text, cr, &layout_bounds,
					  &origin_x, &origin_y);
  cairo_move_to (cr, origin_x, origin_y);
  pango_cairo_show_layout (cr, layout);
  g_object_unref (layout);
}


static gdouble
goo_canvas_text_get_requested_height (GooCanvasItem	*item,
				      cairo_t		*cr,
				      gdouble            width)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasItemSimpleData *simple_data = simple->simple_data;
  GooCanvasText *text = (GooCanvasText*) item;
  PangoLayout *layout;
  gdouble height;

  /* If we have a transformation besides a simple scale & translation, just
     return -1 as we can't adjust the height in that case. */
  if (simple_data->clip_path_commands
      || (simple_data->transform && (simple_data->transform->xy != 0.0
				     || simple_data->transform->yx != 0.0)))
    return -1;

  cairo_save (cr);
  if (simple_data->transform)
    cairo_transform (cr, simple_data->transform);

  /* Convert the width from the parent's coordinate space. Note that we only
     need to support a simple scale operation here. */
  text->layout_width = width;
  if (simple_data->transform)
    text->layout_width /= simple_data->transform->xx;

  /* Create layout with given width. */
  layout = goo_canvas_text_create_layout (text, cr, &simple->bounds,
					  NULL, NULL);
  g_object_unref (layout);

  /* Convert to the parent's coordinate space. As above,  we only need to
     support a simple scale operation here. */
  height = simple->bounds.y2 - simple->bounds.y1;
  if (simple_data->transform)
    height *= simple_data->transform->yy;

  /* Convert the item's bounds to device space. */
  goo_canvas_item_simple_user_bounds_to_device (simple, cr, &simple->bounds);

  cairo_restore (cr);

  /* Return the new requested height of the text. */
  return height;
}


static void
goo_canvas_text_set_model    (GooCanvasItem      *item,
			      GooCanvasItemModel *model)
{
  GooCanvasItemSimple *simple = (GooCanvasItemSimple*) item;
  GooCanvasText *text = (GooCanvasText*) item;
  GooCanvasTextModel *tmodel = (GooCanvasTextModel*) model;

  /* If our text_data was allocated, free it. */
  if (!simple->model)
    g_slice_free (GooCanvasTextData, text->text_data);

  /* Now use the new model's text_data instead. */
  text->text_data = &tmodel->text_data;

  /* Let the parent GooCanvasItemSimple code do the rest. */
  goo_canvas_item_simple_set_model (simple, model);
}


static void
canvas_item_interface_init (GooCanvasItemIface *iface)
{
  iface->get_requested_height = goo_canvas_text_get_requested_height;
  iface->set_model            = goo_canvas_text_set_model;
}


static void
goo_canvas_text_class_init (GooCanvasTextClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;
  GooCanvasItemSimpleClass *simple_class = (GooCanvasItemSimpleClass*) klass;

  gobject_class->finalize = goo_canvas_text_finalize;

  gobject_class->get_property = goo_canvas_text_get_property;
  gobject_class->set_property = goo_canvas_text_set_property;

  simple_class->simple_update        = goo_canvas_text_update;
  simple_class->simple_paint         = goo_canvas_text_paint;
  simple_class->simple_is_item_at    = goo_canvas_text_is_item_at;

  goo_canvas_text_install_common_properties (gobject_class);
}



/**
 * SECTION:goocanvastextmodel
 * @Title: GooCanvasTextModel
 * @Short_Description: a model for text items.
 *
 * GooCanvasTextModel represents a model for text items.
 *
 * It is a subclass of #GooCanvasItemModelSimple and so inherits all of the
 * style properties such as "fill-color".
 *
 * It also implements the #GooCanvasItemModel interface, so you can use the
 * #GooCanvasItemModel functions such as goo_canvas_item_model_raise() and
 * goo_canvas_item_model_rotate().
 *
 * To create a #GooCanvasTextModel use goo_canvas_text_model_new().
 *
 * To get or set the properties of an existing #GooCanvasTextModel, use
 * g_object_get() and g_object_set().
 *
 * To respond to events such as mouse clicks on the text item you must connect
 * to the signal handlers of the corresponding #GooCanvasText objects.
 * (See goo_canvas_get_item() and #GooCanvas::item-created.)
 */

static void item_model_interface_init (GooCanvasItemModelIface *iface);
static void goo_canvas_text_model_finalize     (GObject            *object);
static void goo_canvas_text_model_get_property (GObject            *object,
						guint               param_id,
						GValue             *value,
						GParamSpec         *pspec);
static void goo_canvas_text_model_set_property (GObject            *object,
						guint               param_id,
						const GValue       *value,
						GParamSpec         *pspec);

G_DEFINE_TYPE_WITH_CODE (GooCanvasTextModel, goo_canvas_text_model,
			 GOO_TYPE_CANVAS_ITEM_MODEL_SIMPLE,
			 G_IMPLEMENT_INTERFACE (GOO_TYPE_CANVAS_ITEM_MODEL,
						item_model_interface_init))


static void
goo_canvas_text_model_class_init (GooCanvasTextModelClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->finalize     = goo_canvas_text_model_finalize;

  gobject_class->get_property = goo_canvas_text_model_get_property;
  gobject_class->set_property = goo_canvas_text_model_set_property;

  goo_canvas_text_install_common_properties (gobject_class);
}


static void
goo_canvas_text_model_init (GooCanvasTextModel *tmodel)
{
  tmodel->text_data.width = -1.0;
  tmodel->text_data.anchor = GTK_ANCHOR_NW;
  tmodel->text_data.ellipsize = PANGO_ELLIPSIZE_NONE;
}


/**
 * goo_canvas_text_model_new:
 * @parent: the parent model, or %NULL. If a parent is specified, it will
 *  assume ownership of the item, and the item will automatically be freed when
 *  it is removed from the parent. Otherwise call g_object_unref() to free it.
 * @string: the text to display.
 * @x: the x coordinate of the text.
 * @y: the y coordinate of the text.
 * @width: the width of the text item, or -1 for unlimited width.
 * @anchor: the position of the text relative to the given @x and @y
 *  coordinates. For example an anchor of %GDK_ANCHOR_NW will result in the
 *  top-left of the text being placed at the given @x and @y coordinates.
 *  An anchor of %GDK_ANCHOR_CENTER will result in the center of the text being
 *  placed at the @x and @y coordinates.
 * @...: optional pairs of property names and values, and a terminating %NULL.
 * 
 * Creates a new text model.
 * 
 * <!--PARAMETERS-->
 *
 * Here's an example showing how to create a text item with the bottom right
 * of the text box placed at (500,500):
 *
 * <informalexample><programlisting>
 *  GooCanvasItemModel *text = goo_canvas_text_model_new (mygroup, "Hello World", 500.0, 500.0, 200.0, GTK_ANCHOR_SE,
 *                                                        "fill-color", "blue",
 *                                                        NULL);
 * </programlisting></informalexample>
 * 
 * Returns: a new text model.
 **/
GooCanvasItemModel*
goo_canvas_text_model_new (GooCanvasItemModel *parent,
			   const char         *string,
			   gdouble             x,
			   gdouble             y,
			   gdouble             width,
			   GtkAnchorType       anchor,
			   ...)
{
  GooCanvasItemModel *model;
  GooCanvasTextModel *tmodel;
  GooCanvasTextData *text_data;
  const char *first_property;
  va_list var_args;

  model = g_object_new (GOO_TYPE_CANVAS_TEXT_MODEL, NULL);
  tmodel = (GooCanvasTextModel*) model;

  text_data = &tmodel->text_data;
  text_data->text = g_strdup (string);
  text_data->x = x;
  text_data->y = y;
  text_data->width = width;
  text_data->anchor = anchor;

  va_start (var_args, anchor);
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
goo_canvas_text_model_finalize (GObject *object)
{
  GooCanvasTextModel *tmodel = (GooCanvasTextModel*) object;

  g_free (tmodel->text_data.text);

  G_OBJECT_CLASS (goo_canvas_text_model_parent_class)->finalize (object);
}


static void
goo_canvas_text_model_get_property (GObject              *object,
				    guint                 prop_id,
				    GValue               *value,
				    GParamSpec           *pspec)
{
  GooCanvasTextModel *tmodel = (GooCanvasTextModel*) object;

  goo_canvas_text_get_common_property (object, &tmodel->text_data, prop_id,
				       value, pspec);
}


static void
goo_canvas_text_model_set_property (GObject              *object,
				    guint                 prop_id,
				    const GValue         *value,
				    GParamSpec           *pspec)
{
  GooCanvasTextModel *tmodel = (GooCanvasTextModel*) object;

  goo_canvas_text_set_common_property (object, &tmodel->text_data, prop_id,
				       value, pspec);
  g_signal_emit_by_name (tmodel, "changed", TRUE);
}


static GooCanvasItem*
goo_canvas_text_model_create_item (GooCanvasItemModel *model,
				   GooCanvas          *canvas)
{
  GooCanvasItem *item;

  item = g_object_new (GOO_TYPE_CANVAS_TEXT, NULL);
  goo_canvas_item_set_model (item, model);

  return item;
}


static void
item_model_interface_init (GooCanvasItemModelIface *iface)
{
  iface->create_item    = goo_canvas_text_model_create_item;
}
