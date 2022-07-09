/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2015  Alexandru Csete, OZ9AEC.

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
#include "sat-pref-interfaces.h"
#include "sat-pref-rig.h"
#include "sat-pref-rot.h"

/**
 * Create and initialise widgets for the hardware interfaces prefs tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be loaded.
 */
GtkWidget      *sat_pref_interfaces_create()
{
    GtkWidget      *nbook;

    nbook = gtk_notebook_new();

    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_rig_create(),
                             gtk_label_new(_("Radios")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_rot_create(),
                             gtk_label_new(_("Rotators")));

    return nbook;
}

/** User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_interfaces_cancel()
{
    sat_pref_rig_cancel();
    sat_pref_rot_cancel();
}

/** User pressed OK. Any changes should be stored in config. */
void sat_pref_interfaces_ok()
{
    sat_pref_rig_ok();
    sat_pref_rot_ok();
}
