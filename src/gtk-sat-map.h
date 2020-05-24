#ifndef __GTK_SAT_MAP_H__
#define __GTK_SAT_MAP_H__ 1

#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <goocanvas.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define SAT_MAP_RANGE_CIRCLE_POINTS    180      /*!< Number of points used to plot a satellite range half circle. */

#define GTK_SAT_MAP(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_sat_map_get_type (), GtkSatMap)
#define GTK_SAT_MAP_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gtk_sat_map_get_type (), GtkSatMapClass)
#define GTK_IS_SAT_MAP(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sat_map_get_type ())
#define GTK_TYPE_SAT_MAP          (gtk_sat_map_get_type ())
#define IS_GTK_SAT_MAP(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sat_map_get_type ())

typedef struct _GtkSatMapClass GtkSatMapClass;

/** Structure that define a sub-satellite point. */
typedef struct {
    double          lat;        /*!< Latitude in decimal degrees North. */
    double          lon;        /*!< Longitude in decimal degrees West. */
} ssp_t;

/** Data storage for ground tracks */
typedef struct {
    GSList         *latlon;     /*!< List of ssp_t */
    GSList         *lines;      /*!< List of GooCanvasPolyLine */
} ground_track_t;

/**
 * Satellite object.
 *
 * This data structure represents a satellite object on the map. It consists of a
 * small square representing the position, a label showinf the satellite name, and
 * the range circle. The range circle can have one or two parts, depending on
 * whether it is split or not. The oldrcnum and newrcnum fields are used for
 * keeping track of whether the range circle has one or two parts.
 *
 */
typedef struct {
    /* flags */
    gboolean        selected;   /*!< Is satellite selected? */
    gboolean        showtrack;  /*!< Show ground track */
    gboolean        showcov;    /*!< Show coverage area. */
    gboolean        istarget;   /*!< is this object the target */

    /* graphical elements */
    GooCanvasItemModel *marker; /*!< A small rectangle showing sat pos. */
    GooCanvasItemModel *shadowm;        /*!< Shadow under satellite marker. */
    GooCanvasItemModel *label;  /*!< Satellite name. */
    GooCanvasItemModel *shadowl;        /*!< Shadow under satellite name */
    GooCanvasItemModel *range1; /*!< First part of the range circle. */
    GooCanvasItemModel *range2; /*!< Second part of the range circle. */

    /* book keeping */
    guint           oldrcnum;   /*!< Number of RC parts in prev. cycle. */
    guint           newrcnum;   /*!< Number of RC parts in this cycle. */
    gint            catnum;     /*!< Catalogue number of satellite. */

    ground_track_t  track_data; /*!< Ground track data. */
    long            track_orbit;        /*!< Orbit when the ground track has been updated. */

} sat_map_obj_t;

#define SAT_MAP_OBJ(obj) ((sat_map_obj_t *)obj)

/** The satellite map data structure. */
typedef struct {
    GtkBox          vbox;

    GtkWidget      *canvas;     /*!< The canvas widget. */

    GooCanvasItemModel *map;    /*!< The canvas map item. */

    gdouble         left_side_lon;      /*!< Left-most longitude (used when center is not 0 lon). */

    GooCanvasItemModel *qthmark;        /*!< QTH marker, e.g. small rectangle. */
    GooCanvasItemModel *qthlabel;       /*!< Label showing the QTH name. */

    GooCanvasItemModel *locnam; /*!< Location name. */
    GooCanvasItemModel *curs;   /*!< Cursor tracking text. */
    GooCanvasItemModel *next;   /*!< Next event text. */
    GooCanvasItemModel *sel;    /*!< Text showing info about the selected satellite. */

    GooCanvasItemModel *gridv[11];      /*!< Vertical grid lines, 30 deg resolution. */
    GooCanvasItemModel *gridvlab[11];   /*!< Vertical grid  labels. */
    GooCanvasItemModel *gridh[5];       /*!< Horizontal grid lines, 30 deg resolution. */
    GooCanvasItemModel *gridhlab[5];    /*!< Horizontal grid labels. */

    GooCanvasItemModel *terminator;     /*!< Outline of sun shadow on Earth. */

    gdouble         terminator_last_tstamp;     /*!< Timestamp of the last terminator drawn. Used to prevent redrawing the terminator too often. */

    gdouble         naos;       /*!< Next event time. */
    gint            ncat;       /*!< Next event catnum. */

    gdouble         tstamp;     /*!< Time stamp for calculations; set by GtkSatModule */

    GKeyFile       *cfgdata;    /*!< Module configuration data. */
    GHashTable     *sats;       /*!< Pointer to satellites (owned by parent GtkSatModule). */
    qth_t          *qth;        /*!< Pointer to current location. */

    GHashTable     *obj;        /*!< Canvas items representing each satellite. */
    GHashTable     *showtracks; /*!< A hash of satellites to show tracks for. */
    GHashTable     *hidecovs;   /*!< A hash of satellites to hide coverage for. */

    guint           x0;         /*!< X0 of the canvas map. */
    guint           y0;         /*!< Y0 of the canvas map. */
    guint           width;      /*!< Map width. */
    guint           height;     /*!< Map height. */

    guint           refresh;    /*!< Refresh rate. */
    guint           counter;    /*!< Cycle counter. */

    gboolean        show_terminator;    // show solar terminator
    gboolean        qthinfo;    /*!< Show the QTH info. */
    gboolean        eventinfo;  /*!< Show info about the next event. */
    gboolean        cursinfo;   /*!< Track the mouse cursor. */
    gboolean        showgrid;   /*!< Show grid on map. */
    gboolean        keepratio;  /*!< Keep map aspect ratio. */
    gboolean        resize;     /*!< Flag indicating that the map has been resized. */

    gchar          *infobgd;    /*!< Background color of info text. */

    GdkPixbuf      *origmap;    /*!< Original map kept here for high quality scaling. */

} GtkSatMap;

struct _GtkSatMapClass {
    GtkBoxClass     parent_class;
};

GType           gtk_sat_map_get_type(void);
GtkWidget      *gtk_sat_map_new(GKeyFile * cfgdata,
                                GHashTable * sats, qth_t * qth);
void            gtk_sat_map_update(GtkWidget * widget);
void            gtk_sat_map_reconf(GtkWidget * widget, GKeyFile * cfgdat);
void            gtk_sat_map_lonlat_to_xy(GtkSatMap * m,
                                         gdouble lon, gdouble lat,
                                         gdouble * x, gdouble * y);

void            gtk_sat_map_reload_sats(GtkWidget * satmap, GHashTable * sats);
void            gtk_sat_map_select_sat(GtkWidget * satmap, gint catnum);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
