/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvas.h - the main canvas widget.
 */
#ifndef __GOO_CANVAS_H__
#define __GOO_CANVAS_H__

#include <gtk/gtk.h>
#include <goocanvasenumtypes.h>
#include <goocanvasellipse.h>
#include <goocanvasgroup.h>
#include <goocanvasimage.h>
#include <goocanvaspath.h>
#include <goocanvaspolyline.h>
#include <goocanvasrect.h>
#include <goocanvastable.h>
#include <goocanvastext.h>
#include <goocanvaswidget.h>

G_BEGIN_DECLS


#define GOO_TYPE_CANVAS            (goo_canvas_get_type ())
#define GOO_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS, GooCanvas))
#define GOO_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS, GooCanvasClass))
#define GOO_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS))
#define GOO_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS))
#define GOO_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS, GooCanvasClass))


typedef struct _GooCanvasClass  GooCanvasClass;

/**
 * GooCanvas
 *
 * The #GooCanvas-struct struct contains private data only.
 */
struct _GooCanvas
{
  GtkContainer container;

  /* The model for the root item, in model/view mode. */
  GooCanvasItemModel *root_item_model;

  /* The root canvas item. */
  GooCanvasItem *root_item;

  /* The bounds of the canvas, in canvas units (not pixels). */
  GooCanvasBounds bounds;

  /* The scale/zoom factor of the canvas, in pixels per device unit. */
  gdouble scale;

  /* Where the canvas is anchored (where it is displayed when it is smaller
     than the entire window). */
  GtkAnchorType anchor;

  /* Idle handler ID, for processing updates. */
  guint idle_id;

  /* This is TRUE if some item in the canvas needs an update. */
  guint need_update : 1;

  /* The item that the mouse is over. */
  GooCanvasItem *pointer_item;

  /* The item that has the pointer grab, or NULL. */
  GooCanvasItem *pointer_grab_item;

  /* This is the item that the grab was started from. When the grab ends
     we synthesize enter/leave notify events from this item. */
  GooCanvasItem *pointer_grab_initial_item;

  /* This is the mouse button that started an implicit pointer grab.
     When the same button is released the implicit grab ends. */
  guint pointer_grab_button;

  /* The item that has the keyboard focus, or NULL. */
  GooCanvasItem *focused_item;

  /* The item that has the keyboard grab, or NULL. */
  GooCanvasItem *keyboard_grab_item;

  /* The synthesized event used for sending enter-notify and leave-notify
     events to items. */
  GdkEventCrossing crossing_event;

  /* The main canvas window, which gets scrolled around. */
  GdkWindow *canvas_window;

  /* The offsets of the canvas within the canvas window, in pixels. These are
     used when the canvas is smaller than the window size and the anchor is not
     NORTH_WEST. */
  gint canvas_x_offset;
  gint canvas_y_offset;

  /* The adjustments used for scrolling. */
  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  /* Freezes any movement of the canvas window, until thawed. This is used
     when we need to set both adjustments and don't want it to scroll twice. */
  gint freeze_count;

  /* A window temporarily mapped above the canvas to stop X from scrolling
     the contents unnecessarily (i.e. when we zoom in/out). */
  GdkWindow *tmp_window;

  /* A hash table mapping canvas item models to canvas items. */
  GHashTable *model_to_item;

  /* The units of the canvas, which applies to all item coords. */
  GtkUnit units;

  /* The horizontal and vertical resolution of the display, in dots per inch.
     This is only needed when units other than pixels are used. */
  gdouble resolution_x, resolution_y;

  /* The multiplers to convert from device units to pixels, taking into account
     the canvas scale, the units setting and the display resolution. */
  gdouble scale_x, scale_y;

  /* The list of child widgets (using GooCanvasWidget items). */
  GList *widget_items;
};

/**
 * GooCanvasClass
 * @create_item: a virtual method that subclasses may override to create custom
 *  canvas items for item models.
 * @item_created: signal emitted when a new canvas item has been created.
 *  Applications can connect to this to setup signal handlers for the new item.
 *
 * The #GooCanvasClass-struct struct contains one virtual method that
 * subclasses may override.
 */
struct _GooCanvasClass
{
  /*< private >*/
  GtkContainerClass parent_class;

  void		 (* set_scroll_adjustments) (GooCanvas          *canvas,
					     GtkAdjustment      *hadjustment,
					     GtkAdjustment      *vadjustment);

  /* Virtual methods. */
  /*< public >*/
  GooCanvasItem* (* create_item)	    (GooCanvas          *canvas,
					     GooCanvasItemModel *model);

  /* Signals. */
  void           (* item_created)	    (GooCanvas          *canvas,
					     GooCanvasItem      *item,
					     GooCanvasItemModel *model);

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
  void (*_goo_canvas_reserved5) (void);
  void (*_goo_canvas_reserved6) (void);
  void (*_goo_canvas_reserved7) (void);
  void (*_goo_canvas_reserved8) (void);
};


GType           goo_canvas_get_type	    (void) G_GNUC_CONST;
GtkWidget*      goo_canvas_new		    (void);

GooCanvasItem*  goo_canvas_get_root_item    (GooCanvas		*canvas);
void            goo_canvas_set_root_item    (GooCanvas		*canvas,
					     GooCanvasItem      *item);

GooCanvasItemModel* goo_canvas_get_root_item_model (GooCanvas	       *canvas);
void                goo_canvas_set_root_item_model (GooCanvas	       *canvas,
						    GooCanvasItemModel *model);

GooCanvasItem*  goo_canvas_get_item	    (GooCanvas		*canvas,
					     GooCanvasItemModel *model);
GooCanvasItem*  goo_canvas_get_item_at	    (GooCanvas		*canvas,
					     gdouble             x,
					     gdouble             y,
					     gboolean            is_pointer_event);
GList*		goo_canvas_get_items_at	    (GooCanvas		*canvas,
					     gdouble		 x,
					     gdouble		 y,
					     gboolean		 is_pointer_event);
GList*		goo_canvas_get_items_in_area(GooCanvas		*canvas,
					     const GooCanvasBounds *area,
					     gboolean		 inside_area,
					     gboolean            allow_overlaps,
					     gboolean            include_containers);

gdouble         goo_canvas_get_scale	    (GooCanvas		*canvas);
void            goo_canvas_set_scale	    (GooCanvas		*canvas,
					     gdouble             scale);

void            goo_canvas_get_bounds	    (GooCanvas		*canvas,
					     gdouble            *left,
					     gdouble            *top,
					     gdouble            *right,
					     gdouble            *bottom);
void            goo_canvas_set_bounds	    (GooCanvas		*canvas,
					     gdouble             left,
					     gdouble             top,
					     gdouble             right,
					     gdouble             bottom);

void            goo_canvas_scroll_to	    (GooCanvas		*canvas,
					     gdouble             left,
					     gdouble             top);

void            goo_canvas_grab_focus	    (GooCanvas		*canvas,
					     GooCanvasItem	*item);

void            goo_canvas_render	    (GooCanvas		   *canvas,
					     cairo_t               *cr,
					     const GooCanvasBounds *bounds,
					     gdouble                scale);

/*
 * Coordinate conversion.
 */
void		goo_canvas_convert_to_pixels	   (GooCanvas       *canvas,
						    gdouble         *x,
						    gdouble         *y);
void		goo_canvas_convert_from_pixels	   (GooCanvas       *canvas,
						    gdouble         *x,
						    gdouble         *y);

void		goo_canvas_convert_to_item_space   (GooCanvas	    *canvas,
						    GooCanvasItem   *item,
						    gdouble         *x,
						    gdouble         *y);
void		goo_canvas_convert_from_item_space (GooCanvas       *canvas,
						    GooCanvasItem   *item,
						    gdouble         *x,
						    gdouble         *y);


/*
 * Pointer/keyboard grabbing & ungrabbing.
 */
GdkGrabStatus	goo_canvas_pointer_grab	    (GooCanvas		*canvas,
					     GooCanvasItem	*item,
					     GdkEventMask        event_mask,
					     GdkCursor		*cursor,
					     guint32             time);
void		goo_canvas_pointer_ungrab   (GooCanvas		*canvas,
					     GooCanvasItem	*item,
					     guint32             time);
GdkGrabStatus	goo_canvas_keyboard_grab    (GooCanvas		*canvas,
					     GooCanvasItem	*item,
					     gboolean            owner_events,
					     guint32             time);
void		goo_canvas_keyboard_ungrab  (GooCanvas		*canvas,
					     GooCanvasItem	*item,
					     guint32             time);


/*
 * Internal functions, mainly for canvas subclasses and item implementations.
 */
cairo_t*	goo_canvas_create_cairo_context	(GooCanvas *canvas);
GooCanvasItem*	goo_canvas_create_item	    (GooCanvas          *canvas,
					     GooCanvasItemModel *model);
void		goo_canvas_unregister_item  (GooCanvas		*canvas,
					     GooCanvasItemModel *model);
void		goo_canvas_update	    (GooCanvas		*canvas);
void		goo_canvas_request_update   (GooCanvas		*canvas);
void		goo_canvas_request_redraw   (GooCanvas		*canvas,
					     const GooCanvasBounds *bounds);
gdouble         goo_canvas_get_default_line_width (GooCanvas    *canvas);


void            goo_canvas_register_widget_item   (GooCanvas          *canvas,
						   GooCanvasWidget    *witem);
void            goo_canvas_unregister_widget_item (GooCanvas          *canvas,
						   GooCanvasWidget    *witem);


G_END_DECLS

#endif /* __GOO_CANVAS_H__ */
