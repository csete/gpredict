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
#ifndef ROTOR_CONF_H
#define ROTOR_CONF_H 1

#include <glib.h>




/** \brief Rotator type definitions. */
typedef enum {
    ROTOR_TYPE_AZ = 1,      /*!< Azimuth rotator. */
    ROTOR_TYPE_EL = 2,      /*!< Elevation rotator. */
    ROTOR_TYPE_AZEL = 3     /*!< Both azimuth and elevation rotator. */
} rotor_type_t;


/** \brief Rotator configuration. */
typedef struct {
    gchar       *name;      /*!< Configuration file name, less .rot */
    gchar       *model;     /*!< Rotator model. */
    guint        id;        /*!< Hamlib ID. */
    rotor_type_t type;      /*!< Rotator type. */
    gchar       *port;      /*!< Device name, e.g. /dev/ttyS0. */
    guint        speed;     /*!< Serial speed. */
} rotor_conf_t;


gboolean rotor_conf_read (rotor_conf_t *conf);
void rotor_conf_save (rtor_conf_t *conf);

#endif
