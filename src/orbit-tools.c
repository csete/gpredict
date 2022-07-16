/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.
    Parts are Copyright John A. Magliacane, KD2BD 1991-2003 (indicated below)

    Authors: Alexandru Csete <oz9aec@gmail.com>
             John A. Magliacane, KD2BD.

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

#include <glib.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "orbit-tools.h"



orbit_type_t
get_orbit_type (sat_t *sat)
{
     orbit_type_t orbit = ORBIT_TYPE_UNKNOWN;

     if (geostationary (sat)) {
          orbit = ORBIT_TYPE_GEO;
     }
     else if (decayed (sat)) {
          orbit = ORBIT_TYPE_DECAYED;
     }
     else {
          orbit = ORBIT_TYPE_UNKNOWN;
     }

     return orbit;
}


/** \brief Determinte whether satellite is in geostationary orbit.
 *  \author John A. Magliacane, KD2BD
 *  \param sat Pointer to satellite data.
 *  \return TRUE if the satellite appears to be in geostationary orbit,
 *          FALSE otherwise.
 *
 * A satellite is in geostationary orbit if
 *
 *     fabs (sat.meanmotion - 1.0027) < 0.0002
 *
 * Note: Apparently, the mean motion can deviate much more from 1.0027 than 0.0002
 */
gboolean
geostationary  (sat_t *sat)
{
     if (fabs (sat->meanmo - 1.0027) < 0.0002)
          return TRUE;
     else
          return FALSE;
}


/** \brief Determine whether satellite has decayed.
 *  \author John A. Magliacane, KD2BD
 *  \author Alexandru Csete, OZ9AEC
 *  \param sat Pointer to satellite data.
 *  \return TRUE if the satellite appears to have decayed, FALSE otherwise.
 *  \bug Modified version of the predict code but it is not tested.
 *
 * A satellite is decayed if
 *
 *    satepoch + ((16.666666 - sat.meanmo) / (10.0*fabs(sat.drag))) < "now"
 *
 */
gboolean
decayed        (sat_t *sat)
{
    
#if 0
    /*this code was used for debugging basically it print out the 
      time of decay for comparison to other sources.
    */
    time_t t;
    gdouble eol;
    char something[100];
    eol=sat->jul_epoch + ((16.666666 - sat->meanmo) / 
                          (10.0 * fabs (sat->tle.xndt2o/(twopi/xmnpda/xmnpda))));
    /* convert julian date to struct tm */
    t = (eol - 2440587.5)*86400.;
    strftime(something,100,"%F %R",gmtime(&t));
    printf("%s Decayed at %s %f\n",sat->nickname,something,eol);
#endif

    /* tle.xndt2o/(twopi/xmnpda/xmnpda) is the value before converted the 
        value matches up with the value in predict 2.2.3 */

     if (sat->jul_epoch + ((16.666666 - sat->meanmo) / 
                           (10.0 * fabs (sat->tle.xndt2o/(twopi/xmnpda/xmnpda)))) < sat->jul_utc)
          return TRUE;
     else
          return FALSE;


}


/** \brief Determine whether satellite ever reaches AOS.
 *  \author John A. Magliacane, KD2BD
 *  \author Alexandru Csete, OZ9AEC
 *  \param sat Pointer to satellite data.
 *  \return TRUE if the satellite will reach AOS, FALSE otherwise.
 *
 */
gboolean
has_aos        (sat_t *sat, qth_t *qth)
{
     double lin, sma, apogee;
     gboolean retcode = FALSE;

     /* FIXME */
     /*the first condition takes care of geostationary satellites and 
       decayed satellites.  The second deals with LEOS from the original 
       predict code.  However, nothing correctly handles geos with poor 
       station keeping that are tracing a figure 8.
     */

     if ((sat->otype == ORBIT_TYPE_GEO) || (decayed(sat))) {
         retcode = FALSE;
     } else {

         if (sat->meanmo == 0.0) {
             retcode = FALSE;
         }
         else {
             
             /* xincl is already in RAD by select_ephemeris */
             lin = sat->tle.xincl;
             if (lin >= pio2)
                 lin = pi - lin;
             
             sma = 331.25 * exp(log(1440.0/sat->meanmo) * (2.0/3.0));
             apogee = sma * (1.0 + sat->tle.eo) - xkmper;
             
             if ((acos(xkmper/(apogee+xkmper))+(lin)) > fabs(qth->lat*de2ra))
                 retcode = TRUE;
             else
                 retcode = FALSE;
             
         }
     }
     return retcode;
}
