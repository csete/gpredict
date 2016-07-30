/*
  Frequency update from SatNogs Services added by Baris DINC (TA7W) July 2016

*/
#ifndef FRQ_UPDATE_H
#define FRQ_UPDATE_H 1

#include <gtk/gtk.h>
#include "sgpsdp/sgp4sdp4.h"


/** \brief FRQ format type flags. */
//typedef enum {
 //   FRQ_TYPE_NASA = 0   /*!< NASA two-line format (3 lines with name). */
//                } frq_type_t;


/** \brief FRQ auto update frequency. */
typedef enum {
    FRQ_AUTO_UPDATE_NEVER = 0,  /*!< No auto-update, just warn after one week. */
    FRQ_AUTO_UPDATE_MONTHLY = 1,
    FRQ_AUTO_UPDATE_WEEKLY = 2,
    FRQ_AUTO_UPDATE_DAILY = 3,
    FRQ_AUTO_UPDATE_NUM
} frq_auto_upd_freq_t;

/** \brief Frequency information is an json array, nxjson does not support json kbject arrays
 *         this struct is used to manipulate the json object array
*/
typedef enum {
    START = 0,                  //starting point
    OBBGN,                      //new obbject starts here
    OBEND,                      //objct end here, next char should be delimiter (comma) or next obect start, or array end
    NXDLM,                      //we have the delimiter, next one will be new object
    FINISH                      //array fineshes here
} m_state;


/** \brief Action to perform when it's time to update FRQ. */
//typedef enum {
//    FRQ_AUTO_UPDATE_NOACT   = 0,  /*!< No action (not a valid option). */
//    FRQ_AUTO_UPDATE_NOTIFY  = 1,  /*!< Notify user. */
//    FRQ_AUTO_UPDATE_GOAHEAD = 2   /*!< Perform unattended update. */
//} frq_auto_upd_action_t;


/** \brief Data structure to hold a FRQ set. */
struct transponder {
    char            uuid[30];   /*!< uuid */
    int             catnum;     /*!< Catalog number. */
    char            description[99];    /*!< Transponder descriptoion */
    int             uplink_low; /*!< Uplink starting frequency */
    int             uplink_high;        /*!< uplink end frequency  */
    int             downlink_low;       /*!< downlink starting frequency */
    int             downlink_high;      /*!< downlink end frequency */
    int             mode_id;    /*!< mode id (from modes files) */
    int             invert;     /*!< inverting / noninverting */
    double          baud;       /*!< baudrate */
    int             alive;      /*!< alive or dead */
};


/** \brief Data structure to hold local FRQ data. */
//typedef struct {
//    tle_t  frq;       /*!< FRQ data. */
//    gchar *filename;  /*!< File name where the FRQ data is from */
//} loc_frq_t;


//void frq_update_from_files (const gchar *dir,
//                            const gchar *filter,
//                            gboolean silent,
//                            GtkWidget *progress,
//                            GtkWidget *label1,
//                            GtkWidget *label2);

void            frq_update_from_network(gboolean silent,
                                        GtkWidget * progress,
                                        GtkWidget * label1,
                                        GtkWidget * label2);

//const gchar *frq_update_freq_to_str (frq_auto_upd_freq_t freq);


#endif
