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

#include "sat-cfg.h"
#include "sat-pref-conditions.h"


static GtkWidget *tzero;
static GtkWidget *minel;
static GtkWidget *numpass;
static GtkWidget *lookahead;
static GtkWidget *res;
static GtkWidget *nument;
static GtkWidget *twspin;

static gboolean dirty = FALSE;  /* used to check whether any changes have occurred */
static gboolean reset = FALSE;


void sat_pref_conditions_cancel()
{
    dirty = FALSE;
    reset = FALSE;
}

void sat_pref_conditions_ok()
{
    if (dirty)
    {
        sat_cfg_set_int(SAT_CFG_INT_PRED_MIN_EL,
                        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                         (minel)));
        sat_cfg_set_int(SAT_CFG_INT_PRED_NUM_PASS,
                        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                         (numpass)));
        sat_cfg_set_int(SAT_CFG_INT_PRED_LOOK_AHEAD,
                        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                         (lookahead)));
        sat_cfg_set_int(SAT_CFG_INT_PRED_RESOLUTION,
                        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                         (res)));
        sat_cfg_set_int(SAT_CFG_INT_PRED_NUM_ENTRIES,
                        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                         (nument)));
        sat_cfg_set_int(SAT_CFG_INT_PRED_TWILIGHT_THLD,
                        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                         (twspin)));
        sat_cfg_set_bool(SAT_CFG_BOOL_PRED_USE_REAL_T0,
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                      (tzero)));

        dirty = FALSE;
    }
    else if (reset)
    {
        sat_cfg_reset_int(SAT_CFG_INT_PRED_MIN_EL);
        sat_cfg_reset_int(SAT_CFG_INT_PRED_NUM_PASS);
        sat_cfg_reset_int(SAT_CFG_INT_PRED_LOOK_AHEAD);
        sat_cfg_reset_int(SAT_CFG_INT_PRED_RESOLUTION);
        sat_cfg_reset_int(SAT_CFG_INT_PRED_NUM_ENTRIES);
        sat_cfg_reset_int(SAT_CFG_INT_PRED_TWILIGHT_THLD);
        sat_cfg_reset_bool(SAT_CFG_BOOL_PRED_USE_REAL_T0);

        reset = FALSE;
    }
}

static void spin_changed_cb(GtkWidget * spinner, gpointer data)
{
    (void)spinner;
    (void)data;

    dirty = TRUE;
}

static void reset_cb(GtkWidget * button, gpointer data)
{
    (void)button;
    (void)data;

    /* get defaults */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minel),
                              sat_cfg_get_int_def(SAT_CFG_INT_PRED_MIN_EL));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(numpass),
                              sat_cfg_get_int_def(SAT_CFG_INT_PRED_NUM_PASS));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookahead),
                              sat_cfg_get_int_def
                              (SAT_CFG_INT_PRED_LOOK_AHEAD));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(res),
                              sat_cfg_get_int_def
                              (SAT_CFG_INT_PRED_RESOLUTION));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(nument),
                              sat_cfg_get_int_def
                              (SAT_CFG_INT_PRED_NUM_ENTRIES));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(twspin),
                              sat_cfg_get_int_def
                              (SAT_CFG_INT_PRED_TWILIGHT_THLD));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tzero),
                                 sat_cfg_get_bool_def
                                 (SAT_CFG_BOOL_PRED_USE_REAL_T0));

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}

static void create_reset_button(GtkBox * vbox)
{
    GtkWidget      *button;
    GtkWidget      *butbox;

    button = gtk_button_new_with_label(_("Reset"));
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(reset_cb), NULL);
    gtk_widget_set_tooltip_text(button,
                                _("Reset settings to the default values."));

    butbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end(GTK_BOX(butbox), button, FALSE, TRUE, 10);

    gtk_box_pack_end(vbox, butbox, FALSE, TRUE, 0);
}

GtkWidget      *sat_pref_conditions_create()
{
    GtkWidget      *table;
    GtkWidget      *label;
    GtkWidget      *vbox;

    dirty = FALSE;
    reset = FALSE;

    table = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(table), FALSE);
    gtk_grid_set_column_homogeneous(GTK_GRID(table), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);

    /* minimum elevation */
    label = gtk_label_new(_("Minimum elevation"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);
    minel = gtk_spin_button_new_with_range(1, 90, 1);
    gtk_widget_set_tooltip_text(minel,
                                _("Elevation threshold for passes.\n"
                                  "Passes with maximum elevation below this limit "
                                  "will be omitted"));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(minel), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(minel), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(minel), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minel),
                              sat_cfg_get_int(SAT_CFG_INT_PRED_MIN_EL));
    g_signal_connect(G_OBJECT(minel), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), minel, 1, 0, 1, 1);
    label = gtk_label_new(_("[deg]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(table),
                    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                    0, 1, 3, 1);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Multiple Passes:</b>"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 2, 1, 1);

    /* number of passes */
    label = gtk_label_new(_("Number of passes to predict"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 3, 1, 1);
    numpass = gtk_spin_button_new_with_range(5, 50, 1);
    gtk_widget_set_tooltip_text(numpass,
                                _("The maximum number of passes to predict."));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(numpass), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(numpass), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(numpass), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(numpass),
                              sat_cfg_get_int(SAT_CFG_INT_PRED_NUM_PASS));
    g_signal_connect(G_OBJECT(numpass), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), numpass, 1, 3, 1, 1);

    /* lookahead */
    label = gtk_label_new(_("Passes should occur within"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 4, 1, 1);
    lookahead = gtk_spin_button_new_with_range(1, 14, 1);
    gtk_widget_set_tooltip_text(lookahead,
                                _("Only passes that occur within the "
                                  "specified number of days will be "
                                  "shown."));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(lookahead), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(lookahead), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(lookahead), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookahead),
                              sat_cfg_get_int(SAT_CFG_INT_PRED_LOOK_AHEAD));
    g_signal_connect(G_OBJECT(lookahead), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), lookahead, 1, 4, 1, 1);

    label = gtk_label_new(_("[days]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 4, 1, 1);

    gtk_grid_attach(GTK_GRID(table),
                    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                    0, 5, 3, 1);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Pass Details:</b>"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 6, 1, 1);

    /* time resolution */
    label = gtk_label_new(_("Time resolution"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 7, 1, 1);
    res = gtk_spin_button_new_with_range(1, 600, 1);
    gtk_widget_set_tooltip_text(res,
                                _("Gpredict will try to show the pass details "
                                  "with the specified time resolution."));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(res), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(res), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(res), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(res),
                              sat_cfg_get_int(SAT_CFG_INT_PRED_RESOLUTION));
    g_signal_connect(G_OBJECT(res), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), res, 1, 7, 1, 1);
    label = gtk_label_new(_("[sec]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 7, 1, 1);

    /* number of entries */
    label = gtk_label_new(_("Number of entries"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 8, 1, 1);
    nument = gtk_spin_button_new_with_range(10, 200, 1);
    gtk_widget_set_tooltip_text(nument,
                                _("Gpredict will try to keep the number "
                                  "of rows in the detailed prediction "
                                  "within this limit."));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(nument), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(nument), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(nument), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(nument),
                              sat_cfg_get_int(SAT_CFG_INT_PRED_NUM_ENTRIES));
    g_signal_connect(G_OBJECT(nument), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), nument, 1, 8, 1, 1);

    gtk_grid_attach(GTK_GRID(table),
                     gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                     0, 9, 3, 1);

    /* satellite visibility */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Satellite Visibility:</b>"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 10, 1, 1);

    /* twilight threshold */
    label = gtk_label_new(_("Twilight threshold"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 11, 1, 1);
    twspin = gtk_spin_button_new_with_range(-18, 0, 1);
    gtk_widget_set_tooltip_text(twspin,
                                _("Satellites are only considered "
                                  "visible if the elevation of the Sun "
                                  "is below the specified threshold.\n"
                                  "  \342\200\242 Astronomical: -18\302\260 to -12\302\260\n"
                                  "  \342\200\242 Nautical: -12\302\260 to -6\302\260\n"
                                  "  \342\200\242 Civil: -6\302\260 to 0\302\260"));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(twspin), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(twspin), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(twspin), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(twspin),
                              sat_cfg_get_int(SAT_CFG_INT_PRED_TWILIGHT_THLD));
    g_signal_connect(G_OBJECT(twspin), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), twspin, 1, 11, 1, 1);
    label = gtk_label_new(_("[deg]"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 11, 1, 1);

    gtk_grid_attach(GTK_GRID(table),
                    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                    0, 12, 3, 1);

    /* T0 for predictions */
    tzero = gtk_check_button_new_with_label(_("Always use real time for "
                                              "pass predictions"));
    gtk_widget_set_tooltip_text(tzero,
                                _("Check this box if you want Gpredict "
                                  "to always use "
                                  "the current (real) time as starting time when predicting "
                                  "future satellite passes.\n\n"
                                  "If you leave the box unchecked and the time controller is "
                                  "active, Gpredict will use the time from the time controller "
                                  "as starting time for predicting satellite passes."));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tzero),
                                 sat_cfg_get_bool
                                 (SAT_CFG_BOOL_PRED_USE_REAL_T0));
    g_signal_connect(G_OBJECT(tzero), "toggled", G_CALLBACK(spin_changed_cb),
                     NULL);

    gtk_grid_attach(GTK_GRID(table), tzero, 0, 13, 3, 1);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

    /* create RESET button */
    create_reset_button(GTK_BOX(vbox));

    return vbox;
}
