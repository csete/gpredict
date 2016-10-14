/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.
  Copyright (C)  2016       Baris DINC (TA7W)

  Comments, questions and bugreports should be submitted via
  http://sourceforge.net/projects/gpredict/
  More details can be found at the project home page:

  http://gpredict.oz9aec.net/

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, visit http://www.fsf.org/
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
    char               uuid[30];        /*!< uuid */
    int                catnum;          /*!< Catalog number. */
    char               description[99]; /*!< Transponder descriptoion */
    unsigned long long uplink_low;      /*!< Uplink starting frequency */
    unsigned long long uplink_high;     /*!< uplink end frequency  */
    unsigned long long downlink_low;    /*!< downlink starting frequency */
    unsigned long long downlink_high;   /*!< downlink end frequency */
    char               mode[20];        /*!< mode (from modes files) */
    int                invert;          /*!< inverting / noninverting */
    double             baud;            /*!< baudrate */
    int                alive;           /*!< alive or dead */
};


/** \brief Data structure to hold a MODES set. */
struct modes {
    int             id;   /*!< id */
    char            name[99];    /*!< Mode name */
};

/** \brief Data structure to hold local FRQ data. */
//typedef struct {
//    tle_t  frq;       /*!< FRQ data. */
//    gchar *filename;  /*!< File name where the FRQ data is from */
//} loc_frq_t;

/** \brief Data structure to hold a MODE set. */
typedef struct {
    guint    modnum;  /*!< Mode number. */
    gchar   *modname; /*!< Mode description. */
} new_mode_t;

/** \brief Data structure to hold a TRSP list. */
typedef struct {
    guint    catnum;  /*!< Catalog number. */
    guint    numtrsp; /*!< Number of transponders. */
} new_trsp_t;


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
