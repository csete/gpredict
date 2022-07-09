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
/** \brief Satellite visibility calculations. */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-sat-data.h"
#include "sat-vis.h"
#include "sat-cfg.h"


static gchar VIS2CHR[SAT_VIS_NUM] = { '-', 'V', 'D', 'E'};

static gchar *VIS2STR[SAT_VIS_NUM] = {
    N_("Unknown"),
    N_("Visible"),
    N_("Daylight"),
    N_("Eclipsed")
};



/** \brief Calculate satellite visibility.
 *  \param sat The satellite structure.
 *  \param qth The QTH
 *  \param jul_utc The time at which the visibility should be calculated.
 *  \return The visibility code.
 *
 */
sat_vis_t
get_sat_vis (sat_t *sat, qth_t *qth, gdouble jul_utc)
{
    gboolean sat_sun_status;
    gdouble  sun_el;
    gdouble  threshold;
    gdouble  eclipse_depth;
    sat_vis_t vis = SAT_VIS_NONE;
    vector_t zero_vector = {0,0,0,0};
    geodetic_t obs_geodetic;

    /* Solar ECI position vector  */
    vector_t solar_vector=zero_vector;

    /* Solar observed az and el vector  */
    obs_set_t solar_set;

    /* FIXME: could be passed as parameter */
    obs_geodetic.lon = qth->lon * de2ra;
    obs_geodetic.lat = qth->lat * de2ra;
    obs_geodetic.alt = qth->alt / 1000.0;
    obs_geodetic.theta = 0;


    Calculate_Solar_Position (jul_utc, &solar_vector);
    Calculate_Obs (jul_utc, &solar_vector, &zero_vector, &obs_geodetic, &solar_set);

    if (Sat_Eclipsed (&sat->pos, &solar_vector, &eclipse_depth)) {
        /* satellite is eclipsed */
        sat_sun_status = FALSE;
    }
    else {
        /* satellite in sunlight => may be visible */
        sat_sun_status = TRUE;
    }


    if (sat_sun_status) {
        sun_el = Degrees (solar_set.el);
        threshold = (gdouble) sat_cfg_get_int (SAT_CFG_INT_PRED_TWILIGHT_THLD);
        
        if (sun_el <= threshold && sat->el >= 0.0)
            vis = SAT_VIS_VISIBLE;
        else
            vis = SAT_VIS_DAYLIGHT;
    }
    else
        vis = SAT_VIS_ECLIPSED;


    return vis;
}



/** \brief Convert visibility to character code. */
gchar
vis_to_chr       (sat_vis_t vis)
{
    return VIS2CHR[vis];
}


/** \brief Convert visibility to character string.
 *  \param vis The visibility code
 *
 * The returned string must be freed when no longer needed.
 */
gchar *
vis_to_str       (sat_vis_t vis)
{
    return g_strdup (_(VIS2STR[vis]));
}

