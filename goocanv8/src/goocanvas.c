/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvas.c - the main canvas widget.
 */

/**
 * SECTION: goocanvas
 * @Title: GooCanvas
 * @Short_Description: the main canvas widget.
 *
 * #GooCanvas is the main widget containing a number of canvas items.
 *
 *
 * Here is a simple example:
 *
 * <informalexample><programlisting>
 *  &num;include &lt;goocanvas.h&gt;
 *  
 *  static gboolean on_rect_button_press (GooCanvasItem  *view,
 *                                        GooCanvasItem  *target,
 *                                        GdkEventButton *event,
 *                                        gpointer        data);
 *  
 *  int
 *  main (int argc, char *argv[])
 *  {
 *    GtkWidget *window, *scrolled_win, *canvas;
 *    GooCanvasItem *root, *rect_item, *text_item;
 *  
 *    /&ast; Initialize GTK+. &ast;/
 *    gtk_set_locale&nbsp;();
 *    gtk_init (&amp;argc, &amp;argv);
 *  
 *    /&ast; Create the window and widgets. &ast;/
 *    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
 *    gtk_window_set_default_size (GTK_WINDOW (window), 640, 600);
 *    gtk_widget_show (window);
 *    g_signal_connect (window, "delete_event", (GtkSignalFunc) on_delete_event,
 *                      NULL);
 *  
 *    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
 *    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
 *                                         GTK_SHADOW_IN);
 *    gtk_widget_show (scrolled_win);
 *    gtk_container_add (GTK_CONTAINER (window), scrolled_win);
 *  
 *    canvas = goo_canvas_new&nbsp;();
 *    gtk_widget_set_size_request (canvas, 600, 450);
 *    goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 1000, 1000);
 *    gtk_widget_show (canvas);
 *    gtk_container_add (GTK_CONTAINER (scrolled_win), canvas);
 *  
 *    root = goo_canvas_get_root_item (GOO_CANVAS (canvas));
 *  
 *    /&ast; Add a few simple items. &ast;/
 *    rect_item = goo_canvas_rect_new (root, 100, 100, 400, 400,
 *                                     "line-width", 10.0,
 *                                     "radius-x", 20.0,
 *                                     "radius-y", 10.0,
 *                                     "stroke-color", "yellow",
 *                                     "fill-color", "red",
 *                                     NULL);
 *  
 *    text_item = goo_canvas_text_new (root, "Hello World", 300, 300, -1,
 *                                     GTK_ANCHOR_CENTER,
 *                                     "font", "Sans 24",
 *                                     NULL);
 *    goo_canvas_item_rotate (text_item, 45, 300, 300);
 *  
 *    /&ast; Connect a signal handler for the rectangle item. &ast;/
 *    g_signal_connect (rect_item, "button_press_event",
 *                      (GtkSignalFunc) on_rect_button_press, NULL);
 *  
 *    /&ast; Pass control to the GTK+ main event loop. &ast;/
 *    gtk_main&nbsp;();
 *  
 *    return 0;
 *  }
 *  
 *  
 *  /&ast; This handles button presses in item views. We simply output a message to
 *     the console. &ast;/
 *  static gboolean
 *  on_rect_button_press (GooCanvasItem  *item,
 *                        GooCanvasItem  *target,
 *                        GdkEventButton *event,
 *                        gpointer        data)
 *  {
 *    g_print ("rect item received button press event\n");
 *    return TRUE;
 *  }
 *  
 * </programlisting></informalexample>
 */
#include <config.h>
#include <math.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "goocanvasatk.h"
#include "goocanvas.h"
#include "goocanvasitemmodel.h"
#include "goocanvasitem.h"
#include "goocanvasgroup.h"
#include "goocanvasmarshal.h"


enum {
  PROP_0,

  PROP_SCALE,
  PROP_ANCHOR,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_UNITS,
  PROP_RESOLUTION_X,
  PROP_RESOLUTION_Y,
  PROP_BACKGROUND_COLOR,
  PROP_BACKGROUND_COLOR_RGB
};

enum {
  ITEM_CREATED,

  LAST_SIGNAL
};

static guint canvas_signals[LAST_SIGNAL] = { 0 };

static void     goo_canvas_dispose	   (GObject          *object);
static void     goo_canvas_finalize	   (GObject          *object);
static void     goo_canvas_realize         (GtkWidget        *widget);
static void     goo_canvas_unrealize	   (GtkWidget	     *widget);
static void     goo_canvas_map		   (GtkWidget	     *widget);
static void     goo_canvas_style_set	   (GtkWidget	     *widget,
					    GtkStyle	     *old_style);
static void     goo_canvas_size_request    (GtkWidget        *widget,
					    GtkRequisition   *requisition);
static void     goo_canvas_size_allocate   (GtkWidget        *widget,
					    GtkAllocation    *allocation);
static void     goo_canvas_set_adjustments (GooCanvas        *canvas,
					    GtkAdjustment    *hadj,
					    GtkAdjustment    *vadj);
static gboolean goo_canvas_expose_event	   (GtkWidget        *widget,
					    GdkEventExpose   *event);
static gboolean goo_canvas_button_press    (GtkWidget        *widget,
					    GdkEventButton   *event);
static gboolean goo_canvas_button_release  (GtkWidget        *widget,
					    GdkEventButton   *event);
static gboolean goo_canvas_motion          (GtkWidget        *widget,
					    GdkEventMotion   *event);
static gboolean goo_canvas_scroll          (GtkWidget        *widget,
					    GdkEventScroll   *event);
static gboolean goo_canvas_focus	   (GtkWidget        *widget,
					    GtkDirectionType  direction);
static gboolean goo_canvas_key_press       (GtkWidget        *widget,
					    GdkEventKey      *event);
static gboolean goo_canvas_key_release     (GtkWidget        *widget,
					    GdkEventKey      *event);
static gboolean goo_canvas_crossing        (GtkWidget        *widget,
					    GdkEventCrossing *event);
static gboolean goo_canvas_focus_in        (GtkWidget        *widget,
					    GdkEventFocus    *event);
static gboolean goo_canvas_focus_out       (GtkWidget        *widget,
					    GdkEventFocus    *event);
static gboolean goo_canvas_grab_broken     (GtkWidget        *widget,
					    GdkEventGrabBroken *event);
static void     goo_canvas_get_property    (GObject          *object,
					    guint             prop_id,
					    GValue           *value,
					    GParamSpec       *pspec);
static void     goo_canvas_set_property    (GObject          *object,
					    guint             prop_id,
					    const GValue     *value,
					    GParamSpec       *pspec);
static void     goo_canvas_remove          (GtkContainer     *container,
					    GtkWidget        *widget);
static void     goo_canvas_forall          (GtkContainer     *container,
					    gboolean          include_internals,
					    GtkCallback       callback,
					    gpointer          callback_data);

static void     set_item_pointer           (GooCanvasItem   **item,
					    GooCanvasItem    *new_item);
static void     update_pointer_item        (GooCanvas        *canvas,
					    GdkEvent         *event);
static void     reconfigure_canvas	   (GooCanvas        *canvas,
					    gboolean          redraw_if_needed);


G_DEFINE_TYPE (GooCanvas, goo_canvas, GTK_TYPE_CONTAINER)

/* This evaluates to TRUE if an item is still in the canvas. */
#define ITEM_IS_VALID(item) (goo_canvas_item_get_canvas (item))

static void
goo_canvas_class_init (GooCanvasClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;
  GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;
  GtkContainerClass *container_class = (GtkContainerClass*) klass;

  gobject_class->dispose	     = goo_canvas_dispose;
  gobject_class->finalize	     = goo_canvas_finalize;
  gobject_class->get_property	     = goo_canvas_get_property;
  gobject_class->set_property	     = goo_canvas_set_property;

  widget_class->realize              = goo_canvas_realize;
  widget_class->unrealize            = goo_canvas_unrealize;
  widget_class->map                  = goo_canvas_map;
  widget_class->size_request         = goo_canvas_size_request;
  widget_class->size_allocate        = goo_canvas_size_allocate;
  widget_class->style_set            = goo_canvas_style_set;
  widget_class->expose_event         = goo_canvas_expose_event;
  widget_class->button_press_event   = goo_canvas_button_press;
  widget_class->button_release_event = goo_canvas_button_release;
  widget_class->motion_notify_event  = goo_canvas_motion;
  widget_class->scroll_event         = goo_canvas_scroll;
  widget_class->focus                = goo_canvas_focus;
  widget_class->key_press_event      = goo_canvas_key_press;
  widget_class->key_release_event    = goo_canvas_key_release;
  widget_class->enter_notify_event   = goo_canvas_crossing;
  widget_class->leave_notify_event   = goo_canvas_crossing;
  widget_class->focus_in_event       = goo_canvas_focus_in;
  widget_class->focus_out_event      = goo_canvas_focus_out;
  widget_class->grab_broken_event    = goo_canvas_grab_broken;

  container_class->remove	     = goo_canvas_remove;
  container_class->forall            = goo_canvas_forall;

  klass->set_scroll_adjustments      = goo_canvas_set_adjustments;

  /* Register our accessible factory, but only if accessibility is enabled. */
  if (!ATK_IS_NO_OP_OBJECT_FACTORY (atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_WIDGET)))
    {
      atk_registry_set_factory_type (atk_get_default_registry (),
				     GOO_TYPE_CANVAS,
				     goo_canvas_accessible_factory_get_type ());
    }

  g_object_class_install_property (gobject_class, PROP_SCALE,
				   g_param_spec_double ("scale",
							_("Scale"),
							_("The magnification factor of the canvas"),
							0.0, G_MAXDOUBLE, 1.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ANCHOR,
				   g_param_spec_enum ("anchor",
						      _("Anchor"),
						      _("Where to place the canvas when it is smaller than the widget's allocated area"),
						      GTK_TYPE_ANCHOR_TYPE,
						      GTK_ANCHOR_NW,
						      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_X1,
				   g_param_spec_double ("x1",
							_("X1"),
							_("The x coordinate of the left edge of the canvas bounds, in canvas units"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y1,
				   g_param_spec_double ("y1",
							_("Y1"),
							_("The y coordinate of the top edge of the canvas bounds, in canvas units"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 0.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_X2,
				   g_param_spec_double ("x2",
							_("X2"),
							_("The x coordinate of the right edge of the canvas bounds, in canvas units"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 1000.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y2,
				   g_param_spec_double ("y2",
							_("Y2"),
							_("The y coordinate of the bottom edge of the canvas bounds, in canvas units"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE, 1000.0,
							G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class, PROP_UNITS,
				   g_param_spec_enum ("units",
						      _("Units"),
						      _("The units to use for the canvas"),
						      GTK_TYPE_UNIT,
						      GTK_UNIT_PIXEL,
						      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RESOLUTION_X,
				   g_param_spec_double ("resolution-x",
							_("Resolution X"),
							_("The horizontal resolution of the display, in dots per inch"),
							0.0, G_MAXDOUBLE,
							96.0,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RESOLUTION_Y,
				   g_param_spec_double ("resolution-y",
							_("Resolution Y"),
							_("The vertical resolution of the display, in dots per inch"),
							0.0, G_MAXDOUBLE,
							96.0,
							G_PARAM_READWRITE));

  /* Convenience properties - writable only. */
  g_object_class_install_property (gobject_class, PROP_BACKGROUND_COLOR,
				   g_param_spec_string ("background-color",
							_("Background Color"),
							_("The color to use for the canvas background"),
							NULL,
							G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_BACKGROUND_COLOR_RGB,
				   g_param_spec_uint ("background-color-rgb",
						      _("Background Color RGB"),
						      _("The color to use for the canvas background, specified as a 24-bit integer value, 0xRRGGBB"),
						      0, G_MAXUINT, 0,
						      G_PARAM_WRITABLE));

  /**
   * GooCanvas::set-scroll-adjustments
   * @canvas: the canvas.
   * @hadjustment: the horizontal adjustment.
   * @vadjustment: the vertical adjustment.
   *
   * This is used when the #GooCanvas is placed inside a #GtkScrolledWindow,
   * to connect up the adjustments so scrolling works properly.
   *
   * It isn't useful for applications.
   */
  widget_class->set_scroll_adjustments_signal =
    g_signal_new ("set_scroll_adjustments",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GooCanvasClass, set_scroll_adjustments),
		  NULL, NULL,
		  goo_canvas_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_ADJUSTMENT,
		  GTK_TYPE_ADJUSTMENT);

  /* Signals. */

  /**
   * GooCanvas::item-created
   * @canvas: the canvas.
   * @item: the new item.
   * @model: the item's model.
   *
   * This is emitted when a new canvas item is created, in model/view mode.
   *
   * Applications can set up signal handlers for the new items here.
   */
  canvas_signals[ITEM_CREATED] =
    g_signal_new ("item-created",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GooCanvasClass, item_created),
		  NULL, NULL,
		  goo_canvas_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  GOO_TYPE_CANVAS_ITEM,
		  GOO_TYPE_CANVAS_ITEM_MODEL);
}


static void
goo_canvas_init (GooCanvas *canvas)
{
  /* We set GTK_CAN_FOCUS by default, so it works as people expect.
     Though developers can turn this off if not needed for efficiency. */
  GTK_WIDGET_SET_FLAGS (canvas, GTK_CAN_FOCUS);

  canvas->scale = 1.0;
  canvas->need_update = TRUE;
  canvas->crossing_event.type = GDK_LEAVE_NOTIFY;
  canvas->anchor = GTK_ANCHOR_NORTH_WEST;

  /* Set the default bounds to a reasonable size. */
  canvas->bounds.x1 = 0.0;
  canvas->bounds.y1 = 0.0;
  canvas->bounds.x2 = 1000.0;
  canvas->bounds.y2 = 1000.0;

  canvas->units = GTK_UNIT_PIXEL;
  canvas->resolution_x = 96.0;
  canvas->resolution_y = 96.0;

  /* Create our own adjustments, in case we aren't inserted into a scrolled
     window. The accessibility code needs these. */
  canvas->hadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							    0.0, 0.0, 0.0));
  canvas->vadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							    0.0, 0.0, 0.0));

  g_object_ref_sink (canvas->hadjustment);
  g_object_ref_sink (canvas->vadjustment);

  canvas->model_to_item = g_hash_table_new (g_direct_hash, g_direct_equal);

  /* Use a simple group as the default root item, which is fine 99% of the
     time. Apps can set their own root item if required. */
  canvas->root_item = goo_canvas_group_new (NULL, NULL);
  goo_canvas_item_set_canvas (canvas->root_item, canvas);
}


/**
 * goo_canvas_new:
 * 
 * Creates a new #GooCanvas widget.
 *
 * A #GooCanvasGroup is created automatically as the root item of the canvas,
 * though this can be overriden with goo_canvas_set_root_item() or
 * goo_canvas_set_root_item_model().
 * 
 * Returns: a new #GooCanvas widget.
 **/
GtkWidget*
goo_canvas_new (void)
{
  return GTK_WIDGET (g_object_new (GOO_TYPE_CANVAS, NULL));
}

static void
goo_canvas_dispose (GObject *object)
{
  GooCanvas *canvas = (GooCanvas*) object;

  if (canvas->model_to_item)
    {
      g_hash_table_destroy (canvas->model_to_item);
      canvas->model_to_item = NULL;
    }

  if (canvas->root_item)
    {
      g_object_unref (canvas->root_item);
      canvas->root_item = NULL;
    }

  if (canvas->root_item_model)
    {
      g_object_unref (canvas->root_item_model);
      canvas->root_item_model = NULL;
    }

  if (canvas->idle_id)
    {
      g_source_remove (canvas->idle_id);
      canvas->idle_id = 0;
    }

  /* Release any references we hold to items. */
  set_item_pointer (&canvas->pointer_item, NULL);
  set_item_pointer (&canvas->pointer_grab_item, NULL);
  set_item_pointer (&canvas->pointer_grab_initial_item, NULL);
  set_item_pointer (&canvas->focused_item, NULL);
  set_item_pointer (&canvas->keyboard_grab_item, NULL);

  if (canvas->hadjustment)
    {
      g_object_unref (canvas->hadjustment);
      canvas->hadjustment = NULL;
    }

  if (canvas->vadjustment)
    {
      g_object_unref (canvas->vadjustment);
      canvas->vadjustment = NULL;
    }

  G_OBJECT_CLASS (goo_canvas_parent_class)->dispose (object);
}


static void
goo_canvas_finalize (GObject *object)
{
  /*GooCanvas *canvas = (GooCanvas*) object;*/

  G_OBJECT_CLASS (goo_canvas_parent_class)->finalize (object);
}


/**
 * goo_canvas_get_default_line_width:
 * @canvas: a #GooCanvas.
 * 
 * Gets the default line width, which depends on the current units setting.
 * 
 * Returns: the default line width of the canvas.
 **/
gdouble
goo_canvas_get_default_line_width (GooCanvas *canvas)
{
  gdouble line_width = 2.0;

  /* We use the same default as cairo when using pixels, i.e. 2 pixels.
     For other units we use 2 points, or thereabouts. */
  switch (canvas->units)
    {
    case GTK_UNIT_PIXEL:
      line_width = 2.0;
      break;
    case GTK_UNIT_POINTS:
      line_width = 2.0;
      break;
    case GTK_UNIT_INCH:
      line_width = 2.0 / 72.0;
      break;
    case GTK_UNIT_MM:
      line_width = 0.7;
      break;
    }

  return line_width;
}


/**
 * goo_canvas_create_cairo_context:
 * @canvas: a #GooCanvas.
 * 
 * Creates a cairo context, initialized with the default canvas settings.
 * 
 * Returns: a new cairo context. It should be freed with cairo_destroy().
 **/
cairo_t*
goo_canvas_create_cairo_context (GooCanvas *canvas)
{
  cairo_t *cr;

  g_return_val_if_fail (canvas->canvas_window != NULL, NULL);

  cr = gdk_cairo_create (canvas->canvas_window);

  /* We use CAIRO_ANTIALIAS_GRAY as the default antialiasing mode, as that is
     what is recommended when using unhinted text. */
  cairo_set_antialias (cr, CAIRO_ANTIALIAS_GRAY);

  /* Set the default line width based on the current units setting. */
  cairo_set_line_width (cr, goo_canvas_get_default_line_width (canvas));

  return cr;
}


static void
goo_canvas_get_property    (GObject            *object,
			    guint               prop_id,
			    GValue             *value,
			    GParamSpec         *pspec)
{
  GooCanvas *canvas = (GooCanvas*) object;

  switch (prop_id)
    {
    case PROP_SCALE:
      g_value_set_double (value, canvas->scale);
      break;
    case PROP_ANCHOR:
      g_value_set_enum (value, canvas->anchor);
      break;
    case PROP_X1:
      g_value_set_double (value, canvas->bounds.x1);
      break;
    case PROP_Y1:
      g_value_set_double (value, canvas->bounds.y1);
      break;
    case PROP_X2:
      g_value_set_double (value, canvas->bounds.x2);
      break;
    case PROP_Y2:
      g_value_set_double (value, canvas->bounds.y2);
      break;
    case PROP_UNITS:
      g_value_set_enum (value, canvas->units);
      break;
    case PROP_RESOLUTION_X:
      g_value_set_double (value, canvas->resolution_x);
      break;
    case PROP_RESOLUTION_Y:
      g_value_set_double (value, canvas->resolution_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
goo_canvas_set_property    (GObject            *object,
			    guint               prop_id,
			    const GValue       *value,
			    GParamSpec         *pspec)
{
  GooCanvas *canvas = (GooCanvas*) object;
  GdkColor color = { 0, 0, 0, 0, };
  gboolean need_reconfigure = FALSE;
  guint rgb;

  switch (prop_id)
    {
    case PROP_SCALE:
      goo_canvas_set_scale (canvas, g_value_get_double (value));
      break;
    case PROP_ANCHOR:
      canvas->anchor = g_value_get_enum (value);
      need_reconfigure = TRUE;
      break;
    case PROP_X1:
      canvas->bounds.x1 = g_value_get_double (value);
      need_reconfigure = TRUE;
      break;
    case PROP_Y1:
      canvas->bounds.y1 = g_value_get_double (value);
      need_reconfigure = TRUE;
      break;
    case PROP_X2:
      canvas->bounds.x2 = g_value_get_double (value);
      need_reconfigure = TRUE;
      break;
    case PROP_Y2:
      canvas->bounds.y2 = g_value_get_double (value);
      need_reconfigure = TRUE;
      break;
    case PROP_UNITS:
      canvas->units = g_value_get_enum (value);
      need_reconfigure = TRUE;
      break;
    case PROP_RESOLUTION_X:
      canvas->resolution_x = g_value_get_double (value);
      need_reconfigure = TRUE;
      break;
    case PROP_RESOLUTION_Y:
      canvas->resolution_y = g_value_get_double (value);
      need_reconfigure = TRUE;
      break;
    case PROP_BACKGROUND_COLOR:
      if (!g_value_get_string (value))
	gtk_widget_modify_base ((GtkWidget*) canvas, GTK_STATE_NORMAL, NULL);
      else if (gdk_color_parse (g_value_get_string (value), &color))
	gtk_widget_modify_base ((GtkWidget*) canvas, GTK_STATE_NORMAL, &color);
      else
	g_warning ("Unknown color: %s", g_value_get_string (value));
      break;
    case PROP_BACKGROUND_COLOR_RGB:
      rgb = g_value_get_uint (value);
      color.red   = ((rgb >> 16) & 0xFF) * 257;
      color.green = ((rgb >> 8)  & 0xFF) * 257;
      color.blue  = ((rgb)       & 0xFF) * 257;
      gtk_widget_modify_base ((GtkWidget*) canvas, GTK_STATE_NORMAL, &color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  if (need_reconfigure)
    reconfigure_canvas (canvas, TRUE);
}


/**
 * goo_canvas_get_root_item_model:
 * @canvas: a #GooCanvas.
 * 
 * Gets the root item model of the canvas.
 * 
 * Returns: the root item model, or %NULL if there is no root item model.
 **/
GooCanvasItemModel*
goo_canvas_get_root_item_model (GooCanvas	*canvas)
{
  g_return_val_if_fail (GOO_IS_CANVAS (canvas), NULL);

  return canvas->root_item_model;
}


/**
 * goo_canvas_set_root_item_model:
 * @canvas: a #GooCanvas.
 * @model: a #GooCanvasItemModel.
 * 
 * Sets the root item model of the canvas.
 *
 * A hierarchy of canvas items will be created, corresponding to the hierarchy
 * of items in the model. Any current canvas items will be removed.
 **/
void
goo_canvas_set_root_item_model (GooCanvas          *canvas,
				GooCanvasItemModel *model)
{
  g_return_if_fail (GOO_IS_CANVAS (canvas));
  g_return_if_fail (GOO_IS_CANVAS_ITEM_MODEL (model));

  if (canvas->root_item_model == model)
    return;

  if (canvas->root_item_model)
    {
      g_object_unref (canvas->root_item_model);
      canvas->root_item_model = NULL;
    }

  if (canvas->root_item)
    {
      g_object_unref (canvas->root_item);
      canvas->root_item = NULL;
    }

  if (model)
    {
      canvas->root_item_model = g_object_ref (model);

      /* Create a hierarchy of canvas items for all the items in the model. */
      canvas->root_item = goo_canvas_create_item (canvas, model);
    }
  else
    {
      /* The model has been reset so we go back to a default root group. */
      canvas->root_item = goo_canvas_group_new (NULL, NULL);
    }

  goo_canvas_item_set_canvas (canvas->root_item, canvas);
  canvas->need_update = TRUE;

  if (GTK_WIDGET_REALIZED (canvas))
    goo_canvas_update (canvas);

  gtk_widget_queue_draw (GTK_WIDGET (canvas));
}


/**
 * goo_canvas_get_root_item:
 * @canvas: a #GooCanvas.
 * 
 * Gets the root item of the canvas, usually a #GooCanvasGroup.
 * 
 * Returns: the root item, or %NULL if there is no root item.
 **/
GooCanvasItem*
goo_canvas_get_root_item (GooCanvas     *canvas)
{
  g_return_val_if_fail (GOO_IS_CANVAS (canvas), NULL);

  return canvas->root_item;
}


/**
 * goo_canvas_set_root_item:
 * @canvas: a #GooCanvas.
 * @item: the root canvas item.
 * 
 * Sets the root item of the canvas. Any existing canvas items are removed.
 **/
void
goo_canvas_set_root_item    (GooCanvas		*canvas,
			     GooCanvasItem      *item)
{
  g_return_if_fail (GOO_IS_CANVAS (canvas));
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));

  if (canvas->root_item == item)
    return;

  /* Remove any current model. */
  if (canvas->root_item_model)
    {
      g_object_unref (canvas->root_item_model);
      canvas->root_item_model = NULL;
    }

  if (canvas->root_item)
    g_object_unref (canvas->root_item);

  canvas->root_item = g_object_ref (item);
  goo_canvas_item_set_canvas (canvas->root_item, canvas);

  canvas->need_update = TRUE;

  if (GTK_WIDGET_REALIZED (canvas))
    goo_canvas_update (canvas);

  gtk_widget_queue_draw (GTK_WIDGET (canvas));
}


/**
 * goo_canvas_get_item:
 * @canvas: a #GooCanvas.
 * @model: a #GooCanvasItemModel.
 * 
 * Gets the canvas item associated with the given #GooCanvasItemModel.
 * This is only useful when goo_canvas_set_root_item_model() has been used to
 * set a model for the canvas.
 *
 * For simple applications you can use goo_canvas_get_item() to set up
 * signal handlers for your items, e.g.
 *
 * <informalexample><programlisting>
 *    item = goo_canvas_get_item (GOO_CANVAS (canvas), my_item);
 *    g_signal_connect (item, "button_press_event",
 *                      (GtkSignalFunc) on_my_item_button_press, NULL);
 * </programlisting></informalexample>
 *
 * More complex applications may want to use the #GooCanvas::item-created
 * signal to hook up their signal handlers.
 *
 * Returns: the canvas item corresponding to the given #GooCanvasItemModel,
 *  or %NULL if no canvas item has been created for it yet.
 **/
GooCanvasItem*
goo_canvas_get_item (GooCanvas          *canvas,
		     GooCanvasItemModel *model)
{
  GooCanvasItem *item;

  g_return_val_if_fail (GOO_IS_CANVAS (canvas), NULL);
  g_return_val_if_fail (GOO_IS_CANVAS_ITEM_MODEL (model), NULL);

  if (canvas->model_to_item)
    item = g_hash_table_lookup (canvas->model_to_item, model);

  /* If the item model has a canvas item check it is valid. */
  g_return_val_if_fail (!item || GOO_IS_CANVAS_ITEM (item), NULL);

  return item;
}


/**
 * goo_canvas_get_item_at:
 * @canvas: a #GooCanvas.
 * @x: the x coordinate of the point.
 * @y: the y coordinate of the point
 * @is_pointer_event: %TRUE if the "pointer-events" property of
 *  items should be used to determine which parts of the item are tested.
 * 
 * Gets the item at the given point.
 * 
 * Returns: the item found at the given point, or %NULL if no item was found.
 **/
GooCanvasItem*
goo_canvas_get_item_at (GooCanvas     *canvas,
			gdouble        x,
			gdouble        y,
			gboolean       is_pointer_event)
{
  cairo_t *cr;
  GooCanvasItem *result = NULL;
  GList *list;

  g_return_val_if_fail (GOO_IS_CANVAS (canvas), NULL);

  /* If no root item is set, just return NULL. */
  if (!canvas->root_item)
    return NULL;

  cr = goo_canvas_create_cairo_context (canvas);
  list = goo_canvas_item_get_items_at (canvas->root_item, x, y, cr,
				       is_pointer_event, TRUE, NULL);
  cairo_destroy (cr);

  /* We just return the top item in the list. */
  if (list)
    result = list->data;

  g_list_free (list);

  return result;
}


/**
 * goo_canvas_get_items_at:
 * @canvas: a #GooCanvas.
 * @x: the x coordinate of the point.
 * @y: the y coordinate of the point
 * @is_pointer_event: %TRUE if the "pointer-events" property of
 *  items should be used to determine which parts of the item are tested.
 * 
 * Gets all items at the given point.
 * 
 * Returns: a list of items found at the given point, with the top item at
 *  the start of the list, or %NULL if no items were found. The list must be
 *  freed with g_list_free().
 **/
GList*
goo_canvas_get_items_at (GooCanvas     *canvas,
			 gdouble        x,
			 gdouble        y,
			 gboolean       is_pointer_event)
{
  cairo_t *cr;
  GList *result;

  g_return_val_if_fail (GOO_IS_CANVAS (canvas), NULL);

  /* If no root item is set, just return NULL. */
  if (!canvas->root_item)
    return NULL;

  cr = goo_canvas_create_cairo_context (canvas);
  result = goo_canvas_item_get_items_at (canvas->root_item, x, y, cr,
					 is_pointer_event, TRUE, NULL);
  cairo_destroy (cr);

  return result;
}


static GList*
goo_canvas_get_items_in_area_recurse (GooCanvas		    *canvas,
				      GooCanvasItem         *item,
				      const GooCanvasBounds *area,
				      gboolean		     inside_area,
				      gboolean               allow_overlaps,
				      gboolean               include_containers,
				      GList                 *found_items)
{
  GooCanvasBounds bounds;
  gboolean completely_inside = FALSE, completely_outside = FALSE;
  gboolean is_container, add_item = FALSE;
  gint n_children, i;

  /* First check the item/container itself. */
  goo_canvas_item_get_bounds (item, &bounds);

  is_container = goo_canvas_item_is_container (item);

  if (bounds.x1 >= area->x1 && bounds.x2 <= area->x2
      && bounds.y1 >= area->y1 && bounds.y2 <= area->y2)
    completely_inside = TRUE;

  if (bounds.x1 > area->x2 || bounds.x2 < area->x1
      || bounds.y1 > area->y2 || bounds.y2 < area->y1)
    completely_outside = TRUE;

  if (inside_area)
    {
      if (completely_inside
	  || (allow_overlaps && !completely_outside))
	add_item = TRUE;
    }
  else
    {
      if (completely_outside
	  || (allow_overlaps && !completely_inside))
	add_item = TRUE;
    }

  if (add_item && (!is_container || include_containers))
    found_items = g_list_prepend (found_items, item);

  /* Now check any children, if appropriate. */
  if ((inside_area && !completely_outside)
      || (!inside_area && !completely_inside))
    {
      n_children = goo_canvas_item_get_n_children (item);
      for (i = 0; i < n_children; i++)
	{
	  GooCanvasItem *child = goo_canvas_item_get_child (item, i);
	  found_items = goo_canvas_get_items_in_area_recurse (canvas, child,
							      area,
							      inside_area,
							      allow_overlaps,
							      include_containers,
							      found_items);
	}
    }

  return found_items;
}


/**
 * goo_canvas_get_items_in_area:
 * @canvas: a #GooCanvas.
 * @area: the area to compare with each item's bounds.
 * @inside_area: %TRUE if items inside @area should be returned, or %FALSE if
 *  items outside @area should be returned.
 * @allow_overlaps: %TRUE if items which are partly inside and partly outside
 *  should be returned.
 * @include_containers: %TRUE if containers should be checked as well as
 *  normal items.
 * 
 * Gets a list of items inside or outside a given area.
 * 
 * Returns: a list of items in the given area, or %NULL if no items are found.
 *  The list should be freed with g_list_free().
 **/
GList*
goo_canvas_get_items_in_area (GooCanvas		    *canvas,
			      const GooCanvasBounds *area,
			      gboolean		     inside_area,
			      gboolean               allow_overlaps,
			      gboolean               include_containers)
{
  g_return_val_if_fail (GOO_IS_CANVAS (canvas), NULL);

  /* If no root item is set, just return NULL. */
  if (!canvas->root_item)
    return NULL;

  return goo_canvas_get_items_in_area_recurse (canvas, canvas->root_item,
					       area, inside_area,
					       allow_overlaps,
					       include_containers, NULL);
}


static void
goo_canvas_realize (GtkWidget *widget)
{
  GooCanvas *canvas;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint width_pixels, height_pixels;
  GList *tmp_list;

  g_return_if_fail (GOO_IS_CANVAS (widget));

  canvas = GOO_CANVAS (widget);
  GTK_WIDGET_SET_FLAGS (canvas, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  /* We want to round the sizes up to the next pixel. */
  width_pixels = ((canvas->bounds.x2 - canvas->bounds.x1) * canvas->scale_x) + 1;
  height_pixels = ((canvas->bounds.y2 - canvas->bounds.y1) * canvas->scale_y) + 1;

  attributes.x = canvas->hadjustment ? - canvas->hadjustment->value : 0,
  attributes.y = canvas->vadjustment ? - canvas->vadjustment->value : 0;
  attributes.width = MAX (width_pixels, widget->allocation.width);
  attributes.height = MAX (height_pixels, widget->allocation.height);
  attributes.event_mask = GDK_EXPOSURE_MASK
			 | GDK_SCROLL_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK
			 | GDK_KEY_PRESS_MASK
			 | GDK_KEY_RELEASE_MASK
			 | GDK_ENTER_NOTIFY_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_FOCUS_CHANGE_MASK
                         | gtk_widget_get_events (widget);

  canvas->canvas_window = gdk_window_new (widget->window,
					  &attributes, attributes_mask);
  gdk_window_set_user_data (canvas->canvas_window, widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.event_mask = 0;

  canvas->tmp_window = gdk_window_new (gtk_widget_get_parent_window (widget),
				       &attributes, attributes_mask);
  gdk_window_set_user_data (canvas->tmp_window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_background (widget->window,
			     &widget->style->base[widget->state]);
  gdk_window_set_background (canvas->canvas_window,
			     &widget->style->base[widget->state]);
  gdk_window_set_back_pixmap (canvas->tmp_window, NULL, FALSE);

  tmp_list = canvas->widget_items;
  while (tmp_list)
    {
      GooCanvasWidget *witem = tmp_list->data;
      tmp_list = tmp_list->next;

      if (witem->widget)
	gtk_widget_set_parent_window (witem->widget, canvas->canvas_window);
    }

  goo_canvas_update (GOO_CANVAS (widget));
}


static void 
goo_canvas_unrealize (GtkWidget *widget)
{
  GooCanvas *canvas;

  g_return_if_fail (GOO_IS_CANVAS (widget));

  canvas = GOO_CANVAS (widget);

  gdk_window_set_user_data (canvas->canvas_window, NULL);
  gdk_window_destroy (canvas->canvas_window);
  canvas->canvas_window = NULL;

  gdk_window_set_user_data (canvas->tmp_window, NULL);
  gdk_window_destroy (canvas->tmp_window);
  canvas->tmp_window = NULL;

  if (GTK_WIDGET_CLASS (goo_canvas_parent_class)->unrealize)
    GTK_WIDGET_CLASS (goo_canvas_parent_class)->unrealize (widget);
}


static void 
goo_canvas_map (GtkWidget *widget)
{
  GooCanvas *canvas;
  GList *tmp_list;

  g_return_if_fail (GOO_IS_CANVAS (widget));

  canvas = GOO_CANVAS (widget);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

  tmp_list = canvas->widget_items;
  while (tmp_list)
    {
      GooCanvasWidget *witem = tmp_list->data;
      tmp_list = tmp_list->next;

      if (witem->widget && GTK_WIDGET_VISIBLE (witem->widget))
	{
	  if (!GTK_WIDGET_MAPPED (witem->widget))
	    gtk_widget_map (witem->widget);
	}
    }

  gdk_window_show (canvas->canvas_window);
  gdk_window_show (widget->window);
}


static void
goo_canvas_style_set (GtkWidget *widget,
		      GtkStyle  *old_style)
{
  if (GTK_WIDGET_CLASS (goo_canvas_parent_class)->style_set)
    GTK_WIDGET_CLASS (goo_canvas_parent_class)->style_set (widget, old_style);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_set_background (widget->window,
				 &widget->style->base[widget->state]);
      gdk_window_set_background (GOO_CANVAS (widget)->canvas_window,
				 &widget->style->base[widget->state]);
    }
}


static void
goo_canvas_configure_hadjustment (GooCanvas *canvas,
				  gint       window_width)
{
  GtkWidget *widget = GTK_WIDGET (canvas);
  GtkAdjustment *adj = canvas->hadjustment;
  gboolean changed = FALSE;
  gboolean value_changed = FALSE;
  gdouble max_value;

  if (adj->upper != window_width)
    {
      adj->upper = window_width;
      changed = TRUE;
    }

  if (adj->page_size != widget->allocation.width)
    {
      adj->page_size = widget->allocation.width;
      adj->page_increment = adj->page_size * 0.9;
      adj->step_increment = adj->page_size * 0.1;
      changed = TRUE;
    }
      
  max_value = MAX (0.0, adj->upper - adj->page_size);
  if (adj->value > max_value)
    {
      adj->value = max_value;
      value_changed = TRUE;
    }
  
  if (changed)
    gtk_adjustment_changed (adj);

  if (value_changed)
    gtk_adjustment_value_changed (adj);
}


static void
goo_canvas_configure_vadjustment (GooCanvas *canvas,
				  gint       window_height)
{
  GtkWidget *widget = GTK_WIDGET (canvas);
  GtkAdjustment *adj = canvas->vadjustment;
  gboolean changed = FALSE;
  gboolean value_changed = FALSE;
  gdouble max_value;

  if (adj->upper != window_height)
    {
      adj->upper = window_height;
      changed = TRUE;
    }

  if (adj->page_size != widget->allocation.height)
    {
      adj->page_size = widget->allocation.height;
      adj->page_increment = adj->page_size * 0.9;
      adj->step_increment = adj->page_size * 0.1;
      changed = TRUE;
    }
      
  max_value = MAX (0.0, adj->upper - adj->page_size);
  if (adj->value > max_value)
    {
      adj->value = max_value;
      value_changed = TRUE;
    }
  
  if (changed)
    gtk_adjustment_changed (adj);

  if (value_changed)
    gtk_adjustment_value_changed (adj);
}


static void
recalculate_scales (GooCanvas *canvas)
{
  switch (canvas->units)
    {
    case GTK_UNIT_PIXEL:
      canvas->scale_x = canvas->scale;
      canvas->scale_y = canvas->scale;
      break;
    case GTK_UNIT_POINTS:
      canvas->scale_x = canvas->scale * (canvas->resolution_x / 72.0);
      canvas->scale_y = canvas->scale * (canvas->resolution_y / 72.0);
      break;
    case GTK_UNIT_INCH:
      canvas->scale_x = canvas->scale * canvas->resolution_x;
      canvas->scale_y = canvas->scale * canvas->resolution_y;
      break;
    case GTK_UNIT_MM:
      /* There are 25.4 mm to an inch. */
      canvas->scale_x = canvas->scale * (canvas->resolution_x / 25.4);
      canvas->scale_y = canvas->scale * (canvas->resolution_y / 25.4);
      break;
    }
}


/* This makes sure the canvas is all set up correctly, i.e. the scrollbar
   adjustments are set, the canvas x & y offsets are calculated, and the
   canvas window is sized. */
static void
reconfigure_canvas (GooCanvas *canvas,
		    gboolean   redraw_if_needed)
{
  gint width_pixels, height_pixels;
  gint window_x = 0, window_y = 0, window_width, window_height;
  gint new_x_offset = 0, new_y_offset = 0;
  GtkWidget *widget;

  widget = GTK_WIDGET (canvas);

  /* Make sure the bounds are sane. */
  if (canvas->bounds.x2 < canvas->bounds.x1)
    canvas->bounds.x2 = canvas->bounds.x1;
  if (canvas->bounds.y2 < canvas->bounds.y1)
    canvas->bounds.y2 = canvas->bounds.y1;

  /* Recalculate scale_x & scale_y. */
  recalculate_scales (canvas);

  /* This is the natural size of the canvas window in pixels, rounded up to
     the next pixel. */
  width_pixels = ((canvas->bounds.x2 - canvas->bounds.x1) * canvas->scale_x) + 1;
  height_pixels = ((canvas->bounds.y2 - canvas->bounds.y1) * canvas->scale_y) + 1;

  /* The actual window size is always at least as big as the widget's window.*/
  window_width = MAX (width_pixels, widget->allocation.width);
  window_height = MAX (height_pixels, widget->allocation.height);

  /* If the width or height is smaller than the window, we need to calculate
     the canvas x & y offsets according to the anchor. */
  if (width_pixels < widget->allocation.width)
    {
      switch (canvas->anchor)
	{
	case GTK_ANCHOR_NORTH_WEST:
	case GTK_ANCHOR_WEST:
	case GTK_ANCHOR_SOUTH_WEST:
	  new_x_offset = 0;
	  break;
	case GTK_ANCHOR_NORTH:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_SOUTH:
	  new_x_offset = (widget->allocation.width - width_pixels) / 2;
	  break;
	case GTK_ANCHOR_NORTH_EAST:
	case GTK_ANCHOR_EAST:
	case GTK_ANCHOR_SOUTH_EAST:
	  new_x_offset = widget->allocation.width - width_pixels;
	  break;
	}
    }

  if (height_pixels < widget->allocation.height)
    {
      switch (canvas->anchor)
	{
	case GTK_ANCHOR_NORTH_WEST:
	case GTK_ANCHOR_NORTH:
	case GTK_ANCHOR_NORTH_EAST:
	  new_y_offset = 0;
	  break;
	case GTK_ANCHOR_WEST:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_EAST:
	  new_y_offset = (widget->allocation.height - height_pixels) / 2;
	  break;
	case GTK_ANCHOR_SOUTH_WEST:
	case GTK_ANCHOR_SOUTH:
	case GTK_ANCHOR_SOUTH_EAST:
	  new_y_offset = widget->allocation.height - height_pixels;
	  break;
	}
    }

  canvas->freeze_count++;

  if (canvas->hadjustment)
    {
      goo_canvas_configure_hadjustment (canvas, window_width);
      window_x = - canvas->hadjustment->value;
    }

  if (canvas->vadjustment)
    {
      goo_canvas_configure_vadjustment (canvas, window_height);
      window_y = - canvas->vadjustment->value;
    }

  canvas->freeze_count--;

  if (GTK_WIDGET_REALIZED (canvas))
    {
      gdk_window_move_resize (canvas->canvas_window, window_x, window_y,
			      window_width, window_height);
    }

  /* If one of the offsets has changed we have to redraw the widget. */
  if (canvas->canvas_x_offset != new_x_offset
      || canvas->canvas_y_offset != new_y_offset)
    {
      canvas->canvas_x_offset = new_x_offset;
      canvas->canvas_y_offset = new_y_offset;

      if (redraw_if_needed)
	gtk_widget_queue_draw (GTK_WIDGET (canvas));
    }
}


static void     
goo_canvas_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  GList *tmp_list;
  GooCanvas *canvas;

  g_return_if_fail (GOO_IS_CANVAS (widget));

  canvas = GOO_CANVAS (widget);

  requisition->width = 0;
  requisition->height = 0;

  tmp_list = canvas->widget_items;

  while (tmp_list)
    {
      GooCanvasWidget *witem = tmp_list->data;
      GtkRequisition child_requisition;
      
      tmp_list = tmp_list->next;

      if (witem->widget)
	gtk_widget_size_request (witem->widget, &child_requisition);
    }
}


static void
goo_canvas_allocate_child_widget (GooCanvas       *canvas,
				  GooCanvasWidget *witem)
{
  GooCanvasBounds bounds;
  GtkAllocation allocation;

  goo_canvas_item_get_bounds ((GooCanvasItem*) witem, &bounds);

  goo_canvas_convert_to_pixels (canvas, &bounds.x1, &bounds.y1);
  goo_canvas_convert_to_pixels (canvas, &bounds.x2, &bounds.y2);

  /* Note that we only really support integers for the bounds, and we don't
     support scaling of a canvas with widget items in it. */
  allocation.x = bounds.x1;
  allocation.y = bounds.y1;
  allocation.width = bounds.x2 - allocation.x;
  allocation.height = bounds.y2 - allocation.y;
  
  gtk_widget_size_allocate (witem->widget, &allocation);
}


static void     
goo_canvas_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GooCanvas *canvas;
  GList *tmp_list;

  g_return_if_fail (GOO_IS_CANVAS (widget));

  canvas = GOO_CANVAS (widget);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    {
      /* We can only allocate our children when we are realized, since we
	 need a window to create a cairo_t which we use for layout. */
      tmp_list = canvas->widget_items;
      while (tmp_list)
	{
	  GooCanvasWidget *witem = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (witem->widget)
	    goo_canvas_allocate_child_widget (canvas, witem);
	}

      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      gdk_window_move_resize (canvas->tmp_window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
    }

  reconfigure_canvas (canvas, TRUE);
}


static void
goo_canvas_adjustment_value_changed (GtkAdjustment *adjustment,
				     GooCanvas     *canvas)
{
  AtkObject *accessible;

  if (!canvas->freeze_count && GTK_WIDGET_REALIZED (canvas))
    {
      gdk_window_move (canvas->canvas_window,
		       - canvas->hadjustment->value,
		       - canvas->vadjustment->value);
      
      /* If this is callback from a signal for one of the scrollbars, process
	 updates here for smoother scrolling. */
      if (adjustment)
	gdk_window_process_updates (canvas->canvas_window, TRUE);

      /* Notify any accessibility modules that the view has changed. */
      accessible = gtk_widget_get_accessible (GTK_WIDGET (canvas));
      g_signal_emit_by_name (accessible, "visible_data_changed");
    }
}


/* Sets either or both adjustments, If hadj or vadj is NULL a new adjustment
   is created. */
static void           
goo_canvas_set_adjustments (GooCanvas     *canvas,
			    GtkAdjustment *hadj,
			    GtkAdjustment *vadj)
{
  gboolean need_reconfigure = FALSE;

  g_return_if_fail (GOO_IS_CANVAS (canvas));

  if (hadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
  else if (canvas->hadjustment)
    hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (vadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
  else if (canvas->vadjustment)
    vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  
  if (canvas->hadjustment && (canvas->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_by_func (canvas->hadjustment,
					    goo_canvas_adjustment_value_changed,
					    canvas);
      g_object_unref (canvas->hadjustment);
    }
  
  if (canvas->vadjustment && (canvas->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_by_func (canvas->vadjustment,
					    goo_canvas_adjustment_value_changed,
					    canvas);
      g_object_unref (canvas->vadjustment);
    }
  
  if (canvas->hadjustment != hadj)
    {
      canvas->hadjustment = hadj;
      g_object_ref_sink (canvas->hadjustment);

      g_signal_connect (canvas->hadjustment, "value_changed",
			G_CALLBACK (goo_canvas_adjustment_value_changed),
			canvas);
      need_reconfigure = TRUE;
    }
  
  if (canvas->vadjustment != vadj)
    {
      canvas->vadjustment = vadj;
      g_object_ref_sink (canvas->vadjustment);

      g_signal_connect (canvas->vadjustment, "value_changed",
			G_CALLBACK (goo_canvas_adjustment_value_changed),
			canvas);
      need_reconfigure = TRUE;
    }

  if (need_reconfigure)
    reconfigure_canvas (canvas, TRUE);
}


/* Sets one of our pointers to an item, adding a reference to it and
   releasing any reference to the current item. */
static void
set_item_pointer (GooCanvasItem **item,
		  GooCanvasItem  *new)
{
  /* If the item hasn't changed, just return. */
  if (*item == new)
    return;

  /* Unref the current item, if it isn't NULL. */
  if (*item)
    g_object_unref (*item);

  /* Set the new item. */
  *item = new;

  /* Add a reference to it, if it isn't NULL. */
  if (*item)
    g_object_ref (*item);
}


/**
 * goo_canvas_get_bounds:
 * @canvas: a #GooCanvas.
 * @left: a pointer to a #gdouble to return the left edge, or %NULL.
 * @top: a pointer to a #gdouble to return the top edge, or %NULL.
 * @right: a pointer to a #gdouble to return the right edge, or %NULL.
 * @bottom: a pointer to a #gdouble to return the bottom edge, or %NULL.
 * 
 * Gets the bounds of the canvas, in canvas units.
 *
 * By default, canvas units are pixels, though the #GooCanvas:units property
 * can be used to change the units to points, inches or millimeters.
 **/
void
goo_canvas_get_bounds	(GooCanvas *canvas,
			 gdouble   *left,
			 gdouble   *top,
			 gdouble   *right,
			 gdouble   *bottom)
{
  g_return_if_fail (GOO_IS_CANVAS (canvas));

  if (left)
    *left = canvas->bounds.x1;
  if (top)
    *top = canvas->bounds.y1;
  if (right)
    *right = canvas->bounds.x2;
  if (bottom)
    *bottom = canvas->bounds.y2;
}


/**
 * goo_canvas_set_bounds:
 * @canvas: a #GooCanvas.
 * @left: the left edge.
 * @top: the top edge.
 * @right: the right edge.
 * @bottom: the bottom edge.
 * 
 * Sets the bounds of the #GooCanvas, in canvas units.
 *
 * By default, canvas units are pixels, though the #GooCanvas:units property
 * can be used to change the units to points, inches or millimeters.
 **/
void
goo_canvas_set_bounds	(GooCanvas *canvas,
			 gdouble    left,
			 gdouble    top,
			 gdouble    right,
			 gdouble    bottom)
{
  g_return_if_fail (GOO_IS_CANVAS (canvas));

  canvas->bounds.x1 = left;
  canvas->bounds.y1 = top;
  canvas->bounds.x2 = right;
  canvas->bounds.y2 = bottom;

  reconfigure_canvas (canvas, TRUE);
}


/**
 * goo_canvas_scroll_to:
 * @canvas: a #GooCanvas.
 * @left: the x coordinate to scroll to.
 * @top: the y coordinate to scroll to.
 * 
 * Scrolls the canvas, placing the given point as close to the top-left of
 * the view as possible.
 **/
void
goo_canvas_scroll_to	     (GooCanvas     *canvas,
			      gdouble        left,
			      gdouble        top)
{
  gdouble x = left, y = top;

  g_return_if_fail (GOO_IS_CANVAS (canvas));

  /* The scrollbar adjustments use pixel values, so convert to pixels. */
  goo_canvas_convert_to_pixels (canvas, &x, &y);

  /* Make sure we stay within the bounds. */
  x = CLAMP (x, canvas->hadjustment->lower,
	     canvas->hadjustment->upper - canvas->hadjustment->page_size);
  y = CLAMP (y, canvas->vadjustment->lower,
	     canvas->vadjustment->upper - canvas->vadjustment->page_size);

  canvas->freeze_count++;

  gtk_adjustment_set_value (canvas->hadjustment, x);
  gtk_adjustment_set_value (canvas->vadjustment, y);

  canvas->freeze_count--;
  goo_canvas_adjustment_value_changed (NULL, canvas);
}


/* This makes sure the given item is displayed, scrolling if necessary. */
static void
goo_canvas_scroll_to_item (GooCanvas     *canvas,
			   GooCanvasItem *item)
{
  GooCanvasBounds bounds;
  gdouble hvalue, vvalue;

  goo_canvas_item_get_bounds (item, &bounds);

  goo_canvas_convert_to_pixels (canvas, &bounds.x1, &bounds.y1);
  goo_canvas_convert_to_pixels (canvas, &bounds.x2, &bounds.y2);

  canvas->freeze_count++;

  /* Remember the current adjustment values. */
  hvalue = canvas->hadjustment->value;
  vvalue = canvas->vadjustment->value;

  /* Update the adjustments so the item is displayed. */
  gtk_adjustment_clamp_page (canvas->hadjustment, bounds.x1, bounds.x2);
  gtk_adjustment_clamp_page (canvas->vadjustment, bounds.y1, bounds.y2);

  canvas->freeze_count--;

  /* If the adjustments have changed we need to scroll. */
  if (hvalue != canvas->hadjustment->value
      || vvalue != canvas->vadjustment->value)
    goo_canvas_adjustment_value_changed (NULL, canvas);
}


/**
 * goo_canvas_get_scale:
 * @canvas: a #GooCanvas.
 * 
 * Gets the current scale of the canvas.
 *
 * The scale specifies the magnification factor of the canvas, e.g. if an item
 * has a width of 2 pixels and the scale is set to 3, it will be displayed with
 * a width of 2 x 3 = 6 pixels.
 *
 * Returns: the current scale setting.
 **/
gdouble
goo_canvas_get_scale	(GooCanvas *canvas)
{
  g_return_val_if_fail (GOO_IS_CANVAS (canvas), 1.0);

  return canvas->scale;
}


/**
 * goo_canvas_set_scale:
 * @canvas: a #GooCanvas.
 * @scale: the new scale setting.
 * 
 * Sets the scale of the canvas.
 *
 * The scale specifies the magnification factor of the canvas, e.g. if an item
 * has a width of 2 pixels and the scale is set to 3, it will be displayed with
 * a width of 2 x 3 = 6 pixels.
 **/
void
goo_canvas_set_scale	(GooCanvas *canvas,
			 gdouble    scale)
{
  gdouble x, y;

  g_return_if_fail (GOO_IS_CANVAS (canvas));

  /* Calculate the coords of the current center point in pixels. */
  x = canvas->hadjustment->value + canvas->hadjustment->page_size / 2;
  y = canvas->vadjustment->value + canvas->vadjustment->page_size / 2;

  /* Convert from pixel units to device units. */
  goo_canvas_convert_from_pixels (canvas, &x, &y);

  /* Show our temporary window above the canvas window, so that the windowing
     system doesn't try to scroll the contents when we change the adjustments.
     Since we are changing the scale we need to redraw everything so the
     scrolling is unnecessary and really ugly.
     FIXME: There is a possible issue with keyboard focus/input methods here,
     since hidden windows can't have the keyboard focus. */
  if (GTK_WIDGET_MAPPED (canvas))
    gdk_window_show (canvas->tmp_window);

  canvas->freeze_count++;

  canvas->scale = scale;
  reconfigure_canvas (canvas, FALSE);

  /* Convert from the center point to the new desired top-left posision. */
  x -= canvas->hadjustment->page_size / canvas->scale_x / 2;
  y -= canvas->vadjustment->page_size / canvas->scale_y / 2;

  /* Now try to scroll to it. */
  goo_canvas_scroll_to (canvas, x, y);

  canvas->freeze_count--;
  goo_canvas_adjustment_value_changed (NULL, canvas);

  /* Now hide the temporary window, so the canvas window will get an expose
     event. */
  if (GTK_WIDGET_MAPPED (canvas))
    gdk_window_hide (canvas->tmp_window);
}


/**
 * goo_canvas_unregister_item:
 * @canvas: a #GooCanvas.
 * @model: the item model whose canvas item is being finalized.
 * 
 * This function is only intended to be used when implementing new canvas
 * items.
 *
 * It should be called in the finalize method of #GooCanvasItem
 * objects, to remove the canvas item from the #GooCanvas's hash table.
 **/
void
goo_canvas_unregister_item (GooCanvas          *canvas,
			    GooCanvasItemModel *model)
{
  if (canvas->model_to_item)
    g_hash_table_remove (canvas->model_to_item, model);
}


/**
 * goo_canvas_create_item:
 * @canvas: a #GooCanvas.
 * @model: the item model to create a canvas item for.
 * 
 * This function is only intended to be used when implementing new canvas
 * items, typically container items such as #GooCanvasGroup.
 *
 * It creates a new canvas item for the given item model, and recursively
 * creates items for any children.
 *
 * It uses the create_item() virtual method if it has been set.
 * Subclasses of #GooCanvas can define this method if they want to use
 * custom views for items.
 *
 * It emits the #GooCanvas::item-created signal after creating the view, so
 * application code can connect signal handlers to the new view if desired.
 * 
 * Returns: a new canvas item.
 **/
GooCanvasItem*
goo_canvas_create_item  (GooCanvas          *canvas,
			 GooCanvasItemModel *model)
{
  GooCanvasItem *item = NULL;

  /* Use the virtual method if it has been set. */
  if (GOO_CANVAS_GET_CLASS (canvas)->create_item)
    item = GOO_CANVAS_GET_CLASS (canvas)->create_item (canvas, model);

  /* The virtual method can return NULL to use the default view for an item. */
  if (!item)
    item = GOO_CANVAS_ITEM_MODEL_GET_IFACE (model)->create_item (model,
								 canvas);

  if (canvas->model_to_item)
    g_hash_table_insert (canvas->model_to_item, model, item);

  /* Emit a signal so apps can hook up signal handlers if they want. */
  g_signal_emit (canvas, canvas_signals[ITEM_CREATED], 0, item, model);

  return item;
}


static void
goo_canvas_update_internal (GooCanvas *canvas,
			    cairo_t   *cr)
{
  GooCanvasBounds bounds;

  /* It is possible that processing the first set of updates causes other
     updates to be scheduled, so we loop round until all are done. Items
     should ensure that they don't cause this to loop forever. */
  while (canvas->need_update)
    {
      canvas->need_update = FALSE;
      if (canvas->root_item)
	goo_canvas_item_update (canvas->root_item, FALSE, cr, &bounds);
    }

  /* Check which item is under the pointer. */
  update_pointer_item (canvas, NULL);
}


/**
 * goo_canvas_update:
 * @canvas: a #GooCanvas.
 * 
 * This function is only intended to be used by subclasses of #GooCanvas or
 * #GooCanvasItem implementations.
 *
 * It updates any items that need updating.
 *
 * If the bounds of items change, they will request a redraw of the old and
 * new bounds so the display is updated correctly.
 **/
void
goo_canvas_update (GooCanvas *canvas)
{
  cairo_t *cr = goo_canvas_create_cairo_context (canvas);
  goo_canvas_update_internal (canvas, cr);
  cairo_destroy (cr);
}


static gint
goo_canvas_idle_handler (GooCanvas *canvas)
{
  GDK_THREADS_ENTER ();

  goo_canvas_update (canvas);

  /* Reset idle id. Note that we do this after goo_canvas_update(), to
     make sure we don't schedule another idle handler while that is running. */
  canvas->idle_id = 0;

  GDK_THREADS_LEAVE ();

  /* Return FALSE to remove the idle handler. */
  return FALSE;
}


/**
 * goo_canvas_request_update:
 * @canvas: a #GooCanvas.
 * 
 * This function is only intended to be used by subclasses of #GooCanvas or
 * #GooCanvasItem implementations.
 *
 * It schedules an update of the #GooCanvas. This will be performed in
 * the idle loop, after all pending events have been handled, but before
 * the canvas has been repainted.
 **/
void
goo_canvas_request_update (GooCanvas   *canvas)
{
  canvas->need_update = TRUE;

  /* We have to wait until we are realized. We'll do a full update then. */
  if (!GTK_WIDGET_REALIZED (canvas))
    return;

  /* We use a higher priority than the normal GTK+ resize/redraw idle handlers
   * so the canvas state will be updated before allocating sizes & redrawing.
   */
  if (!canvas->idle_id)
    canvas->idle_id = g_idle_add_full (GTK_PRIORITY_RESIZE - 5, (GSourceFunc) goo_canvas_idle_handler, canvas, NULL);
}


/**
 * goo_canvas_request_redraw:
 * @canvas: a #GooCanvas.
 * @bounds: the bounds to redraw.
 * 
 * This function is only intended to be used by subclasses of #GooCanvas or
 * #GooCanvasItem implementations.
 *
 * Requests that the given bounds be redrawn.
 **/
void
goo_canvas_request_redraw (GooCanvas             *canvas,
			   const GooCanvasBounds *bounds)
{
  GdkRectangle rect;

  if (!GTK_WIDGET_DRAWABLE (canvas) || (bounds->x1 == bounds->x2))
    return;

  /* We subtract one from the left & top edges, in case anti-aliasing makes
     the drawing use an extra pixel. */
  rect.x = (double) (bounds->x1 - canvas->bounds.x1) * canvas->scale_x - 1;
  rect.y = (double) (bounds->y1 - canvas->bounds.y1) * canvas->scale_y - 1;

  /* We add an extra one here for the same reason. (The other extra one is to
     round up to the next pixel.) And one for luck! */
  rect.width = (double) (bounds->x2 - canvas->bounds.x1) * canvas->scale_x
    - rect.x + 2 + 1;
  rect.height = (double) (bounds->y2 - canvas->bounds.y1) * canvas->scale_y
    - rect.y + 2 + 1;

  rect.x += canvas->canvas_x_offset;
  rect.y += canvas->canvas_y_offset;

  gdk_window_invalidate_rect (canvas->canvas_window, &rect, FALSE);
}


static gboolean
goo_canvas_expose_event (GtkWidget      *widget,
			 GdkEventExpose *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);
  GooCanvasBounds bounds;
  cairo_t *cr;

  if (!canvas->root_item)
    return FALSE;

  if (event->window != canvas->canvas_window)
    return FALSE;

  cr = goo_canvas_create_cairo_context (canvas);

  if (canvas->need_update)
    goo_canvas_update_internal (canvas, cr);

  bounds.x1 = ((event->area.x - canvas->canvas_x_offset) / canvas->scale_x)
    + canvas->bounds.x1;
  bounds.y1 = ((event->area.y - canvas->canvas_y_offset) / canvas->scale_y)
    + canvas->bounds.y1;
  bounds.x2 = (event->area.width / canvas->scale_x) + bounds.x1;
  bounds.y2 = (event->area.height / canvas->scale_y) + bounds.y1;

  /* Translate it to use the canvas pixel offsets (used when the canvas is
     smaller than the window and the anchor isn't set to NORTH_WEST). */
  cairo_translate (cr, canvas->canvas_x_offset, canvas->canvas_y_offset);

  /* Scale it so we can use canvas coordinates. */
  cairo_scale (cr, canvas->scale_x, canvas->scale_y);

  /* Translate it so the top-left of the canvas becomes (0,0). */
  cairo_translate (cr, -canvas->bounds.x1, -canvas->bounds.y1);

  goo_canvas_item_paint (canvas->root_item, cr, &bounds, canvas->scale);

  cairo_destroy (cr);

  GTK_WIDGET_CLASS (goo_canvas_parent_class)->expose_event (widget, event);

  return FALSE;
}


/**
 * goo_canvas_render:
 * @canvas: a #GooCanvas.
 * @cr: a cairo context.
 * @bounds: the area to render, or %NULL to render the entire canvas.
 * @scale: the scale to compare with each item's visibility
 * threshold to see if they should be rendered. This only affects items that
 * have their visibility set to %GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD.
 * 
 * Renders all or part of a canvas to the given cairo context.
 **/
void
goo_canvas_render (GooCanvas             *canvas,
		   cairo_t               *cr,
		   const GooCanvasBounds *bounds,
		   gdouble                scale)
{
  /* Set the default line width based on the current units setting. */
  cairo_set_line_width (cr, goo_canvas_get_default_line_width (canvas));

  if (bounds)
    {
      /* Clip to the given bounds. */
      cairo_new_path (cr);
      cairo_move_to (cr, bounds->x1, bounds->y1);
      cairo_line_to (cr, bounds->x2, bounds->y1);
      cairo_line_to (cr, bounds->x2, bounds->y2);
      cairo_line_to (cr, bounds->x1, bounds->y2);
      cairo_close_path (cr);
      cairo_clip (cr);

      goo_canvas_item_paint (canvas->root_item, cr, bounds, scale);
    }
  else
    {
      goo_canvas_item_paint (canvas->root_item, cr, &canvas->bounds, scale);
    }
}


/*
 * Keyboard/Mouse Event & Grab Handling.
 */

/* Initializes a synthesized crossing event from a given button press/release,
   motion, or crossing event. */
static void
initialize_crossing_event (GooCanvas *canvas,
			   GdkEvent  *event)
{
  GdkEventCrossing *crossing_event = &canvas->crossing_event;

  /* Initialize the crossing event. */
  crossing_event->type       = event->any.type;
  crossing_event->window     = event->any.window;
  crossing_event->send_event = event->any.send_event;
  crossing_event->subwindow  = NULL;
  crossing_event->detail     = GDK_NOTIFY_ANCESTOR;
  crossing_event->focus      = FALSE;
  crossing_event->mode       = GDK_CROSSING_NORMAL;

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      crossing_event->time   = event->motion.time;
      crossing_event->x      = event->motion.x;
      crossing_event->y      = event->motion.y;
      crossing_event->x_root = event->motion.x_root;
      crossing_event->y_root = event->motion.y_root;
      crossing_event->state  = event->motion.state;
      break;

    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      crossing_event->time   = event->crossing.time;
      crossing_event->x      = event->crossing.x;
      crossing_event->y      = event->crossing.y;
      crossing_event->x_root = event->crossing.x_root;
      crossing_event->y_root = event->crossing.y_root;
      crossing_event->state  = event->crossing.state;
      break;

    default:
      /* It must be a button press/release event. */
      crossing_event->time   = event->button.time;
      crossing_event->x      = event->button.x;
      crossing_event->y      = event->button.y;
      crossing_event->x_root = event->button.x_root;
      crossing_event->y_root = event->button.y_root;
      crossing_event->state  = event->button.state;
      break;
    }
}


/* Emits a signal for an item and all its parents, until one of them
   returns TRUE. */
static gboolean
propagate_event (GooCanvas     *canvas,
		 GooCanvasItem *item,
		 gchar         *signal_name,
		 GdkEvent      *event)
{
  GooCanvasItem *ancestor;
  gboolean stop_emission = FALSE, valid;

  /* Don't emit any events if the canvas is not realized. */
  if (!GTK_WIDGET_REALIZED (canvas))
    return FALSE;

  if (item)
    {
      /* Check if the item is still in the canvas. */
      if (!ITEM_IS_VALID (item))
	return FALSE;
      ancestor = item;
    }
  else
    {
      /* If there is no target item, we send the event to the root item,
	 with target set to NULL. */
      ancestor = canvas->root_item;
    }

  /* Make sure the item pointer remains valid throughout the emission. */
  if (item)
    g_object_ref (item);

  while (ancestor)
    {
      g_object_ref (ancestor);

      g_signal_emit_by_name (ancestor, signal_name, item, event,
			     &stop_emission);

      /* Check if the ancestor is still in the canvas. */
      valid = ITEM_IS_VALID (ancestor) ? TRUE : FALSE;

      g_object_unref (ancestor);

      if (stop_emission || !valid)
	break;

      ancestor = goo_canvas_item_get_parent (ancestor);
    }

  if (item)
    g_object_unref (item);

  return stop_emission;
}


/* This is called to emit pointer events - enter/leave notify, motion notify,
   and button press/release. */
static gboolean
emit_pointer_event (GooCanvas *canvas,
		    gchar     *signal_name,
		    GdkEvent  *original_event)
{
  GdkEvent event = *original_event;
  GooCanvasItem *target_item = canvas->pointer_item;
  double *x, *y, *x_root, *y_root;

  /* Check if an item has grabbed the pointer. */
  if (canvas->pointer_grab_item)
    {
      /* When the pointer is grabbed, it receives all the pointer motion,
	 button press/release events and its own enter/leave notify events.
	 Enter/leave notify events for other items are discarded. */
      if ((event.type == GDK_ENTER_NOTIFY || event.type == GDK_LEAVE_NOTIFY)
	  && canvas->pointer_item != canvas->pointer_grab_item)
	return FALSE;

      target_item = canvas->pointer_grab_item;
    }

  /* Check if the target item is still in the canvas. */
  if (target_item && !ITEM_IS_VALID (target_item))
    return FALSE;

  /* Translate the x & y coordinates to the item's space. */
  switch (event.type)
    {
    case GDK_MOTION_NOTIFY:
      x = &event.motion.x;
      y = &event.motion.y;
      x_root = &event.motion.x_root;
      y_root = &event.motion.y_root;
      break;
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      x = &event.crossing.x;
      y = &event.crossing.y;
      x_root = &event.crossing.x_root;
      y_root = &event.crossing.y_root;
      break;
    default:
      /* It must be a button press/release event. */
      x = &event.button.x;
      y = &event.button.y;
      x_root = &event.button.x_root;
      y_root = &event.button.y_root;
      break;
    }

  /* Add 0.5 to the pixel coordinates so we use the center of the pixel. */
  *x += 0.5;
  *y += 0.5;

  /* Convert to the canvas coordinate space. */
  goo_canvas_convert_from_pixels (canvas, x, y);

  /* Copy to the x_root & y_root fields. */
  *x_root = *x;
  *y_root = *y;

  /* Convert to the item's coordinate space. */
  goo_canvas_convert_to_item_space (canvas, target_item, x, y);

  return propagate_event (canvas, target_item, signal_name, &event);
}


/* Finds the item that the mouse is over, using the given event's
 * coordinates. It emits enter/leave events for items as appropriate.
 */
static void
update_pointer_item (GooCanvas *canvas,
		     GdkEvent  *event)
{
  GooCanvasItem *new_item = NULL;

  if (event)
    initialize_crossing_event (canvas, event);

  /* If the event type is GDK_LEAVE_NOTIFY, the mouse has left the canvas,
     so we leave new_item as NULL, otherwise we find which item is
     underneath the mouse. Note that we initialize the type to GDK_LEAVE_NOTIFY
     in goo_canvas_init() to indicate the mouse isn't in the canvas. */
  if (canvas->crossing_event.type != GDK_LEAVE_NOTIFY && canvas->root_item)
    {
      double x = canvas->crossing_event.x;
      double y = canvas->crossing_event.y;

      goo_canvas_convert_from_pixels (canvas, &x, &y);
      new_item = goo_canvas_get_item_at (canvas, x, y, TRUE);
    }

  /* If the current item hasn't changed, just return. */
  if (new_item == canvas->pointer_item)
    return;

  /* Ref the new item, in case it is removed below. */
  if (new_item)
    g_object_ref (new_item);

  /* Emit a leave-notify event for the current item. */
  if (canvas->pointer_item)
    {
      canvas->crossing_event.type = GDK_LEAVE_NOTIFY;
      emit_pointer_event (canvas, "leave_notify_event",
			  (GdkEvent*) &canvas->crossing_event);
    }

  /* If there is no new item we are done. */
  if (!new_item)
    {
      set_item_pointer (&canvas->pointer_item, NULL);
      return;
    }

  /* If the new item isn't in the canvas any more, don't use it. */
  if (!ITEM_IS_VALID (new_item))
    {
      set_item_pointer (&canvas->pointer_item, NULL);
      g_object_unref (new_item);
      return;
    }

  /* Emit an enter-notify for the new current item. */
  set_item_pointer (&canvas->pointer_item, new_item);
  canvas->crossing_event.type = GDK_ENTER_NOTIFY;
  emit_pointer_event (canvas, "enter_notify_event",
		      (GdkEvent*) &canvas->crossing_event);

  g_object_unref (new_item);
}


static gboolean
goo_canvas_crossing        (GtkWidget        *widget,
			    GdkEventCrossing *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);

  if (event->window != canvas->canvas_window)
    return FALSE;

  /* This will result in synthesizing focus_in/out events as appropriate. */
  update_pointer_item (canvas, (GdkEvent*) event);

  return FALSE;
}


static gboolean
goo_canvas_motion          (GtkWidget      *widget,
			    GdkEventMotion *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);

  if (event->window != canvas->canvas_window)
    return FALSE;

  /* For motion notify hint events we need to call gdk_window_get_pointer()
     to let X know we're ready for another pointer event. */
  if (event->is_hint)
    gdk_window_get_pointer (event->window, NULL, NULL, NULL);

  update_pointer_item (canvas, (GdkEvent*) event);

  return emit_pointer_event (canvas, "motion_notify_event", (GdkEvent*) event);
}


static gboolean
goo_canvas_button_press (GtkWidget      *widget,
			 GdkEventButton *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);
  GdkDisplay *display;

  if (event->window != canvas->canvas_window)
    return FALSE;

  update_pointer_item (canvas, (GdkEvent*) event);

  /* Check if this is the start of an implicit pointer grab, i.e. if we
     don't already have a grab and the app has no active grab. */
  display = gtk_widget_get_display (widget);
  if (!canvas->pointer_grab_item
      && !gdk_display_pointer_is_grabbed (display))
    {
      set_item_pointer (&canvas->pointer_grab_initial_item,
			canvas->pointer_item);
      set_item_pointer (&canvas->pointer_grab_item,
			canvas->pointer_item);
      canvas->pointer_grab_button = event->button;
    }

  return emit_pointer_event (canvas, "button_press_event", (GdkEvent*) event);
}


static gboolean
goo_canvas_button_release (GtkWidget      *widget,
			   GdkEventButton *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);
  GdkDisplay *display;
  gboolean retval;

  if (event->window != canvas->canvas_window)
    return FALSE;

  update_pointer_item (canvas, (GdkEvent*) event);

  retval = emit_pointer_event (canvas, "button_release_event",
			       (GdkEvent*) event);

  /* Check if an implicit (passive) grab has ended. */
  display = gtk_widget_get_display (widget);
  if (canvas->pointer_grab_item
      && event->button == canvas->pointer_grab_button
      && !gdk_display_pointer_is_grabbed (display))
    {
      /* We set the pointer item back to the item it was in before the
	 grab, so we'll synthesize enter/leave notify events as appropriate. */
      if (canvas->pointer_grab_initial_item
	  && ITEM_IS_VALID (canvas->pointer_grab_initial_item))
	set_item_pointer (&canvas->pointer_item,
			  canvas->pointer_grab_initial_item);
      else
	set_item_pointer (&canvas->pointer_item, NULL);

      set_item_pointer (&canvas->pointer_grab_item, NULL);
      set_item_pointer (&canvas->pointer_grab_initial_item, NULL);

      update_pointer_item (canvas, (GdkEvent*) event);
    }

  return retval;
}


static gint
goo_canvas_scroll	(GtkWidget      *widget,
			 GdkEventScroll *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);
  GtkAdjustment *adj;
  gdouble delta, new_value;

  if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_DOWN)
    adj = canvas->vadjustment;
  else
    adj = canvas->hadjustment;

  delta = pow (adj->page_size, 2.0 / 3.0);

  if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_LEFT)
    delta = - delta;

  new_value = CLAMP (adj->value + delta, adj->lower,
		     adj->upper - adj->page_size);
      
  gtk_adjustment_set_value (adj, new_value);

  return TRUE;
}


static gboolean
goo_canvas_focus_in        (GtkWidget      *widget,
			    GdkEventFocus  *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);

  GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

  if (canvas->focused_item)
    return propagate_event (canvas, canvas->focused_item,
			    "focus_in_event", (GdkEvent*) event);
  else
    return FALSE;
}


static gboolean
goo_canvas_focus_out       (GtkWidget      *widget,
			    GdkEventFocus  *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

  if (canvas->focused_item)
    return propagate_event (canvas, canvas->focused_item,
			    "focus_out_event", (GdkEvent*) event);
  else
    return FALSE;
}


static gboolean
goo_canvas_key_press       (GtkWidget      *widget,
			    GdkEventKey    *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);
	
  if (canvas->focused_item)
    if (propagate_event (canvas, canvas->focused_item, "key_press_event",
			 (GdkEvent*) event))
    return TRUE;

  return GTK_WIDGET_CLASS (goo_canvas_parent_class)->key_press_event (widget, event);
}


static gboolean
goo_canvas_key_release     (GtkWidget      *widget,
			    GdkEventKey    *event)
{
  GooCanvas *canvas = GOO_CANVAS (widget);
	
  if (canvas->focused_item)
    if (propagate_event (canvas, canvas->focused_item, "key_release_event",
			 (GdkEvent*) event))
    return TRUE;

  return GTK_WIDGET_CLASS (goo_canvas_parent_class)->key_release_event (widget, event);
}


static void
generate_grab_broken (GooCanvas     *canvas,
		      GooCanvasItem *item,
		      gboolean       keyboard,
		      gboolean       implicit)

{
  GdkEventGrabBroken event;
  
  if (!ITEM_IS_VALID (item))
    return;

  event.type = GDK_GRAB_BROKEN;
  event.window = canvas->canvas_window;
  event.send_event = 0;
  event.keyboard = keyboard;
  event.implicit = implicit;
  event.grab_window = event.window;

  propagate_event (canvas, item, "grab_broken_event",
		   (GdkEvent*) &event);
}


static gboolean
goo_canvas_grab_broken     (GtkWidget          *widget,
			    GdkEventGrabBroken *event)
{
  GooCanvas *canvas;

  g_return_val_if_fail (GOO_IS_CANVAS (widget), FALSE);

  canvas = GOO_CANVAS (widget);

  if (event->keyboard)
    {
      if (canvas->keyboard_grab_item)
	{
	  generate_grab_broken (canvas, canvas->keyboard_grab_item,
				event->keyboard, event->implicit);
	  set_item_pointer (&canvas->keyboard_grab_item, NULL);
	}
    }
  else
    {
      if (canvas->pointer_grab_item)
	{
	  generate_grab_broken (canvas, canvas->pointer_grab_item,
				event->keyboard, event->implicit);
	  set_item_pointer (&canvas->pointer_grab_item, NULL);
	}
    }

  return TRUE;
}


/**
 * goo_canvas_grab_focus:
 * @canvas: a #GooCanvas.
 * @item: the item to grab the focus.
 * 
 * Grabs the keyboard focus for the given item.
 **/
void
goo_canvas_grab_focus (GooCanvas     *canvas,
		       GooCanvasItem *item)
{
  GdkEventFocus event;

  g_return_if_fail (GOO_IS_CANVAS (canvas));
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (GTK_WIDGET_CAN_FOCUS (canvas));

  if (canvas->focused_item) {
    event.type = GDK_FOCUS_CHANGE;
    event.window = canvas->canvas_window;
    event.send_event = FALSE;
    event.in = FALSE;

    propagate_event (canvas, canvas->focused_item,
		     "focus_out_event", (GdkEvent*) &event);
  }

  set_item_pointer (&canvas->focused_item, item);

  gtk_widget_grab_focus (GTK_WIDGET (canvas));

  if (canvas->focused_item) {
    event.type = GDK_FOCUS_CHANGE;                        
    event.window = canvas->canvas_window;
    event.send_event = FALSE;                             
    event.in = TRUE;                                      

    propagate_event (canvas, canvas->focused_item,
		     "focus_in_event", (GdkEvent*) &event);
  }                               
}


/*
 * Pointer/keyboard grabbing & ungrabbing.
 */

/**
 * goo_canvas_pointer_grab:
 * @canvas: a #GooCanvas.
 * @item: the item to grab the pointer for.
 * @event_mask: the events to receive during the grab.
 * @cursor: the cursor to display during the grab, or NULL.
 * @time: the time of the event that lead to the pointer grab. This should
 *  come from the relevant #GdkEvent.
 * 
 * Attempts to grab the pointer for the given item.
 * 
 * Returns: %GDK_GRAB_SUCCESS if the grab succeeded.
 **/
GdkGrabStatus
goo_canvas_pointer_grab (GooCanvas     *canvas,
			 GooCanvasItem *item,
			 GdkEventMask   event_mask,
			 GdkCursor     *cursor,
			 guint32        time)
{
  GdkGrabStatus status = GDK_GRAB_SUCCESS;

  g_return_val_if_fail (GOO_IS_CANVAS (canvas), GDK_GRAB_NOT_VIEWABLE);
  g_return_val_if_fail (GOO_IS_CANVAS_ITEM (item), GDK_GRAB_NOT_VIEWABLE);

  /* If another item already has the pointer grab, we need to synthesize a
     grab-broken event for the current grab item. */
  if (canvas->pointer_grab_item
      && canvas->pointer_grab_item != item)
    {
      generate_grab_broken (canvas, canvas->pointer_grab_item,
			    FALSE, FALSE);
      set_item_pointer (&canvas->pointer_grab_item, NULL);
    }

  /* This overrides any existing grab. */
  status = gdk_pointer_grab (canvas->canvas_window, FALSE,
			     event_mask, NULL, cursor, time);

  if (status == GDK_GRAB_SUCCESS)
    {
      set_item_pointer (&canvas->pointer_grab_initial_item,
			     canvas->pointer_item);
      set_item_pointer (&canvas->pointer_grab_item,
			     item);
    }

  return status;
}


/**
 * goo_canvas_pointer_ungrab:
 * @canvas: a #GooCanvas.
 * @item: the item that has the grab.
 * @time: the time of the event that lead to the pointer ungrab. This should
 *  come from the relevant #GdkEvent.
 * 
 * Ungrabs the pointer, if the given item has the pointer grab.
 **/
void
goo_canvas_pointer_ungrab (GooCanvas     *canvas,
			   GooCanvasItem *item,
			   guint32        time)
{
  GdkDisplay *display;

  g_return_if_fail (GOO_IS_CANVAS (canvas));
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));

  /* If the item doesn't actually have the pointer grab, just return. */
  if (canvas->pointer_grab_item != item)
    return;

  /* If it is an active pointer grab, ungrab it explicitly. */
  display = gtk_widget_get_display (GTK_WIDGET (canvas));
  if (gdk_display_pointer_is_grabbed (display))
    gdk_display_pointer_ungrab (display, time);

  /* We set the pointer item back to the item it was in before the
     grab, so we'll synthesize enter/leave notify events as appropriate. */
  if (canvas->pointer_grab_initial_item
      && ITEM_IS_VALID (canvas->pointer_grab_initial_item))
    set_item_pointer (&canvas->pointer_item,
		      canvas->pointer_grab_initial_item);
  else
    set_item_pointer (&canvas->pointer_item, NULL);

  set_item_pointer (&canvas->pointer_grab_item, NULL);
  set_item_pointer (&canvas->pointer_grab_initial_item, NULL);

  update_pointer_item (canvas, NULL);
 }


/**
 * goo_canvas_keyboard_grab:
 * @canvas: a #GooCanvas.
 * @item: the item to grab the keyboard for.
 * @owner_events: %TRUE if keyboard events for this application will be
 *  reported normally, or %FALSE if all keyboard events will be reported with
 *  respect to the grab item.
 * @time: the time of the event that lead to the keyboard grab. This should
 *  come from the relevant #GdkEvent.
 * 
 * Attempts to grab the keyboard for the given item.
 * 
 * Returns: %GDK_GRAB_SUCCESS if the grab succeeded.
 **/
GdkGrabStatus
goo_canvas_keyboard_grab (GooCanvas     *canvas,
			  GooCanvasItem *item,
			  gboolean       owner_events,
			  guint32        time)
{
  GdkGrabStatus status = GDK_GRAB_SUCCESS;

  g_return_val_if_fail (GOO_IS_CANVAS (canvas), GDK_GRAB_NOT_VIEWABLE);
  g_return_val_if_fail (GOO_IS_CANVAS_ITEM (item), GDK_GRAB_NOT_VIEWABLE);

  /* Check if the item already has the keyboard grab. */
  if (canvas->keyboard_grab_item == item)
    return GDK_GRAB_ALREADY_GRABBED;

  /* If another item already has the keyboard grab, we need to synthesize a
     grab-broken event for the current grab item. */
  if (canvas->keyboard_grab_item)
    {
      generate_grab_broken (canvas, canvas->keyboard_grab_item, TRUE, FALSE);
      set_item_pointer (&canvas->keyboard_grab_item, NULL);
    }

  /* This overrides any existing grab. */
  status = gdk_keyboard_grab (canvas->canvas_window,
			      owner_events, time);

  if (status == GDK_GRAB_SUCCESS)
    set_item_pointer (&canvas->keyboard_grab_item, item);

  return status;
}


/**
 * goo_canvas_keyboard_ungrab:
 * @canvas: a #GooCanvas.
 * @item: the item that has the keyboard grab.
 * @time: the time of the event that lead to the keyboard ungrab. This should
 *  come from the relevant #GdkEvent.
 * 
 * Ungrabs the keyboard, if the given item has the keyboard grab.
 **/
void
goo_canvas_keyboard_ungrab (GooCanvas     *canvas,
			    GooCanvasItem *item,
			    guint32        time)
{
  GdkDisplay *display;

  g_return_if_fail (GOO_IS_CANVAS (canvas));
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));

  /* If the item doesn't actually have the keyboard grab, just return. */
  if (canvas->keyboard_grab_item != item)
    return;

  set_item_pointer (&canvas->keyboard_grab_item, NULL);

  display = gtk_widget_get_display (GTK_WIDGET (canvas));
  gdk_display_keyboard_ungrab (display, time);
}


/*
 * Coordinate conversion.
 */

/**
 * goo_canvas_convert_to_pixels:
 * @canvas: a #GooCanvas.
 * @x: a pointer to the x coordinate to convert.
 * @y: a pointer to the y coordinate to convert.
 * 
 * Converts a coordinate from the canvas coordinate space to pixels.
 *
 * The canvas coordinate space is specified in the call to
 * goo_canvas_set_bounds().
 *
 * The pixel coordinate space specifies pixels from the top-left of the entire
 * canvas window, according to the current scale setting.
 * See goo_canvas_set_scale().
 **/
void
goo_canvas_convert_to_pixels (GooCanvas     *canvas,
			      gdouble       *x,
			      gdouble       *y)
{
  *x = ((*x - canvas->bounds.x1) * canvas->scale_x) + canvas->canvas_x_offset;
  *y = ((*y - canvas->bounds.y1) * canvas->scale_y) + canvas->canvas_y_offset;
}


/**
 * goo_canvas_convert_from_pixels:
 * @canvas: a #GooCanvas.
 * @x: a pointer to the x coordinate to convert.
 * @y: a pointer to the y coordinate to convert.
 * 
 * Converts a coordinate from pixels to the canvas coordinate space.
 *
 * The pixel coordinate space specifies pixels from the top-left of the entire
 * canvas window, according to the current scale setting.
 * See goo_canvas_set_scale().
 *
 * The canvas coordinate space is specified in the call to
 * goo_canvas_set_bounds().
 *
 **/
void
goo_canvas_convert_from_pixels (GooCanvas     *canvas,
				gdouble       *x,
				gdouble       *y)
{
  *x = ((*x - canvas->canvas_x_offset) / canvas->scale_x) + canvas->bounds.x1;
  *y = ((*y - canvas->canvas_y_offset) / canvas->scale_y) + canvas->bounds.y1;
}


static void
goo_canvas_convert_from_window_pixels (GooCanvas     *canvas,
				       gdouble       *x,
				       gdouble       *y)
{
  *x += canvas->hadjustment->value;
  *y += canvas->vadjustment->value;
  goo_canvas_convert_from_pixels (canvas, x, y);
}


/**
 * goo_canvas_convert_to_item_space:
 * @canvas: a #GooCanvas.
 * @item: a #GooCanvasItem.
 * @x: a pointer to the x coordinate to convert.
 * @y: a pointer to the y coordinate to convert.
 * 
 * Converts a coordinate from the canvas coordinate space to the given
 * item's coordinate space, applying all transformation matrices including the
 * item's own transformation matrix, if it has one.
 **/
void
goo_canvas_convert_to_item_space (GooCanvas     *canvas,
				  GooCanvasItem *item,
				  gdouble       *x,
				  gdouble       *y)
{
  GooCanvasItem *tmp = item, *parent, *child;
  GList *list = NULL, *l;
  cairo_matrix_t item_transform, inverse = { 1, 0, 0, 1, 0, 0 };
  gboolean has_transform;

  /* Step up from the item to the top, pushing items onto the list. */
  while (tmp)
    {
      list = g_list_prepend (list, tmp);
      tmp = goo_canvas_item_get_parent (tmp);
    }

  /* Now step down applying the inverse of each item's transformation. */
  for (l = list; l; l = l->next)
    {
      parent = (GooCanvasItem*) l->data;
      child = l->next ? (GooCanvasItem*) l->next->data : NULL;
      has_transform = goo_canvas_item_get_transform_for_child (parent, child,
							       &item_transform);
      if (has_transform)
	{
	  cairo_matrix_invert (&item_transform);
	  cairo_matrix_multiply (&inverse, &inverse, &item_transform);
	}
    }
  g_list_free (list);

  /* Now convert the coordinates. */
  cairo_matrix_transform_point (&inverse, x, y);
}


/**
 * goo_canvas_convert_from_item_space:
 * @canvas: a #GooCanvas.
 * @item: a #GooCanvasItem.
 * @x: a pointer to the x coordinate to convert.
 * @y: a pointer to the y coordinate to convert.
 * 
 * Converts a coordinate from the given item's coordinate space to the canvas
 * coordinate space, applying all transformation matrices including the
 * item's own transformation matrix, if it has one.
 **/
void
goo_canvas_convert_from_item_space (GooCanvas     *canvas,
				    GooCanvasItem *item,
				    gdouble       *x,
				    gdouble       *y)
{
  GooCanvasItem *tmp = item, *parent, *child;
  GList *list = NULL, *l;
  cairo_matrix_t item_transform, transform = { 1, 0, 0, 1, 0, 0 };
  gboolean has_transform;

  /* Step up from the item to the top, pushing items onto the list. */
  while (tmp)
    {
      list = g_list_prepend (list, tmp);
      tmp = goo_canvas_item_get_parent (tmp);
    }

  /* Now step down applying each item's transformation. */
  for (l = list; l; l = l->next)
    {
      parent = (GooCanvasItem*) l->data;
      child = l->next ? (GooCanvasItem*) l->next->data : NULL;
      has_transform = goo_canvas_item_get_transform_for_child (parent, child,
							       &item_transform);
      if (has_transform)
	{
	  cairo_matrix_multiply (&transform, &transform, &item_transform);
	}
    }
  g_list_free (list);

  /* Now convert the coordinates. */
  cairo_matrix_transform_point (&transform, x, y);
}


/*
 * Keyboard focus navigation.
 */

typedef struct _GooCanvasFocusData GooCanvasFocusData;
struct _GooCanvasFocusData
{
  /* The item to start from, usually the currently focused item, or NULL. */
  GooCanvasItem *start_item;

  /* The bounds of the start item. We try to find the next closest one in the
     desired direction. */
  GooCanvasBounds start_bounds;
  gdouble start_center_x, start_center_y;

  /* The direction to move the focus in. */
  GtkDirectionType  direction;

  /* The text direction of the widget. */
  GtkTextDirection text_direction;

  /* The best item found so far, and the offsets for it. */
  GooCanvasItem *best_item;
  gdouble best_x_offset, best_y_offset, best_score;

  /* The offsets for the item being tested. */
  GooCanvasBounds current_bounds;
  gdouble current_x_offset, current_y_offset, current_score;
};


/* This tries to figure out the bounds of the start item or widget. */
static void
goo_canvas_get_start_bounds (GooCanvas          *canvas,
			     GooCanvasFocusData *data)
{
  GooCanvasBounds *bounds;
  GtkWidget *toplevel, *focus_widget;
  GtkAllocation *allocation;
  gint focus_widget_x, focus_widget_y;

  /* If an item is currently focused, we just need its bounds. */
  if (data->start_item)
    {
      goo_canvas_item_get_bounds (data->start_item, &data->start_bounds);
      return;
    }

  /* Otherwise try to get the currently focused widget in the window. */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (canvas));
  bounds = &data->start_bounds;
  if (toplevel && GTK_IS_WINDOW (toplevel)
      && GTK_WINDOW (toplevel)->focus_widget)
    {
      focus_widget = GTK_WINDOW (toplevel)->focus_widget;

      /* Translate the allocation to be relative to the GooCanvas.
	 Skip ancestor widgets as the coords won't help. */
      if (!gtk_widget_is_ancestor (GTK_WIDGET (canvas), focus_widget)
	  && gtk_widget_translate_coordinates (focus_widget,
					       GTK_WIDGET (canvas),
					       0, 0,
					       &focus_widget_x,
					       &focus_widget_y))
	{
	  /* Translate into device units. */
	  bounds->x1 = focus_widget_x;
	  bounds->y1 = focus_widget_y;
	  bounds->x2 = focus_widget_x + focus_widget->allocation.width;
	  bounds->y2 = focus_widget_y + focus_widget->allocation.height;

	  goo_canvas_convert_from_window_pixels (canvas, &bounds->x1,
						 &bounds->y1);
	  goo_canvas_convert_from_window_pixels (canvas, &bounds->x2,
						 &bounds->y2);
	  return;
	}
    }

  /* As a last resort, we guess a starting position based on the direction. */
  allocation = &GTK_WIDGET (canvas)->allocation;
  switch (data->direction)
    {
    case GTK_DIR_DOWN:
    case GTK_DIR_RIGHT:
      /* Start from top-left. */
      bounds->x1 = 0.0;
      bounds->y1 = 0.0;
      break;

    case GTK_DIR_UP:
      /* Start from bottom-left. */
      bounds->x1 = 0.0;
      bounds->y1 = allocation->height;
      break;

    case GTK_DIR_LEFT:
      /* Start from top-right. */
      bounds->x1 = allocation->width;
      bounds->y1 = 0.0;
      break;

    case GTK_DIR_TAB_FORWARD:
      bounds->y1 = 0.0;
      if (data->text_direction == GTK_TEXT_DIR_RTL)
	/* Start from top-right. */
	bounds->x1 = allocation->width;
      else
	/* Start from top-left. */
	bounds->x1 = 0.0;
      break;

    case GTK_DIR_TAB_BACKWARD:
      bounds->y1 = allocation->height;
      if (data->text_direction == GTK_TEXT_DIR_RTL)
	/* Start from bottom-left. */
	bounds->x1 = 0.0;
      else
	/* Start from bottom-right. */
	bounds->x1 = allocation->width;
      break;
    }

  goo_canvas_convert_from_window_pixels (canvas, &bounds->x1, &bounds->y1);
  bounds->x2 = bounds->x1;
  bounds->y2 = bounds->y1;
}


/* Check if the given item is a better candidate for the focus than
   the current best one in the data struct. */
static gboolean
goo_canvas_focus_check_is_best (GooCanvas          *canvas,
				GooCanvasItem      *item,
				GooCanvasFocusData *data)
{
  gdouble center_x, center_y;
  gdouble abs_x_offset = 0.0, abs_y_offset = 0.0;

  data->current_score = 0.0;

  goo_canvas_item_get_bounds (item, &data->current_bounds);
  center_x = (data->current_bounds.x1 + data->current_bounds.x2) / 2.0;
  center_y = (data->current_bounds.y1 + data->current_bounds.y2) / 2.0;

  /* Calculate the offsets of the center of this item from the center
     of the current focus item or widget. */
  data->current_x_offset = center_x - data->start_center_x;
  data->current_y_offset = center_y - data->start_center_y;

  /* If the item overlaps the current one at all we use 0 as the offset. */
  if (data->current_bounds.x1 > data->start_bounds.x2
      || data->current_bounds.x2 < data->start_bounds.x2)
      abs_x_offset = fabs (data->current_x_offset);

  if (data->current_bounds.y1 > data->start_bounds.y2
      || data->current_bounds.y2 < data->start_bounds.y2)
      abs_y_offset = fabs (data->current_y_offset);

  /* FIXME: I'm still not sure about the score calculations here. There
     are still a few odd jumps when using keyboard focus navigation. */
  switch (data->direction)
    {
    case GTK_DIR_UP:
      /* If the y offset is > 0 we can discard this item. */
      if (data->current_y_offset >= 0 || abs_x_offset > abs_y_offset)
	return FALSE;

      /* Compute a score (lower is best) and check if it is the best. */
      data->current_score = abs_x_offset * 2 + abs_y_offset;
      if (!data->best_item || data->current_score < data->best_score)
	return TRUE;
      break;

    case GTK_DIR_DOWN:
      /* If the y offset is < 0 we can discard this item. */
      if (data->current_y_offset <= 0 || abs_x_offset > abs_y_offset)
	return FALSE;

      /* Compute a score (lower is best) and check if it is the best. */
      data->current_score = abs_x_offset /** 2*/ + abs_y_offset;
      if (!data->best_item || data->current_score < data->best_score)
	return TRUE;
      break;

    case GTK_DIR_LEFT:
      /* If the x offset is > 0 we can discard this item. */
      if (data->current_x_offset >= 0 || abs_y_offset > abs_x_offset)
	return FALSE;

      /* Compute a score (lower is best) and check if it is the best. */
      data->current_score = abs_y_offset * 2 + abs_x_offset;
      if (!data->best_item || data->current_score < data->best_score)
	return TRUE;
      break;

    case GTK_DIR_RIGHT:
      /* If the x offset is < 0 we can discard this item. */
      if (data->current_x_offset <= 0 || abs_y_offset > abs_x_offset)
	return FALSE;

      /* Compute a score (lower is best) and check if it is the best. */
      data->current_score = abs_y_offset * 2 + abs_x_offset;
      if (!data->best_item || data->current_score < data->best_score)
	return TRUE;
      break;

    case GTK_DIR_TAB_BACKWARD:
      /* We need to handle this differently depending on text direction. */
      if (data->text_direction == GTK_TEXT_DIR_RTL)
	{
	  /* If the y offset is > 0, or it is 0 and the x offset > 0 we can
	     discard this item. */
	  if (data->current_y_offset > 0
	      || (data->current_y_offset == 0 && data->current_x_offset < 0))
	    return FALSE;

	  /* If the y offset is > the current best y offset, this is best. */
	  if (!data->best_item || data->current_y_offset > data->best_y_offset)
	    return TRUE;

	  /* If the y offsets are the same, choose the largest x offset. */
	  if (data->current_y_offset == data->best_y_offset
	      && data->current_x_offset < data->best_x_offset)
	    return TRUE;
	}
      else
	{
	  /* If the y offset is > 0, or it is 0 and the x offset > 0 we can
	     discard this item. */
	  if (data->current_y_offset > 0
	      || (data->current_y_offset == 0 && data->current_x_offset > 0))
	    return FALSE;

	  /* If the y offset is > the current best y offset, this is best. */
	  if (!data->best_item || data->current_y_offset > data->best_y_offset)
	    return TRUE;

	  /* If the y offsets are the same, choose the largest x offset. */
	  if (data->current_y_offset == data->best_y_offset
	      && data->current_x_offset > data->best_x_offset)
	    return TRUE;
	}
      break;

    case GTK_DIR_TAB_FORWARD:
      /* We need to handle this differently depending on text direction. */
      if (data->text_direction == GTK_TEXT_DIR_RTL)
	{
	  /* If the y offset is < 0, or it is 0 and the x offset > 0 we can
	     discard this item. */
	  if (data->current_y_offset < 0
	      || (data->current_y_offset == 0 && data->current_x_offset > 0))
	    return FALSE;

	  /* If the y offset is < the current best y offset, this is best. */
	  if (!data->best_item || data->current_y_offset < data->best_y_offset)
	    return TRUE;

	  /* If the y offsets are the same, choose the largest x offset. */
	  if (data->current_y_offset == data->best_y_offset
	      && data->current_x_offset > data->best_x_offset)
	    return TRUE;
	}
      else
	{
	  /* If the y offset is < 0, or it is 0 and the x offset < 0 we can
	     discard this item. */
	  if (data->current_y_offset < 0
	      || (data->current_y_offset == 0 && data->current_x_offset < 0))
	    return FALSE;

	  /* If the y offset is < the current best y offset, this is best. */
	  if (!data->best_item || data->current_y_offset < data->best_y_offset)
	    return TRUE;

	  /* If the y offsets are the same, choose the smallest x offset. */
	  if (data->current_y_offset == data->best_y_offset
	      && data->current_x_offset < data->best_x_offset)
	    return TRUE;
	}
      break;
    }

  return FALSE;
}


/* Recursively look for the next best item in the desired direction. */
static void
goo_canvas_focus_recurse (GooCanvas          *canvas,
			  GooCanvasItem      *item,
			  GooCanvasFocusData *data)
{
  GooCanvasItem *child;
  gboolean can_focus = FALSE, is_best;
  gint n_children, i;

  /* If the item is not a possible candidate, just return. */
  is_best = goo_canvas_focus_check_is_best (canvas, item, data);

  if (is_best && goo_canvas_item_is_visible (item))
    {
      /* Check if the item can take the focus. */
      if (GOO_IS_CANVAS_WIDGET (item))
	{
	  /* We have to assume that widget items can take focus, since any
	     of their descendants may take the focus. */
	  if (((GooCanvasWidget*) item)->widget)
	    can_focus = TRUE;
	}
      else
	{
	  g_object_get (item, "can-focus", &can_focus, NULL);
	}

      /* If the item can take the focus itself, and it isn't the current
	 focus item, save its score and return. (If it is a container it takes
	 precedence over its children). */
      if (can_focus && item != data->start_item)
	{
	  data->best_item = item;
	  data->best_x_offset = data->current_x_offset;
	  data->best_y_offset = data->current_y_offset;
	  data->best_score = data->current_score;
	  return;
	}
    }

  /* If the item is a container, check the children recursively. */
  n_children = goo_canvas_item_get_n_children (item);
  if (n_children)
    {
      /* Check if we can skip the entire group. */
      switch (data->direction)
	{
	case GTK_DIR_UP:
	  /* If the group is below the bottom of the current focused item
	     we can skip it. */
	  if (data->current_bounds.y1 > data->start_bounds.y2)
	    return;
	  break;
	case GTK_DIR_DOWN:
	  /* If the group is above the top of the current focused item
	     we can skip it. */
	  if (data->current_bounds.y2 < data->start_bounds.y1)
	    return;
	  break;
	case GTK_DIR_LEFT:
	  /* If the group is to the right of the current focused item
	     we can skip it. */
	  if (data->current_bounds.x1 > data->start_bounds.x2)
	    return;
	  break;
	case GTK_DIR_RIGHT:
	  /* If the group is to the left of the current focused item
	     we can skip it. */
	  if (data->current_bounds.x2 < data->start_bounds.x1)
	    return;
	  break;
	default:
	  break;
	}

      for (i = 0; i < n_children; i++)
	{
	  child = goo_canvas_item_get_child (item, i);
	  goo_canvas_focus_recurse (canvas, child, data);
	}
    }
}


/* FIXME: We could add support for a focus chain, like GtkContainer. */
static gboolean
goo_canvas_focus (GtkWidget        *widget,
		  GtkDirectionType  direction)
{
  GooCanvas *canvas;
  GooCanvasFocusData data;
  GtkWidget *old_focus_child;
  gboolean found_item;
  gint try;

  g_return_val_if_fail (GOO_IS_CANVAS (widget), FALSE);

  canvas = GOO_CANVAS (widget);

  /* If keyboard navigation has been turned off for the canvas, return FALSE.*/
  if (!GTK_WIDGET_CAN_FOCUS (canvas))
    return FALSE;

  /* If a child widget has the focus, try moving the focus within that. */
  old_focus_child = GTK_CONTAINER (canvas)->focus_child;
  if (old_focus_child && gtk_widget_child_focus (old_focus_child, direction))
    return TRUE;

  data.direction = direction;
  data.text_direction = gtk_widget_get_direction (widget);
  data.start_item = NULL;

  if (GTK_WIDGET_HAS_FOCUS (canvas))
    data.start_item = canvas->focused_item;
  else if (old_focus_child && GOO_IS_CANVAS_WIDGET (old_focus_child))
    data.start_item = g_object_get_data (G_OBJECT (old_focus_child),
					 "goo-canvas-item");

  /* Keep looping until we find an item to focus or we fail. I've added a
     limit on the number of tries just in case we get into an infinite loop. */
  for (try = 1; try < 1000; try++)
    {
      /* Get the bounds of the currently focused item or widget. */
      goo_canvas_get_start_bounds (canvas, &data);
      data.start_center_x = (data.start_bounds.x1 + data.start_bounds.x2) / 2.0;
      data.start_center_y = (data.start_bounds.y1 + data.start_bounds.y2) / 2.0;
      data.best_item = NULL;

      /* Recursively look for the next best item in the desired direction. */
      goo_canvas_focus_recurse (canvas, canvas->root_item, &data);

      /* If we found an item to focus, grab the focus and return TRUE. */
      if (!data.best_item)
	break;

      if (GOO_IS_CANVAS_WIDGET (data.best_item))
	{
	  found_item = gtk_widget_child_focus (((GooCanvasWidget*) data.best_item)->widget, direction);
	}
      else
	{
	  found_item = TRUE;
	  goo_canvas_grab_focus (canvas, data.best_item);
	}
      
      if (found_item)
	{
	  goo_canvas_scroll_to_item (canvas, data.best_item);
	  return TRUE;
	}

      /* Try again from the last item tried. */
      data.start_item = data.best_item;
    }

  return FALSE;
}


/**
 * goo_canvas_register_widget_item:
 * @canvas: a #GooCanvas.
 * @witem: a #GooCanvasWidget item.
 * 
 * This function should only be used by #GooCanvasWidget and subclass
 * implementations.
 *
 * It registers a widget item with the canvas, so that the canvas can do the
 * necessary actions to move and resize the widget as needed.
 **/
void
goo_canvas_register_widget_item   (GooCanvas          *canvas,
				   GooCanvasWidget    *witem)
{
  g_return_if_fail (GOO_IS_CANVAS (canvas));
  g_return_if_fail (GOO_IS_CANVAS_WIDGET (witem));

  canvas->widget_items = g_list_append (canvas->widget_items, witem);
}


/**
 * goo_canvas_unregister_widget_item:
 * @canvas: a #GooCanvas.
 * @witem: a #GooCanvasWidget item.
 * 
 * This function should only be used by #GooCanvasWidget and subclass
 * implementations.
 *
 * It unregisters a widget item from the canvas, when the item is no longer in
 * the canvas.
 **/
void
goo_canvas_unregister_widget_item (GooCanvas          *canvas,
				   GooCanvasWidget    *witem)
{
  GList *tmp_list;
  GooCanvasWidget *tmp_witem;

  g_return_if_fail (GOO_IS_CANVAS (canvas));
  g_return_if_fail (GOO_IS_CANVAS_WIDGET (witem));

  tmp_list = canvas->widget_items;
  while (tmp_list)
    {
      tmp_witem = tmp_list->data;
      if (tmp_witem == witem)
	break;
      tmp_list = tmp_list->next;
    }

  if (tmp_list)
    {
      canvas->widget_items = g_list_remove_link (canvas->widget_items,
						 tmp_list);
      g_list_free_1 (tmp_list);
    }
}


static void
goo_canvas_forall (GtkContainer *container,
		   gboolean      include_internals,
		   GtkCallback   callback,
		   gpointer      callback_data)
{
  GooCanvas *canvas;
  GList *tmp_list;
  GooCanvasWidget *witem;

  g_return_if_fail (GOO_IS_CANVAS (container));
  g_return_if_fail (callback != NULL);

  canvas = GOO_CANVAS (container);

  tmp_list = canvas->widget_items;
  while (tmp_list)
    {
      witem = tmp_list->data;
      tmp_list = tmp_list->next;

      if (witem->widget)
	(* callback) (witem->widget, callback_data);
    }
}


static void
goo_canvas_remove (GtkContainer *container, 
		   GtkWidget    *widget)
{
  GooCanvas *canvas;
  GList *tmp_list;
  GooCanvasWidget *witem;
  GooCanvasItem *parent;
  gint child_num;
  
  g_return_if_fail (GOO_IS_CANVAS (container));
  
  canvas = GOO_CANVAS (container);

  tmp_list = canvas->widget_items;
  while (tmp_list)
    {
      witem = tmp_list->data;
      tmp_list = tmp_list->next;

      if (witem->widget == widget)
	{
	  parent = goo_canvas_item_get_parent ((GooCanvasItem*) witem);
	  child_num = goo_canvas_item_find_child (parent,
						  (GooCanvasItem*) witem);
	  goo_canvas_item_remove_child (parent, child_num);

	  break;
	}
    }
}
