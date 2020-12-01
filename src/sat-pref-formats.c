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
#include <time.h>

#include "sat-cfg.h"
#include "sat-pref-formats.h"
#include "sat-pref-qth.h"

/* *INDENT-OFF* */
static const gchar *tztips =
    N_("Display local time instead of UTC. Note: The local time is "
       "that of your operating system and not the local time at the "
       "location, which you select as " "tracking reference.");

static const gchar *tftips =
    N_("Enter a format string using the following codes:\n\n"
       "\t%Y\tYear with century.\n" "\t%m\tMonth (01-12).\n"
       "\t%d\tDay of the month (01-31).\n" "\t%j\tDay of the year (001-366).\n"
       "\t%H\tHour (00-23).\n" "\t%M\tMinute (00-59).\n"
       "\t%S\tSeconds (00-59).\n\n"
       "See the user manual for more codes and examples.");

static const gchar *nsewtips =
    N_("Checking this box will cause geographical "
       "coordinates to be displayed using a suffix "
       "instead of sign (eg. 23.43\302\260W "
       "instead of -23.43\302\260).");

static const gchar *imptips =
    N_("Display distances using Imperial units, for "
       "example miles instead of kilometres.");
/* *INDENT-ON* */


static GtkWidget *tzcheck;      /* "time zone" check button */
static GtkWidget *tfentry;      /* time format entry */
static GtkWidget *tflabel;      /* time format label, preview */
static GtkWidget *tfreset;      /* time format reset button */
static GtkWidget *nsewcheck;    /* N/S/W/E check button */
static GtkWidget *impcheck;     /* Use imperial units */
static guint    timer;
static gboolean useimporg;      /* original value for use imperial */

static gboolean tfprev_cb(gpointer data);
static void     systog_cb(GtkToggleButton * togglebutton, gpointer user_data);
static void     reset_cb(GtkWidget * button, gpointer data);

/** Create and initialise widgets for number formats tab. */
GtkWidget      *sat_pref_formats_create()
{
    GtkWidget      *vbox, *tfbox;
    gchar          *text;

    /* use local time */
    tzcheck =
        gtk_check_button_new_with_label(_("Show local time instead of UTC."));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tzcheck),
                                 sat_cfg_get_bool
                                 (SAT_CFG_BOOL_USE_LOCAL_TIME));
    gtk_widget_set_tooltip_text(tzcheck, _(tztips));

    /* time format */
    tfentry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(tfentry), TIME_FORMAT_MAX_LENGTH);
    gtk_widget_set_tooltip_text(tfentry, _(tftips));

    text = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
    gtk_entry_set_text(GTK_ENTRY(tfentry), text);
    g_free(text);

    tflabel = gtk_label_new("--/--/-- --:--:--");

    /* periodic update of preview label */
    timer = g_timeout_add(1000, tfprev_cb, NULL);

    /* reset button */
    tfreset = gtk_button_new_with_label(_("Reset"));
    g_signal_connect(tfreset, "clicked", G_CALLBACK(reset_cb), NULL);
    gtk_widget_set_tooltip_text(tfreset, _("Reset to default value"));

    tfbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(tfbox), FALSE);
    gtk_box_pack_start(GTK_BOX(tfbox), gtk_label_new(_("Time format:")),
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tfbox), tfentry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tfbox), tflabel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tfbox), tfreset, FALSE, FALSE, 5);

    /* N/S/W/E */
    nsewcheck =
        gtk_check_button_new_with_label(_
                                        ("Use N/S/E/W for geographical coordinates."));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nsewcheck),
                                 sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW));
    gtk_widget_set_tooltip_text(nsewcheck, _(nsewtips));

    /* unit */
    useimporg = sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL);
    impcheck =
        gtk_check_button_new_with_label(_
                                        ("Use Imperial units instead of Metric."));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(impcheck), useimporg);
    gtk_widget_set_tooltip_text(impcheck, _(imptips));
    /* connect sat-pref-qth hook */
    g_signal_connect(impcheck, "toggled", G_CALLBACK(systog_cb), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_box_pack_start(GTK_BOX(vbox), tzcheck, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tfbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), nsewcheck, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), impcheck, FALSE, FALSE, 0);

    return vbox;
}

/** User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_formats_cancel()
{
    /* restore imperial setting to it's original value */
    sat_cfg_set_bool(SAT_CFG_BOOL_USE_IMPERIAL, useimporg);

    g_source_remove(timer);
}

/** User pressed OK. Any changes should be stored in config. */
void sat_pref_formats_ok()
{
    g_source_remove(timer);

    sat_cfg_set_bool(SAT_CFG_BOOL_USE_LOCAL_TIME,
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tzcheck)));
    sat_cfg_set_str(SAT_CFG_STR_TIME_FORMAT,
                    gtk_entry_get_text(GTK_ENTRY(tfentry)));
    sat_cfg_set_bool(SAT_CFG_BOOL_USE_NSEW,
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                  (nsewcheck)));
    sat_cfg_set_bool(SAT_CFG_BOOL_USE_IMPERIAL,
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                  (impcheck)));
}

static gboolean tfprev_cb(gpointer data)
{
    const gchar    *fmtstr;
    GDateTime      *now;
    gchar          *text;

    (void)data;

    fmtstr = gtk_entry_get_text(GTK_ENTRY(tfentry));

    /* get either local time or UTC depending on check box */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tzcheck)))
	now = g_date_time_new_now_local();
    else
	now = g_date_time_new_now_utc();

    text = g_date_time_format(now, fmtstr);
    gtk_label_set_text(GTK_LABEL(tflabel), text);

    g_date_time_unref(now);
    g_free(text);

    return TRUE;
}

/** Manage system toggle button signals */
static void systog_cb(GtkToggleButton * togglebutton, gpointer user_data)
{
    (void)user_data;

    sat_cfg_set_bool(SAT_CFG_BOOL_USE_IMPERIAL,
                     gtk_toggle_button_get_active(togglebutton));
    sat_pref_qth_sys_changed(gtk_toggle_button_get_active(togglebutton));
}

/** Reset time format string to default */
static void reset_cb(GtkWidget * button, gpointer data)
{
    gchar          *fmtstr;

    (void)button;
    (void)data;

    fmtstr = sat_cfg_get_str_def(SAT_CFG_STR_TIME_FORMAT);
    gtk_entry_set_text(GTK_ENTRY(tfentry), fmtstr);
    g_free(fmtstr);
}
