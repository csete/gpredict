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

#include <glib.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "qth-data.h"
#include "sat-log.h"
#include "config-keys.h"
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
qth_data_read (const gchar *filename, qth_t *qth)
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
        qth->data = NULL;

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
qth_data_save (const gchar *filename, qth_t *qth)
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
qth_data_free (qth_t *qth)
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

