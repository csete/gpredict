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
#ifndef SAT_VISIBILITY_H
#define SAT_VISIBILITY_H


#include <glib.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-sat-data.h"



/** \brief Satellite visibility. */
typedef enum {
     SAT_VIS_NONE = 0,     /*!< Unknown/undefined. */
     SAT_VIS_VISIBLE,      /*!< Visible. */
     SAT_VIS_DAYLIGHT,     /*!< Satellite is in daylight. */
     SAT_VIS_ECLIPSED,     /*!< Satellite is eclipsed. */
     SAT_VIS_NUM
} sat_vis_t;





sat_vis_t  get_sat_vis (sat_t *sat, qth_t *qth, gdouble jul_utc);
gchar      vis_to_chr  (sat_vis_t vis);
gchar     *vis_to_str  (sat_vis_t vis);

#endif
