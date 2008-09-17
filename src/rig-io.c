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
/** \brief Hardware driver wrapper
 * 
 * The purpose of this module is to provide a generic interace to the
 * hardware driver, regardless of whether it is a single/dual rig hamlib,
 * or any other interface.
 */

#include <glib.h>


/** \brief Initialise hardware interface.
 * \param uplink Name of the uplink configuration.
 * \param downlink Name of the downlink configuration.
 * \return 0 if successful, non-zero if an error occured
 * 
 */
gint rig_io_init (const gchar* uplink, const gchar *downlink)
{
    return 0;
}

/** \brief Shut down hardware interface.
 * 
 */
gint rig_io_shutdown ()
{
    return 0;
}


/** \brief Set uplink frequency.
 * \param freq The desired uplink frequency in Hz.
 * 
 */
void rig_io_set_uplink_freq (gdouble freq)
{
}


/** \brief Read uplink frequency.
 * \param freq The current uplink frequency in Hz.
 * 
 */
void rig_io_get_uplink_freq (gdouble *freq)
{
    *freq = 0.0;
}


/** \brief Set downlink frequency
 * \param The desired downlink frequency in Hz.
 * 
 */
void rig_io_set_downlink_freq (gdouble freq)
{
}


/** \brief Read downlink frequency.
 * \param freq The current downlink frequency in Hz.
 * 
 */
void rig_io_get_downlink_freq (gdouble *freq)
{
    *freq = 0.0;
}


