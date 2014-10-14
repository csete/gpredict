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
#include "sat-log.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "tle-tools.h"




/** \brief Convert NASA 2-line orbital element set to tle_t structure.
 *  \param line1 The first line containing the satellite name.
 *  \param line2 The second line.
 *  \param line3 The third line.
 *  \param checksum Flag indicating whether to perform checksum check.
 *  \param tle Pointer to a tle_t structure where the TLE data will be put.
 *  \return TLE_CONV_SUCCESS if the conversion is successful or TLE_CONV_ERROR
 *          if an error has occurred during the conversion. 
 *
 * This function converts NASA 2-line orbital element data (as read from a tle
 * file) into a tle_t structure, which is used all over in gpredict. Note that
 * a standard 2-line data actually consists of three lines, the extra line
 * containing the name of the satellite in a field of 25 characters.
 *
 * The flag checksum can be used to control whether verification of the checksum
 * should be fperformed or not. If the flag is TRUE and the checksum is invalid,
 * an error message is logged and the function returns TLE_CONV_ERROR. The caller
 * can still ignore the error code since the tle structure will be populated;
 * however, the data may be nonsense.
 */
gint
twoline2tle (gchar *line1, gchar *line2, gchar *line3,
             gboolean checksum, tle_t *tle)
{
    /* FIXME */
    (void) checksum; /* avoid unused parameter compiler warning */

    /* check function parameters */
    if G_UNLIKELY((line1 == NULL) || (line2 == NULL) || (line3 == NULL)) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: NULL input data!"), __func__);
        return TLE_CONV_ERROR;
    }
    if G_UNLIKELY(tle == NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: NULL output storage!"), __func__);
        return TLE_CONV_ERROR;
    }



    return TLE_CONV_SUCCESS;
}



/** \brief Convert internal tle_t structure to NASA 2-line oribital element set.
 *  \param tle Pointer to the tle_t structure that holds the data to be
 *             converted.
 *  \param line1 Pointer to unallocated memory where the first line will be
 *               stored. The pointer should be freed when no longer needed.
 *  \param line2 Pointer to unallocated memory where the second line will be
 *               stored. The pointer should be freed when no longer needed.
 *  \param line3 Pointer to unallocated memory where the third line will be
 *               stored. The pointer should be freed when no longer needed.
 *  \return TLE_CONV_SUCCESS if conversion went OK, TLE_CONV_ERROR otherwise.
 *
 * \note An error message will be logged if an error occurs.
 */
gint
tle2twoline (tle_t *tle, gchar *line1, gchar *line2, gchar *line3)
{
    (void) tle; /* avoid unused parameter compiler warning */
    (void) line1; /* avoid unused parameter compiler warning */
    (void) line2; /* avoid unused parameter compiler warning */
    (void) line3; /* avoid unused parameter compiler warning */
    
    return TLE_CONV_SUCCESS;
}




