/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2007  Alexandru Csete.

    Authors: Alexandru Csete <oz9aec@gmail.com>

    Comments, questions and bugreports should be submitted via
    http://sourceforge.net/projects/groundstation/
    More details can be found at the project home page:

            http://groundstation.sourceforge.net/
 
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



/** \brief RS232 control line usage definitions. */
typedef enum {
    LINE_UNDEF = 0,     /*!< Undefined. */
    LINE_OFF,           /*!< Line should be permanently OFF. */
    LINE_ON,            /*!< Line should be permanently ON. */
    LINE_PTT,           /*!< Line used for PTT control. */
    LINE_CW             /*!< Line used for CW keying. */
} ctrl_stat_t;


/** \brief Radio type definitions. */
typedef enum {
    RADIO_TYPE_RX = 0,      /*!< Radio used as receiver only. */
    RADIO_TYPE_TX,          /*!< Radio used as TX only. */
    RADIO_TYPE_TRX,         /*!< Radio use as both TX and RX. */
    RADIO_TYPE_FULL_DUP     /*!< Full duplex radio. */
} radio_type_t;

/** \brief Radio configuration. */
typedef struct {
    gchar       *name;      /*!< Configuration file name, less .rig. */
    gchar       *model;     /*!< Radio model, e.g. ICOM IC-910H. */
    guint        id;        /*!< Hamlib ID. */
    radio_type_t type;      /*!< Radio type. */
    gchar       *port;      /*!< Device name, e.g. /dev/ttyS0. */
    guint        speed;     /*!< Serial speed. */
    guint        civ;       /*!< ICOM CI-V address. */
    ctrl_stat_t  dtr;       /*!< DTR line usage. */
    ctrl_stat_t  rts;       /*!< PTT line usage. */
} radio_conf_t;


gboolean radio_conf_read (radio_conf_t *conf);
void radio_conf_save (radio_conf_t *conf);

#endif
