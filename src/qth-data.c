/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC
  Copyright (C)  2011-2012  Charles Suprin, AA1VS
 
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

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#ifdef HAS_LIBGPS
#include <gps.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "locator.h"
#include "orbit-tools.h"
#include "qth-data.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

void            qth_validate(qth_t * qth);


/**
 * Read QTH data from file.
 *
 * \param filename The file to read from.
 * \param qth Pointer to a qth_t data structure where the data will be stored.
 * \return FALSE if an error occurred, TRUE otherwise.
 *
 * \note The function uses the new key=value file parser from glib.
 */
gint qth_data_read(const gchar * filename, qth_t * qth)
{
    GError         *error = NULL;
    gchar          *buff;
    gchar         **buffv;

    qth->data = g_key_file_new();
    g_key_file_set_list_separator(qth->data, ';');

    /* bail out with error message if data can not be read */
    g_key_file_load_from_file(qth->data, filename, G_KEY_FILE_KEEP_COMMENTS,
                              &error);
    if (error != NULL)
    {
        g_key_file_free(qth->data);
        qth->data = NULL;

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not load data from %s (%s)"),
                    __func__, filename, error->message);

        return FALSE;
    }

    /* send a debug message, then read data */
    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: QTH data: %s"), __func__,
                filename);

    /*** FIXME: should check that strings are UTF-8? */
    /* QTH Name */
    buff = g_path_get_basename(filename);
    buffv = g_strsplit(buff, ".qth", 0);
    qth->name = g_strdup(buffv[0]);

    g_free(buff);
    g_strfreev(buffv);

    /* QTH location */
    qth->loc = g_key_file_get_string(qth->data, QTH_CFG_MAIN_SECTION,
                                     QTH_CFG_LOC_KEY, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: QTH has no location (%s)."), __func__,
                    error->message);

        qth->loc = g_strdup("");
        g_clear_error(&error);
    }

    /* QTH description */
    qth->desc = g_key_file_get_string(qth->data, QTH_CFG_MAIN_SECTION,
                                      QTH_CFG_DESC_KEY, &error);
    if ((qth->desc == NULL) || (error != NULL))
    {
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: QTH has no description."),
                    __func__);

        qth->desc = g_strdup("");
        g_clear_error(&error);
    }

    /* Weather station */
    qth->wx = g_key_file_get_string(qth->data, QTH_CFG_MAIN_SECTION,
                                    QTH_CFG_WX_KEY, &error);
    if ((qth->wx == NULL) || (error != NULL))
    {
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: QTH has no weather station."),
                    __func__);

        qth->wx = g_strdup("");
        g_clear_error(&error);
    }

    /* QTH Latitude */
    buff = g_key_file_get_string(qth->data, QTH_CFG_MAIN_SECTION,
                                 QTH_CFG_LAT_KEY, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading QTH latitude (%s)."), __func__,
                    error->message);

        g_clear_error(&error);

        if (buff != NULL)
            g_free(buff);

        qth->lat = 0.0;
    }
    else
    {
        qth->lat = g_ascii_strtod(buff, NULL);
        g_free(buff);
    }

    /* QTH Longitude */
    buff = g_key_file_get_string(qth->data, QTH_CFG_MAIN_SECTION,
                                 QTH_CFG_LON_KEY, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading QTH longitude (%s)."), __func__,
                    error->message);

        g_clear_error(&error);

        if (buff != NULL)
            g_free(buff);

        qth->lon = 0.0;
    }
    else
    {
        qth->lon = g_ascii_strtod(buff, NULL);
        g_free(buff);
    }

    /* QTH Altitude */
    qth->alt = g_key_file_get_integer(qth->data, QTH_CFG_MAIN_SECTION,
                                      QTH_CFG_ALT_KEY, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading QTH altitude (%s)."), __func__,
                    error->message);

        g_clear_error(&error);

        if (buff != NULL)
            g_free(buff);

        qth->alt = 0;
    }

    /* QTH Type */
    qth->type = g_key_file_get_integer(qth->data, QTH_CFG_MAIN_SECTION,
                                       QTH_CFG_TYPE_KEY, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Error reading QTH type (%s)."), __func__,
                    error->message);

        g_clear_error(&error);
        qth->type = QTH_STATIC_TYPE;
    }

#if HAS_LIBGPS
    /* GPSD Port */
    qth->gpsd_port = g_key_file_get_integer(qth->data,
                                            QTH_CFG_MAIN_SECTION,
                                            QTH_CFG_GPSD_PORT_KEY, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading GPSD port (%s)."), __func__,
                    error->message);

        g_clear_error(&error);
        qth->gpsd_port = 2947;
    }

    /* GPSD Server */
    qth->gpsd_server = g_key_file_get_string(qth->data,
                                             QTH_CFG_MAIN_SECTION,
                                             QTH_CFG_GPSD_SERVER_KEY, &error);
    if ((qth->gpsd_server == NULL) || (error != NULL))
    {
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: QTH has no GPSD Server."),
                    __func__);

        qth->gpsd_server = g_strdup("");
        g_clear_error(&error);
    }
#endif

    /* set QRA based on data */
    if (longlat2locator(qth->lon, qth->lat, qth->qra, 2) != RIG_OK)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Could not set QRA for %s at %f, %f."),
                    __func__, qth->name, qth->lon, qth->lat);
    }

    qth_validate(qth);

    /* Now, send debug message and return */
    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: QTH data: %s, %.4f, %.4f, %d"),
                __func__, qth->name, qth->lat, qth->lon, qth->alt);

    return TRUE;
}


/**
 * Save the QTH data to a file.
 *
 * \param filename The file to save to.
 * \param qth Pointer to a qth_t data structure from which the data will be read.
 */
gint qth_data_save(const gchar * filename, qth_t * qth)
{
    gchar          *buff;
    gint            ok = 1;

    qth->data = g_key_file_new();
    g_key_file_set_list_separator(qth->data, ';');

    /* description */
    if (qth->desc && (g_utf8_strlen(qth->desc, -1) > 0))
    {
        g_key_file_set_string(qth->data, QTH_CFG_MAIN_SECTION,
                              QTH_CFG_DESC_KEY, qth->desc);
    }

    /* location */
    if (qth->loc && (g_utf8_strlen(qth->loc, -1) > 0))
    {
        g_key_file_set_string(qth->data, QTH_CFG_MAIN_SECTION, QTH_CFG_LOC_KEY,
                              qth->loc);
    }

    /* latitude */
    /*    buff = g_strdup_printf ("%.4f", qth->lat); */
    buff = g_malloc(10);
    buff = g_ascii_dtostr(buff, 9, qth->lat);
    g_key_file_set_string(qth->data, QTH_CFG_MAIN_SECTION, QTH_CFG_LAT_KEY,
                          buff);
    g_free(buff);

    /* longitude */
    /*     buff = g_strdup_printf ("%.4f", qth->lon); */
    buff = g_malloc(10);
    buff = g_ascii_dtostr(buff, 9, qth->lon);
    g_key_file_set_string(qth->data, QTH_CFG_MAIN_SECTION, QTH_CFG_LON_KEY,
                          buff);
    g_free(buff);

    /* altitude */
    g_key_file_set_integer(qth->data, QTH_CFG_MAIN_SECTION, QTH_CFG_ALT_KEY,
                           qth->alt);

    /* weather station */
    if (qth->wx && (g_utf8_strlen(qth->wx, -1) > 0))
    {
        g_key_file_set_string(qth->data, QTH_CFG_MAIN_SECTION, QTH_CFG_WX_KEY,
                              qth->wx);
    }

    /* qth type */
    /* static, gpsd... */
    g_key_file_set_integer(qth->data, QTH_CFG_MAIN_SECTION, QTH_CFG_TYPE_KEY,
                           qth->type);

#if HAS_LIBGPS
    /* gpsd server */
    if (qth->gpsd_server && (g_utf8_strlen(qth->gpsd_server, -1) > 0))
    {
        g_key_file_set_string(qth->data,
                              QTH_CFG_MAIN_SECTION, QTH_CFG_GPSD_SERVER_KEY,
                              qth->gpsd_server);
    }
    /* gpsd port */
    g_key_file_set_integer(qth->data, QTH_CFG_MAIN_SECTION,
                           QTH_CFG_GPSD_PORT_KEY, qth->gpsd_port);
#endif

    /* saving code */
    ok = !(gpredict_save_key_file(qth->data, filename));

    return ok;
}

void qth_data_free(qth_t * qth)
{
    /* stop any updating */
    qth_data_update_stop(qth);

    if (qth->name)
    {
        g_free(qth->name);
        qth->name = NULL;
    }

    if (qth->loc)
    {
        g_free(qth->loc);
        qth->loc = NULL;
    }

    if (qth->desc)
    {
        g_free(qth->desc);
        qth->desc = NULL;
    }

    if (qth->qra)
    {
        g_free(qth->qra);
        qth->qra = NULL;
    }

    if (qth->wx)
    {
        g_free(qth->wx);
        qth->wx = NULL;
    }

    if (qth->data)
    {
        g_key_file_free(qth->data);
        qth->data = NULL;
    }

    g_free(qth);
}

/**
 * Update the qth data by whatever method is appropriate.
 *
 * \param qth the qth data structure to update
 * \param qth the time at which the qth is to be computed. this may be ignored by gps updates.
 */
gboolean qth_data_update(qth_t * qth, gdouble t)
{
#ifndef HAS_LIBGPS
    (void)qth;
    (void)t;
    return FALSE;
#else
    gboolean        retval = FALSE;
    guint           num_loops = 0;

    if (qth->type != QTH_GPSD_TYPE)
        return FALSE;

    if (((t - qth->gpsd_update) > 30.0 / 86400.0) &&
        (t - qth->gpsd_connected > 30.0 / 86400.0))
    {
        /* if needed restart the gpsd interface */
        qth_data_update_stop(qth);
        qth_data_update_init(qth);
        qth->gpsd_connected = t;
    }

    if (qth->gps_data != NULL)
    {
        switch (GPSD_API_MAJOR_VERSION)
        {
        case 4:
#if GPSD_API_MAJOR_VERSION==4
            while (gps_waiting(qth->gps_data) == TRUE)
            {
                /* this is a watchdog in case there is a problem with the gpsd code. */
                /* if the server was up and has failed then gps_waiting in 2.92 confirmed */
                /* will return 1 (supposedly fixed in later versions.) */
                /* if we do not do this as a while loop, the gpsd packets can backup  */
                /*   and no longer be in sync with the gps receiver */
                num_loops++;
                if (num_loops > 1000)
                {
                    retval = FALSE;
                    break;
                }
                if (gps_poll(qth->gps_data) == 0)
                {
                    /* handling packet_set inline with 
                       http://gpsd.berlios.de/client-howto.html
                     */
                    if (qth->gps_data->set & PACKET_SET)
                    {
                        if (qth->gps_data->fix.mode >= MODE_2D)
                        {
                            if (qth->lat != qth->gps_data->fix.latitude)
                            {
                                qth->lat = qth->gps_data->fix.latitude;
                                retval = TRUE;
                            }
                            if (qth->lon != qth->gps_data->fix.longitude)
                            {
                                qth->lon = qth->gps_data->fix.longitude;
                                retval = TRUE;
                            }
                        }

                        if (qth->gps_data->fix.mode == MODE_3D)
                        {
                            if (qth->alt != qth->gps_data->fix.altitude)
                            {
                                qth->alt = qth->gps_data->fix.altitude;
                                retval = TRUE;
                            }
                        }
                        else
                        {
                            if (qth->alt != 0)
                            {
                                qth->alt = 0;
                                retval = TRUE;
                            }
                        }
                    }
                }
            }
#endif
            break;
        case 5:
#if GPSD_API_MAJOR_VERSION==5
            while (gps_waiting(qth->gps_data, 0) == 1)
            {
                /* see comment from above */
                /* hopefully not needed but does not hurt anything. */
                num_loops++;
                if (num_loops > 1000)
                {
                    retval = FALSE;
                    break;
                }
                if (gps_read(qth->gps_data) == 0)
                {
                    /* handling packet_set inline with
                       http://gpsd.berlios.de/client-howto.html
                     */
                    if (qth->gps_data->set & PACKET_SET)
                    {
                        if (qth->gps_data->fix.mode >= MODE_2D)
                        {
                            if (qth->lat != qth->gps_data->fix.latitude)
                            {
                                qth->lat = qth->gps_data->fix.latitude;
                                retval = TRUE;
                            }
                            if (qth->lon != qth->gps_data->fix.longitude)
                            {
                                qth->lon = qth->gps_data->fix.longitude;
                                retval = TRUE;
                            }
                        }

                        if (qth->gps_data->fix.mode == MODE_3D)
                        {
                            if (qth->alt != qth->gps_data->fix.altitude)
                            {
                                qth->alt = qth->gps_data->fix.altitude;
                                retval = TRUE;
                            }
                        }
                        else
                        {
                            if (qth->alt != 0)
                            {
                                qth->alt = 0;
                                retval = TRUE;
                            }
                        }
                    }
                }
            }
#endif
            break;

	case 6:		/* for libgps 3.17 */
#if GPSD_API_MAJOR_VERSION==6
            while (gps_waiting(qth->gps_data, 0) == 1)
            {
                /* see comment from above */
                /* hopefully not needed but does not hurt anything. */
                num_loops++;
                if (num_loops > 1000)
                {
                    retval = FALSE;
                    break;
                }
                if (gps_read(qth->gps_data) > 0)
                {
                    /* handling packet_set inline with 
                       http://gpsd.berlios.de/client-howto.html
                     */
                    if (qth->gps_data->set & PACKET_SET)
                    {
                        if (qth->gps_data->fix.mode >= MODE_2D)
                        {
                            if (qth->lat != qth->gps_data->fix.latitude)
                            {
                                qth->lat = qth->gps_data->fix.latitude;
                                retval = TRUE;
                            }
                            if (qth->lon != qth->gps_data->fix.longitude)
                            {
                                qth->lon = qth->gps_data->fix.longitude;
                                retval = TRUE;
                            }
                        }

                        if (qth->gps_data->fix.mode == MODE_3D)
                        {
                            if (qth->alt != qth->gps_data->fix.altitude)
                            {
                                qth->alt = qth->gps_data->fix.altitude;
                                retval = TRUE;
                            }
                        }
                        else
                        {
                            if (qth->alt != 0)
                            {
                                qth->alt = 0;
                                retval = TRUE;
                            }
                        }
                    }
                }
            }
#endif
            break;
        default:
            break;
        }
        if (retval == TRUE)
        {
            qth->gpsd_update = t;
        }
    }

    qth_validate(qth);
    if (longlat2locator(qth->lon, qth->lat, qth->qra, 2) != RIG_OK)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not set QRA for %s at %f, %f."),
                    __func__, qth->name, qth->lon, qth->lat);
    }

    return retval;
#endif  // HAS_LIBGPS
}

/**
 * Initialize whatever structures inside the qth_t stucture for later updates.
 *
 * \param qth the qth data structure to update
 * 
 * Initial intention of this is to open sockets and ports to gpsd 
 * and other like services to update the qth position.
 */
gboolean qth_data_update_init(qth_t * qth)
{
#ifdef HAS_LIBGPS
    char           *port;
#endif
    gboolean        retval = FALSE;


    if (qth->type != QTH_GPSD_TYPE)
        return FALSE;

    /** FIXME: refactor */

#ifdef HAS_LIBGPS
    switch (GPSD_API_MAJOR_VERSION)
    {
    case 4:
#if GPSD_API_MAJOR_VERSION==4
        /* open the connection to gpsd and start the stream */
        qth->gps_data = g_try_new0(struct gps_data_t, 1);

        port = g_strdup_printf("%d", qth->gpsd_port);
        if (gps_open_r(qth->gpsd_server, port, qth->gps_data) == -1)
        {
            free(qth->gps_data);
            qth->gps_data = NULL;
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Could not open gpsd at  %s:%d"),
                        __func__, qth->gpsd_server, qth->gpsd_port);
            retval = FALSE;
        }
        else
        {
            (void)gps_stream(qth->gps_data, WATCH_ENABLE, NULL);
            retval = TRUE;
        }
        g_free(port);
#endif
        break;
    case 5:
#if GPSD_API_MAJOR_VERSION==5
        /* open the connection to gpsd and start the stream */
        qth->gps_data = g_try_new0(struct gps_data_t, 1);

        port = g_strdup_printf("%d", qth->gpsd_port);
        if (gps_open(qth->gpsd_server, port, qth->gps_data) == -1)
        {
            free(qth->gps_data);
            qth->gps_data = NULL;
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Could not open gpsd at  %s:%d"),
                        __func__, qth->gpsd_server, qth->gpsd_port);
            retval = FALSE;
        }
        else
        {
            (void)gps_stream(qth->gps_data, WATCH_ENABLE, NULL);
            retval = TRUE;
        }
        g_free(port);
#endif
        break;
    case 6:	/* for libgps 3.17 */
#if GPSD_API_MAJOR_VERSION==6
        /* open the connection to gpsd and start the stream */
        qth->gps_data = g_try_new0(struct gps_data_t, 1);

        port = g_strdup_printf("%d", qth->gpsd_port);
        if (gps_open(qth->gpsd_server, port, qth->gps_data) == -1)
        {
            free(qth->gps_data);
            qth->gps_data = NULL;
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Could not open gpsd at  %s:%d"),
                        __func__, qth->gpsd_server, qth->gpsd_port);
            retval = FALSE;
        }
        else
        {
            (void)gps_stream(qth->gps_data, WATCH_ENABLE, NULL);
            retval = TRUE;
        }
        g_free(port);
#endif
        break;
    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unsupported gpsd api major version (%d)"),
                    __func__, GPSD_API_MAJOR_VERSION);


        return FALSE;
        break;
    }
#else
    return FALSE;
#endif

    return retval;
}

/**
 * Shutdown and free structures inside the qth_t stucture were used for updates.
 *
 * \param qth the qth data structure to update
 * 
 * Initial intention of this is to open sockets and ports to gpsd 
 * and other like services to update the qth position.
 */
void qth_data_update_stop(qth_t * qth)
{
    if (qth->type != QTH_GPSD_TYPE)
        return;

    /* close gpsd socket */
    if (qth->gps_data != NULL)
    {
#ifdef HAS_LIBGPS
        switch (GPSD_API_MAJOR_VERSION)
        {
        case 4:
#if GPSD_API_MAJOR_VERSION==4
	    gps_close(qth->gps_data);
#endif
	    break;
        case 5:
#if GPSD_API_MAJOR_VERSION==5
	    gps_close(qth->gps_data);
#endif
	    break;
	case 6:		/* for libgps 3.17 */
#if GPSD_API_MAJOR_VERSION==6
            gps_close(qth->gps_data);
#endif
            break;
        default:
            break;
        }
#endif
        qth->gps_data = NULL;
    }
}

/**
 * Load initial values into the qth_t data structure
 *
 * \param qth the qth data structure to update
 */
void qth_init(qth_t * qth)
{
    qth->lat = 0;
    qth->lon = 0;
    qth->alt = 0;
    qth->type = QTH_STATIC_TYPE;
    qth->gps_data = NULL;
    qth->name = NULL;
    qth->loc = NULL;
    qth->gpsd_port = 0;
    qth->gpsd_server = NULL;
    qth->gpsd_update = 0.0;
    qth->gpsd_connected = 0.0;
    qth->qra = g_strdup("AA00");
}

/**
 * Load safe values into the qth_t data structure
 *
 * \param qth the qth data structure to update
 *
 * This can be used if some operation is suspected of corrupting the structure
 * or entering invalid data.  Originally it is based on code that reset values
 * if a load/read of a .qth failed.
 */
void qth_safe(qth_t * qth)
{
    qth->name = g_strdup(_("Error"));
    qth->loc = g_strdup(_("Error"));
    qth->type = QTH_STATIC_TYPE;
    qth->lat = 0;
    qth->lon = 0;
    qth->alt = 0;
    qth->gps_data = NULL;
}

/**
 * Copy values from qth structure to qth_small structure
 *
 * \param qth the qth data structure to backup
 * \param qth_small the data structure to store values
 *
 * This is intended for copying only the minimal qth data to a structure for
 * tagging and later comparison.
 */
void qth_small_save(qth_t * qth, qth_small_t * qth_small)
{
    qth_small->lat = qth->lat;
    qth_small->lon = qth->lon;
    qth_small->alt = qth->alt;
}

/**
 * Validate the contents of a qth structure and correct if neccessary
 *
 * \param qth the qth data structure to cleanup
 */
void qth_validate(qth_t * qth)
{
    /* check that the values are not set to nonsense such as nan or inf.
     * if so error it and set to zero. */
    if (!isnormal(qth->lat) && (qth->lat != 0))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: QTH data had bogus lat %f"), __func__, qth->lat);
        qth->lat = 0.0;

    }
    if (!isnormal(qth->lon) && (qth->lon != 0))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: QTH data had bogus lon %f"), __func__, qth->lon);
        qth->lon = 0.0;
    }

    /* check that qth->lat and qth->lon are in a reasonable range 
       and if not wrap them back */
    if (fabs(qth->lat) > 90.0 || fabs(qth->lon) > 180.0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _
                    ("%s: File contained bogus QTH data. Correcting: %s, %.4f, %.4f, %d"),
                    __func__, qth->name, qth->lat, qth->lon, qth->alt);

        qth->lat = fmod(qth->lat, 360.0);
        while ((qth->lat) > 180.0)
        {
            qth->lat = qth->lat - 180.0;
            qth->lon = qth->lon + 180.0;
        }

        while ((qth->lat) < -180.0)
        {
            qth->lat = qth->lat + 180.0;
            qth->lon = qth->lon + 180.0;
        }
        if (qth->lat > 90.0)
        {
            qth->lat = 180.0 - qth->lat;
            qth->lon = qth->lon + 180.0;
        }

        if (qth->lat < -90.0)
        {
            qth->lat = -180.0 - qth->lat;
            qth->lon = qth->lon + 180.0;
        }

        qth->lon = fmod(qth->lon + 180.0, 360.0) - 180;
        if (qth->lon < -180.0)
        {
            qth->lon += 360.0;
        }
        else if (qth->lon > 180.0)
        {
            qth->lon -= 360.0;
        }
    }
}

/**
 * Compute the distance between a location in a qth_t structure and
 * qth_small_t structure.
 * 
 * \param qth the qth data structure
 * \param qth_small the data structure
 * 
 * This is intended for measuring distance between the current qth and
 * the position that tagged some data in qth_small.
 */
double qth_small_dist(qth_t * qth, qth_small_t qth_small)
{
    double          distance, azimuth;

    qrb(qth->lon, qth->lat, qth_small.lon, qth_small.lat, &distance, &azimuth);

    return (distance);
}
