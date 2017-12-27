#ifndef __GTK_SINGLE_SAT_H__
#define __GTK_SINGLE_SAT_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "gtk-sat-module.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/** Symbolic references to columns */
typedef enum {
    SINGLE_SAT_FIELD_AZ = 0,    /*!< Azimuth. */
    SINGLE_SAT_FIELD_EL,        /*!< Elvation. */
    SINGLE_SAT_FIELD_DIR,       /*!< Direction, satellite on its way up or down. */
    SINGLE_SAT_FIELD_RA,        /*!< Right Ascension. */
    SINGLE_SAT_FIELD_DEC,       /*!< Declination. */
    SINGLE_SAT_FIELD_RANGE,     /*!< Range. */
    SINGLE_SAT_FIELD_RANGE_RATE,        /*!< Range rate. */
    SINGLE_SAT_FIELD_NEXT_EVENT,        /*!< Next event AOS or LOS depending on El. */
    SINGLE_SAT_FIELD_AOS,       /*!< Next AOS regardless of El. */
    SINGLE_SAT_FIELD_LOS,       /*!< Next LOS regardless of El. */
    SINGLE_SAT_FIELD_LAT,       /*!< Latitude. */
    SINGLE_SAT_FIELD_LON,       /*!< Longitude. */
    SINGLE_SAT_FIELD_SSP,       /*!< Sub satellite point grid square */
    SINGLE_SAT_FIELD_FOOTPRINT, /*!< Footprint. */
    SINGLE_SAT_FIELD_ALT,       /*!< Altitude. */
    SINGLE_SAT_FIELD_VEL,       /*!< Velocity. */
    SINGLE_SAT_FIELD_DOPPLER,   /*!< Doppler shift at 100 MHz. */
    SINGLE_SAT_FIELD_LOSS,      /*!< Path Loss at 100 MHz. */
    SINGLE_SAT_FIELD_DELAY,     /*!< Signal delay */
    SINGLE_SAT_FIELD_MA,        /*!< Mean Anomaly. */
    SINGLE_SAT_FIELD_PHASE,     /*!< Phase. */
    SINGLE_SAT_FIELD_ORBIT,     /*!< Orbit Number. */
    SINGLE_SAT_FIELD_VISIBILITY,        /*!< Visibility. */
    SINGLE_SAT_FIELD_NUMBER
} single_sat_field_t;

/** Fieldnum Flags */
typedef enum {
    SINGLE_SAT_FLAG_AZ = 1 << SINGLE_SAT_FIELD_AZ,      /*!< Azimuth. */
    SINGLE_SAT_FLAG_EL = 1 << SINGLE_SAT_FIELD_EL,      /*!< Elvation. */
    SINGLE_SAT_FLAG_DIR = 1 << SINGLE_SAT_FIELD_DIR,    /*!< Direction */
    SINGLE_SAT_FLAG_RA = 1 << SINGLE_SAT_FIELD_RA,      /*!< Right Ascension. */
    SINGLE_SAT_FLAG_DEC = 1 << SINGLE_SAT_FIELD_DEC,    /*!< Declination. */
    SINGLE_SAT_FLAG_RANGE = 1 << SINGLE_SAT_FIELD_RANGE,        /*!< Range. */
    SINGLE_SAT_FLAG_RANGE_RATE = 1 << SINGLE_SAT_FIELD_RANGE_RATE,      /*!< Range rate. */
    SINGLE_SAT_FLAG_NEXT_EVENT = 1 << SINGLE_SAT_FIELD_NEXT_EVENT,      /*!< Next event. */
    SINGLE_SAT_FLAG_AOS = 1 << SINGLE_SAT_FIELD_AOS,    /*!< Next AOS. */
    SINGLE_SAT_FLAG_LOS = 1 << SINGLE_SAT_FIELD_LOS,    /*!< Next LOS. */
    SINGLE_SAT_FLAG_LAT = 1 << SINGLE_SAT_FIELD_LAT,    /*!< Latitude. */
    SINGLE_SAT_FLAG_LON = 1 << SINGLE_SAT_FIELD_LON,    /*!< Longitude. */
    SINGLE_SAT_FLAG_SSP = 1 << SINGLE_SAT_FIELD_SSP,    /*!< SSP grid square */
    SINGLE_SAT_FLAG_FOOTPRINT = 1 << SINGLE_SAT_FIELD_FOOTPRINT,        /*!< Footprint. */
    SINGLE_SAT_FLAG_ALT = 1 << SINGLE_SAT_FIELD_ALT,    /*!< Altitude. */
    SINGLE_SAT_FLAG_VEL = 1 << SINGLE_SAT_FIELD_VEL,    /*!< Velocity. */
    SINGLE_SAT_FLAG_DOPPLER = 1 << SINGLE_SAT_FIELD_DOPPLER,    /*!< Doppler shift. */
    SINGLE_SAT_FLAG_LOSS = 1 << SINGLE_SAT_FIELD_LOSS,  /*!< Path Loss. */
    SINGLE_SAT_FLAG_DELAY = 1 << SINGLE_SAT_FIELD_DELAY,        /*!< Delay */
    SINGLE_SAT_FLAG_MA = 1 << SINGLE_SAT_FIELD_MA,      /*!< Mean Anomaly. */
    SINGLE_SAT_FLAG_PHASE = 1 << SINGLE_SAT_FIELD_PHASE,        /*!< Phase. */
    SINGLE_SAT_FLAG_ORBIT = 1 << SINGLE_SAT_FIELD_ORBIT,        /*!< Orbit Number. */
    SINGLE_SAT_FLAG_VISIBILITY = 1 << SINGLE_SAT_FIELD_VISIBILITY       /*!< Visibility. */
} single_sat_flag_t;

#define GTK_TYPE_SINGLE_SAT          (gtk_single_sat_get_type ())
#define GTK_SINGLE_SAT(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                        gtk_single_sat_get_type (),\
                                        GtkSingleSat)
#define GTK_SINGLE_SAT_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                        gtk_single_sat_get_type (),\
                                        GtkSingleSatClass)
#define IS_GTK_SINGLE_SAT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_single_sat_get_type ())

typedef struct _gtk_single_sat GtkSingleSat;
typedef struct _GtkSingleSatClass GtkSingleSatClass;

struct _gtk_single_sat {
    GtkBox          vbox;

    GtkWidget      *header;     /*!< Header label, ie. satellite name. */

    GtkWidget      *labels[SINGLE_SAT_FIELD_NUMBER];    /*!< GtkLabels displaying the data. */

    GtkWidget      *swin;       /*!< scrolled window. */
    GtkWidget      *table;      /*!< table. */

    GtkWidget      *popup_button;       /*!< Popup button. */


    GKeyFile       *cfgdata;    /*!< Configuration data. */
    GSList         *sats;       /*!< Satellites. */
    qth_t          *qth;        /*!< Pointer to current location. */


    guint32         flags;      /*!< Flags indicating which columns are visible. */
    guint           refresh;    /*!< Refresh rate. */
    guint           counter;    /*!< cycle counter. */
    guint           selected;   /*!< index of selected sat. */

    gdouble         tstamp;     /*!< time stamp of calculations; update by GtkSatModule */

    void            (*update) (GtkWidget * widget);     /*!< update function */
};

struct _GtkSingleSatClass {
    GtkBoxClass     parent_class;
};

GType           gtk_single_sat_get_type(void);
GtkWidget      *gtk_single_sat_new(GKeyFile * cfgdata,
                                   GHashTable * sats,
                                   qth_t * qth, guint32 fields);
void            gtk_single_sat_update(GtkWidget * widget);
void            gtk_single_sat_reconf(GtkWidget * widget,
                                      GKeyFile * newcfg,
                                      GHashTable * sats,
                                      qth_t * qth, gboolean local);

void            gtk_single_sat_reload_sats(GtkWidget * single_sat,
                                           GHashTable * sats);
void            gtk_single_sat_select_sat(GtkWidget * single_sat, gint catnum);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* __GTK_SINGLE_SAT_H__ */
