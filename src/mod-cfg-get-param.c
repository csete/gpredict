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

/**
 * \brief Utilities to read module configuration parameters.
 *
 * This file contains utility functions that can can be used by modules
 * to read configuration parameters from the GKeyFile of the module. If
 * a parameter does not exist in the GKeyFile the corrersponding value
 * from sat-cfg will be returned.
 *
 * The intended use of these functions is to read parameters when creating
 * new modules. A module may have it's own configuration for most settings
 * while for some settings the global/default values may be needed. To avoid
 * too much repetitive coding in the module implementations, the functions
 * warp this code into one convenient function call.
 */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "sat-cfg.h"
#include "sat-log.h"


/**
 * \brief Get boolean parameter.
 * \param f The configuration data for the module.
 * \param sec Configuration section in the cfg data (see config-keys.h).
 * \param key Configuration key in the cfg data (see config-keys.h).
 * \param p SatCfg index to use as fallback.
 */
gboolean mod_cfg_get_bool(GKeyFile * f, const gchar * sec, const gchar * key,
                          sat_cfg_bool_e p)
{
    GError         *error = NULL;
    gboolean        param;

    /* check whether parameter is present in GKeyFile, otherwise use sat-cfg */
    if (g_key_file_has_key(f, sec, key, NULL))
    {
        param = g_key_file_get_boolean(f, sec, key, &error);

        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Failed to read boolean (%s)"),
                        __func__, error->message);

            g_clear_error(&error);

            /* get a timeout from global config */
            param = sat_cfg_get_bool(p);
        }
    }
    else
    {
        param = sat_cfg_get_bool(p);
    }

    return param;
}


gint mod_cfg_get_int(GKeyFile * f, const gchar * sec, const gchar * key,
                     sat_cfg_int_e p)
{
    GError         *error = NULL;
    gint            param;

    /* check whether parameter is present in GKeyFile */
    if (g_key_file_has_key(f, sec, key, NULL))
    {
        param = g_key_file_get_integer(f, sec, key, &error);

        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_WARN,
                        _("%s: Failed to read integer (%s)"),
                        __func__, error->message);

            g_clear_error(&error);

            /* get a timeout from global config */
            param = sat_cfg_get_int(p);
        }
    }
    /* get value from sat-cfg */
    else
    {
        param = sat_cfg_get_int(p);
    }

    return param;
}


gchar          *mod_cfg_get_str(GKeyFile * f, const gchar * sec,
                                const gchar * key, sat_cfg_str_e p)
{
    GError         *error = NULL;
    gchar          *param;

    /* check whether parameter is present in GKeyFile, otherwise use sat-cfg */
    if (g_key_file_has_key(f, sec, key, NULL))
    {
        param = g_key_file_get_string(f, sec, key, &error);

        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_WARN,
                        _("%s: Failed to read string (%s)"),
                        __func__, error->message);

            g_clear_error(&error);

            /* get a timeout from global config */
            param = sat_cfg_get_str(p);
        }
    }
    else
    {
        param = sat_cfg_get_str(p);
    }

    return param;
}

/**
 * \brief Load an integer list into a hash table that uses the 
 * existinence of datain the hash as a boolean.
 * It loads NULL's into the hash table.  
 */
void mod_cfg_get_integer_list_boolean(GKeyFile * cfgdata,
                                      const gchar * section, const gchar * key,
                                      GHashTable * dest)
{
    gint           *sats = NULL;
    gsize           length;
    GError         *error = NULL;
    guint           i;
    guint          *tkey;

    if (!g_key_file_has_key(cfgdata, section, key, NULL))
        return;

    sats = g_key_file_get_integer_list(cfgdata, section, key, &length, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_WARN,
                    _("%s: Failed to get integer list: %s"),
                    __func__, error->message);

        g_clear_error(&error);

        /* GLib API says nothing about the contents in case of error */
        if (sats)
        {
            g_free(sats);
        }

        return;
    }

    /* read each satellite into hash table */
    for (i = 0; i < length; i++)
    {
        tkey = g_new0(guint, 1);
        *tkey = sats[i];
        //printf("loading sat %d\n",sats[i]);
        if (!(g_hash_table_lookup_extended(dest, tkey, NULL, NULL)))
        {
            /* just add a one to the value so there is presence indicator */
            g_hash_table_insert(dest, tkey, NULL);
        }
    }
    g_free(sats);
}

/**
 * \brief Convert the "boolean" hash back into an integer list and 
 * save it to the cfgdata.
 */
void mod_cfg_set_integer_list_boolean(GKeyFile * cfgdata, GHashTable * hash,
                                      const gchar * cfgsection,
                                      const gchar * cfgkey)
{
    gint           *showtrack;
    gint           *something;
    gint            i, length;
    GList          *keys = g_hash_table_get_keys(hash);

    length = g_list_length(keys);
    if (g_list_length(keys) > 0)
    {
        showtrack = g_try_new0(gint, g_list_length(keys));
        for (i = 0; i < length; i++)
        {
            something = g_list_nth_data(keys, i);
            showtrack[i] = *something;
        }
        g_key_file_set_integer_list(cfgdata,
                                    cfgsection,
                                    cfgkey, showtrack, g_list_length(keys));
    }
    else
    {
        g_key_file_remove_key(cfgdata, cfgsection, cfgkey, NULL);
    }

    g_list_free(keys);
}
