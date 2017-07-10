/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>

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
#ifndef SAT_PASS_DIALOGS_H
#define SAT_PASS_DIALOGS_H 1

#include <glib.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "predict-tools.h"

/** Column definitions for multi-pass listings. */
typedef enum {
    MULTI_PASS_COL_AOS_TIME = 0,        /*!< AOS time. */
    MULTI_PASS_COL_TCA,         /*!< Time of closest approach. */
    MULTI_PASS_COL_LOS_TIME,    /*!< LOS time. */
    MULTI_PASS_COL_DURATION,    /*!< Duration. */
    MULTI_PASS_COL_MAX_EL,      /*!< Maximum elevation. */
    MULTI_PASS_COL_AOS_AZ,      /*!< Azimuth at AOS. */
    MULTI_PASS_COL_MAX_EL_AZ,   /*!< Azimuth at max el. */
    MULTI_PASS_COL_LOS_AZ,      /*!< Azimuth at LOS. */
    MULTI_PASS_COL_ORBIT,       /*!< Orbit number. */
    MULTI_PASS_COL_VIS,         /*!< Visibility. */
    MULTI_PASS_COL_NUMBER
} multi_pass_col_t;

/** Column flags for multi-pass listings. */
typedef enum {
    MULTI_PASS_FLAG_AOS_TIME = 1 << MULTI_PASS_COL_AOS_TIME,    /*!< AOS time. */
    MULTI_PASS_FLAG_TCA = 1 << MULTI_PASS_COL_TCA,      /*!< Time of closest approach. */
    MULTI_PASS_FLAG_LOS_TIME = 1 << MULTI_PASS_COL_LOS_TIME,    /*!< LOS time. */
    MULTI_PASS_FLAG_DURATION = 1 << MULTI_PASS_COL_DURATION,    /*!< Duration. */
    MULTI_PASS_FLAG_MAX_EL = 1 << MULTI_PASS_COL_MAX_EL,        /*!< Maximum elevation. */
    MULTI_PASS_FLAG_AOS_AZ = 1 << MULTI_PASS_COL_AOS_AZ,        /*!< Azimuth at AOS. */
    MULTI_PASS_FLAG_MAX_EL_AZ = 1 << MULTI_PASS_COL_MAX_EL_AZ,  /*!< Azimuth at max el. */
    MULTI_PASS_FLAG_LOS_AZ = 1 << MULTI_PASS_COL_LOS_AZ,        /*!< Azimuth at LOS. */
    MULTI_PASS_FLAG_ORBIT = 1 << MULTI_PASS_COL_ORBIT,  /*!< Orbit number. */
    MULTI_PASS_FLAG_VIS = 1 << MULTI_PASS_COL_VIS       /*!< Visibility. */
} multi_pass_flag_t;

/** Column definition for single-pass listings. */
typedef enum {
    SINGLE_PASS_COL_TIME = 0,
    SINGLE_PASS_COL_AZ,         /*!< Azimuth. */
    SINGLE_PASS_COL_EL,         /*!< Elvation. */
    SINGLE_PASS_COL_RA,         /*!< Right Ascension. */
    SINGLE_PASS_COL_DEC,        /*!< Declination. */
    SINGLE_PASS_COL_RANGE,      /*!< Range. */
    SINGLE_PASS_COL_RANGE_RATE, /*!< Range rate. */
    SINGLE_PASS_COL_LAT,        /*!< Latitude. */
    SINGLE_PASS_COL_LON,        /*!< Longitude. */
    SINGLE_PASS_COL_SSP,        /*!< Sub satellite point grid square */
    SINGLE_PASS_COL_FOOTPRINT,  /*!< Footprint. */
    SINGLE_PASS_COL_ALT,        /*!< Altitude. */
    SINGLE_PASS_COL_VEL,        /*!< Velocity. */
    SINGLE_PASS_COL_DOPPLER,    /*!< Doppler shift at 100 MHz. */
    SINGLE_PASS_COL_LOSS,       /*!< Path Loss at 100 MHz. */
    SINGLE_PASS_COL_DELAY,      /*!< Signal delay */
    SINGLE_PASS_COL_MA,         /*!< Mean Anomaly. */
    SINGLE_PASS_COL_PHASE,      /*!< Phase. */
    SINGLE_PASS_COL_VIS,        /*!< Visibility. */
    SINGLE_PASS_COL_NUMBER
} single_pass_col_t;

/** Column flags for single-pass listings. */
typedef enum {
    SINGLE_PASS_FLAG_TIME = 1 << SINGLE_PASS_COL_TIME,
    SINGLE_PASS_FLAG_AZ = 1 << SINGLE_PASS_COL_AZ,      /*!< Azimuth. */
    SINGLE_PASS_FLAG_EL = 1 << SINGLE_PASS_COL_EL,      /*!< Elvation. */
    SINGLE_PASS_FLAG_RA = 1 << SINGLE_PASS_COL_RA,      /*!< Right Ascension. */
    SINGLE_PASS_FLAG_DEC = 1 << SINGLE_PASS_COL_DEC,    /*!< Declination. */
    SINGLE_PASS_FLAG_RANGE = 1 << SINGLE_PASS_COL_RANGE,        /*!< Range. */
    SINGLE_PASS_FLAG_RANGE_RATE = 1 << SINGLE_PASS_COL_RANGE_RATE,      /*!< Range rate. */
    SINGLE_PASS_FLAG_LAT = 1 << SINGLE_PASS_COL_LAT,    /*!< Latitude. */
    SINGLE_PASS_FLAG_LON = 1 << SINGLE_PASS_COL_LON,    /*!< Longitude. */
    SINGLE_PASS_FLAG_SSP = 1 << SINGLE_PASS_COL_SSP,    /*!< Sub satellite point grid square */
    SINGLE_PASS_FLAG_FOOTPRINT = 1 << SINGLE_PASS_COL_FOOTPRINT,        /*!< Footprint. */
    SINGLE_PASS_FLAG_ALT = 1 << SINGLE_PASS_COL_ALT,    /*!< Altitude. */
    SINGLE_PASS_FLAG_VEL = 1 << SINGLE_PASS_COL_VEL,    /*!< Velocity. */
    SINGLE_PASS_FLAG_DOPPLER = 1 << SINGLE_PASS_COL_DOPPLER,    /*!< Doppler shift at 100 MHz. */
    SINGLE_PASS_FLAG_LOSS = 1 << SINGLE_PASS_COL_LOSS,  /*!< Path Loss at 100 MHz. */
    SINGLE_PASS_FLAG_DELAY = 1 << SINGLE_PASS_COL_DELAY,        /*!< Signal delay */
    SINGLE_PASS_FLAG_MA = 1 << SINGLE_PASS_COL_MA,      /*!< Mean Anomaly. */
    SINGLE_PASS_FLAG_PHASE = 1 << SINGLE_PASS_COL_PHASE,        /*!< Phase. */
    SINGLE_PASS_FLAG_VIS = 1 << SINGLE_PASS_COL_VIS     /*!< Visibility. */
} single_pass_flag_t;

void            show_pass(const gchar * satname, qth_t * qth, pass_t * pass,
                          GtkWidget * toplevel);
void            show_passes(const gchar * satname, qth_t * qth,
                            GSList * passes, GtkWidget * toplevel);

#endif
