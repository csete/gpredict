/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2008  Alexandru Csete.

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
#ifndef RADIO_CONF_H
#define RADIO_CONF_H 1

#include <glib.h>


/** \brief Radio types. */
typedef enum {
    RIG_TYPE_RX = 0,    /*!< Rig can only be used as receiver */
    RIG_TYPE_TX,        /*!< Rig can only be used as transmitter */
    RIG_TYPE_TRX,       /*!< Rig can be used as RX/TX (simplex only) */
    RIG_TYPE_DUPLEX     /*!< Rig is a full duplex radio, e.g. IC910 */
} rig_type_t;


/** \brief Radio configuration. */
typedef struct {
    gchar       *name;      /*!< Configuration file name, without .rig. */
    gchar       *host;      /*!< hostname or IP */
    gint         port;      /*!< port number */
    gdouble      lo;        /*!< local oscillator freq in Hz (using double for
                                 compatibility with rest of code) */
    rig_type_t   type;      /*!< Radio type */
    gboolean     ptt;       /*!< Flag indicating that we should read PTT status (needed for RX, TX, and TRX) */
} radio_conf_t;


gboolean radio_conf_read (radio_conf_t *conf);
void radio_conf_save (radio_conf_t *conf);


#endif
