/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2015  Alexandru Csete.

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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sat-log.h"
#include "compat.h"
#include "gpredict-utils.h"

#include "radio-conf.h"

#define GROUP           "Radio"
#define KEY_HOST        "Host"
#define KEY_PORT        "Port"
#define KEY_CYCLE       "Cycle"
#define KEY_LO          "LO"
#define KEY_LOUP        "LO_UP"
#define KEY_TYPE        "Type"
#define KEY_PTT         "PTT"
#define KEY_VFO_DOWN    "VFO_DOWN"
#define KEY_VFO_UP      "VFO_UP"
#define KEY_SIG_AOS     "SIGNAL_AOS"
#define KEY_SIG_LOS     "SIGNAL_LOS"

#define DEFAULT_CYCLE_MS    1000

/**
 * \brief Read radio configuration.
 * \param conf Pointer to a radio_conf_t structure where the data will be
 *             stored.
 * \return TRUE if the configuration was read successfully, FALSE if an
 *         error has occurred.
 * 
 * This function reads a radio configuration from a .rig file into conf.
 * conf->name must contain the file name of the configuration (no path, just
 * file name).
 */
gboolean radio_conf_read(radio_conf_t * conf)
{
    GKeyFile       *cfg = NULL;
    gchar          *confdir;
    gchar          *fname;
    GError         *error = NULL;


    if (conf->name == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: NULL configuration name!"), __func__);
        return FALSE;
    }

    confdir = get_hwconf_dir();
    fname = g_strconcat(confdir, G_DIR_SEPARATOR_S, conf->name, ".rig", NULL);
    g_free(confdir);

    /* open .rig file */
    cfg = g_key_file_new();
    g_key_file_load_from_file(cfg, fname, 0, NULL);

    if (cfg == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not load file %s\n"), __func__, fname);
        g_free(fname);

        return FALSE;
    }

    g_free(fname);

    /* read parameters */
    conf->host = g_key_file_get_string(cfg, GROUP, KEY_HOST, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading radio conf from %s (%s)."),
                    __func__, conf->name, error->message);
        g_clear_error(&error);
        g_key_file_free(cfg);
        return FALSE;
    }

    conf->port = g_key_file_get_integer(cfg, GROUP, KEY_PORT, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading radio conf from %s (%s)."),
                    __func__, conf->name, error->message);
        g_clear_error(&error);
        g_key_file_free(cfg);
        return FALSE;
    }

    /* cycle period is only saved if not default */
    if (g_key_file_has_key(cfg, GROUP, KEY_CYCLE, NULL))
    {
        conf->cycle = g_key_file_get_integer(cfg, GROUP, KEY_CYCLE, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading radio conf from %s (%s)."),
                        __func__, conf->name, error->message);
            g_clear_error(&error);
            g_key_file_free(cfg);
            return FALSE;
        }
        if (conf->cycle < 10)
            conf->cycle = 10;
    }
    else
    {
        conf->cycle = DEFAULT_CYCLE_MS;
    }

    /* KEY_LO is optional */
    if (g_key_file_has_key(cfg, GROUP, KEY_LO, NULL))
    {
        conf->lo = g_key_file_get_double(cfg, GROUP, KEY_LO, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading radio conf from %s (%s)."),
                        __func__, conf->name, error->message);
            g_clear_error(&error);
            g_key_file_free(cfg);
            return FALSE;
        }
    }
    else
    {
        conf->lo = 0.0;
    }

    /* KEY_LOUP is optional */
    if (g_key_file_has_key(cfg, GROUP, KEY_LOUP, NULL))
    {
        conf->loup = g_key_file_get_double(cfg, GROUP, KEY_LOUP, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading radio conf from %s (%s)."),
                        __func__, conf->name, error->message);
            g_clear_error(&error);
            g_key_file_free(cfg);
            return FALSE;
        }
    }
    else
    {
        conf->loup = 0.0;
    }

    /* Radio type */
    conf->type = g_key_file_get_integer(cfg, GROUP, KEY_TYPE, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading radio conf from %s (%s)."),
                    __func__, conf->name, error->message);
        g_clear_error(&error);
        g_key_file_free(cfg);
        return FALSE;
    }

    /* PTT Type */
    conf->ptt = g_key_file_get_integer(cfg, GROUP, KEY_PTT, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading radio conf from %s (%s)."),
                    __func__, conf->name, error->message);
        g_clear_error(&error);
        g_key_file_free(cfg);
        return FALSE;
    }

    /* VFO up and down, only if radio is full-duplex */
    if (conf->type == RIG_TYPE_DUPLEX)
    {
        /* downlink */
        if (g_key_file_has_key(cfg, GROUP, KEY_VFO_DOWN, NULL))
        {
            conf->vfoDown =
                g_key_file_get_integer(cfg, GROUP, KEY_VFO_DOWN, &error);
            if (error != NULL)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: Error reading radio conf from %s (%s)."),
                            __func__, conf->name, error->message);
                g_clear_error(&error);
                conf->vfoDown = VFO_SUB;
            }
        }

        /* uplink */
        if (g_key_file_has_key(cfg, GROUP, KEY_VFO_UP, NULL))
        {
            conf->vfoUp =
                g_key_file_get_integer(cfg, GROUP, KEY_VFO_UP, &error);
            if (error != NULL)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: Error reading radio conf from %s (%s)."),
                            __func__, conf->name, error->message);
                g_clear_error(&error);
                conf->vfoUp = VFO_MAIN;
            }
        }
    }

    /* Signal AOS and LOS */
    conf->signal_aos = g_key_file_get_boolean(cfg, GROUP, KEY_SIG_AOS, NULL);
    conf->signal_los = g_key_file_get_boolean(cfg, GROUP, KEY_SIG_LOS, NULL);

    g_key_file_free(cfg);
    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Read radio configuration %s"), __func__, conf->name);

    return TRUE;
}

/**
 * \brief Save radio configuration.
 * \param conf Pointer to the radio configuration.
 * 
 * This function saves the radio configuration stored in conf to a
 * .rig file. conf->name must contain the file name of the configuration
 * (no path, just file name).
 */
void radio_conf_save(radio_conf_t * conf)
{
    GKeyFile       *cfg = NULL;
    gchar          *confdir;
    gchar          *fname;

    if (conf->name == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: NULL configuration name!"), __func__);
        return;
    }

    /* create a config structure */
    cfg = g_key_file_new();

    g_key_file_set_string(cfg, GROUP, KEY_HOST, conf->host);
    g_key_file_set_integer(cfg, GROUP, KEY_PORT, conf->port);
    g_key_file_set_double(cfg, GROUP, KEY_LO, conf->lo);
    g_key_file_set_double(cfg, GROUP, KEY_LOUP, conf->loup);
    g_key_file_set_integer(cfg, GROUP, KEY_TYPE, conf->type);
    g_key_file_set_integer(cfg, GROUP, KEY_PTT, conf->ptt);

    if (conf->cycle == DEFAULT_CYCLE_MS)
        g_key_file_remove_key(cfg, GROUP, KEY_CYCLE, NULL);
    else
        g_key_file_set_integer(cfg, GROUP, KEY_CYCLE, conf->cycle);

    if (conf->type == RIG_TYPE_DUPLEX)
    {
        g_key_file_set_integer(cfg, GROUP, KEY_VFO_UP, conf->vfoUp);
        g_key_file_set_integer(cfg, GROUP, KEY_VFO_DOWN, conf->vfoDown);
    }

    g_key_file_set_boolean(cfg, GROUP, KEY_SIG_AOS, conf->signal_aos);
    g_key_file_set_boolean(cfg, GROUP, KEY_SIG_LOS, conf->signal_los);

    confdir = get_hwconf_dir();
    fname = g_strconcat(confdir, G_DIR_SEPARATOR_S, conf->name, ".rig", NULL);
    g_free(confdir);

    gpredict_save_key_file(cfg, fname);

    g_free(fname);
    g_key_file_free(cfg);
}
