/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2016  Alexandru Csete.

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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gpredict-utils.h"
#include "sat-log.h"
#include "trsp-conf.h"

#define KEY_UP_LOW      "UP_LOW"
#define KEY_UP_HIGH     "UP_HIGH"
#define KEY_DOWN_LOW    "DOWN_LOW"
#define KEY_DOWN_HIGH   "DOWN_HIGH"
#define KEY_INVERT      "INVERT"
#define KEY_MODE        "MODE"
#define KEY_BAUD        "BAUD"

static void check_trsp_freq(trsp_t * trsp)
{
    /* ensure we don't have any negative frequencies */
    if (trsp->downlow < 0)
        trsp->downlow = 0;
    if (trsp->downhigh < 0)
        trsp->downhigh = 0;
    if (trsp->uplow < 0)
        trsp->uplow = 0;
    if (trsp->uphigh < 0)
        trsp->uphigh = 0;

    if (trsp->downlow == 0 && trsp->downhigh > 0)
        trsp->downlow = trsp->downhigh;
    else if (trsp->downhigh == 0 && trsp->downlow > 0)
        trsp->downhigh = trsp->downlow;

    if (trsp->uplow == 0 && trsp->uphigh > 0)
        trsp->uplow = trsp->uphigh;
    else if (trsp->uphigh == 0 && trsp->uplow > 0)
        trsp->uphigh = trsp->uplow;
}

/**
 * Read transponder data file.
 * 
 * @param catnum The catalog number of the satellite to read transponders for.
 * @return  The new transponder list.
 */
GSList *read_transponders(guint catnum)
{
    GSList         *trsplist = NULL;
    trsp_t         *trsp;
    GKeyFile       *cfg = NULL;
    GError         *error = NULL;
    gchar          *name, *fname;
    gchar         **groups;
    gsize           numgrp, i;

#define INFO_MSG N_("%s: Could not read %s from %s:'%s'. Using default.")

    name = g_strdup_printf("%d.trsp", catnum);
    fname = trsp_file_name(name);

    cfg = g_key_file_new();
    if (!g_key_file_load_from_file
        (cfg, fname, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading %s: %s"),
                    __FILE__, fname, error->message);
        g_clear_error(&error);

        g_key_file_free(cfg);

        return NULL;
    }

    /* get list of transponders */
    groups = g_key_file_get_groups(cfg, &numgrp);

    /* load each transponder */
    if (numgrp == 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: %s contains 0 transponders"),
                    __func__, fname);

        goto done;
    }

    for (i = 0; i < numgrp; i++)
    {
        trsp = g_try_new(trsp_t, 1);
        if (G_UNLIKELY(trsp == NULL))
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Failed to allocate memory for trsp data"),
                        __func__);
            goto done;
        }

        /* read transponder data */
        trsp->name = g_strdup(groups[i]);

        trsp->uplow = g_key_file_get_int64(cfg, groups[i], KEY_UP_LOW, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_UP_LOW,
                        name, groups[i]);
            g_clear_error(&error);
        }

        trsp->uphigh = g_key_file_get_int64(cfg, groups[i], KEY_UP_HIGH,
                                            &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_UP_HIGH,
                        name, groups[i]);
            g_clear_error(&error);
        }

        trsp->downlow = g_key_file_get_int64(cfg, groups[i], KEY_DOWN_LOW,
                                             &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_DOWN_LOW,
                        name, groups[i]);
            g_clear_error(&error);
        }

        trsp->downhigh = g_key_file_get_int64(cfg, groups[i], KEY_DOWN_HIGH,
                                              &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_DOWN_HIGH,
                        name, groups[i]);
            g_clear_error(&error);
        }

        /* check data to ensure consistency */
        check_trsp_freq(trsp);

        trsp->invert = g_key_file_get_boolean(cfg, groups[i],
                                              KEY_INVERT, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_INVERT,
                        name, groups[i]);
            g_clear_error(&error);
            trsp->invert = FALSE;
        }

        trsp->mode = g_key_file_get_string(cfg, groups[i], KEY_MODE, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_MODE,
                        name, groups[i]);
            g_clear_error(&error);
        }

        trsp->baud = g_key_file_get_double(cfg, groups[i], KEY_BAUD, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_BAUD,
                        name, groups[i]);
            g_clear_error(&error);
        }

        /* add transponder to list */
        trsplist = g_slist_append(trsplist, trsp);
    }

  done:
    g_strfreev(groups);
    g_key_file_free(cfg);
    g_free(name);
    g_free(fname);

    return trsplist;
}

/**
 * Write transponder list to file.
 *
 * @param catnum The catalog number of the satellite.
 * @param trsplist Pointer to a GSList of trsp_t structures.
 *
 * The transponder list is written to a file called "catnum.trsp". If the file
 * already exists, its contents will be deleted.
 */
void write_transponders(guint catnum, GSList * trsp_list)
{
    trsp_t         *trsp;
    GKeyFile       *trsp_data = NULL;
    gchar          *file_name;
    gchar          *trsp_file;
    gint            i, n, trsp_written;

    file_name = g_strdup_printf("%d.trsp", catnum);
    trsp_file = trsp_file_name(file_name);
    trsp_data = g_key_file_new();
    trsp_written = 0;

    n = g_slist_length(trsp_list);
    for (i = 0; i < n; i++)
    {
        trsp = (trsp_t *) g_slist_nth_data(trsp_list, i);
        if (!trsp->name)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Skipping transponder at index %d (no name)"),
                        __func__, i);
            continue;
        }

        if (trsp->uplow > 0)
            g_key_file_set_int64(trsp_data, trsp->name, KEY_UP_LOW,
                                 trsp->uplow);
        if (trsp->uphigh > 0)
            g_key_file_set_int64(trsp_data, trsp->name, KEY_UP_HIGH,
                                 trsp->uphigh);
        if (trsp->downlow > 0)
            g_key_file_set_int64(trsp_data, trsp->name, KEY_DOWN_LOW,
                                 trsp->downlow);
        if (trsp->downhigh > 0)
            g_key_file_set_int64(trsp_data, trsp->name, KEY_DOWN_HIGH,
                                 trsp->downhigh);
        if (trsp->baud > 0.0)
            g_key_file_set_double(trsp_data, trsp->name, KEY_BAUD, trsp->baud);
        if (trsp->invert)
            g_key_file_set_boolean(trsp_data, trsp->name, KEY_INVERT, TRUE);
        if (trsp->mode)
            g_key_file_set_string(trsp_data, trsp->name, KEY_MODE, trsp->name);
    }

    if (gpredict_save_key_file(trsp_data, trsp_file))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error writing transponder data to %s"),
                    __func__, file_name);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("Wrote %d transmponders to %s"),
                    trsp_written, trsp_file_name);
    }

    g_key_file_free(trsp_data);
    g_free(file_name);
    g_free(trsp_file);
}

/**
 * Free transponder list.
 *
 * @param trsplist Pointer to a GSList of trsp_t structures.
 */
void free_transponders(GSList * trsplist)
{
    gint            i, n;
    trsp_t         *trsp;

    n = g_slist_length(trsplist);
    for (i = 0; i < n; i++)
    {
        trsp = (trsp_t *) g_slist_nth_data(trsplist, i);
        g_free(trsp->name);
        if (trsp->mode)
            g_free(trsp->mode);
        g_free(trsp);
    }
    g_slist_free(trsplist);
    trsplist = NULL;
}
