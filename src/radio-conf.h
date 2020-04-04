/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2015  Alexandru Csete.

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
    RIG_TYPE_RX = 0,            /*!< Rig can only be used as receiver */
    RIG_TYPE_TX,                /*!< Rig can only be used as transmitter */
    RIG_TYPE_TRX,               /*!< Rig can be used as RX/TX (half-duplex only) */
    RIG_TYPE_DUPLEX,            /*!< Rig is a full duplex radio, e.g. IC910 */
    RIG_TYPE_TOGGLE_AUTO,       /*!< Special mode for FT-817, 857 and 897 using auto T/R switch */
    RIG_TYPE_TOGGLE_MAN         /*!< Special mode for FT-817, 857 and 897 using manual T/R switch */
} rig_type_t;

typedef enum {
    PTT_TYPE_NONE = 0,          /*!< Don't read PTT */
    PTT_TYPE_CAT,               /*!< Read PTT using get_ptt CAT command */
    PTT_TYPE_DCD                /*!< Read PTT using get_dcd */
} ptt_type_t;

typedef enum {
    VFO_NONE = 0,
    VFO_A,
    VFO_B,
    VFO_MAIN,
    VFO_SUB
} vfo_t;

/** \brief Radio configuration. */
typedef struct {
    gchar          *name;       /*!< Configuration file name, without .rig. */
    gchar          *host;       /*!< hostname or IP */
    gint            port;       /*!< port number */
    gint            cycle;      /*!< cycle period in msec */
    gdouble         lo;         /*!< local oscillator freq in Hz (using double for
                                   compatibility with rest of code). Downlink. */
    gdouble         loup;       /*!< local oscillator freq in Hz for uplink. */
    rig_type_t      type;       /*!< Radio type */
    ptt_type_t      ptt;        /*!< PTT type (needed for RX, TX, and TRX) */
    vfo_t           vfoDown;    /*!< Downlink VFO for full-duplex radios */
    vfo_t           vfoUp;      /*!< Uplink VFO for full-duplex radios */

    gboolean        signal_aos; /*!< Send AOS notification to RIG */
    gboolean        signal_los; /*!< Send LOS notification to RIG */
} radio_conf_t;


gboolean        radio_conf_read(radio_conf_t * conf);
void            radio_conf_save(radio_conf_t * conf);


#endif
