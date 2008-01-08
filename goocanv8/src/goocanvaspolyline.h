/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvaspolyline.h - polyline item, with optional arrows.
 */
#ifndef __GOO_CANVAS_POLYLINE_H__
#define __GOO_CANVAS_POLYLINE_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"

G_BEGIN_DECLS


/**
 * GooCanvasPoints
 * @coords: the coordinates of the points, in pairs.
 * @num_points: the number of points.
 * @ref_count: the reference count of the struct.
 *
 * #GooCairoPoints represents an array of points.
 */
typedef struct _GooCanvasPoints GooCanvasPoints;
struct _GooCanvasPoints
{
  double *coords;
  int num_points;
  int ref_count;
};

#define GOO_TYPE_CANVAS_POINTS goo_canvas_points_get_type()
GType            goo_canvas_points_get_type (void);
GooCanvasPoints* goo_canvas_points_new      (int              num_points);
GooCanvasPoints* goo_canvas_points_ref      (GooCanvasPoints *points);
void             goo_canvas_points_unref    (GooCanvasPoints *points);


#define NUM_ARROW_POINTS     5		/* number of points in an arrowhead */

typedef struct _GooCanvasPolylineArrowData GooCanvasPolylineArrowData;
struct _GooCanvasPolylineArrowData
{
  /* These are specified in multiples of the line width, e.g. if arrow_width
     is 2 the width of the arrow will be twice the width of the line. */
  gdouble arrow_width, arrow_length, arrow_tip_length;

  gdouble line_start[2], line_end[2];
  gdouble start_arrow_coords[NUM_ARROW_POINTS * 2];
  gdouble end_arrow_coords[NUM_ARROW_POINTS * 2];
};


/* This is the data used by both model and view classes. */
typedef struct _GooCanvasPolylineData   GooCanvasPolylineData;
struct _GooCanvasPolylineData
{
  gdouble *coords;

  GooCanvasPolylineArrowData *arrow_data;

  guint num_points	   : 16;
  guint close_path	   : 1;
  guint start_arrow	   : 1;
  guint end_arrow          : 1;
  guint reconfigure_arrows : 1;
};


#define GOO_TYPE_CANVAS_POLYLINE            (goo_canvas_polyline_get_type ())
#define GOO_CANVAS_POLYLINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_POLYLINE, GooCanvasPolyline))
#define GOO_CANVAS_POLYLINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_POLYLINE, GooCanvasPolylineClass))
#define GOO_IS_CANVAS_POLYLINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_POLYLINE))
#define GOO_IS_CANVAS_POLYLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_POLYLINE))
#define GOO_CANVAS_POLYLINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_POLYLINE, GooCanvasPolylineClass))


typedef struct _GooCanvasPolyline       GooCanvasPolyline;
typedef struct _GooCanvasPolylineClass  GooCanvasPolylineClass;

/**
 * GooCanvasPolyline
 *
 * The #GooCanvasPolyline-struct struct contains private data only.
 */
struct _GooCanvasPolyline
{
  GooCanvasItemSimple parent;

  GooCanvasPolylineData *polyline_data;
};

struct _GooCanvasPolylineClass
{
  GooCanvasItemSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_polyline_get_type       (void) G_GNUC_CONST;

GooCanvasItem*      goo_canvas_polyline_new            (GooCanvasItem      *parent,
							gboolean            close_path,
							gint                num_points,
							...);

GooCanvasItem*      goo_canvas_polyline_new_line       (GooCanvasItem      *parent,
							gdouble             x1,
							gdouble             y1,
							gdouble             x2,
							gdouble             y2,
							...);



#define GOO_TYPE_CANVAS_POLYLINE_MODEL            (goo_canvas_polyline_model_get_type ())
#define GOO_CANVAS_POLYLINE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_POLYLINE_MODEL, GooCanvasPolylineModel))
#define GOO_CANVAS_POLYLINE_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_POLYLINE_MODEL, GooCanvasPolylineModelClass))
#define GOO_IS_CANVAS_POLYLINE_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_POLYLINE_MODEL))
#define GOO_IS_CANVAS_POLYLINE_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_POLYLINE_MODEL))
#define GOO_CANVAS_POLYLINE_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_POLYLINE_MODEL, GooCanvasPolylineModelClass))


typedef struct _GooCanvasPolylineModel       GooCanvasPolylineModel;
typedef struct _GooCanvasPolylineModelClass  GooCanvasPolylineModelClass;

/**
 * GooCanvasPolylineModel
 *
 * The #GooCanvasPolylineModel-struct struct contains private data only.
 */
struct _GooCanvasPolylineModel
{
  GooCanvasItemModelSimple parent_object;

  GooCanvasPolylineData polyline_data;
};

struct _GooCanvasPolylineModelClass
{
  GooCanvasItemModelSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType               goo_canvas_polyline_model_get_type  (void) G_GNUC_CONST;

GooCanvasItemModel* goo_canvas_polyline_model_new      (GooCanvasItemModel *parent,
							gboolean            close_path,
							gint                num_points,
							...);

GooCanvasItemModel* goo_canvas_polyline_model_new_line (GooCanvasItemModel *parent,
							gdouble             x1,
							gdouble             y1,
							gdouble             x2,
							gdouble             y2,
							...);

G_END_DECLS

#endif /* __GOO_CANVAS_POLYLINE_H__ */
