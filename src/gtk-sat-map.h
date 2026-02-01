#ifndef __GTK_SAT_MAP_H__
#define __GTK_SAT_MAP_H__ 1

#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gi18n.h>
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
    GSList         *lines;      /*!< List of line segments (stored as point arrays) */
} ground_track_t;

/**
 * Satellite object.
 *
 * This data structure represents a satellite object on the map. It consists of a
 * small square representing the position, a label showing the satellite name, and
 * the range circle. The range circle can have one or two parts, depending on
 * whether it is split or not. The oldrcnum and newrcnum fields are used for
 * keeping track of whether the range circle has one or two parts.
 *
 */
typedef struct {
    /* flags */
    gboolean        selected;   /*!< Is satellite selected? */
    gboolean        showtrack;  /*!< Show ground track */
    gboolean        showcov;    /*!< Show coverage area. FIXME: redundant*/
    gboolean        istarget;   /*!< is this object the target */

    /* graphical elements - stored as coordinates for Cairo drawing */
    gfloat          x;          /*!< X position of marker */
    gfloat          y;          /*!< Y position of marker */
    gchar          *nickname;   /*!< Satellite nickname for label */
    gchar          *tooltip;    /*!< Tooltip text */

    /* Range circle points */
    gdouble        *range1_points;  /*!< First part of the range circle points. */
    gint            range1_count;   /*!< Number of points in range1. */
    gdouble        *range2_points;  /*!< Second part of the range circle points. */
    gint            range2_count;   /*!< Number of points in range2. */

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

    GtkWidget      *canvas;     /*!< The drawing area widget. */

    gdouble         left_side_lon;      /*!< Left-most longitude (used when center is not 0 lon). */

    /* Text elements */
    gchar          *locnam_text; /*!< Location name text. */
    gchar          *curs_text;   /*!< Cursor tracking text. */
    gchar          *next_text;   /*!< Next event text. */
    gchar          *sel_text;    /*!< Text showing info about the selected satellite. */

    /* Grid line positions (calculated during draw) */
    gboolean        grid_lines_valid;  /*!< Whether grid lines need recalculation */

    /* Terminator points */
    gdouble        *terminator_points;  /*!< Terminator polyline points. */
    gint            terminator_count;   /*!< Number of terminator points. */
    gdouble         terminator_last_tstamp;     /*!< Timestamp of the last terminator drawn. */

    gdouble         naos;       /*!< Next event time. */
    gint            ncat;       /*!< Next event catnum. */

    gdouble         tstamp;     /*!< Time stamp for calculations; set by GtkSatModule */

    GKeyFile       *cfgdata;    /*!< Module configuration data. */
    GHashTable     *sats;       /*!< Pointer to satellites (owned by parent GtkSatModule). */
    qth_t          *qth;        /*!< Pointer to current location. */

    GHashTable     *obj;        /*!< Satellite objects (sat_map_obj_t) for each satellite. */
    GHashTable     *showtracks; /*!< A hash of satellites to show tracks for. */
    GHashTable     *hidecovs;   /*!< A hash of satellites to hide coverage for. */

    guint           x0;         /*!< X0 of the canvas map. */
    guint           y0;         /*!< Y0 of the canvas map. */
    guint           width;      /*!< Map width. */
    guint           height;     /*!< Map height. */

    guint           refresh;    /*!< Refresh rate. */
    guint           counter;    /*!< Cycle counter. */

    gboolean        satname;    /*!< Show the satellite name. */
    gboolean        satfp;      /*!< Show the satellite footprint. */
    gboolean        satmarker;  /*!< Show the satellite marker. */
    gboolean        show_terminator;    /*!< show solar terminator. */
    gboolean        qthinfo;    /*!< Show the QTH info. */
    gboolean        eventinfo;  /*!< Show info about the next event. */
    gboolean        cursinfo;   /*!< Track the mouse cursor. */
    gboolean        showgrid;   /*!< Show grid on map. */
    gboolean        keepratio;  /*!< Keep map aspect ratio. */
    gboolean        resize;     /*!< Flag indicating that the map has been resized. */

    gchar          *infobgd;    /*!< Background color of info text. */
    guint32         col_qth;    /*!< QTH marker color. */
    guint32         col_info;   /*!< Info text color. */
    guint32         col_grid;   /*!< Grid color. */
    guint32         col_tick;   /*!< Tick color. */
    guint32         col_sat;    /*!< Satellite color. */
    guint32         col_sat_sel; /*!< Selected satellite color. */
    guint32         col_shadow; /*!< Shadow color. */
    guint32         col_track;  /*!< Track color. */
    guint32         col_terminator; /*!< Terminator color. */

    GdkPixbuf      *origmap;    /*!< Original map kept here for high quality scaling. */
    GdkPixbuf      *map;        /*!< Scaled map for current size. */

    gchar          *font;       /*!< Default font name */

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
