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
#ifndef __QTH_DATA_H__
#define __QTH_DATA_H__ 1

#include <glib.h>
#include "sgpsdp/sgp4sdp4.h"



/** \brief QTH data structure in human readable form. */
typedef struct {
     gchar    *name;   /*!< Name, eg. callsign. */
     gchar    *loc;    /*!< Location, eg City, Country. */
     gchar    *desc;   /*!< Short description. */
     gdouble   lat;    /*!< Latitude in dec. deg. North. */
     gdouble   lon;    /*!< Longitude in dec. deg. East. */
     gint      alt;    /*!< Altitude above sea level in meters. */
     gchar    *qra;    /*!< QRA locator */
     gchar    *wx;     /*!< Weather station code (4 chars). */

     GKeyFile *data;   /*!< Raw data from cfg file. */
} qth_t;


gint qth_data_read (const gchar *filename, qth_t *qth);
gint qth_data_save (const gchar *filename, qth_t *qth);
void qth_data_free (qth_t *qth);


#endif
