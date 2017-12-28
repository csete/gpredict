#ifndef __GTK_POLAR_VIEW_H__
#define __GTK_POLAR_VIEW_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <goocanvas.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "predict-tools.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* *INDENT-ON* */

/** \brief Number of time ticks. */
#define TRACK_TICK_NUM 4


#define GTK_POLAR_VIEW(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_polar_view_get_type (), GtkPolarView)
#define GTK_POLAR_VIEW_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gtk_polar_view_get_type (), GtkPolarViewClass)
#define GTK_IS_POLAR_VIEW(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_polar_view_get_type ())
#define GTK_TYPE_POLAR_VIEW          (gtk_polar_view_get_type ())
#define IS_GTK_POLAR_VIEW(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_polar_view_get_type ())

typedef struct _GtkPolarView GtkPolarView;
typedef struct _GtkPolarViewClass GtkPolarViewClass;


/** Satellite object on graph. */
typedef struct {
    gboolean        selected;   /*!< Satellite is selected. */
    gboolean        showtrack;  /*!< Show ground track. */
    gboolean        istarget;   /*!< Is this object the target. */
    pass_t         *pass;       /*!< Details of the current pass. */
    GooCanvasItemModel *marker; /*!< Item showing position of satellite. */
    GooCanvasItemModel *label;  /*!< Item showing the satellite name. */
    GooCanvasItemModel *track;  /*!< Sky track. */
    GooCanvasItemModel *trtick[TRACK_TICK_NUM]; /*!< Time ticks along the sky track */
} sat_obj_t;

#define SAT_OBJ(obj) ((sat_obj_t *)obj)


    /* graph orientation; start at 12
       o'clock and go clockwise */
typedef enum {
    POLAR_VIEW_NESW = 0,        /*!< Normal / usual */
    POLAR_VIEW_NWSE = 1,
    POLAR_VIEW_SENW = 2,
    POLAR_VIEW_SWNE = 3
} polar_view_swap_t;


    /* pole identifier */
typedef enum {
    POLAR_VIEW_POLE_N = 0,
    POLAR_VIEW_POLE_E = 1,
    POLAR_VIEW_POLE_S = 2,
    POLAR_VIEW_POLE_W = 3
} polar_view_pole_t;


struct _GtkPolarView {
    GtkBox          vbox;

    GtkWidget      *canvas;     /*!< The canvas widget */

    GooCanvasItemModel *bgd;
    GooCanvasItemModel *C00, *C30, *C60;        /*!< 0, 30 and 60 deg elevation circles */
    GooCanvasItemModel *hl, *vl;        /*!< horizontal and vertical lines */
    GooCanvasItemModel *N, *S, *E, *W;  /*!< North, South, East and West labels */
    GooCanvasItemModel *locnam; /*!< Location name */
    GooCanvasItemModel *curs;   /*!< cursor tracking text */
    GooCanvasItemModel *next;   /*!< next event text */
    GooCanvasItemModel *sel;    /*!< Text showing info about selected satellite. */

    GHashTable     *showtracks_on;
    GHashTable     *showtracks_off;

    gdouble         naos;       /*!< Next event time */
    gint            ncat;       /*!< Next event catnum */

    gdouble         tstamp;     /*!< Time stamp for calculations; set by GtkSatModule */

    GKeyFile       *cfgdata;    /*!< module configuration data */
    GHashTable     *sats;       /*!< Satellites. */
    qth_t          *qth;        /*!< Pointer to current location. */

    GHashTable     *obj;        /*!< Canvas items representing each visible satellite */

    guint           cx;         /*!< center X */
    guint           cy;         /*!< center Y */
    guint           r;          /*!< radius */
    guint           size;       /*!< Size of the box = min(h,w) */

    guint           refresh;    /*!< Refresh rate. */
    guint           counter;    /*!< cycle counter. */

    polar_view_swap_t swap;

    gboolean        qthinfo;    /*!< Show the QTH info. */
    gboolean        eventinfo;  /*!< Show info about the next event. */
    gboolean        cursinfo;   /*!< Track the mouse cursor. */
    gboolean        extratick;  /*!< Show extra ticks */
    gboolean        showtrack;  /*!< Automatically show sky tracks. */
    gboolean        resize;     /*!< Flag indicating that the view has been resized. */
};

struct _GtkPolarViewClass {
    GtkBoxClass     parent_class;
};


GType           gtk_polar_view_get_type(void);

GtkWidget      *gtk_polar_view_new(GKeyFile * cfgdata,
                                   GHashTable * sats, qth_t * qth);
void            gtk_polar_view_update(GtkWidget * widget);
void            gtk_polar_view_reconf(GtkWidget * widget, GKeyFile * cfgdat);
void            gtk_polar_view_reload_sats(GtkWidget * polv,
                                           GHashTable * sats);
void            gtk_polar_view_select_sat(GtkWidget * widget, gint catnum);
void            gtk_polar_view_create_track(GtkPolarView * pv, sat_obj_t * obj,
                                            sat_t * sat);
void            gtk_polar_view_delete_track(GtkPolarView * pv, sat_obj_t * obj,
                                            sat_t * sat);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif /* __cplusplus */
/* *INDENT-ON* */

#endif /* __GTK_POLAR_VIEW_H__ */
