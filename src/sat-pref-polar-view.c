/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC

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
#include "gpredict-utils.h"
#include "gtk-polar-view.h"
#include "mod-cfg-get-param.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-polar-view.h"

/* orientation selectors */
static GtkWidget *nesw, *nwse, *senw, *swne;

/* content selectors */
static GtkWidget *qth, *next, *curs, *xtick;

/* colour selectors */
static GtkWidget *bgd, *axis, *tick, *sat, *ssat, *track, *info;

/* Misc */
static GtkWidget *showtrack;

/* misc bookkeeping */
static gint     orient = POLAR_VIEW_NESW;
static gboolean dirty = FALSE;
static gboolean reset = FALSE;


/**
 * Manage orientation changes.
 *
 * @param but the button that received the signal
 * @param data User data (always NULL).
 *
 * Note that this function is called also for the button that is unchecked,
 *
 */
static void orient_chganged(GtkToggleButton * but, gpointer data)
{
    if (gtk_toggle_button_get_active(but))
    {
        orient = GPOINTER_TO_INT(data);
        dirty = TRUE;
    }
}

/**
 * Manage check-box actions.
 *
 * @param but The check-button that has been toggled.
 * @param daya User data (always NULL).
 *
 * We don't need to do anything but set the dirty flag since the values can
 * always be obtained from the global widgets.
 */
static void content_changed(GtkToggleButton * but, gpointer data)
{
    (void)but;
    (void)data;

    dirty = TRUE;
}

/**
 * Manage color and font changes.
 *
 * @param but The color/font picker button that received the signal.
 * @param data User data (always NULL).
 *
 * We don't need to do anything but set the dirty flag since the values can
 * always be obtained from the global widgets.
 */
static void colour_changed(GtkWidget * but, gpointer data)
{
    (void)but;
    (void)data;

    dirty = TRUE;
}

/**
 * Manage RESET button signals.
 *
 * @param button The RESET button.
 * @param cfg Pointer to the module configuration or NULL in global mode.
 *
 * This function is called when the user clicks on the RESET button. In global mode
 * (when cfg = NULL) the function will reset the settings to the efault values, while
 * in "local" mode (when cfg != NULL) the function will reset the module settings to
 * the global settings. This is done by removing the corresponding key from the GKeyFile.
 */
static void reset_cb(GtkWidget * button, gpointer cfg)
{
    guint           cfg_rgba;   /* 0xRRGGBBAA encoded color */
    GdkRGBA         gdk_rgba;

    (void)button;

    if (cfg == NULL)
    {
        /* global mode, get defaults */

        /* extra contents */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(qth),
                                     sat_cfg_get_bool_def
                                     (SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(next),
                                     sat_cfg_get_bool_def
                                     (SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(curs),
                                     sat_cfg_get_bool_def
                                     (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xtick),
                                     sat_cfg_get_bool_def
                                     (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));

        /* colours */
        cfg_rgba = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_BGD_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(bgd), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_AXIS_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(axis), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_TICK_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(tick), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_SAT_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(sat), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_SAT_SEL_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(ssat), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_TRACK_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(track), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_INFO_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(info), &gdk_rgba);

        /* misc */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showtrack),
                                     sat_cfg_get_bool_def
                                     (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));
    }
    else
    {
        /* local mode, get global value */

        /* extra contents */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(qth),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(next),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(curs),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xtick),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));

        /* colours */
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_BGD_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(bgd), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_AXIS_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(axis), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_TICK_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(tick), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(sat), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_SEL_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(ssat), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_TRACK_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(track), &gdk_rgba);

        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_INFO_COL);
        rgba_from_cfg(cfg_rgba, &gdk_rgba);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(info), &gdk_rgba);

        /* misc */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showtrack),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));
    }

    /* orientation needs some special attention */
    if (cfg == NULL)
    {
        orient = sat_cfg_get_int_def(SAT_CFG_INT_POLAR_ORIENTATION);
    }
    else
    {
        orient = sat_cfg_get_int(SAT_CFG_INT_POLAR_ORIENTATION);
    }

    switch (orient)
    {

    case POLAR_VIEW_NESW:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nesw), TRUE);
        break;

    case POLAR_VIEW_NWSE:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nwse), TRUE);
        break;

    case POLAR_VIEW_SENW:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(senw), TRUE);
        break;

    case POLAR_VIEW_SWNE:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(swne), TRUE);
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Invalid chart orientation %d (using N/E/S/W)"),
                    __FILE__, __func__, orient);
        orient = POLAR_VIEW_NESW;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nesw), TRUE);
        break;
    }

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}


/**
 * Create orientation chooser.
 *
 * @param cfg The module configuration or NULL in global mode.
 * @param vbox The container box in which the widgets should be packed into.
 *
 * The orientation selector consists of a radio-group having four radio
 * buttons: N/E/S/W, N/W/S/E, S/E/N/W, and S/W/N/E
 */
static void create_orient_selector(GKeyFile * cfg, GtkBox * vbox)
{
    GtkWidget      *label;
    GtkWidget      *hbox;

    /* create header */
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Orientation:</b>"));
    gtk_box_pack_start(vbox, label, FALSE, TRUE, 0);

    /* horizontal box to contain the radio buttons */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
    gtk_box_pack_start(vbox, hbox, FALSE, TRUE, 0);

    /* N/E/S/W */
    nesw = gtk_radio_button_new_with_label(NULL, _("N/E/S/W"));
    gtk_box_pack_start(GTK_BOX(hbox), nesw, FALSE, TRUE, 0);
    gtk_widget_set_tooltip_text(nesw, _("\tN\n" "W\t\tE\n" "\tS"));

    /* N/W/S/E */
    nwse =
        gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(nesw),
                                                    _("N/W/S/E"));
    gtk_box_pack_start(GTK_BOX(hbox), nwse, FALSE, TRUE, 0);
    gtk_widget_set_tooltip_text(nwse, _("\tN\n" "E\t\tW\n" "\tS"));
    /* S/E/N/W */
    senw =
        gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(nesw),
                                                    _("S/E/N/W"));
    gtk_box_pack_start(GTK_BOX(hbox), senw, FALSE, TRUE, 0);
    gtk_widget_set_tooltip_text(senw, _("\tS\n" "W\t\tE\n" "\tN"));

    /* S/W/N/E */
    swne =
        gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(nesw),
                                                    _("S/W/N/E"));
    gtk_box_pack_start(GTK_BOX(hbox), swne, FALSE, TRUE, 0);
    gtk_widget_set_tooltip_text(swne, _("\tS\n" "E\t\tW\n" "\tN"));

    /* read orientation */
    if (cfg != NULL)
    {
        orient = mod_cfg_get_int(cfg,
                                 MOD_CFG_POLAR_SECTION,
                                 MOD_CFG_POLAR_ORIENTATION,
                                 SAT_CFG_INT_POLAR_ORIENTATION);
    }
    else
    {
        sat_cfg_get_int(SAT_CFG_INT_POLAR_ORIENTATION);
    }

    switch (orient)
    {

    case POLAR_VIEW_NESW:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nesw), TRUE);
        break;

    case POLAR_VIEW_NWSE:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nwse), TRUE);
        break;

    case POLAR_VIEW_SENW:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(senw), TRUE);
        break;

    case POLAR_VIEW_SWNE:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(swne), TRUE);
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%f:%d: Invalid PolarView orientation (%d)"),
                    __FILE__, __LINE__, orient);
        orient = POLAR_VIEW_NESW;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nesw), TRUE);
        break;
    }

    /* finally, connect the signals */
    g_signal_connect(nesw, "toggled",
                     G_CALLBACK(orient_chganged),
                     GINT_TO_POINTER(POLAR_VIEW_NESW));
    g_signal_connect(nwse, "toggled",
                     G_CALLBACK(orient_chganged),
                     GINT_TO_POINTER(POLAR_VIEW_NWSE));
    g_signal_connect(senw, "toggled",
                     G_CALLBACK(orient_chganged),
                     GINT_TO_POINTER(POLAR_VIEW_SENW));
    g_signal_connect(swne, "toggled",
                     G_CALLBACK(orient_chganged),
                     GINT_TO_POINTER(POLAR_VIEW_SWNE));
}

/**
 * Create content selector widgets.
 * @param cfg The module configuration or NULL in global mode.
 * @param vbox The container box in which the widgets should be packed into.
 *
 * This function creates the widgets that are used to select what content, besides
 * the satellites, should be drawn on the polar view. Choices are QTH info, next
 * event, cursor coordinates, and extra tick marks.
 */
static void create_bool_selectors(GKeyFile * cfg, GtkBox * vbox)
{
    GtkWidget      *label;
    GtkWidget      *hbox;

    /* create header */
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Extra Contents:</b>"));
    gtk_box_pack_start(vbox, label, FALSE, TRUE, 0);

    /* horizontal box to contain the radio buttons */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
    gtk_box_pack_start(vbox, hbox, FALSE, TRUE, 0);

    /* QTH info */
    qth = gtk_check_button_new_with_label(_("QTH Info"));
    gtk_widget_set_tooltip_text(qth,
                                _
                                ("Show location information on the polar plot"));
    if (cfg != NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(qth),
                                     mod_cfg_get_bool(cfg,
                                                      MOD_CFG_POLAR_SECTION,
                                                      MOD_CFG_POLAR_SHOW_QTH_INFO,
                                                      SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(qth),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
    }
    g_signal_connect(qth, "toggled", G_CALLBACK(content_changed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), qth, FALSE, TRUE, 0);

    /* Next Event */
    next = gtk_check_button_new_with_label(_("Next Event"));
    gtk_widget_set_tooltip_text(next,
                                _
                                ("Show which satellites comes up next and at what time"));
    if (cfg != NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(next),
                                     mod_cfg_get_bool(cfg,
                                                      MOD_CFG_POLAR_SECTION,
                                                      MOD_CFG_POLAR_SHOW_NEXT_EVENT,
                                                      SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(next),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
    }
    g_signal_connect(next, "toggled", G_CALLBACK(content_changed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), next, FALSE, TRUE, 0);

    /* Cursor position */
    curs = gtk_check_button_new_with_label(_("Cursor Position"));
    gtk_widget_set_tooltip_text(curs,
                                _
                                ("Show the azimuth and elevation of the mouse pointer"));
    if (cfg != NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(curs),
                                     mod_cfg_get_bool(cfg,
                                                      MOD_CFG_POLAR_SECTION,
                                                      MOD_CFG_POLAR_SHOW_CURS_TRACK,
                                                      SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(curs),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
    }
    g_signal_connect(curs, "toggled", G_CALLBACK(content_changed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), curs, FALSE, TRUE, 0);

    /* Extra tick marks */
    xtick = gtk_check_button_new_with_label(_("Extra Az Ticks"));
    gtk_widget_set_tooltip_text(xtick,
                                _
                                ("Show extra tick marks for every 30\302\260"));
    if (cfg != NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xtick),
                                     mod_cfg_get_bool(cfg,
                                                      MOD_CFG_POLAR_SECTION,
                                                      MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS,
                                                      SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xtick),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));
    }
    g_signal_connect(xtick, "toggled", G_CALLBACK(content_changed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), xtick, FALSE, TRUE, 0);
}

/**
 * Create color selector widgets.
 *
 * @param cfg The module configuration or NULL in global mode.
 * @param vbox The container box in which the widgets should be packed into.
 *
 * This function creates the widgets for selecting colours for the plot background,
 * axes, tick labels, satellites, track, and info text.
 */
static void create_colour_selectors(GKeyFile * cfg, GtkBox * vbox)
{
    GtkWidget      *label;
    GtkWidget      *table;
    guint           cfg_rgba;   /* 0xRRGGBBAA encoded color */
    GdkRGBA         gdk_rgba;

    /* create header */
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Colours:</b>"));
    gtk_box_pack_start(vbox, label, FALSE, TRUE, 0);

    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table), FALSE);
    gtk_grid_set_row_homogeneous(GTK_GRID(table), TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
    gtk_grid_set_row_spacing(GTK_GRID(table), 3);
    gtk_box_pack_start(vbox, table, FALSE, TRUE, 0);

    /* background */
    label = gtk_label_new(_("Background:"));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);
    bgd = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(bgd), TRUE);
    gtk_grid_attach(GTK_GRID(table), bgd, 1, 0, 1, 1);
    gtk_widget_set_tooltip_text(bgd, _("Click to select background colour"));
    if (cfg != NULL)
    {
        cfg_rgba = mod_cfg_get_int(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_BGD_COL,
                                   SAT_CFG_INT_POLAR_BGD_COL);
    }
    else
    {
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_BGD_COL);
    }
    rgba_from_cfg(cfg_rgba, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(bgd), &gdk_rgba);
    g_signal_connect(bgd, "color-set", G_CALLBACK(colour_changed), NULL);

    /* Axis */
    label = gtk_label_new(_("Axes/Circles:"));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 0, 1, 1);
    axis = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(axis), TRUE);
    gtk_grid_attach(GTK_GRID(table), axis, 3, 0, 1, 1);
    gtk_widget_set_tooltip_text(axis, _("Click to select the axis colour"));
    if (cfg != NULL)
    {
        cfg_rgba = mod_cfg_get_int(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_AXIS_COL,
                                   SAT_CFG_INT_POLAR_AXIS_COL);
    }
    else
    {
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_AXIS_COL);
    }
    rgba_from_cfg(cfg_rgba, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(axis), &gdk_rgba);
    g_signal_connect(axis, "color-set", G_CALLBACK(colour_changed), NULL);

    /* tick labels */
    label = gtk_label_new(_("Tick Labels:"));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 4, 0, 1, 1);
    tick = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(tick), TRUE);
    gtk_grid_attach(GTK_GRID(table), tick, 5, 0, 1, 1);
    gtk_widget_set_tooltip_text(tick,
                                _
                                ("Click to select the colour for tick labels"));
    if (cfg != NULL)
    {
        cfg_rgba = mod_cfg_get_int(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_TICK_COL,
                                   SAT_CFG_INT_POLAR_TICK_COL);
    }
    else
    {
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_TICK_COL);
    }
    rgba_from_cfg(cfg_rgba, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(tick), &gdk_rgba);
    g_signal_connect(tick, "color-set", G_CALLBACK(colour_changed), NULL);

    /* satellite */
    label = gtk_label_new(_("Satellite:"));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);
    sat = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(sat), TRUE);
    gtk_grid_attach(GTK_GRID(table), sat, 1, 1, 1, 1);
    gtk_widget_set_tooltip_text(sat, _("Click to select satellite colour"));
    if (cfg != NULL)
    {
        cfg_rgba = mod_cfg_get_int(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SAT_COL,
                                   SAT_CFG_INT_POLAR_SAT_COL);
    }
    else
    {
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_COL);
    }
    rgba_from_cfg(cfg_rgba, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(sat), &gdk_rgba);
    g_signal_connect(sat, "color-set", G_CALLBACK(colour_changed), NULL);

    /* selected satellite */
    label = gtk_label_new(_("Selected Sat.:"));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 1, 1, 1);
    ssat = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(ssat), TRUE);
    gtk_grid_attach(GTK_GRID(table), ssat, 3, 1, 1, 1);
    gtk_widget_set_tooltip_text(ssat,
                                _
                                ("Click to select colour for selected satellites"));
    if (cfg != NULL)
    {
        cfg_rgba = mod_cfg_get_int(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SAT_SEL_COL,
                                   SAT_CFG_INT_POLAR_SAT_SEL_COL);
    }
    else
    {
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_SEL_COL);
    }
    rgba_from_cfg(cfg_rgba, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(ssat), &gdk_rgba);
    g_signal_connect(ssat, "color-set", G_CALLBACK(colour_changed), NULL);

    /* track */
    label = gtk_label_new(_("Sky Track:"));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 4, 1, 1, 1);
    track = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(track), TRUE);
    gtk_grid_attach(GTK_GRID(table), track, 5, 1, 1, 1);
    gtk_widget_set_tooltip_text(track, _("Click to select track colour"));
    if (cfg != NULL)
    {
        cfg_rgba = mod_cfg_get_int(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_TRACK_COL,
                                   SAT_CFG_INT_POLAR_TRACK_COL);
    }
    else
    {
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_TRACK_COL);
    }
    rgba_from_cfg(cfg_rgba, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(track), &gdk_rgba);
    g_signal_connect(track, "color-set", G_CALLBACK(colour_changed), NULL);

    /* Info */
    label = gtk_label_new(_("Info Text:"));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 2, 1, 1);
    info = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(info), TRUE);
    gtk_grid_attach(GTK_GRID(table), info, 1, 2, 1, 1);
    gtk_widget_set_tooltip_text(info, _("Click to select background colour"));
    if (cfg != NULL)
    {
        cfg_rgba = mod_cfg_get_int(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_INFO_COL,
                                   SAT_CFG_INT_POLAR_INFO_COL);
    }
    else
    {
        cfg_rgba = sat_cfg_get_int(SAT_CFG_INT_POLAR_INFO_COL);
    }
    rgba_from_cfg(cfg_rgba, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(info), &gdk_rgba);
    g_signal_connect(info, "color-set", G_CALLBACK(colour_changed), NULL);
}

/**
 * Create miscellaneous config widgets.
 * @param cfg The module configuration or NULL in global mode.
 * @param vbox The container box in which the widgets should be packed into.
 *
 */
static void create_misc_selectors(GKeyFile * cfg, GtkBox * vbox)
{
    GtkWidget      *label;
    GtkWidget      *hbox;

    /* create header */
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Miscellaneous:</b>"));
    gtk_box_pack_start(vbox, label, FALSE, TRUE, 0);

    /* horizontal box to contain the radio buttons */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
    gtk_box_pack_start(vbox, hbox, FALSE, TRUE, 0);

    /* show sky tracks */
    showtrack = gtk_check_button_new_with_label(_("Show the sky tracks "
                                                  "automatically"));
    gtk_widget_set_tooltip_text(showtrack,
                                _("Automatically show the sky track of a "
                                  "satellite when it comes in range. You can always turn the "
                                  "sky track OFF for each individual object"));
    if (cfg != NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showtrack),
                                     mod_cfg_get_bool(cfg,
                                                      MOD_CFG_POLAR_SECTION,
                                                      MOD_CFG_POLAR_SHOW_TRACK_AUTO,
                                                      SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showtrack),
                                     sat_cfg_get_bool
                                     (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));
    }
    g_signal_connect(showtrack, "toggled", G_CALLBACK(content_changed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), showtrack, FALSE, TRUE, 0);
}

/**
 * Create RESET button.
 * @param cfg Config data or NULL in global mode.
 * @param vbox The container.
 *
 * This function creates and sets up the RESET button.
 */
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

/* User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_polar_view_cancel(GKeyFile * cfg)
{
    (void)cfg;

    dirty = FALSE;
}

/* User pressed OK. Any changes should be stored in config. */
void sat_pref_polar_view_ok(GKeyFile * cfg)
{
    GdkRGBA         gdk_rgba;
    guint           rgba;

    if (dirty)
    {
        if (cfg != NULL)
        {
            /* use g_key_file_set_xxx */

            /* orientation */
            g_key_file_set_integer(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_ORIENTATION, orient);
            /* extra contents */
            g_key_file_set_boolean(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_QTH_INFO,
                                   gtk_toggle_button_get_active
                                   (GTK_TOGGLE_BUTTON(qth)));
            g_key_file_set_boolean(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_NEXT_EVENT,
                                   gtk_toggle_button_get_active
                                   (GTK_TOGGLE_BUTTON(next)));
            g_key_file_set_boolean(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_CURS_TRACK,
                                   gtk_toggle_button_get_active
                                   (GTK_TOGGLE_BUTTON(curs)));
            g_key_file_set_boolean(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS,
                                   gtk_toggle_button_get_active
                                   (GTK_TOGGLE_BUTTON(xtick)));

            /* colours */
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(bgd), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            g_key_file_set_integer(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_BGD_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(axis), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            g_key_file_set_integer(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_AXIS_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(tick), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            g_key_file_set_integer(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_TICK_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(sat), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            g_key_file_set_integer(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SAT_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(ssat), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            g_key_file_set_integer(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SAT_SEL_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(info), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            g_key_file_set_integer(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_INFO_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(track), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            g_key_file_set_integer(cfg, MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_TRACK_COL, rgba);

            /* misc */
            g_key_file_set_boolean(cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_TRACK_AUTO,
                                   gtk_toggle_button_get_active
                                   (GTK_TOGGLE_BUTTON(showtrack)));

        }
        else
        {
            /* use sat_cfg_set_xxx */

            /* orientation */
            sat_cfg_set_int(SAT_CFG_INT_POLAR_ORIENTATION, orient);

            /* extra contents */
            sat_cfg_set_bool(SAT_CFG_BOOL_POL_SHOW_QTH_INFO,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (qth)));
            sat_cfg_set_bool(SAT_CFG_BOOL_POL_SHOW_NEXT_EV,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (next)));
            sat_cfg_set_bool(SAT_CFG_BOOL_POL_SHOW_CURS_TRACK,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (curs)));
            sat_cfg_set_bool(SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (xtick)));

            /* colours */
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(bgd), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            sat_cfg_set_int(SAT_CFG_INT_POLAR_BGD_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(axis), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            sat_cfg_set_int(SAT_CFG_INT_POLAR_AXIS_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(tick), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            sat_cfg_set_int(SAT_CFG_INT_POLAR_TICK_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(sat), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            sat_cfg_set_int(SAT_CFG_INT_POLAR_SAT_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(ssat), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            sat_cfg_set_int(SAT_CFG_INT_POLAR_SAT_SEL_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(info), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            sat_cfg_set_int(SAT_CFG_INT_POLAR_INFO_COL, rgba);

            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(track), &gdk_rgba);
            rgba = rgba_to_cfg(&gdk_rgba);
            sat_cfg_set_int(SAT_CFG_INT_POLAR_TRACK_COL, rgba);

            /* misc */
            sat_cfg_set_bool(SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (showtrack)));
        }

        dirty = FALSE;
    }
    else if (reset)
    {
        if (cfg != NULL)
        {
            /* use g_key_file_remove_key */
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_ORIENTATION, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_SHOW_QTH_INFO, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_SHOW_NEXT_EVENT, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_SHOW_CURS_TRACK, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_BGD_COL, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_AXIS_COL, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_TICK_COL, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_SAT_COL, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_SAT_SEL_COL, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_INFO_COL, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_TRACK_COL, NULL);
            g_key_file_remove_key(cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_SHOW_TRACK_AUTO, NULL);
        }
        else
        {

            /* use sat_cfg_reset_xxx */
            /* orientation */
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_ORIENTATION);
            /* extra contents */
            sat_cfg_reset_bool(SAT_CFG_BOOL_POL_SHOW_QTH_INFO);
            sat_cfg_reset_bool(SAT_CFG_BOOL_POL_SHOW_NEXT_EV);
            sat_cfg_reset_bool(SAT_CFG_BOOL_POL_SHOW_CURS_TRACK);
            sat_cfg_reset_bool(SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);

            /* colours */
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_BGD_COL);
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_AXIS_COL);
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_TICK_COL);
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_SAT_COL);
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_SAT_SEL_COL);
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_INFO_COL);
            sat_cfg_reset_int(SAT_CFG_INT_POLAR_TRACK_COL);

            /* misc */
            sat_cfg_reset_bool(SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO);
        }
        reset = FALSE;
    }
}

/*
 * Create and initialise widgets for the polar view preferences tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be loaded.
 */
GtkWidget      *sat_pref_polar_view_create(GKeyFile * cfg)
{
    GtkWidget      *vbox;

    /* create vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);

    /* create the components */
    create_orient_selector(cfg, GTK_BOX(vbox));
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, TRUE, 10);
    create_bool_selectors(cfg, GTK_BOX(vbox));
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, TRUE, 10);
    create_colour_selectors(cfg, GTK_BOX(vbox));
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, TRUE, 10);
    create_misc_selectors(cfg, GTK_BOX(vbox));
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, TRUE, 10);
    create_reset_button(cfg, GTK_BOX(vbox));

    reset = FALSE;
    dirty = FALSE;

    return vbox;
}
