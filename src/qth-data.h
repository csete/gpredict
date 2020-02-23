#ifndef __QTH_DATA_H__
#define __QTH_DATA_H__ 1

#include <glib.h>
#include "sgpsdp/sgp4sdp4.h"

/** QTH data structure in human readable form. */
typedef struct {
    gchar          *name;       /*!< Name, eg. callsign. */
    gchar          *loc;        /*!< Location, eg City, Country. */
    gchar          *desc;       /*!< Short description. */
    gdouble         lat;        /*!< Latitude in dec. deg. North. */
    gdouble         lon;        /*!< Longitude in dec. deg. East. */
    gint            alt;        /*!< Altitude above sea level in meters. */
    gchar          *qra;        /*!< QRA locator */
    gchar          *wx;         /*!< Weather station code (4 chars). */
    gint            type;       /*!< QTH type (static,gpsd). */
    gchar          *gpsd_server;        /*!< GPSD Server name. */
    gint            gpsd_port;  /*!< GPSD Server port. */
    gdouble         gpsd_update;        /*!< Time last GPSD update was received. */
    gdouble         gpsd_connected;     /*!< Time last GPSD update was last attempted to connect. */
    struct gps_data_t *gps_data;        /*!< gpsd data structure. */
    GKeyFile       *data;       /*!< Raw data from cfg file. */
} qth_t;

/** Compact QTH data structure for tagging data and comparing. */
typedef struct {
    gdouble         lat;        /*!< Latitude in dec. deg. North. */
    gdouble         lon;        /*!< Longitude in dec. deg. East. */
    gint            alt;        /*!< Altitude above sea level in meters. */
} qth_small_t;

typedef enum {
    QTH_STATIC_TYPE = 0,
    QTH_GPSD_TYPE
} qth_data_type;


gint            qth_data_read(const gchar * filename, qth_t * qth);
gint            qth_data_save(const gchar * filename, qth_t * qth);
void            qth_data_free(qth_t * qth);
gboolean        qth_data_update(qth_t * qth, gdouble t);
gboolean        qth_data_update_init(qth_t * qth);
void            qth_data_update_stop(qth_t * qth);
double          qth_small_dist(qth_t * qth, qth_small_t qth_small);
void            qth_small_save(qth_t * qth, qth_small_t * qth_small);
void            qth_init(qth_t * qth);
void            qth_safe(qth_t * qth);

#endif
