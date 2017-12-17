#ifndef __GTK_SAT_MODULE_H__
#define __GTK_SAT_MODULE_H__ 1

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "qth-data.h"
#include "gtk-sat-data.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/** The state of a module */
typedef enum {
    GTK_SAT_MOD_STATE_DOCKED = 0,       /*!< The module is docked into the notebook. */
    GTK_SAT_MOD_STATE_WINDOW,   /*!< The module is in it's own window. */
    GTK_SAT_MOD_STATE_FULLSCREEN        /*!< The module is in FULLSCREEN mode :-) */
} gtk_sat_mod_state_t;

/** View types */
typedef enum {
    GTK_SAT_MOD_VIEW_LIST = 0,  /*!< GtkSatList */
    GTK_SAT_MOD_VIEW_MAP,       /*!< GtkSatMap */
    GTK_SAT_MOD_VIEW_POLAR,     /*!< GtkPolarView */
    GTK_SAT_MOD_VIEW_SINGLE,    /*!< GtkSingleSat */
    GTK_SAT_MOD_VIEW_EVENT,     /*!< GtkEventList */
    GTK_SAT_MOD_VIEW_NUM,       /*!< Number of modules */
} gtk_sat_mod_view_t;


#define GTK_TYPE_SAT_MODULE         (gtk_sat_module_get_type ())
#define GTK_SAT_MODULE(obj)         G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                    gtk_sat_module_get_type (),     \
                                    GtkSatModule)
#define GTK_SAT_MODULE_CLASS(klass) G_TYPE_CHECK_CLASS_CAST (klass,\
                                    gtk_sat_module_get_type (),\
                                    GtkSatModuleClass)
#define IS_GTK_SAT_MODULE(obj)      G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sat_module_get_type ())

typedef struct _gtk_sat_module GtkSatModule;
typedef struct _GtkSatModuleClass GtkSatModuleClass;

struct _gtk_sat_module {
    GtkBox          vbox;

    gchar          *name;       /*!< The module name */

    GtkWidget      *popup_button;       /*!< popup menu button. */
    GtkWidget      *close_button;       /*!< The close button */

    GtkWidget      *win;        /*!< Window when module is not docked */

    GtkWidget      *rotctrlwin; /*!< Rotator controller window */
    GtkWidget      *rotctrl;    /*!< Rotator controller widget */
    GtkWidget      *rigctrlwin; /*!< Radio controller window */
    GtkWidget      *rigctrl;    /*!< Radio controller widget */
    GtkWidget      *skgwin;     /*!< Sky at glance window */
    GtkWidget      *skg;        /*!< Sky at glance widget */
    gdouble         lastSkgUpd; /*!< Daynum of last GtkSkyGlance update */
    qth_small_t     lastSkgUpdqth;      /*!< QTH information for last GtkSkyGlance update. */

    GtkWidget      *header;
    guint           head_count;
    guint           head_timeout;
    guint           event_count;
    guint           event_timeout;

    /* layout and children */
    guint          *grid;       /*!< The grid layout array [(type,left,right,top,bottom),...] */
    guint           nviews;     /*!< The number of views */
    GSList         *views;      /*!< Pointers to the views */

    GKeyFile       *cfgdata;    /*!< Configuration data. */
    qth_t          *qth;        /*!< QTH information. */
    qth_small_t     qth_event;  /*!< QTH information for last AOS/LOS update. */
    GHashTable     *satellites; /*!< Satellites. */

    guint32         timeout;    /*!< Timeout value [msec] */

    gtk_sat_mod_state_t state;  /*!< The state of the module. */

    guint           timerid;    /*!< The timeout ID (FIXME: REMOVE) */
    GMutex          busy;       /*!< Flag indicating whether timeout has
                                   finished or not. Also used for blocking
                                   the module during TLE update. */

    /* time keeping */
    gdouble         rtNow;      /*!< Real-time in this cycle */
    gdouble         rtPrev;     /*!< Real-time in previous cycle */

    gint            throttle;   /*!< Time throttle. */
    gdouble         tmgPdnum;   /*!< Daynum at previous update. */
    gdouble         tmgCdnum;   /*!< Daynum at current update. */
    gboolean        tmgActive;  /*!< Flag indicating whether time mgr is active */
    GtkWidget      *tmgFactor;  /*!< Spin button for throttle value selection 2..10 */
    GtkWidget      *tmgCal;     /*!< Calendar widget for selecting date */
    GtkWidget      *tmgHour;    /*!< Spin button for setting the hour */
    GtkWidget      *tmgMin;     /*!< Spin button for setting the minutes */
    GtkWidget      *tmgSec;     /*!< Spin button for setting the seconds */
    GtkWidget      *tmgMsec;    /*!< Spin button for setting the milliseconds */
    GtkWidget      *tmgSlider;  /*!< Slider for manual time "dragging" */
    GtkWidget      *tmgStop;    /*!< Stop button; throttle = 0 */
    GtkWidget      *tmgFwd;     /*!< Forward */
    GtkWidget      *tmgBwd;     /*!< Backward */
    GtkWidget      *tmgReset;   /*!< Reset button */
    GtkWidget      *tmgWin;     /*!< Window containing the widgets. */
    GtkWidget      *tmgState;   /*!< Status label indicating RT/SRT/MAN */

    gboolean        reset;      /*!< Flag indicating whether time reset is in progress */

    /* auto-tracking */
    gint            target;     /*!< Target satellite */
    gboolean        autotrack;  /*!< Whether automatic tracking is enabled */

    /* location structure */
    struct gps_data_t *gps_data;        /*!< GPSD data structure */
};

struct _GtkSatModuleClass {
    GtkBoxClass     parent_class;
};

GType           gtk_sat_module_get_type(void);
GtkWidget      *gtk_sat_module_new(const gchar * cfgfile);

void            gtk_sat_module_close_cb(GtkWidget * button, gpointer data);
void            gtk_sat_module_config_cb(GtkWidget * button, gpointer data);

void            gtk_sat_module_reload_sats(GtkSatModule * module);
void            gtk_sat_module_reconf(GtkSatModule * module, gboolean local);
void            gtk_sat_module_select_sat(GtkSatModule * module, gint catnum);

void            gtk_sat_module_fix_size(GtkWidget * module);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* __GTK_SAT_MODULE_H__ */
