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
#include "sat-pref-conditions.h"
#include "sat-pref-multi-pass.h"
#include "sat-pref-predict.h"
#include "sat-pref-single-pass.h"
#include "sat-pref-sky-at-glance.h"


/**
 * Create and initialise widgets for the predictor tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be loaded.
 */
GtkWidget      *sat_pref_predict_create()
{
    GtkWidget      *nbook;

    nbook = gtk_notebook_new();

    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_conditions_create(),
                             gtk_label_new(_("Pass Conditions")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_multi_pass_create(),
                             gtk_label_new(_("Multiple Passes")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_single_pass_create(),
                             gtk_label_new(_("Single Pass")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_sky_at_glance_create(),
                             gtk_label_new(_("Sky at a Glance")));

    return nbook;
}

/** User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_predict_cancel()
{
    sat_pref_conditions_cancel();
    sat_pref_multi_pass_cancel();
    sat_pref_single_pass_cancel();
    sat_pref_sky_at_glance_cancel();
}

/** User pressed OK. Any changes should be stored in config. */
void sat_pref_predict_ok()
{
    sat_pref_conditions_ok();
    sat_pref_multi_pass_ok();
    sat_pref_single_pass_ok();
    sat_pref_sky_at_glance_ok();
}
