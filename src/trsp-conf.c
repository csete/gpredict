/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
#include "sat-log.h"
#include "compat.h"

#include "trsp-conf.h"

#define KEY_UP_LOW      "UP_LOW"
#define KEY_UP_HIGH     "UP_HIGH"
#define KEY_DOWN_LOW    "DOWN_LOW"
#define KEY_DOWN_HIGH   "DOWN_HIGH"
#define KEY_INVERT      "INVERT"
#define KEY_MODE        "MODE"

/** \brief Read transponder data file.
 *  \param catnum The catalog number of the satellite to read transponders for.
 *  \return  The new transponder list.
 */
GSList *read_transponders (guint catnum)
{
    GSList    *trsplist = NULL;
    trsp_t    *trsp;
    GKeyFile  *cfg = NULL;
    GError    *error = NULL;
    gchar     *name,*fname;
    gchar    **groups;
    gsize      numgrp,i;
    
    
    name = g_strdup_printf ("%d.trsp", catnum);
    fname = trsp_file_name (name);
    
    cfg = g_key_file_new ();
    if (!g_key_file_load_from_file (cfg, fname, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading %s: %s"),
                     __FILE__, fname, error->message);
        g_clear_error (&error);

        g_key_file_free (cfg);
        
        return NULL;
    }
    
    /* get list of transponders */
    groups = g_key_file_get_groups (cfg, &numgrp);
    
    /* load each transponder */
    if (numgrp == 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: %s contains 0 transponders"),
                     __FUNCTION__, fname);
    }
    else {
        for (i = 0; i < numgrp; i++) {
            trsp = g_try_new (trsp_t, 1);
            if G_UNLIKELY(trsp == NULL) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                              _("%s: Failed to allocate memory for transponder data :-("),
                              __FUNCTION__);
            }
            else {
                /* read transponder data */
                trsp->name = g_strdup (groups[i]);
                
                trsp->uplow = g_key_file_get_double (cfg, groups[i], KEY_UP_LOW, &error);
                if (error != NULL) {
                    sat_log_log (SAT_LOG_LEVEL_WARN,
                                  _("%s: Error reading %s:%s from %s. Using default."),
                                  __FUNCTION__, groups[i], KEY_UP_LOW, name);
                    g_clear_error (&error);
                    trsp->uplow = 0.0;
                }
                
                trsp->uphigh = g_key_file_get_double (cfg, groups[i], KEY_UP_HIGH, &error);
                if (error != NULL) {
                    sat_log_log (SAT_LOG_LEVEL_WARN,
                                  _("%s: Error reading %s:%s from %s. Using default."),
                                  __FUNCTION__, groups[i], KEY_UP_HIGH, name);
                    g_clear_error (&error);
                    trsp->uphigh = trsp->uplow;
                }
                
                trsp->downlow = g_key_file_get_double (cfg, groups[i], KEY_DOWN_LOW, &error);
                if (error != NULL) {
                    sat_log_log (SAT_LOG_LEVEL_WARN,
                                  _("%s: Error reading %s:%s from %s. Using default."),
                                  __FUNCTION__, groups[i], KEY_DOWN_LOW, name);
                    g_clear_error (&error);
                    trsp->downlow = 0.0;
                }
                
                trsp->downhigh = g_key_file_get_double (cfg, groups[i], KEY_DOWN_HIGH, &error);
                if (error != NULL) {
                    sat_log_log (SAT_LOG_LEVEL_WARN,
                                  _("%s: Error reading %s:%s from %s. Using default."),
                                  __FUNCTION__, groups[i], KEY_DOWN_HIGH, name);
                    g_clear_error (&error);
                    trsp->downhigh = trsp->downlow;
                }
                
                trsp->invert = g_key_file_get_boolean (cfg, groups[i], KEY_INVERT, &error);
                if (error != NULL) {
                    sat_log_log (SAT_LOG_LEVEL_WARN,
                                  _("%s: Error reading %s:%s from %s. Assume non-inverting."),
                                  __FUNCTION__, groups[i], KEY_INVERT, name);
                    g_clear_error (&error);
                    trsp->invert = FALSE;
                }
                
                trsp->mode = g_key_file_get_string (cfg, groups[i], KEY_MODE, &error);
                if (error != NULL) {
                    sat_log_log (SAT_LOG_LEVEL_WARN,
                                  _("%s: Error reading %s:%s from %s"),
                                  __FUNCTION__, groups[i], KEY_MODE, name);
                    g_clear_error (&error);
                }

                /* add transponder to list */
                trsplist = g_slist_append (trsplist, trsp);
            }
            
        }
    }
    
    g_strfreev (groups);
    g_key_file_free (cfg);
    g_free (name);
    g_free (fname);

    
    return trsplist;
}


/** \brief Write transponder list for satellite.
 *  \param catnum The catlog number of the satellite.
 *  \param trsplist Pointer to a GSList of trsp_t structures.
 */
void write_transponders (guint catnum, GSList *trsplist)
{
    // FIXME
    sat_log_log (SAT_LOG_LEVEL_BUG, _("%s: Not implemented!"), __FUNCTION__);
}

/** \brief Free transponder list.
 *  \param trsplist Pointer to a GSList of trsp_t structures.
 *
 * This functions free all memory occupied by the transponder list.
 */
void free_transponders (GSList *trsplist)
{
    gint i, n;
    trsp_t *trsp;
    
    n = g_slist_length (trsplist);
    for (i = 0; i < n; i++) {
        trsp = (trsp_t *) g_slist_nth_data (trsplist, i);
        g_free (trsp->name);
        g_free (trsp);
        if (trsp->mode)
            g_free (trsp->mode);
    }
    g_slist_free (trsplist);
    trsplist = NULL;
}
