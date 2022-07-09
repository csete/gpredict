/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>
           Daniel Estevez <daniel@destevez.net>

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
#include "gtk-sat-data.h"
#include "sat-log.h"
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include "orbit-tools.h"
#include "time-tools.h"
#include "compat.h"


/**
 * Read TLE data for a given satellite into memory.
 *
 * @param catnum The catalog number of the satellite.
 * @param sat Pointer to a valid sat_t structure.
 * @return 0 if successful, 1 if an I/O error occurred,
 *         2 if the TLE data appears to be bad.
 *
 */
gint gtk_sat_data_read_sat(gint catnum, sat_t * sat)
{
    guint           errorcode = 0;
    GError         *error = NULL;
    GKeyFile       *data;
    gchar          *filename = NULL, *path = NULL;
    gchar          *tlestr1, *tlestr2, *rawtle;


    /* ensure that sat != NULL */
    g_return_val_if_fail(sat != NULL, 1);

    /* .sat file names */
    filename = g_strdup_printf("%d.sat", catnum);
    path = sat_file_name_from_catnum(catnum);

    /* open .sat file */
    data = g_key_file_new();
    if (!g_key_file_load_from_file
        (data, path, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        /* an error occurred */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to load data from %s (%s)"),
                    __func__, path, error->message);

        g_clear_error(&error);

        errorcode = 1;
    }
    else
    {
        /* read name, nickname, and website */
        sat->name = g_key_file_get_string(data, "Satellite", "NAME", &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading NAME from %s (%s)"),
                        __func__, path, error->message);
            g_clear_error(&error);
            sat->name = g_strdup("Error");
        }
        sat->nickname =
            g_key_file_get_string(data, "Satellite", "NICKNAME", &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Satellite %d has no NICKNAME"),
                        __func__, catnum);
            g_clear_error(&error);
            sat->nickname = g_strdup(sat->name);
        }

        sat->website = g_key_file_get_string(data, "Satellite", "WEBSITE", NULL);       /* website may be NULL */

        /* get TLE data */
        tlestr1 = g_key_file_get_string(data, "Satellite", "TLE1", NULL);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading TLE line 1 from %s (%s)"),
                        __func__, path, error->message);
            g_clear_error(&error);
        }
        tlestr2 = g_key_file_get_string(data, "Satellite", "TLE2", NULL);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading TLE line 2 from %s (%s)"),
                        __func__, path, error->message);
            g_clear_error(&error);
        }

        rawtle = g_strconcat(tlestr1, tlestr2, NULL);

        if (!Good_Elements(rawtle))
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: TLE data for %d appears to be bad"),
                        __func__, catnum);
            errorcode = 2;
        }
        else
        {
            Convert_Satellite_Data(rawtle, &sat->tle);
        }
        if (g_key_file_has_key(data, "Satellite", "STATUS", NULL))
            sat->tle.status =
                g_key_file_get_integer(data, "Satellite", "STATUS", NULL);

        g_free(tlestr1);
        g_free(tlestr2);
        g_free(rawtle);

        /* VERY, VERY important! If not done, some sats
           will not get initialised, the first time SGP4/SDP4
           is called. Consequently, the resulting data will
           be NAN, INF or similar nonsense.
           For some reason, not even using g_new0 seems to
           be enough.
         */
        sat->flags = 0;

        select_ephemeris(sat);

        /* initialise variable fields */
        sat->jul_utc = 0.0;
        sat->tsince = 0.0;
        sat->az = 0.0;
        sat->el = 0.0;
        sat->range = 0.0;
        sat->range_rate = 0.0;
        sat->ra = 0.0;
        sat->dec = 0.0;
        sat->ssplat = 0.0;
        sat->ssplon = 0.0;
        sat->alt = 0.0;
        sat->velo = 0.0;
        sat->ma = 0.0;
        sat->footprint = 0.0;
        sat->phase = 0.0;
        sat->aos = 0.0;
        sat->los = 0.0;

        /* calculate satellite data at epoch */
        gtk_sat_data_init_sat(sat, NULL);
    }

    g_free(filename);
    g_free(path);
    g_key_file_free(data);

    return errorcode;
}

/**
 * Initialise satellite data.
 *
 * @param sat The satellite to initialise.
 * @param qth Optional QTH info, use (0,0) if NULL.
 *
 * This function calculates the satellite data at t = 0, ie. epoch time
 * The function is called automatically by gtk_sat_data_read_sat.
 */
void gtk_sat_data_init_sat(sat_t * sat, qth_t * qth)
{
    geodetic_t      obs_geodetic;
    obs_set_t       obs_set;
    geodetic_t      sat_geodetic;
    double          jul_utc, age;

    g_return_if_fail(sat != NULL);

    jul_utc = Julian_Date_of_Epoch(sat->tle.epoch);     // => tsince = 0.0
    sat->jul_epoch = jul_utc;

    /* initialise observer location */
    if (qth != NULL)
    {
        obs_geodetic.lon = qth->lon * de2ra;
        obs_geodetic.lat = qth->lat * de2ra;
        obs_geodetic.alt = qth->alt / 1000.0;
        obs_geodetic.theta = 0;
    }
    else
    {
        obs_geodetic.lon = 0.0;
        obs_geodetic.lat = 0.0;
        obs_geodetic.alt = 0.0;
        obs_geodetic.theta = 0;
    }

    /* execute computations */
    if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
        SDP4(sat, 0.0);
    else
        SGP4(sat, 0.0);

    /* scale position and velocity to km and km/sec */
    Convert_Sat_State(&sat->pos, &sat->vel);

    /* get the velocity of the satellite */
    Magnitude(&sat->vel);
    sat->velo = sat->vel.w;
    Calculate_Obs(jul_utc, &sat->pos, &sat->vel, &obs_geodetic, &obs_set);
    Calculate_LatLonAlt(jul_utc, &sat->pos, &sat_geodetic);

    while (sat_geodetic.lon < -pi)
        sat_geodetic.lon += twopi;

    while (sat_geodetic.lon > (pi))
        sat_geodetic.lon -= twopi;

    sat->az = Degrees(obs_set.az);
    sat->el = Degrees(obs_set.el);
    sat->range = obs_set.range;
    sat->range_rate = obs_set.range_rate;
    sat->ssplat = Degrees(sat_geodetic.lat);
    sat->ssplon = Degrees(sat_geodetic.lon);
    sat->alt = sat_geodetic.alt;
    sat->ma = Degrees(sat->phase);
    sat->ma *= 256.0 / 360.0;
    sat->footprint = 2.0 * xkmper * acos(xkmper / sat->pos.w);
    age = 0.0;
    sat->orbit = (long)floor((sat->tle.xno * xmnpda / twopi +
                              age * sat->tle.bstar * ae) * age -
                             (sat->tle.xmo + sat->tle.omegao) / twopi) +
        sat->tle.revnum;

    /* orbit type */
    sat->otype = get_orbit_type(sat);
}

/**
 * Copy satellite data.
 *
 * @param source Pointer to the source satellite to copy data from.
 * @param dest Pointer to the destination satellite to copy data into.
 * @param qth Pointer to the observer data (needed to initialize sat)
 *
 * This function copies the satellite data from a source sat_t structure into
 * the destination. The function copies the tle_t data and calls gtk_sat_data_inti_sat()
 * function for initializing the other fields.
 *
 */
void gtk_sat_data_copy_sat(const sat_t * source, sat_t * dest, qth_t * qth)
{
    guint           i;

    g_return_if_fail((source != NULL) && (dest != NULL));

    dest->tle.epoch = source->tle.epoch;
    dest->tle.epoch_year = source->tle.epoch_year;
    dest->tle.epoch_day = source->tle.epoch_day;
    dest->tle.epoch_fod = source->tle.epoch_fod;
    dest->tle.xndt2o = source->tle.xndt2o;
    dest->tle.xndd6o = source->tle.xndd6o;
    dest->tle.bstar = source->tle.bstar;
    dest->tle.xincl = source->tle.xincl;
    dest->tle.xnodeo = source->tle.xnodeo;
    dest->tle.eo = source->tle.eo;
    dest->tle.omegao = source->tle.omegao;
    dest->tle.xmo = source->tle.xmo;
    dest->tle.xno = source->tle.xno;
    dest->tle.catnr = source->tle.catnr;
    dest->tle.elset = source->tle.elset;
    dest->tle.revnum = source->tle.revnum;

    dest->name = g_strdup(source->name);
    dest->nickname = g_strdup(source->nickname);
    for (i = 0; i < 9; i++)
        dest->tle.idesg[i] = source->tle.idesg[i];

    dest->tle.status = source->tle.status;
    dest->tle.xincl1 = source->tle.xincl1;
    dest->tle.omegao1 = source->tle.omegao1;

    dest->otype = source->otype;

    /* very important */
    dest->flags = 0;
    select_ephemeris(dest);

    /* initialise variable fields */
    dest->jul_utc = 0.0;
    dest->tsince = 0.0;
    dest->az = 0.0;
    dest->el = 0.0;
    dest->range = 0.0;
    dest->range_rate = 0.0;
    dest->ra = 0.0;
    dest->dec = 0.0;
    dest->ssplat = 0.0;
    dest->ssplon = 0.0;
    dest->alt = 0.0;
    dest->velo = 0.0;
    dest->ma = 0.0;
    dest->footprint = 0.0;
    dest->phase = 0.0;
    dest->aos = 0.0;
    dest->los = 0.0;

    gtk_sat_data_init_sat(dest, qth);
}

/**
 * Free satellite data
 *
 * @param sat Pointer to the satellite data to free
 * 
 * This function frees the memory that has been dyunamically allocated
 * when creating a new satellite object.
 */
void gtk_sat_data_free_sat(sat_t * sat)
{
    if (!sat)
        return;

    if (sat->name)
    {
        g_free(sat->name);
        sat->name = NULL;
    }
    if (sat->nickname)
    {
        g_free(sat->nickname);
        sat->nickname = NULL;
    }
    if (sat->website)
    {
        g_free(sat->website);
        sat->website = NULL;
    }

    g_free(sat);
}
