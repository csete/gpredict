/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2019  Alexandru Csete.

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

#include "rotor-conf.h"
#include "gpredict-utils.h"

#define GROUP           "Rotator"
#define KEY_HOST        "Host"
#define KEY_PORT        "Port"
#define KEY_CYCLE       "Cycle"
#define KEY_AZTYPE      "AzType"
#define KEY_MINAZ       "MinAz"
#define KEY_MAXAZ       "MaxAz"
#define KEY_MINEL       "MinEl"
#define KEY_MAXEL       "MaxEl"
#define KEY_AZSTOPPOS   "AzStopPos"
#define KEY_THLD        "Threshold"

#define DEFAULT_CYCLE_MS    1000
#define DEFAULT_THLD_DEG    5.0

/**
 * \brief Read rotator configuration.
 * \param conf Pointer to a rotor_conf_t structure where the data will be
 *             stored.
 * 
 * This function reads a rotoator configuration from a .rot file into conf.
 * conf->name must contain the file name of the configuration (no path, just
 * file name and without the .rot extension).
 */
gboolean rotor_conf_read(rotor_conf_t * conf)
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
    fname = g_strconcat(confdir, G_DIR_SEPARATOR_S, conf->name, ".rot", NULL);
    g_free(confdir);

    /* open .rot file */
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
                    _("%s: Error reading rotor conf from %s (%s)."),
                    __func__, conf->name, error->message);
        g_clear_error(&error);
        g_key_file_free(cfg);
        return FALSE;
    }

    conf->port = g_key_file_get_integer(cfg, GROUP, KEY_PORT, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading rotor conf from %s (%s)."),
                    __func__, conf->name, error->message);
        g_clear_error(&error);
        g_key_file_free(cfg);
        return FALSE;
    }

    /* cycle period and threshold are only saved if not default */
    if (g_key_file_has_key(cfg, GROUP, KEY_CYCLE, NULL))
    {
        conf->cycle = g_key_file_get_integer(cfg, GROUP, KEY_CYCLE, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading rotor conf from %s (%s)."),
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

    if (g_key_file_has_key(cfg, GROUP, KEY_THLD, NULL))
    {
        conf->threshold = g_key_file_get_double(cfg, GROUP, KEY_THLD, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading rotor conf from %s (%s)."),
                        __func__, conf->name, error->message);
            g_clear_error(&error);
            g_key_file_free(cfg);
            return FALSE;
        }
        if (conf->threshold < 0.1)
            conf->threshold = 0.1;
    }
    else
    {
        conf->threshold = DEFAULT_THLD_DEG;
    }

    conf->aztype = g_key_file_get_integer(cfg, GROUP, KEY_AZTYPE, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Az type not defined for %s. Assuming 0..360\302\260"),
                    __func__, conf->name);
        g_clear_error(&error);

        conf->aztype = ROT_AZ_TYPE_360;
    }

    conf->minaz = g_key_file_get_double(cfg, GROUP, KEY_MINAZ, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: MinAz not defined for %s. Assuming 0\302\260."),
                    __func__, conf->name);
        g_clear_error(&error);
        conf->minaz = 0.0;
    }

    conf->maxaz = g_key_file_get_double(cfg, GROUP, KEY_MAXAZ, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: MaxAz not defined for %s. Assuming 360\302\260."),
                    __func__, conf->name);
        g_clear_error(&error);
        conf->maxaz = 360.0;
    }

    conf->minel = g_key_file_get_double(cfg, GROUP, KEY_MINEL, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: MinEl not defined for %s. Assuming 0\302\260."),
                    __func__, conf->name);
        g_clear_error(&error);
        conf->minel = 0.0;
    }

    conf->maxel = g_key_file_get_double(cfg, GROUP, KEY_MAXEL, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: MaxEl not defined for %s. Assuming 90\302\260."),
                    __func__, conf->name);
        g_clear_error(&error);
        conf->maxel = 90.0;
    }

    conf->azstoppos = g_key_file_get_double(cfg, GROUP, KEY_AZSTOPPOS, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: AzStopPos not defined for %s. Assuming at minaz (%f\302\260)."),
                    __func__, conf->name, conf->minaz);
        g_clear_error(&error);
        conf->azstoppos = conf->minaz;
    }

    g_key_file_free(cfg);

    return TRUE;
}

/**
 * \brief Save rotator configuration.
 * \param conf Pointer to the rotator configuration.
 * 
 * This function saves the rotator configuration stored in conf to a
 * .rig file. conf->name must contain the file name of the configuration
 * (no path, just file name and without the .rot extension).
 */
void rotor_conf_save(rotor_conf_t * conf)
{
    GKeyFile       *cfg = NULL;
    gchar          *confdir;
    gchar          *fname;

    if (conf->name == NULL)
        return;

    /* create a config structure */
    cfg = g_key_file_new();

    g_key_file_set_string(cfg, GROUP, KEY_HOST, conf->host);
    g_key_file_set_integer(cfg, GROUP, KEY_PORT, conf->port);
    g_key_file_set_integer(cfg, GROUP, KEY_AZTYPE, conf->aztype);
    g_key_file_set_double(cfg, GROUP, KEY_MINAZ, conf->minaz);
    g_key_file_set_double(cfg, GROUP, KEY_MAXAZ, conf->maxaz);
    g_key_file_set_double(cfg, GROUP, KEY_MINEL, conf->minel);
    g_key_file_set_double(cfg, GROUP, KEY_MAXEL, conf->maxel);
    g_key_file_set_double(cfg, GROUP, KEY_AZSTOPPOS, conf->azstoppos);

    if (conf->cycle == DEFAULT_CYCLE_MS)
        g_key_file_remove_key(cfg, GROUP, KEY_CYCLE, NULL);
    else
        g_key_file_set_integer(cfg, GROUP, KEY_CYCLE, conf->cycle);

    if (conf->threshold == DEFAULT_THLD_DEG)
        g_key_file_remove_key(cfg, GROUP, KEY_THLD, NULL);
    else
        g_key_file_set_double(cfg, GROUP, KEY_THLD, conf->threshold);

    /* build filename */
    confdir = get_hwconf_dir();
    fname = g_strconcat(confdir, G_DIR_SEPARATOR_S, conf->name, ".rot", NULL);
    g_free(confdir);

    /* save information */
    gpredict_save_key_file(cfg, fname);

    /* cleanup */
    g_free(fname);
    g_key_file_free(cfg);
}
