/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete.

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

#include "compat.h"
#include "sat-log.h"
#include "trsp-conf.h"

#define KEY_UP_LOW      "UP_LOW"
#define KEY_UP_HIGH     "UP_HIGH"
#define KEY_DOWN_LOW    "DOWN_LOW"
#define KEY_DOWN_HIGH   "DOWN_HIGH"
#define KEY_INVERT      "INVERT"
#define KEY_MODE        "MODE"

/**
 * Read transponder data file.
 * 
 * \param catnum The catalog number of the satellite to read transponders for.
 * \return  The new transponder list.
 */
GSList         *read_transponders(guint catnum)
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
                        _("%s: Failed to allocate memory for transponder data"),
                        __func__);
            goto done;
        }

        /* read transponder data */
        trsp->name = g_strdup(groups[i]);

        trsp->uplow = g_key_file_get_double(cfg, groups[i], KEY_UP_LOW, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_UP_LOW,
                        name, groups[i]);
            g_clear_error(&error);
            trsp->uplow = 0.0;
        }

        trsp->uphigh = g_key_file_get_double(cfg, groups[i], KEY_UP_HIGH,
                                             &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_UP_HIGH,
                        name, groups[i]);
            g_clear_error(&error);
            trsp->uphigh = trsp->uplow;
        }

        trsp->downlow = g_key_file_get_double(cfg, groups[i],
                                              KEY_DOWN_LOW, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_UP_HIGH,
                        name, groups[i]);
            g_clear_error(&error);
            trsp->downlow = 0.0;
        }

        trsp->downhigh = g_key_file_get_double(cfg, groups[i], KEY_DOWN_HIGH,
                                               &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_DOWN_HIGH,
                        name, groups[i]);
            g_clear_error(&error);
            trsp->downhigh = trsp->downlow;
        }

        trsp->invert = g_key_file_get_boolean(cfg, groups[i],
                                              KEY_INVERT, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_DOWN_HIGH,
                        name, groups[i]);
            g_clear_error(&error);
            trsp->invert = FALSE;
        }

        trsp->mode = g_key_file_get_string(cfg, groups[i],
                                           KEY_MODE, &error);
        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, INFO_MSG, __func__, KEY_DOWN_HIGH,
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
 * Write transponder list for satellite.
 * \param catnum The catlog number of the satellite.
 * \param trsplist Pointer to a GSList of trsp_t structures.
 */
void write_transponders(guint catnum, GSList * trsplist)
{
    // FIXME
    (void)catnum;               /* avoid unused parameter compiler warning */
    (void)trsplist;             /* avoid unused parameter compiler warning */
    sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Not implemented!"), __func__);
}

/**
 * Free transponder list.
 * \param trsplist Pointer to a GSList of trsp_t structures.
 *
 * This functions free all memory occupied by the transponder list.
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
