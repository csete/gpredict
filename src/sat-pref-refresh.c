/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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

#include "config-keys.h"
#include "mod-cfg-get-param.h"
#include "sat-cfg.h"
#include "sat-pref-refresh.h"


static GtkWidget *dataspin;     /* spin button for module refresh rate */
static GtkWidget *listspin;     /* spin button for list view */
static GtkWidget *mapspin;      /* spin button for map view */
static GtkWidget *polarspin;    /* spin button for polar view */
static GtkWidget *singlespin;   /* spin button for single-sat view */

static gboolean dirty = FALSE;  /* used to check whether any changes have occurred */
static gboolean reset = FALSE;


/* User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_refresh_cancel(GKeyFile * cfg)
{
    (void)cfg;

    dirty = FALSE;
}

/* User pressed OK. Any changes should be stored in config. */
void sat_pref_refresh_ok(GKeyFile * cfg)
{
    if (dirty)
    {
        if (cfg != NULL)
        {
            g_key_file_set_integer(cfg,
                                   MOD_CFG_GLOBAL_SECTION,
                                   MOD_CFG_TIMEOUT_KEY,
                                   gtk_spin_button_get_value_as_int
                                   (GTK_SPIN_BUTTON(dataspin)));

            g_key_file_set_integer(cfg,
                                   MOD_CFG_LIST_SECTION,
                                   MOD_CFG_LIST_REFRESH,
                                   gtk_spin_button_get_value_as_int
                                   (GTK_SPIN_BUTTON(listspin)));

            g_key_file_set_integer(cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_REFRESH,
                                   gtk_spin_button_get_value_as_int
                                   (GTK_SPIN_BUTTON(mapspin)));

            g_key_file_set_integer(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_REFRESH,
                                   gtk_spin_button_get_value_as_int
                                   (GTK_SPIN_BUTTON(polarspin)));

            g_key_file_set_integer(cfg,
                                   MOD_CFG_SINGLE_SAT_SECTION,
                                   MOD_CFG_SINGLE_SAT_REFRESH,
                                   gtk_spin_button_get_value_as_int
                                   (GTK_SPIN_BUTTON(singlespin)));
        }
        else
        {
            sat_cfg_set_int(SAT_CFG_INT_MODULE_TIMEOUT,
                            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                             (dataspin)));

            sat_cfg_set_int(SAT_CFG_INT_LIST_REFRESH,
                            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                             (listspin)));

            sat_cfg_set_int(SAT_CFG_INT_MAP_REFRESH,
                            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                             (mapspin)));

            sat_cfg_set_int(SAT_CFG_INT_POLAR_REFRESH,
                            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                             (polarspin)));

            sat_cfg_set_int(SAT_CFG_INT_SINGLE_SAT_REFRESH,
                            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                             (singlespin)));
        }
    }
    else if (reset)
    {
        /* we have to reset the values to global or default settings */
        if (cfg == NULL)
        {
            /* reset values in sat-cfg */
            sat_cfg_reset_int(SAT_CFG_INT_MODULE_TIMEOUT);
            sat_cfg_reset_int(SAT_CFG_INT_LIST_REFRESH);
            sat_cfg_reset_int(SAT_CFG_INT_MAP_REFRESH);
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_REFRESH);
            sat_cfg_reset_int(SAT_CFG_INT_SINGLE_SAT_REFRESH);
        }
        else
        {
            /* remove keys */
            g_key_file_remove_key((GKeyFile *) (cfg),
                                  MOD_CFG_GLOBAL_SECTION,
                                  MOD_CFG_TIMEOUT_KEY, NULL);
            g_key_file_remove_key((GKeyFile *) (cfg),
                                  MOD_CFG_LIST_SECTION,
                                  MOD_CFG_LIST_REFRESH, NULL);
            g_key_file_remove_key((GKeyFile *) (cfg),
                                  MOD_CFG_MAP_SECTION,
                                  MOD_CFG_MAP_REFRESH, NULL);
            g_key_file_remove_key((GKeyFile *) (cfg),
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_REFRESH, NULL);
            g_key_file_remove_key((GKeyFile *) (cfg),
                                  MOD_CFG_SINGLE_SAT_SECTION,
                                  MOD_CFG_SINGLE_SAT_REFRESH, NULL);
        }
    }

    dirty = FALSE;
    reset = FALSE;
}

static void spin_changed_cb(GtkWidget * spinner, gpointer data)
{
    (void)spinner;
    (void)data;

    dirty = TRUE;
}

/**
 * Reset settings.
 *
 * @param button The RESET button.
 * @param cfg Pointer to the module config or NULL in global mode.
 *
 * This function is called when the user clicks on the RESET button. In global mode
 * (when cfg = NULL) the function will reset the settings to the efault values, while
 * in "local" mode (when cfg != NULL) the function will reset the module settings to
 * the global settings. This is done by removing the corresponding key from the GKeyFile.
 */
static void reset_cb(GtkWidget * button, gpointer cfg)
{
    gint            val;

    (void)button;

    /* views */
    if (cfg == NULL)
    {
        /* global mode, get defaults */
        val = sat_cfg_get_int_def(SAT_CFG_INT_MODULE_TIMEOUT);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataspin), val);
        val = sat_cfg_get_int_def(SAT_CFG_INT_LIST_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(listspin), val);
        val = sat_cfg_get_int_def(SAT_CFG_INT_MAP_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mapspin), val);
        val = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(polarspin), val);
        val = sat_cfg_get_int_def(SAT_CFG_INT_SINGLE_SAT_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(singlespin), val);
    }
    else
    {
        /* local mode, get global value */
        val = sat_cfg_get_int(SAT_CFG_INT_MODULE_TIMEOUT);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataspin), val);
        val = sat_cfg_get_int(SAT_CFG_INT_LIST_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(listspin), val);
        val = sat_cfg_get_int(SAT_CFG_INT_MAP_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mapspin), val);
        val = sat_cfg_get_int(SAT_CFG_INT_POLAR_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(polarspin), val);
        val = sat_cfg_get_int(SAT_CFG_INT_SINGLE_SAT_REFRESH);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(singlespin), val);
    }

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}

static void create_reset_button(GKeyFile * cfg, GtkBox * vbox)
{
    GtkWidget      *button;
    GtkWidget      *butbox;

    button = gtk_button_new_with_label(_("Reset"));
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(reset_cb), cfg);

    if (cfg == NULL)
    {
        gtk_widget_set_tooltip_text(button,
                                    _
                                    ("Reset settings to the default values."));
    }
    else
    {
        gtk_widget_set_tooltip_text(button,
                                    _
                                    ("Reset module settings to the global values."));
    }

    butbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end(GTK_BOX(butbox), button, FALSE, TRUE, 10);

    gtk_box_pack_end(vbox, butbox, FALSE, TRUE, 0);

}

GtkWidget      *sat_pref_refresh_create(GKeyFile * cfg)
{
    GtkWidget      *table;
    GtkWidget      *vbox;
    GtkWidget      *label;
    gint            val;

    dirty = FALSE;
    reset = FALSE;

    table = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);

    /* data refresh */
    label = gtk_label_new(_("Refresh data every"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    dataspin = gtk_spin_button_new_with_range(100, 10000, 1);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(dataspin), 1, 100);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dataspin), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(dataspin),
                                      GTK_UPDATE_IF_VALID);
    if (cfg != NULL)
    {
        val = mod_cfg_get_int(cfg,
                              MOD_CFG_GLOBAL_SECTION,
                              MOD_CFG_TIMEOUT_KEY, SAT_CFG_INT_MODULE_TIMEOUT);
    }
    else
    {
        val = sat_cfg_get_int(SAT_CFG_INT_MODULE_TIMEOUT);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dataspin), val);
    g_signal_connect(G_OBJECT(dataspin), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), dataspin, 1, 0, 1, 1);

    label = gtk_label_new(_("[msec]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(table),
                    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 1, 3, 1);

    /* List View */
    label = gtk_label_new(_("Refresh list view every"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 2, 1, 1);

    listspin = gtk_spin_button_new_with_range(1, 50, 1);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(listspin), 1, 5);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(listspin), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(listspin),
                                      GTK_UPDATE_IF_VALID);
    if (cfg != NULL)
    {
        val = mod_cfg_get_int(cfg,
                              MOD_CFG_LIST_SECTION,
                              MOD_CFG_LIST_REFRESH, SAT_CFG_INT_LIST_REFRESH);
    }
    else
    {
        val = sat_cfg_get_int(SAT_CFG_INT_LIST_REFRESH);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(listspin), val);
    g_signal_connect(G_OBJECT(listspin), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), listspin, 1, 2, 1, 1);

    label = gtk_label_new(_("[cycle]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 2, 1, 1);

    /* Map View */
    label = gtk_label_new(_("Refresh map view every"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 3, 1, 1);

    mapspin = gtk_spin_button_new_with_range(1, 50, 1);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(mapspin), 1, 5);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mapspin), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mapspin),
                                      GTK_UPDATE_IF_VALID);
    if (cfg != NULL)
    {
        val = mod_cfg_get_int(cfg,
                              MOD_CFG_MAP_SECTION,
                              MOD_CFG_MAP_REFRESH, SAT_CFG_INT_MAP_REFRESH);
    }
    else
    {
        val = sat_cfg_get_int(SAT_CFG_INT_MAP_REFRESH);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mapspin), val);
    g_signal_connect(G_OBJECT(mapspin), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), mapspin, 1, 3, 1, 1);

    label = gtk_label_new(_("[cycle]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 3, 1, 1);

    /* Polar View */
    label = gtk_label_new(_("Refresh polar view every"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 4, 1, 1);

    polarspin = gtk_spin_button_new_with_range(1, 50, 1);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(polarspin), 1, 5);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(polarspin), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(polarspin),
                                      GTK_UPDATE_IF_VALID);
    if (cfg != NULL)
    {
        val = mod_cfg_get_int(cfg,
                              MOD_CFG_POLAR_SECTION,
                              MOD_CFG_POLAR_REFRESH,
                              SAT_CFG_INT_POLAR_REFRESH);
    }
    else
    {
        val = sat_cfg_get_int(SAT_CFG_INT_POLAR_REFRESH);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(polarspin), val);
    g_signal_connect(G_OBJECT(polarspin), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), polarspin, 1, 4, 1, 1);

    label = gtk_label_new(_("[cycle]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 4, 1, 1);

    /* Single-Sat View */
    label = gtk_label_new(_("Refresh single-sat view every"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 5, 1, 1);

    singlespin = gtk_spin_button_new_with_range(1, 50, 1);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(singlespin), 1, 5);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(singlespin), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(singlespin),
                                      GTK_UPDATE_IF_VALID);
    if (cfg != NULL)
    {
        val = mod_cfg_get_int(cfg,
                              MOD_CFG_SINGLE_SAT_SECTION,
                              MOD_CFG_SINGLE_SAT_REFRESH,
                              SAT_CFG_INT_SINGLE_SAT_REFRESH);
    }
    else
    {
        val = sat_cfg_get_int(SAT_CFG_INT_SINGLE_SAT_REFRESH);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(singlespin), val);
    g_signal_connect(G_OBJECT(singlespin), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), singlespin, 1, 5, 1, 1);

    label = gtk_label_new(_("[cycle]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 5, 1, 1);

    /* create vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

    /* create RESET button */
    create_reset_button(cfg, GTK_BOX(vbox));

    return vbox;
}
