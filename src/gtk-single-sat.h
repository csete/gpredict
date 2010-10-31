/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
#ifndef __GTK_SINGLE_SAT_H__
#define __GTK_SINGLE_SAT_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gtk-sat-module.h"
#include "gtk-sat-data.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** \brief Symbolic references to columns */
typedef enum {
     SINGLE_SAT_FIELD_AZ = 0,      /*!< Azimuth. */
     SINGLE_SAT_FIELD_EL,          /*!< Elvation. */
     SINGLE_SAT_FIELD_DIR,         /*!< Direction, satellite on its way up or down. */
     SINGLE_SAT_FIELD_RA,          /*!< Right Ascension. */
     SINGLE_SAT_FIELD_DEC,         /*!< Declination. */
     SINGLE_SAT_FIELD_RANGE,       /*!< Range. */
     SINGLE_SAT_FIELD_RANGE_RATE,  /*!< Range rate. */
     SINGLE_SAT_FIELD_NEXT_EVENT,  /*!< Next event AOS or LOS depending on El. */
     SINGLE_SAT_FIELD_AOS,         /*!< Next AOS regardless of El. */
     SINGLE_SAT_FIELD_LOS,         /*!< Next LOS regardless of El. */
     SINGLE_SAT_FIELD_LAT,         /*!< Latitude. */
     SINGLE_SAT_FIELD_LON,         /*!< Longitude. */
     SINGLE_SAT_FIELD_SSP,         /*!< Sub satellite point grid square */
     SINGLE_SAT_FIELD_FOOTPRINT,   /*!< Footprint. */
     SINGLE_SAT_FIELD_ALT,         /*!< Altitude. */
     SINGLE_SAT_FIELD_VEL,         /*!< Velocity. */
     SINGLE_SAT_FIELD_DOPPLER,     /*!< Doppler shift at 100 MHz.*/
     SINGLE_SAT_FIELD_LOSS,        /*!< Path Loss at 100 MHz. */
     SINGLE_SAT_FIELD_DELAY,       /*!< Signal delay */
     SINGLE_SAT_FIELD_MA,          /*!< Mean Anomaly. */
     SINGLE_SAT_FIELD_PHASE,       /*!< Phase. */
     SINGLE_SAT_FIELD_ORBIT,       /*!< Orbit Number. */
     SINGLE_SAT_FIELD_VISIBILITY,  /*!< Visibility. */
     SINGLE_SAT_FIELD_NUMBER
} single_sat_field_t;



/** \brief Fieldumn Flags */
typedef enum {
     SINGLE_SAT_FLAG_AZ         = 1 << SINGLE_SAT_FIELD_AZ,          /*!< Azimuth. */
     SINGLE_SAT_FLAG_EL         = 1 << SINGLE_SAT_FIELD_EL,          /*!< Elvation. */
     SINGLE_SAT_FLAG_DIR        = 1 << SINGLE_SAT_FIELD_DIR,         /*!< Direction */
     SINGLE_SAT_FLAG_RA         = 1 << SINGLE_SAT_FIELD_RA,          /*!< Right Ascension. */
     SINGLE_SAT_FLAG_DEC        = 1 << SINGLE_SAT_FIELD_DEC,         /*!< Declination. */
     SINGLE_SAT_FLAG_RANGE      = 1 << SINGLE_SAT_FIELD_RANGE,       /*!< Range. */
     SINGLE_SAT_FLAG_RANGE_RATE = 1 << SINGLE_SAT_FIELD_RANGE_RATE,  /*!< Range rate. */
     SINGLE_SAT_FLAG_NEXT_EVENT = 1 << SINGLE_SAT_FIELD_NEXT_EVENT,  /*!< Next event. */
     SINGLE_SAT_FLAG_AOS        = 1 << SINGLE_SAT_FIELD_AOS,         /*!< Next AOS. */
     SINGLE_SAT_FLAG_LOS        = 1 << SINGLE_SAT_FIELD_LOS,         /*!< Next LOS. */
     SINGLE_SAT_FLAG_LAT        = 1 << SINGLE_SAT_FIELD_LAT,         /*!< Latitude. */
     SINGLE_SAT_FLAG_LON        = 1 << SINGLE_SAT_FIELD_LON,         /*!< Longitude. */
     SINGLE_SAT_FLAG_SSP        = 1 << SINGLE_SAT_FIELD_SSP,         /*!< SSP grid square */
     SINGLE_SAT_FLAG_FOOTPRINT  = 1 << SINGLE_SAT_FIELD_FOOTPRINT,   /*!< Footprint. */
     SINGLE_SAT_FLAG_ALT        = 1 << SINGLE_SAT_FIELD_ALT,         /*!< Altitude. */
     SINGLE_SAT_FLAG_VEL        = 1 << SINGLE_SAT_FIELD_VEL,         /*!< Velocity. */
     SINGLE_SAT_FLAG_DOPPLER    = 1 << SINGLE_SAT_FIELD_DOPPLER,     /*!< Doppler shift. */
     SINGLE_SAT_FLAG_LOSS       = 1 << SINGLE_SAT_FIELD_LOSS,        /*!< Path Loss. */
     SINGLE_SAT_FLAG_DELAY      = 1 << SINGLE_SAT_FIELD_DELAY,       /*!< Delay */
     SINGLE_SAT_FLAG_MA         = 1 << SINGLE_SAT_FIELD_MA,          /*!< Mean Anomaly. */
     SINGLE_SAT_FLAG_PHASE      = 1 << SINGLE_SAT_FIELD_PHASE,       /*!< Phase. */
     SINGLE_SAT_FLAG_ORBIT      = 1 << SINGLE_SAT_FIELD_ORBIT,       /*!< Orbit Number. */
     SINGLE_SAT_FLAG_VISIBILITY = 1 << SINGLE_SAT_FIELD_VISIBILITY   /*!< Visibility. */
} single_sat_flag_t;



#define GTK_TYPE_SINGLE_SAT          (gtk_single_sat_get_type ())
#define GTK_SINGLE_SAT(obj)          GTK_CHECK_CAST (obj,\
                                         gtk_single_sat_get_type (),\
                                   GtkSingleSat)

#define GTK_SINGLE_SAT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass,\
                                    gtk_single_sat_get_type (),\
                                    GtkSingleSatClass)

#define IS_GTK_SINGLE_SAT(obj)       GTK_CHECK_TYPE (obj, gtk_single_sat_get_type ())


typedef struct _gtk_single_sat      GtkSingleSat;
typedef struct _GtkSingleSatClass   GtkSingleSatClass;


struct _gtk_single_sat
{
     GtkVBox vbox;
    
     
     GtkWidget    *header;       /*!< Header label, ie. satellite name. */
     
     GtkWidget    *labels[SINGLE_SAT_FIELD_NUMBER];  /*!< GtkLabels displaying the data. */
     
     GtkWidget    *swin;         /*!< scrolled window. */
     GtkWidget    *table;        /*!< table. */
     
     GtkWidget    *popup_button; /*!< Popup button. */
                                
     
     GKeyFile     *cfgdata;      /*!< Configuration data. */
     GSList       *sats;         /*!< Satellites. */
     qth_t        *qth;          /*!< Pointer to current location. */
                                
     
     guint32       flags;        /*!< Flags indicating which columns are visible. */
     guint         refresh;      /*!< Refresh rate. */
     guint         counter;      /*!< cycle counter. */
     gint          selected;     /*!< index of selected sat. */

     gdouble       tstamp;       /*!< time stamp of calculations; update by GtkSatModule */
     
     void (* update) (GtkWidget *widget);  /*!< update function */
};

struct _GtkSingleSatClass
{
     GtkVBoxClass parent_class;
};



GtkType        gtk_single_sat_get_type        (void);
GtkWidget*     gtk_single_sat_new             (GKeyFile   *cfgdata,
                                                          GHashTable *sats,
                                                          qth_t      *qth,
                                                          guint32     fields);
void           gtk_single_sat_update          (GtkWidget  *widget);
void           gtk_single_sat_reconf          (GtkWidget  *widget,
                                                          GKeyFile   *newcfg,
                                                          GHashTable *sats,
                                                          qth_t      *qth,
                                                          gboolean    local);


void gtk_single_sat_reload_sats (GtkWidget *single_sat, GHashTable *sats);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_SINGLE_SAT_H__ */
