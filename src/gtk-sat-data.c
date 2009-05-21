/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2008  Alexandru Csete, OZ9AEC.

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

#include <glib.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-sat-data.h"
#include "sat-log.h"
#include "config-keys.h"
#include "tle-lookup.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "orbit-tools.h"
#include "time-tools.h"


/** \brief Read QTH data from file.
 *  \param filename The file to read from.
 *  \param qth Pointer to a qth_t data structure where the data will be stored.
 *  \return FALSE if an error occurred, TRUE otherwise.
 *
 *  \note The function uses the new key=value file parser from glib.
 */
gint
gtk_sat_data_read_qth (const gchar *filename, qth_t *qth)
{
    GError *error = NULL;
    gchar  *buff;
    gchar  **buffv;

    qth->data = g_key_file_new ();
    g_key_file_set_list_separator (qth->data, ';');

    /* bail out with error message if data can not be read */
    if (!g_key_file_load_from_file (qth->data, filename,
                                    G_KEY_FILE_KEEP_COMMENTS, &error)) {

        g_key_file_free (qth->data);

        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Could not load data from %s (%s)"),
                     __FUNCTION__, filename, error->message);

        return FALSE;
    }

    /* send a debug message, then read data */
    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                 _("%s: QTH data: %s"),
                 __FUNCTION__, filename);

    /*** FIXME: should check that strings are UTF-8? */
    /* QTH Name */
    buff = g_path_get_basename (filename);
    buffv = g_strsplit (buff, ".qth", 0);
    qth->name = g_strdup (buffv[0]);

    g_free (buff);
    g_strfreev (buffv);
    /* g_key_file_get_string (qth->data, */
    /*                        QTH_CFG_MAIN_SECTION, */
    /*                        QTH_CFG_NAME_KEY, */
    /*                        &error); */
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR, 
                     _("%s: Error reading QTH name (%s)."),
                     __FUNCTION__, error->message);

        qth->name = g_strdup (_("ERROR"));
        g_clear_error (&error);
    }

    /* QTH location */
    qth->loc = g_key_file_get_string (qth->data,
                                      QTH_CFG_MAIN_SECTION,
                                      QTH_CFG_LOC_KEY,
                                      &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("%s: QTH has no location (%s)."),
                     __FUNCTION__, error->message);

        qth->loc = g_strdup ("");
        g_clear_error (&error);
    }

    /* QTH description */
    qth->desc = g_key_file_get_string (qth->data,
                                       QTH_CFG_MAIN_SECTION,
                                       QTH_CFG_DESC_KEY,
                                       &error);
    if ((qth->desc == NULL) || (error != NULL)) {
        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("%s: QTH has no description."),
                     __FUNCTION__);

        qth->desc = g_strdup ("");
        g_clear_error (&error);
    }

    /* Weather station */
    qth->wx = g_key_file_get_string (qth->data,
                                     QTH_CFG_MAIN_SECTION,
                                     QTH_CFG_WX_KEY,
                                     &error);
    if ((qth->wx == NULL) || (error != NULL)) {
        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("%s: QTH has no weather station."),
                     __FUNCTION__);

        qth->wx = g_strdup ("");
        g_clear_error (&error);
    }

    /* QTH Latitude */
    buff = g_key_file_get_string (qth->data,
                                  QTH_CFG_MAIN_SECTION,
                                  QTH_CFG_LAT_KEY,
                                  &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading QTH latitude (%s)."),
                     __FUNCTION__, error->message);

        g_clear_error (&error);

        if (buff != NULL)
            g_free (buff);

        qth->lat = 0.0;
    }
    else {
        qth->lat = g_ascii_strtod (buff, NULL);
        g_free (buff);
    }

    /* QTH Longitude */
    buff = g_key_file_get_string (qth->data,
                                  QTH_CFG_MAIN_SECTION,
                                  QTH_CFG_LON_KEY,
                                  &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading QTH longitude (%s)."),
                     __FUNCTION__, error->message);

        g_clear_error (&error);

        if (buff != NULL)
            g_free (buff);

        qth->lon = 0.0;
    }
    else {
        qth->lon = g_ascii_strtod (buff, NULL);
        g_free (buff);
    }

    /* QTH Altitude */
    qth->alt = g_key_file_get_integer (qth->data,
                                       QTH_CFG_MAIN_SECTION,
                                       QTH_CFG_ALT_KEY,
                                       &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading QTH altitude (%s)."),
                     __FUNCTION__, error->message);

        g_clear_error (&error);

        if (buff != NULL)
            g_free (buff);

        qth->alt = 0;
    }
    else {
    }

    /* Now, send debug message and return */
    sat_log_log (SAT_LOG_LEVEL_MSG,
                 _("%s: QTH data: %s, %.4f, %.4f, %d"),
                 __FUNCTION__,
                 qth->name,
                 qth->lat,
                 qth->lon,
                 qth->alt);

    return TRUE;
}


/** \brief Save the QTH data to a file.
 * \param filename The file to save to.
 * \param qth Pointer to a qth_t data structure from which the data will be read.
 */
gint
gtk_sat_data_save_qth (const gchar *filename, qth_t *qth)
{
    GError     *error = NULL;
    gchar      *buff;
    GIOChannel *cfgfile;
    gsize       length;
    gsize       written;
    gchar      *cfgstr;
    gint        ok = 1;

    qth->data = g_key_file_new ();
    g_key_file_set_list_separator (qth->data, ';');

    /* name */
    /*     if (qth->name) { */
    /*         g_key_file_set_string (qth->data, */
    /*                        QTH_CFG_MAIN_SECTION, */
    /*                        QTH_CFG_NAME_KEY, */
    /*                        qth->name); */
    /*     } */

    /* description */
    if (qth->desc && (g_utf8_strlen (qth->desc, -1) > 0)) {
        g_key_file_set_string (qth->data,
                               QTH_CFG_MAIN_SECTION,
                               QTH_CFG_DESC_KEY,
                               qth->desc);
    }
    
    /* location */
    if (qth->loc && (g_utf8_strlen (qth->loc, -1) > 0)) {
        g_key_file_set_string (qth->data,
                               QTH_CFG_MAIN_SECTION,
                               QTH_CFG_LOC_KEY,
                               qth->loc);
    }

    /* latitude */
    /*    buff = g_strdup_printf ("%.4f", qth->lat);*/
    buff = g_malloc (10);
    buff = g_ascii_dtostr (buff, 9, qth->lat);
    g_key_file_set_string (qth->data,
                           QTH_CFG_MAIN_SECTION,
                           QTH_CFG_LAT_KEY,
                           buff);
    g_free (buff);

    /* longitude */
    /*     buff = g_strdup_printf ("%.4f", qth->lon); */
    buff = g_malloc (10);
    buff = g_ascii_dtostr (buff, 9, qth->lon);
    g_key_file_set_string (qth->data,
                           QTH_CFG_MAIN_SECTION,
                           QTH_CFG_LON_KEY,
                           buff);
    g_free (buff);

    /* altitude */
    g_key_file_set_integer (qth->data,
                            QTH_CFG_MAIN_SECTION,
                            QTH_CFG_ALT_KEY,
                            qth->alt);

    /* weather station */
    if (qth->wx && (g_utf8_strlen (qth->wx, -1) > 0)) {
        g_key_file_set_string (qth->data,
                               QTH_CFG_MAIN_SECTION,
                               QTH_CFG_WX_KEY,
                               qth->wx);
    }

    /* saving code */

    /* convert configuration data struct to charachter string */
    cfgstr = g_key_file_to_data (qth->data, &length, &error);

    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Could not create QTH data (%s)."),
                     __FUNCTION__, error->message);

        g_clear_error (&error);

        ok = 0;
    }
    else {

        cfgfile = g_io_channel_new_file (filename, "w", &error);

        if (error != NULL) {
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Could not create QTH file %s\n%s."),
                         __FUNCTION__, filename, error->message);

            g_clear_error (&error);

            ok = 0;
        }
        else {
            g_io_channel_write_chars (cfgfile,
                                      cfgstr,
                                      length,
                                      &written,
                                      &error);

            g_io_channel_shutdown (cfgfile, TRUE, NULL);
            g_io_channel_unref (cfgfile);

            if (error != NULL) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: Error writing QTH data (%s)."),
                             __FUNCTION__, error->message);

                g_clear_error (&error);

                ok = 0;
            }
            else if (length != written) {
                sat_log_log (SAT_LOG_LEVEL_WARN,
                             _("%s: Wrote only %d out of %d chars."),
                             __FUNCTION__, written, length);

                ok = 0;
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: QTH data saved."),
                             __FUNCTION__);

                ok = 1;
            }
        }

        g_free (cfgstr);
    }

    return ok;
}


/** \brief Free QTH resources.
 *  \param qth The qth data structure to free.
 */
void
gtk_sat_data_free_qth (qth_t *qth)
{
    if (qth->name) {
        g_free (qth->name);
        qth->name = NULL;
    }

    if (qth->loc) {
        g_free (qth->loc);
        qth->loc = NULL;
    }

    if (qth->desc) {
        g_free (qth->desc);
        qth->desc = NULL;
    }

    if (qth->qra) {
        g_free (qth->qra);
        qth->qra = NULL;
    }

    if (qth->wx) {
        g_free (qth->wx);
        qth->wx = NULL;
    }

    if (qth->data) {
        g_key_file_free (qth->data);
        qth->data = NULL;
    }

    g_free (qth);
}


/** \brief Read TLE data for a given satellite into memory.
 *  \param catnum The catalog number of the satellite.
 *  \param sat Pointerto a valid sat_t structure.
 *  \return 0 if successfull, 1 if the satellite could not
 *          be found in any of the data files, 2 if the tle
 *          data has wrong checksum and finally 3 if the tle file
 *          in which the satellite should be, could not be read or opened.
 *
 * \bug We should use g_io_channel
 */
gint
gtk_sat_data_read_sat (gint catnum, sat_t *sat)
{
    FILE    *fp;
    gchar    tle_str[3][80];
    gchar   *filename = NULL, *path = NULL;
    gchar   *b;
    gchar    catstr[6];
    guint    i;
    guint    catnr;
    gboolean found = FALSE;
    guint    errorcode = 0;

    filename = tle_lookup (catnum);

    if (!filename) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Can not find #%d in any .tle file."),
                     __FUNCTION__, catnum);

        return 1;
    }

    /* create full file path */
    path = g_strdup_printf ("%s%s.gpredict2%stle%s%s",
                            g_get_home_dir (),
                            G_DIR_SEPARATOR_S,
                            G_DIR_SEPARATOR_S,
                            G_DIR_SEPARATOR_S,
                            filename);

    fp = fopen (path, "r");

    if (fp != NULL) {
 
        while (fgets (tle_str[0], 80, fp) && !found) {
            
            /* read second and third lines */
            b = fgets (tle_str[1], 80, fp);
            b = fgets (tle_str[2], 80, fp);

            /* copy catnum and convert to integer */
            for (i = 2; i < 7; i++) {
                catstr[i-2] = tle_str[1][i];
            }
            catstr[5] = '\0';
            catnr = (guint) g_ascii_strtod (catstr, NULL);

            if (catnr == catnum) {

                sat_log_log (SAT_LOG_LEVEL_DEBUG,
                             _("%s: Found #%d in %s"),
                             __FUNCTION__,
                             catnum,
                             path);

                found = TRUE;

                if (Get_Next_Tle_Set (tle_str, &sat->tle) != 1) {
                    /* TLE data not good */
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                 _("%s: Invalid data for #%d"),
                                 __FUNCTION__,
                                 catnum);

                    errorcode = 2;
                }
                else {
                    /* DATA OK, phew... */
                    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                                 _("%s: Good data for #%d"),
                                 __FUNCTION__,
                                 catnum);

                    /* VERY, VERY important! If not done, some sats
                       will not get initialised, the first time SGP4/SDP4
                       is called. Consequently, the resulting data will 
                       be NAN, INF or similar nonsense.
                       For some reason, not even using g_new0 seems to
                       be enough.
                    */
                    sat->flags = 0;

                    select_ephemeris (sat);

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
                    gtk_sat_data_init_sat (sat, NULL);

                }
            }

        }

        fclose (fp);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Failed to open %s"),
                     __FUNCTION__, path);

        errorcode = 3;
    }

    g_free (path);
    g_free (filename);

    return errorcode;
}


/** \brief Initialise satellite data.
 *  \param sat The satellite to initialise.
 *  \param qth Optional QTH info, use (0,0) if NULL.
 *
 * This function calculates the satellite data at t = 0, ie. epoch time
 * The function is called automatically by gtk_sat_data_read_sat.
 */
void
gtk_sat_data_init_sat (sat_t *sat, qth_t *qth)
{
    geodetic_t obs_geodetic;
    obs_set_t obs_set;
    geodetic_t sat_geodetic;
    double jul_utc, age;

    g_return_if_fail (sat != NULL);

    jul_utc = Julian_Date_of_Epoch (sat->tle.epoch); // => tsince = 0.0
    sat->jul_epoch = jul_utc;

    /* initialise observer location */
    if (qth != NULL) {
        obs_geodetic.lon = qth->lon * de2ra;
        obs_geodetic.lat = qth->lat * de2ra;
        obs_geodetic.alt = qth->alt / 1000.0;
        obs_geodetic.theta = 0;
    }
    else {
        obs_geodetic.lon = 0.0;
        obs_geodetic.lat = 0.0;
        obs_geodetic.alt = 0.0;
        obs_geodetic.theta = 0;
    }

    /* execute computations */
    if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
        SDP4 (sat, 0.0);
    else
        SGP4 (sat, 0.0);

    /* scale position and velocity to km and km/sec */
    Convert_Sat_State (&sat->pos, &sat->vel);

    /* get the velocity of the satellite */
    Magnitude (&sat->vel);
    sat->velo = sat->vel.w;
    Calculate_Obs (jul_utc, &sat->pos, &sat->vel, &obs_geodetic, &obs_set);
    Calculate_LatLonAlt (jul_utc, &sat->pos, &sat_geodetic);

    while (sat_geodetic.lon < -pi)
        sat_geodetic.lon += twopi;
    
    while (sat_geodetic.lon > (pi))
        sat_geodetic.lon -= twopi;

    sat->az = Degrees (obs_set.az);
    sat->el = Degrees (obs_set.el);
    sat->range = obs_set.range;
    sat->range_rate = obs_set.range_rate;
    sat->ssplat = Degrees (sat_geodetic.lat);
    sat->ssplon = Degrees (sat_geodetic.lon);
    sat->alt = sat_geodetic.alt;
    sat->ma = Degrees (sat->phase);
    sat->ma *= 256.0/360.0;
    sat->footprint = 2.0 * xkmper * acos (xkmper/sat->pos.w);
    age = 0.0;
    sat->orbit = (long) floor((sat->tle.xno * xmnpda/twopi +
                               age * sat->tle.bstar * ae) * age +
                              sat->tle.xmo/twopi) + sat->tle.revnum - 1;

    /* orbit type */
    sat->otype = get_orbit_type (sat);
}

/** \brief Copy satellite data.
  * \param source Pointer to the source satellite to copy data from.
  * \param dest Pointer to the destination satellite to copy data into.
  * \param qth Pointer to the observer data (needed to initialize sat)
  *
  * This function copies the satellite data from a source sat_t structure into
  * the destination. The function copies the tle_t data and calls gtk_sat_data_inti_sat()
  * function for initializing the other fields.
  *
  */
void gtk_sat_data_copy_sat (sat_t *source, sat_t *dest, qth_t *qth)
{
    guint i;

    g_return_if_fail ((source != NULL) && (dest != NULL));

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

    for (i = 0; i < 25; i++)
        dest->tle.sat_name[i] = source->tle.sat_name[i];
    for (i = 0; i < 9; i++)
        dest->tle.idesg[i] = source->tle.idesg[i];

    dest->tle.status = source->tle.status;
    dest->tle.xincl1 = source->tle.xincl1;
    dest->tle.omegao1 = source->tle.omegao1;

    gtk_sat_data_init_sat (dest, qth);
}
