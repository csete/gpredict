/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasitem.h - interface for canvas items & groups.
 */
#ifndef __GOO_CANVAS_ITEM_H__
#define __GOO_CANVAS_ITEM_H__

#include <gtk/gtk.h>
#include "goocanvasstyle.h"

G_BEGIN_DECLS


/**
 * GooCanvasAnimateType
 * @GOO_CANVAS_ANIMATE_FREEZE: the item remains in the final position.
 * @GOO_CANVAS_ANIMATE_RESET: the item is moved back to the initial position.
 * @GOO_CANVAS_ANIMATE_RESTART: the animation is restarted from the initial
 *  position.
 * @GOO_CANVAS_ANIMATE_BOUNCE: the animation bounces back and forth between the
 *  start and end positions.
 *
 * #GooCanvasAnimateType is used to specify what happens when the end of an
 * animation is reached.
 */
typedef enum
{
  GOO_CANVAS_ANIMATE_FREEZE,
  GOO_CANVAS_ANIMATE_RESET,
  GOO_CANVAS_ANIMATE_RESTART,
  GOO_CANVAS_ANIMATE_BOUNCE
} GooCanvasAnimateType;


/**
 * GooCanvasBounds
 * @x1: the left edge.
 * @y1: the top edge.
 * @x2: the right edge.
 * @y2: the bottom edge.
 *
 * #GooCanvasBounds represents the bounding box of an item in the canvas.
 */
typedef struct _GooCanvasBounds GooCanvasBounds;
struct _GooCanvasBounds
{
  gdouble x1, y1, x2, y2;
};


#define GOO_TYPE_CANVAS_ITEM            (goo_canvas_item_get_type ())
#define GOO_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_ITEM, GooCanvasItem))
#define GOO_CANVAS_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_ITEM, GooCanvasItemClass))
#define GOO_IS_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_ITEM))
#define GOO_CANVAS_ITEM_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GOO_TYPE_CANVAS_ITEM, GooCanvasItemIface))


/* Workaround for circular dependencies. Include this file first. */
typedef struct _GooCanvas           GooCanvas;
typedef struct _GooCanvasItemModel  GooCanvasItemModel;


/**
 * GooCanvasItem
 *
 * #GooCanvasItem is a typedef used for objects that implement the
 * #GooCanvasItem interface.
 *
 * (There is no actual #GooCanvasItem struct, since it is only an interface.
 * But using '#GooCanvasItem' is more helpful than using '#GObject'.)
 */
typedef struct _GooCanvasItem       GooCanvasItem;


/**
 * GooCanvasItemIface
 * @get_canvas: returns the canvas the item is in.
 * @set_canvas: sets the canvas the item is in.
 * @get_n_children: returns the number of children of the item.
 * @get_child: returns the child at the given index.
 * @request_update: requests that an update is scheduled.
 * @add_child: adds a child.
 * @move_child: moves a child up or down the stacking order.
 * @remove_child: removes a child.
 * @get_child_property: gets a child property of a given child item,
 *  e.g. the "row" or "column" property of an item in a #GooCanvasTable.
 * @set_child_property: sets a child property for a given child item.
 * @get_transform_for_child: gets the transform used to lay out a given child.
 * @get_parent: gets the item's parent.
 * @set_parent: sets the item's parent.
 * @get_bounds: gets the bounds of the item.
 * @get_items_at: gets all the items at the given point.
 * @update: updates the item, if needed. It recalculates the bounds of the item
 *  and requests redraws of parts of the canvas if necessary.
 * @paint: renders the item to the given cairo context.
 * @get_requested_area: returns the requested area of the item, in its parent's
 *  coordinate space. This is only used for items in layout containers such as
 *  #GooCanvasTable.
 * @allocate_area: allocates the item's area, in its parent's coordinate space.
 *  The item must recalculate its bounds and request redraws of parts of the
 *  canvas if necessary. This is only used for items in layout containers such
 *  as #GooCanvasTable.
 * @get_transform: gets the item's transformation matrix.
 * @set_transform: sets the item's transformation matrix.
 * @get_style: gets the item's style.
 * @set_style: sets the item's style.
 * @is_visible: returns %TRUE if the item is currently visible.
 * @get_requested_height: returns the requested height of the item,
 *  given a particular allocated width, using the parent's coordinate space.
 * @get_model: gets the model that the canvas item is viewing.
 * @set_model: sets the model that the canvas item will view.
 * @enter_notify_event: signal emitted when the mouse enters the item.
 * @leave_notify_event: signal emitted when the mouse leaves the item.
 * @motion_notify_event: signal emitted when the mouse moves within the item.
 * @button_press_event: signal emitted when a mouse button is pressed within
 *  the item.
 * @button_release_event: signal emitted when a mouse button is released.
 * @focus_in_event: signal emitted when the item receices the keyboard focus.
 * @focus_out_event: signal emitted when the item loses the keyboard focus.
 * @key_press_event: signal emitted when a key is pressed.
 * @key_release_event: signal emitted when a key is released.
 * @grab_broken_event: signal emitted when a grab that the item has is lost.
 * @child_notify: signal emitted when a child property is changed.
 *
 * #GooCanvasItemIFace holds the virtual methods that make up the
 * #GooCanvasItem interface.
 *
 * Simple canvas items only need to implement the get_parent(), set_parent(),
 * get_bounds(), get_items_at(), update() and paint() methods (and also
 * get_requested_area() and allocate_area() if they are going to be used
 * inside a layout container like #GooCanvasTable).
 *
 * Items that support transforms should also implement get_transform() and
 * set_transform(). Items that support styles should implement get_style()
 * and set_style().
 *
 * Container items must implement get_canvas(), set_canvas(),
 * get_n_children(), get_child() and request_update(). Containers that support
 * dynamic changes to their children should implement add_child(),
 * move_child() and remove_child(). Layout containers like #GooCanvasTable
 * may implement get_child_property(), set_child_property() and
 * get_transform_for_child().
 */
typedef struct _GooCanvasItemIface  GooCanvasItemIface;

struct _GooCanvasItemIface
{
  /*< private >*/
  GTypeInterface base_iface;

  /*< public >*/
  /* Virtual methods that group items must implement. */
  GooCanvas*		(* get_canvas)			(GooCanvasItem		*item);
  void			(* set_canvas)			(GooCanvasItem		*item,
							 GooCanvas		*canvas);
  gint			(* get_n_children)		(GooCanvasItem		*item);
  GooCanvasItem*	(* get_child)			(GooCanvasItem		*item,
							 gint			 child_num);
  void			(* request_update)		(GooCanvasItem		*item);

  /* Virtual methods that group items may implement. */
  void			(* add_child)			(GooCanvasItem		*item,
							 GooCanvasItem		*child,
							 gint			 position);
  void			(* move_child)			(GooCanvasItem		*item,
							 gint			 old_position,
							 gint			 new_position);
  void			(* remove_child)		(GooCanvasItem		*item,
							 gint			 child_num);
  void			(* get_child_property)		(GooCanvasItem		*item,
							 GooCanvasItem		*child,
							 guint			 property_id,
							 GValue			*value,
							 GParamSpec		*pspec);
  void			(* set_child_property)		(GooCanvasItem		*item,
							 GooCanvasItem		*child,
							 guint			 property_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
  gboolean		(* get_transform_for_child)	(GooCanvasItem		*item,
							 GooCanvasItem		*child,
							 cairo_matrix_t		*transform);

  /* Virtual methods that all canvas items must implement. */
  GooCanvasItem*	(* get_parent)			(GooCanvasItem		*item);
  void			(* set_parent)			(GooCanvasItem		*item,
							 GooCanvasItem		*parent);
  void			(* get_bounds)			(GooCanvasItem		*item,
							 GooCanvasBounds	*bounds);
  GList*		(* get_items_at)		(GooCanvasItem		*item,
							 gdouble		 x,
							 gdouble		 y,
							 cairo_t		*cr,
							 gboolean		 is_pointer_event,
							 gboolean		 parent_is_visible,
							 GList                  *found_items);
  void			(* update)			(GooCanvasItem		*item,
							 gboolean		 entire_tree,
							 cairo_t		*cr,
							 GooCanvasBounds	*bounds);
  void			(* paint)			(GooCanvasItem		*item,
							 cairo_t		*cr,
							 const GooCanvasBounds	*bounds,
							 gdouble		 scale);

  gboolean		(* get_requested_area)		(GooCanvasItem		*item,
							 cairo_t		*cr,
							 GooCanvasBounds	*requested_area);
  void			(* allocate_area)		(GooCanvasItem		*item,
							 cairo_t		*cr,
							 const GooCanvasBounds	*requested_area,
							 const GooCanvasBounds	*allocated_area,
							 gdouble		 x_offset,
							 gdouble		 y_offset);

  /* Virtual methods that canvas items may implement. */
  gboolean		(* get_transform)		(GooCanvasItem		*item,
							 cairo_matrix_t		*transform);
  void			(* set_transform)		(GooCanvasItem		*item,
							 const cairo_matrix_t	*transform);
  GooCanvasStyle*	(* get_style)			(GooCanvasItem		*item);
  void			(* set_style)			(GooCanvasItem		*item,
							 GooCanvasStyle		*style);
  gboolean		(* is_visible)			(GooCanvasItem		*item);
  gdouble               (* get_requested_height)	(GooCanvasItem		*item,
							 cairo_t		*cr,
							 gdouble		 width);

  /* Virtual methods that model/view items must implement. */
  GooCanvasItemModel*	(* get_model)			(GooCanvasItem		*item);
  void			(* set_model)			(GooCanvasItem		*item,
							 GooCanvasItemModel	*model);


  /* Signals. */
  gboolean		(* enter_notify_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventCrossing	*event);
  gboolean		(* leave_notify_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventCrossing	*event);
  gboolean		(* motion_notify_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventMotion		*event);
  gboolean		(* button_press_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventButton		*event);
  gboolean		(* button_release_event)	(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventButton		*event);
  gboolean		(* focus_in_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventFocus		*event);
  gboolean		(* focus_out_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventFocus		*event);
  gboolean		(* key_press_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventKey		*event);
  gboolean		(* key_release_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventKey		*event);
  gboolean		(* grab_broken_event)		(GooCanvasItem		*item,
							 GooCanvasItem		*target,
							 GdkEventGrabBroken	*event);
  void			(* child_notify)		(GooCanvasItem		*item,
							 GParamSpec		*pspec);

  /*< private >*/

  /* We might use this in future to support tooltips. */
  gboolean		(* query_tooltip)		(GooCanvasItem		*item,
							 gdouble		 x,
							 gdouble		 y,
							 gboolean		 keyboard_tooltip,
							 gpointer /*GtkTooltip*/		*tooltip);


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


GType              goo_canvas_item_get_type       (void) G_GNUC_CONST;


/*
 * Group functions - these should only be called on container items.
 */
gint               goo_canvas_item_get_n_children (GooCanvasItem   *item);
GooCanvasItem*     goo_canvas_item_get_child      (GooCanvasItem   *item,
						   gint             child_num);
gint               goo_canvas_item_find_child     (GooCanvasItem   *item,
						   GooCanvasItem   *child);
void               goo_canvas_item_add_child      (GooCanvasItem   *item,
						   GooCanvasItem   *child,
						   gint             position);
void               goo_canvas_item_move_child     (GooCanvasItem   *item,
						   gint             old_position,
						   gint             new_position);
void               goo_canvas_item_remove_child   (GooCanvasItem   *item,
						   gint             child_num);

void  goo_canvas_item_get_child_properties        (GooCanvasItem   *item,
						   GooCanvasItem   *child,
						   ...) G_GNUC_NULL_TERMINATED;
void  goo_canvas_item_set_child_properties        (GooCanvasItem   *item,
						   GooCanvasItem   *child,
						   ...) G_GNUC_NULL_TERMINATED;
void  goo_canvas_item_get_child_properties_valist (GooCanvasItem   *item,
						   GooCanvasItem   *child,
						   va_list	    var_args);
void  goo_canvas_item_set_child_properties_valist (GooCanvasItem   *item,
						   GooCanvasItem   *child,
						   va_list	    var_args);

gboolean goo_canvas_item_get_transform_for_child  (GooCanvasItem   *item,
						   GooCanvasItem   *child,
						   cairo_matrix_t  *transform);


/*
 * Item functions - these are safe to call on all items.
 */
GooCanvas*         goo_canvas_item_get_canvas     (GooCanvasItem   *item);
void               goo_canvas_item_set_canvas     (GooCanvasItem   *item,
						   GooCanvas       *canvas);
GooCanvasItem*     goo_canvas_item_get_parent     (GooCanvasItem   *item);
void               goo_canvas_item_set_parent	  (GooCanvasItem   *item,
						   GooCanvasItem   *parent);
void               goo_canvas_item_remove         (GooCanvasItem   *item);
gboolean           goo_canvas_item_is_container   (GooCanvasItem   *item);

void               goo_canvas_item_raise          (GooCanvasItem   *item,
						   GooCanvasItem   *above);
void               goo_canvas_item_lower          (GooCanvasItem   *item,
						   GooCanvasItem   *below);

gboolean           goo_canvas_item_get_transform  (GooCanvasItem   *item,
						   cairo_matrix_t  *transform);
void               goo_canvas_item_set_transform  (GooCanvasItem         *item,
						   const cairo_matrix_t  *transform);
void               goo_canvas_item_set_simple_transform (GooCanvasItem   *item,
							 gdouble          x,
							 gdouble          y,
							 gdouble          scale,
							 gdouble          rotation);

void               goo_canvas_item_translate      (GooCanvasItem   *item,
						   gdouble          tx,
						   gdouble          ty);
void               goo_canvas_item_scale          (GooCanvasItem   *item,
						   gdouble          sx,
						   gdouble          sy);
void               goo_canvas_item_rotate         (GooCanvasItem   *item,
						   gdouble          degrees,
						   gdouble          cx,
						   gdouble          cy);
void               goo_canvas_item_skew_x         (GooCanvasItem   *item,
						   gdouble          degrees,
						   gdouble          cx,
						   gdouble          cy);
void               goo_canvas_item_skew_y         (GooCanvasItem   *item,
						   gdouble          degrees,
						   gdouble          cx,
						   gdouble          cy);

GooCanvasStyle*    goo_canvas_item_get_style      (GooCanvasItem   *item);
void               goo_canvas_item_set_style      (GooCanvasItem   *item,
						   GooCanvasStyle  *style);

void               goo_canvas_item_animate        (GooCanvasItem   *item,
						   gdouble           x,
						   gdouble           y,
						   gdouble           scale,
						   gdouble           degrees,
						   gboolean          absolute,
						   gint             duration,
						   gint             step_time,
						   GooCanvasAnimateType type);
void               goo_canvas_item_stop_animation (GooCanvasItem   *item);



void               goo_canvas_item_get_bounds	  (GooCanvasItem   *item,
						   GooCanvasBounds *bounds);
GList*		   goo_canvas_item_get_items_at   (GooCanvasItem   *item,
						   gdouble          x,
						   gdouble          y,
						   cairo_t         *cr,
						   gboolean         is_pointer_event,
						   gboolean         parent_is_visible,
						   GList           *found_items);
gboolean           goo_canvas_item_is_visible     (GooCanvasItem   *item);

GooCanvasItemModel* goo_canvas_item_get_model	  (GooCanvasItem      *item);
void                goo_canvas_item_set_model	  (GooCanvasItem      *item,
						   GooCanvasItemModel *model);

void               goo_canvas_item_request_update (GooCanvasItem   *item);
void		   goo_canvas_item_ensure_updated (GooCanvasItem   *item);
void               goo_canvas_item_update         (GooCanvasItem   *item,
						   gboolean         entire_tree,
						   cairo_t         *cr,
						   GooCanvasBounds *bounds);
void               goo_canvas_item_paint          (GooCanvasItem         *item,
						   cairo_t               *cr,
						   const GooCanvasBounds *bounds,
						   gdouble                scale);

gboolean	   goo_canvas_item_get_requested_area (GooCanvasItem	*item,
						       cairo_t          *cr,
						       GooCanvasBounds  *requested_area);
gdouble            goo_canvas_item_get_requested_height (GooCanvasItem  *item,
							 cairo_t	*cr,
							 gdouble         width);
void		   goo_canvas_item_allocate_area      (GooCanvasItem	     *item,
						       cairo_t               *cr,
						       const GooCanvasBounds *requested_area,
						       const GooCanvasBounds *allocated_area,
						       gdouble                x_offset,
						       gdouble                y_offset);



/*
 * Functions to support child properties when implementing new canvas items.
 */
void         goo_canvas_item_class_install_child_property (GObjectClass *iclass,
							   guint	 property_id,
							   GParamSpec	*pspec);
GParamSpec*  goo_canvas_item_class_find_child_property	  (GObjectClass	*iclass,
							   const gchar	*property_name);
GParamSpec** goo_canvas_item_class_list_child_properties  (GObjectClass	*iclass,
							   guint	*n_properties);




G_END_DECLS

#endif /* __GOO_CANVAS_ITEM_H__ */
