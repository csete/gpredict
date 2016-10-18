/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete.

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
#ifndef TRSP_CONF_H
#define TRSP_CONF_H 1

#include <glib.h>

/* NOTE For beacons uplow=uphigh=0 and downlow=downhigh */
typedef struct {
    gchar          *name;       /*!< The name of the transponder (same as config group) */
    gint64          uplow;      /*!< Lower limit of uplink. */
    gint64          uphigh;     /*!< Upper limit of uplink. */
    gint64          downlow;    /*!< Lower limit of downlink. */
    gint64          downhigh;   /*!< Upper limit of donlink. */
    gdouble         baud;       /*!< Baud rate > */
    gboolean        invert;     /*!< Flag indicating whether transponder is inverting. */
    gchar          *mode;       /*!< Mode descriptor. */
} trsp_t;

/* The actual data would then be a singly linked list with pointers to transponder_t structures */

GSList         *read_transponders(guint catnum);
void            write_transponders(guint catnum, GSList * trsplist);
void            free_transponders(GSList * trsplist);

#endif
