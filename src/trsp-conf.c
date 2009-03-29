/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2008  Alexandru Csete.

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
#define KEY_INV         "INVERT"


GSList *read_tranponders (guint catnum)
{
    GKeyFile  *cfg = NULL;
    GError    *error = NULL;
    gchar *name,*fname,*confdir;
    
    name = g_strdup_printf ("%d.tsrp", catnum);
    confdir = get_conf_dir();
    fname = g_strconcat (confdir, G_DIR_SEPARATOR_S,
                         "trsp", G_DIR_SEPARATOR_S,
                         name, NULL);
    
    cfg = g_key_file_new ();
    if (!g_key_file_load_from_file (cfg, fname, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error reading %s: %s"),
                     __FILE__, fname, error->message);
        g_clear_error (&error);
        g_key_file_free (cfg);
    }
    
    g_key_file_free (cfg);
    g_free (name);
    g_free (confdir);
    g_free (fname);


    return NULL;
}

void write_transponders (guint catnum, GSList *transp)
{
    
}

