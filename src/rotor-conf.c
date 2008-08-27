/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2007  Alexandru Csete.

    Authors: Alexandru Csete <oz9aec@gmail.com>

    Comments, questions and bugreports should be submitted via
    http://sourceforge.net/projects/groundstation/
    More details can be found at the project home page:

            http://groundstation.sourceforge.net/
 
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

#define GROUP           "Rotator"
#define KEY_HOST        "Host"
#define KEY_PORT        "Port"
#define KEY_MINAZ       "MinAz"
#define KEY_MAXAZ       "MaxAz"
#define KEY_MINEL       "MinEl"
#define KEY_MAXEL       "MaxEl"


/** \brief Read rotator configuration.
 * \param conf Pointer to a rotor_conf_t structure where the data will be
 *             stored.
 * 
 * This function reads a rotoator configuration from a .rot file into conf.
 * conf->name must contain the file name of the configuration (no path, just
 * file name and without the .rot extension).
 */
gboolean rotor_conf_read (rotor_conf_t *conf)
{
    GKeyFile *cfg = NULL;
    gchar    *confdir;
    gchar    *fname;
    GError   *error = NULL;
    
    
    if (conf->name == NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: NULL configuration name!"),
                       __FUNCTION__);
        return FALSE;
    }
    
    confdir = get_conf_dir();
    fname = g_strconcat (confdir, G_DIR_SEPARATOR_S,
                         "hwconf", G_DIR_SEPARATOR_S,
                         conf->name, ".rot", NULL);
    g_free (confdir);
    
    /* open .grc file */
    cfg = g_key_file_new ();
    g_key_file_load_from_file(cfg, fname, 0, NULL);
    
    if (cfg == NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Could not load file %s\n"),
                     __FUNCTION__, fname);
        g_free (fname);
        
        return FALSE;
    }
    
    g_free (fname);
    
    /* read parameters */
    conf->host = g_key_file_get_string (cfg, GROUP, KEY_HOST, &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading rotor conf from %s (%s)."),
                       __FUNCTION__, conf->name, error->message);
        g_clear_error (&error);
        g_key_file_free (cfg);
        return FALSE;
    }
    
    conf->port = g_key_file_get_string (cfg, GROUP, KEY_PORT, &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading rotor conf from %s (%s)."),
                       __FUNCTION__, conf->name, error->message);
        g_clear_error (&error);
        g_key_file_free (cfg);
        return FALSE;
    }
    
    conf->minaz = g_key_file_get_double (cfg, GROUP, KEY_MINAZ, &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading rotor conf from %s (%s)."),
                       __FUNCTION__, conf->name, error->message);
        g_clear_error (&error);
        g_key_file_free (cfg);
        return FALSE;
    }
    
    conf->maxaz = g_key_file_get_double (cfg, GROUP, KEY_MAXAZ, &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading rotor conf from %s (%s)."),
                       __FUNCTION__, conf->name, error->message);
        g_clear_error (&error);
        g_key_file_free (cfg);
        return FALSE;
    }
    
    conf->minel = g_key_file_get_double (cfg, GROUP, KEY_MINEL, &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading rotor conf from %s (%s)."),
                       __FUNCTION__, conf->name, error->message);
        g_clear_error (&error);
        g_key_file_free (cfg);
        return FALSE;
    }
    
    conf->maxel = g_key_file_get_double (cfg, GROUP, KEY_MAXEL, &error);
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading rotor conf from %s (%s)."),
                       __FUNCTION__, conf->name, error->message);
        g_clear_error (&error);
        g_key_file_free (cfg);
        return FALSE;
    }
    
    g_key_file_free (cfg);
    
    return TRUE;
}


/** \brief Save rotator configuration.
 * \param conf Pointer to the rotator configuration.
 * 
 * This function saves the rotator configuration stored in conf to a
 * .rig file. conf->name must contain the file name of the configuration
 * (no path, just file name and without the .rot extension).
 */
void rotor_conf_save (rotor_conf_t *conf)
{
    GKeyFile *cfg = NULL;
    gchar    *confdir;
    gchar    *fname;
    gchar    *data;
    gsize     len;
    
    if (conf->name == NULL)
        return;
    
    /* create a config structure */
    cfg = g_key_file_new();
    
    g_key_file_set_string  (cfg, GROUP, KEY_HOST, conf->host);
    g_key_file_set_integer (cfg, GROUP, KEY_HOST, conf->port);
    g_key_file_set_double  (cfg, GROUP, KEY_MINAZ, conf->minaz);
    g_key_file_set_double  (cfg, GROUP, KEY_MAXAZ, conf->maxaz);
    g_key_file_set_double  (cfg, GROUP, KEY_MINEL, conf->minel);
    g_key_file_set_double  (cfg, GROUP, KEY_MAXEL, conf->maxel);
    
    /* convert to text sdata */
    data = g_key_file_to_data (cfg, &len, NULL);
    
    confdir = get_conf_dir();
    fname = g_strconcat (confdir, G_DIR_SEPARATOR_S,
                         "hwconf", G_DIR_SEPARATOR_S,
                         conf->name, ".rot", NULL);
    g_free (confdir);
        
    g_file_set_contents (fname, data, len, NULL);
    
    g_free (fname);
    g_free (data);
    g_key_file_free (cfg);
}
