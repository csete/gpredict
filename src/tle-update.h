/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.

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
#ifndef TLE_UPDATE_H
#define TLE_UPDATE_H 1

#include <gtk/gtk.h>
#include "sgpsdp/sgp4sdp4.h"


/** TLE format type flags. */
typedef enum {
    TLE_TYPE_NASA = 0           /*!< NASA two-line format (3 lines with name). */
} tle_type_t;


/** TLE auto update frequency. */
typedef enum {
    TLE_AUTO_UPDATE_NEVER = 0,  /*!< No auto-update, just warn after one week. */
    TLE_AUTO_UPDATE_MONTHLY = 1,
    TLE_AUTO_UPDATE_WEEKLY = 2,
    TLE_AUTO_UPDATE_DAILY = 3,
    TLE_AUTO_UPDATE_NUM
} tle_auto_upd_freq_t;


/** Action to perform when it's time to update TLE. */
typedef enum {
    TLE_AUTO_UPDATE_NOACT = 0,  /*!< No action (not a valid option). */
    TLE_AUTO_UPDATE_NOTIFY = 1, /*!< Notify user. */
    TLE_AUTO_UPDATE_GOAHEAD = 2 /*!< Perform unattended update. */
} tle_auto_upd_action_t;


/** Data structure to hold a TLE set. */
typedef struct {
    guint           catnum;     /*!< Catalog number. */
    gdouble         epoch;      /*!< Epoch. */
    gchar          *satname;    /*!< Satellite name. */
    gchar          *line1;      /*!< Line 1. */
    gchar          *line2;      /*!< Line 2. */
    gchar          *srcfile;    /*!< The file where TLE comes from (needed for cat) */
    gboolean        isnew;      /*!< Flag indicating whether sat is new. */
    op_stat_t       status;     /*!< Enum indicating current satellite status. */
} new_tle_t;


/** Data structure to hold local TLE data. */
typedef struct {
    tle_t           tle;        /*!< TLE data. */
    gchar          *filename;   /*!< File name where the TLE data is from */
} loc_tle_t;


void            tle_update_from_files(const gchar * dir,
                                      const gchar * filter,
                                      gboolean silent,
                                      GtkWidget * progress,
                                      GtkWidget * label1, GtkWidget * label2);

void            tle_update_from_network(gboolean silent,
                                        GtkWidget * progress,
                                        GtkWidget * label1,
                                        GtkWidget * label2);

const gchar    *tle_update_freq_to_str(tle_auto_upd_freq_t freq);

#endif
