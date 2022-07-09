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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "sat-cfg.h"
#include "sat-pref-layout.h"
#include "sat-pref-list-view.h"
#include "sat-pref-map-view.h"
#include "sat-pref-modules.h"
#include "sat-pref-polar-view.h"
#include "sat-pref-refresh.h"
#include "sat-pref-single-sat.h"


/**
 * Create and initialise widgets for the modules prefs tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be loaded.
 *
 * @note The "modules" tab is different from the others in that it is used
 *       by both the saf-pref dialog to edit global settings and the mod-cfg
 *       dialog to edit individual module settings. Therefore, the create,
 *       ok and cancel function take a GKeyFile as parameter. If the parameter
 *       is NULL we are in global config mode (sat-pref) otherwise in local
 *       config mode.
 */
GtkWidget      *sat_pref_modules_create(GKeyFile * cfg)
{
    GtkWidget      *nbook;

    nbook = gtk_notebook_new();

    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_layout_create(cfg),
                             gtk_label_new(_("Layout")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_refresh_create(cfg),
                             gtk_label_new(_("Refresh Rates")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_list_view_create(cfg),
                             gtk_label_new(_("List View")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_map_view_create(cfg),
                             gtk_label_new(_("Map View")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_polar_view_create(cfg),
                             gtk_label_new(_("Polar View")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_single_sat_create(cfg),
                             gtk_label_new(_("Single Sat View")));

    return nbook;
}

/** User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_modules_cancel(GKeyFile * cfg)
{
    sat_pref_layout_cancel(cfg);
    sat_pref_refresh_cancel(cfg);
    sat_pref_list_view_cancel(cfg);
    sat_pref_map_view_cancel(cfg);
    sat_pref_polar_view_cancel(cfg);
    sat_pref_single_sat_cancel(cfg);
}

/** User pressed OK. Any changes should be stored in config. */
void sat_pref_modules_ok(GKeyFile * cfg)
{
    sat_pref_layout_ok(cfg);
    sat_pref_refresh_ok(cfg);
    sat_pref_list_view_ok(cfg);
    sat_pref_map_view_ok(cfg);
    sat_pref_polar_view_ok(cfg);
    sat_pref_single_sat_ok(cfg);
}
