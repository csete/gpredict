/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "sat-pref-debug.h"
#include "sat-pref-formats.h"
#include "sat-pref-general.h"
#include "sat-pref-qth.h"
#include "sat-pref-tle.h"

/**
 * Create and initialise widgets for the general prefs tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be loaded.
 */
GtkWidget      *sat_pref_general_create()
{
    GtkWidget      *nbook;

    nbook = gtk_notebook_new();

    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_formats_create(),
                             gtk_label_new(_("Number Formats")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_qth_create(),
                             gtk_label_new(_("Ground Stations")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_tle_create(),
                             gtk_label_new(_("TLE Update")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_debug_create(),
                             gtk_label_new(_("Message Logs")));

    return nbook;
}

/** User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_general_cancel()
{
    sat_pref_formats_cancel();
    sat_pref_qth_cancel();
    sat_pref_tle_cancel();
    sat_pref_debug_cancel();
}

/** User pressed OK. Any changes should be stored in config. */
void sat_pref_general_ok()
{
    sat_pref_formats_ok();
    sat_pref_qth_ok();
    sat_pref_tle_ok();
    sat_pref_debug_ok();
}
