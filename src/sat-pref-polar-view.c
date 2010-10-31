/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "sat-cfg.h"
#include "sat-log.h"
#include "gpredict-utils.h"
#include "mod-cfg-get-param.h"
#include "config-keys.h"
#include "gtk-polar-view.h"
#include "sat-pref-polar-view.h"


/* orientation selectors */
static GtkWidget *nesw,*nwse,*senw,*swne;

/* content selectors */
static GtkWidget *qth,*next,*curs,*xtick;

/* colour selectors */
static GtkWidget *bgd,*axis,*tick,*sat,*ssat,*track,*info;

/* Misc */
static GtkWidget *showtrack;


/* misc bookkeeping */
static gint orient = POLAR_VIEW_NESW;
static gboolean dirty = FALSE;
static gboolean reset = FALSE;


/* private functions:create widgets */
static void create_orient_selector  (GKeyFile *cfg, GtkBox *vbox);
static void create_bool_selectors   (GKeyFile *cfg, GtkBox *vbox);
static void create_colour_selectors (GKeyFile *cfg, GtkBox *vbox);
static void create_misc_selectors   (GKeyFile *cfg, GtkBox *vbox);
static void create_reset_button     (GKeyFile *cfg, GtkBox *vbox);

/* private function: callbacks */
static void orient_chganged (GtkToggleButton *but, gpointer data);
static void content_changed (GtkToggleButton *but, gpointer data);
static void colour_changed  (GtkWidget *but, gpointer data);
static void reset_cb        (GtkWidget *button, gpointer cfg);


/** \brief Create and initialise widgets for the polar view preferences tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be laoded.
 */
GtkWidget *sat_pref_polar_view_create (GKeyFile *cfg)
{
    GtkWidget *vbox;


    /* create vertical box */
    vbox = gtk_vbox_new (FALSE, 5); // !!!
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);

    /* create the components */
    create_orient_selector (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_bool_selectors (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_colour_selectors (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_misc_selectors (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_reset_button (cfg, GTK_BOX (vbox));

    reset = FALSE;
    dirty = FALSE;

    return vbox;
}


/** \brief Create orientation chooser.
 *  \param cfg The module configuration or NULL in global mode.
 *  \param vbox The container box in which the widgets should be packed into.
 *
 * The orientation selector consists of a radio-group having four radio
 * buttons: N/E/S/W, N/W/S/E, S/E/N/W, and S/W/N/E
 */
static void create_orient_selector  (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget *label;
    GtkWidget *hbox;
    GtkTooltips *tips;



    /* create header */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),
                          _("<b>Orientation:</b>"));
    gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);

    /* horizontal box to contain the radio buttons */
    hbox = gtk_hbox_new (TRUE, 10);
    gtk_box_pack_start (vbox, hbox, FALSE, TRUE, 0);

    /* N/E/S/W */
    nesw = gtk_radio_button_new_with_label (NULL, "N/E/S/W");
    gtk_box_pack_start (GTK_BOX (hbox), nesw, FALSE, TRUE, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, nesw,
                          "\tN\n"\
                          "W\t\tE\n"\
                          "\tS",
                          NULL);

    /* N/W/S/E */
    nwse = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (nesw), "N/W/S/E");
    gtk_box_pack_start (GTK_BOX (hbox), nwse, FALSE, TRUE, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, nwse,
                          "\tN\n"\
                          "E\t\tW\n"\
                          "\tS",
                          NULL);

    /* S/E/N/W */
    senw = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (nesw), "S/E/N/W");
    gtk_box_pack_start (GTK_BOX (hbox), senw, FALSE, TRUE, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, senw,
                          "\tS\n"\
                          "W\t\tE\n"\
                          "\tN",
                          NULL);

    /* S/W/N/E */
    swne = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (nesw), "S/W/N/E");
    gtk_box_pack_start (GTK_BOX (hbox), swne, FALSE, TRUE, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, swne,
                          "\tS\n"\
                          "E\t\tW\n"\
                          "\tW",
                          NULL);

    /* read orientation */
    if (cfg != NULL) {
        orient = mod_cfg_get_int (cfg,
                                  MOD_CFG_POLAR_SECTION,
                                  MOD_CFG_POLAR_ORIENTATION,
                                  SAT_CFG_INT_POLAR_ORIENTATION);
    }
    else {
        sat_cfg_get_int (SAT_CFG_INT_POLAR_ORIENTATION);
    }

    switch (orient) {

    case POLAR_VIEW_NESW:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nesw), TRUE);
        break;

    case POLAR_VIEW_NWSE:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nwse), TRUE);
        break;

    case POLAR_VIEW_SENW:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (senw), TRUE);
        break;

    case POLAR_VIEW_SWNE:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (swne), TRUE);
        break;

    default:
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%f:%d: Invalid PolarView orientation (%d)"),
                     __FILE__, __LINE__, orient);
        orient = POLAR_VIEW_NESW;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nesw), TRUE);
        break;
    }

    /* finally, connect the signals */
    g_signal_connect (nesw, "toggled",
                      G_CALLBACK (orient_chganged),
                      GINT_TO_POINTER (POLAR_VIEW_NESW));
    g_signal_connect (nwse, "toggled",
                      G_CALLBACK (orient_chganged),
                      GINT_TO_POINTER (POLAR_VIEW_NWSE));
    g_signal_connect (senw, "toggled",
                      G_CALLBACK (orient_chganged),
                      GINT_TO_POINTER (POLAR_VIEW_SENW));
    g_signal_connect (swne, "toggled",
                      G_CALLBACK (orient_chganged),
                      GINT_TO_POINTER (POLAR_VIEW_SWNE));

}


/** \brief Create content selector widgets.
 *  \param cfg The module configuration or NULL in global mode.
 *  \param vbox The container box in which the widgets should be packed into.
 *
 * This function creates the widgets that are used to select what content, besides
 * the satellites, should be drawn on the polar view. Choices are QTH info, next
 * event, cursor coordinates, and extra tick marks.
 */
static void create_bool_selectors   (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget *label;
    GtkTooltips *tips;
    GtkWidget *hbox;


    /* create header */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),
                          _("<b>Extra Contents:</b>"));
    gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);

    /* horizontal box to contain the radio buttons */
    hbox = gtk_hbox_new (TRUE, 10);
    gtk_box_pack_start (vbox, hbox, FALSE, TRUE, 0);

    /* QTH info */
    qth = gtk_check_button_new_with_label (_("QTH Info"));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, qth,
                          _("Show location information on the polar plot"),
                          NULL);
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_POLAR_SECTION,
                                                        MOD_CFG_POLAR_SHOW_QTH_INFO,
                                                        SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
    }
    g_signal_connect (qth, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), qth, FALSE, TRUE, 0);

    /* Next Event */
    next = gtk_check_button_new_with_label (_("Next Event"));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, next,
                          _("Show which satellites comes up next and at what time"),
                          NULL);
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_POLAR_SECTION,
                                                        MOD_CFG_POLAR_SHOW_NEXT_EVENT,
                                                        SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
    }
    g_signal_connect (next, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), next, FALSE, TRUE, 0);

    /* Cursor position */
    curs = gtk_check_button_new_with_label (_("Cursor Position"));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, curs,
                          _("Show the azimuth and elevation of the mouse pointer"),
                          NULL);
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_POLAR_SECTION,
                                                        MOD_CFG_POLAR_SHOW_CURS_TRACK,
                                                        SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
    }
    g_signal_connect (curs, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), curs, FALSE, TRUE, 0);


    /* Extra tick marks */
    xtick = gtk_check_button_new_with_label (_("Extra Az Ticks"));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, xtick,
                          _("Show extra tick marks for every 30\302\260"),
                          NULL);
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xtick),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_POLAR_SECTION,
                                                        MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS,
                                                        SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xtick),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));
    }
    g_signal_connect (xtick, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), xtick, FALSE, TRUE, 0);

}


/** \brief Create color selector widgets.
 *  \param cfg The module configuration or NULL in global mode.
 *  \param vbox The container box in which the widgets should be packed into.
 *
 * This function creates the widgets for selecting colours for the plot background,
 * axes, tick labels, satellites, track, and info text.
 */
static void create_colour_selectors (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget   *label;
    GtkTooltips *tips;
    GtkWidget   *table;
    guint        rgba;   /* RRGGBBAA encoded colour */
    guint16      alpha;  /* alpha channel 16 bits */
    GdkColor     col;    /* GdkColor colour representation */


    /* create header */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),
                          _("<b>Colours:</b>"));
    gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);

    /* horizontal box to contain the radio buttons */
    table = gtk_table_new (3, 6, TRUE);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (table), 3);
    gtk_box_pack_start (vbox, table, FALSE, TRUE, 0);


    /* background */
    label = gtk_label_new (_("Background:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      GTK_FILL, GTK_FILL, 0, 0);
    bgd = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (bgd), TRUE);
    gtk_table_attach (GTK_TABLE (table), bgd, 1, 2, 0, 1,
                      GTK_FILL , GTK_FILL, 0, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, bgd,
                          _("Click to select background colour"),
                          NULL);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_BGD_COL,
                                SAT_CFG_INT_POLAR_BGD_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_BGD_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (bgd), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (bgd), alpha);
    g_signal_connect (bgd, "color-set", G_CALLBACK (colour_changed), NULL);

    /* Axis */
    label = gtk_label_new (_("Axes/Circles:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                      GTK_FILL, GTK_FILL, 0, 0);
    axis = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (axis), TRUE);
    gtk_table_attach (GTK_TABLE (table), axis, 3, 4, 0, 1,
                      GTK_FILL , GTK_FILL, 0, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, axis,
                          _("Click to select the axis colour"),
                          NULL);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_AXIS_COL,
                                SAT_CFG_INT_POLAR_AXIS_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_AXIS_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (axis), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (axis), alpha);
    g_signal_connect (axis, "color-set", G_CALLBACK (colour_changed), NULL);

    /* tick labels */
    label = gtk_label_new (_("Tick Labels:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 4, 5, 0, 1,
                      GTK_FILL, GTK_FILL, 0, 0);
    tick = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (tick), TRUE);
    gtk_table_attach (GTK_TABLE (table), tick, 5, 6, 0, 1,
                      GTK_FILL , GTK_FILL, 0, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, tick,
                          _("Click to select the colour for tick labels"),
                          NULL);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_TICK_COL,
                                SAT_CFG_INT_POLAR_TICK_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_TICK_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (tick), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (tick), alpha);
    g_signal_connect (tick, "color-set", G_CALLBACK (colour_changed), NULL);

    /* satellite */
    label = gtk_label_new (_("Satellite:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    sat = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (sat), TRUE);
    gtk_table_attach (GTK_TABLE (table), sat, 1, 2, 1, 2,
                      GTK_FILL , GTK_FILL, 0, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, sat,
                          _("Click to select satellite colour"),
                          NULL);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_SAT_COL,
                                SAT_CFG_INT_POLAR_SAT_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_SAT_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (sat), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (sat), alpha);
    g_signal_connect (sat, "color-set", G_CALLBACK (colour_changed), NULL);

    /* selected satellite */
    label = gtk_label_new (_("Selected Sat.:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    ssat = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (ssat), TRUE);
    gtk_table_attach (GTK_TABLE (table), ssat, 3, 4, 1, 2,
                      GTK_FILL , GTK_FILL, 0, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, ssat,
                          _("Click to select colour for selected satellites"),
                          NULL);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_SAT_SEL_COL,
                                SAT_CFG_INT_POLAR_SAT_SEL_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_SAT_SEL_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (ssat), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (ssat), alpha);
    g_signal_connect (ssat, "color-set", G_CALLBACK (colour_changed), NULL);

    /* tack */
    label = gtk_label_new (_("Sky Track:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    track = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (track), TRUE);
    gtk_table_attach (GTK_TABLE (table), track, 5, 6, 1, 2,
                      GTK_FILL , GTK_FILL, 0, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, track,
                          _("Click to select track colour"),
                          NULL);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_TRACK_COL,
                                SAT_CFG_INT_POLAR_TRACK_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_TRACK_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (track), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (track), alpha);
    g_signal_connect (track, "color-set", G_CALLBACK (colour_changed), NULL);

    /* Info */
    label = gtk_label_new (_("Info Text:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                      GTK_FILL, GTK_FILL, 0, 0);
    info = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (info), TRUE);
    gtk_table_attach (GTK_TABLE (table), info, 1, 2, 2, 3,
                      GTK_FILL , GTK_FILL, 0, 0);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, info,
                          _("Click to select background colour"),
                          NULL);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_INFO_COL,
                                SAT_CFG_INT_POLAR_INFO_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_INFO_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (info), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (info), alpha);
    g_signal_connect (info, "color-set", G_CALLBACK (colour_changed), NULL);


    // *info;

}



/** \brief Create miscellaneous config widgets.
 *  \param cfg The module configuration or NULL in global mode.
 *  \param vbox The container box in which the widgets should be packed into.
 *
 */
static void
        create_misc_selectors   (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget *label;
    GtkWidget *hbox;


    /* create header */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),
                          _("<b>Miscellaneous:</b>"));
    gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);

    /* horizontal box to contain the radio buttons */
    hbox = gtk_hbox_new (TRUE, 10);
    gtk_box_pack_start (vbox, hbox, FALSE, TRUE, 0);

    /* show sky tracks */
    showtrack = gtk_check_button_new_with_label (_("Show the sky tracks automatically"));
    gtk_widget_set_tooltip_text (showtrack,
                                 _("Automatically show the sky track of a satellite "\
                                   "when it comes in range. You can always turn the "\
                                   "sky track OFF for each individual object"));
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (showtrack),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_POLAR_SECTION,
                                                        MOD_CFG_POLAR_SHOW_TRACK_AUTO,
                                                        SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (showtrack),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));
    }
    g_signal_connect (showtrack, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), showtrack, FALSE, TRUE, 0);

}




/** \brief Create RESET button.
 *  \param cfg Config data or NULL in global mode.
 *  \param vbox The container.
 *
 * This function creates and sets up the RESET button.
 */
static void create_reset_button     (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget   *button;
    GtkWidget   *butbox;



    button = gtk_button_new_with_label (_("Reset"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (reset_cb), cfg);

    if (cfg == NULL) {
        gtk_widget_set_tooltip_text (button,
                                     _("Reset settings to the default values."));

    }
    else {
        gtk_widget_set_tooltip_text (button,
                                     _("Reset module settings to the global values."));
    }

    butbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end (GTK_BOX (butbox), button, FALSE, TRUE, 10);

    gtk_box_pack_end (vbox, butbox, FALSE, TRUE, 0);
}


/** \brief Manage orientation changes.
 *  \param but the button that received the signal
 *  \param data User data (always NULL).
 *
 * Note that this function is called also for the button that is unchecked,
 *
 */
static void orient_chganged (GtkToggleButton *but, gpointer data)
{
    if (gtk_toggle_button_get_active (but)) {

        orient = GPOINTER_TO_INT (data);
        dirty = TRUE;

    }
}


/** \brief Manage check-box actions.
 *  \param but The check-button that has been toggled.
 *  \param daya User data (always NULL).
 *
 * We don't need to do anything but set the dirty flag since the values can
 * always be obtained from the global widgets.
 */
static void content_changed    (GtkToggleButton *but, gpointer data)
{
    dirty = TRUE;
}


/** \brief Manage color and font changes.
 *  \param but The color/font picker button that received the signal.
 *  \param data User data (always NULL).
 *
 * We don't need to do anything but set the dirty flag since the values can
 * always be obtained from the global widgets.
 */
static void colour_changed     (GtkWidget *but, gpointer data)
{
    dirty = TRUE;
}


/** \brief Managge RESET button signals.
 *  \param button The RESET button.
 *  \param cfg Pointer to the module configuration or NULL in global mode.
 *
 * This function is called when the user clicks on the RESET button. In global mode
 * (when cfg = NULL) the function will reset the settings to the efault values, while
 * in "local" mode (when cfg != NULL) the function will reset the module settings to
 * the global settings. This is done by removing the corresponding key from the GKeyFile.
 */
static void reset_cb           (GtkWidget *button, gpointer cfg)
{
    GdkColor col;
    guint16  alpha;
    guint    rgba;


    if (cfg == NULL) {
        /* global mode, get defaults */

        /* extra contents */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xtick),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));

        /* colours */
        rgba = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_BGD_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (bgd), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (bgd), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_AXIS_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (axis), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (axis), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_TICK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (tick), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (tick), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_SAT_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (sat), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (sat), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_SAT_SEL_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (ssat), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (ssat), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_TRACK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (track), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (track), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_INFO_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (info), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (info), alpha);

        /* misc */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (showtrack),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));

    }
    else {
        /* local mode, get global value */

        /* extra contents */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_QTH_INFO));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_NEXT_EV));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xtick),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS));

        /* colours */
        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_BGD_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (bgd), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (bgd), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_AXIS_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (axis), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (axis), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_TICK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (tick), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (tick), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_SAT_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (sat), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (sat), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_SAT_SEL_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (ssat), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (ssat), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_TRACK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (track), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (track), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_POLAR_INFO_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (info), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (info), alpha);

        /* misc */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (showtrack),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO));

    }

    /* orientation needs some special attention */
    if (cfg == NULL) {
        orient = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_ORIENTATION);
    }
    else {
        orient = sat_cfg_get_int (SAT_CFG_INT_POLAR_ORIENTATION);
    }

    switch (orient) {

    case POLAR_VIEW_NESW:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nesw), TRUE);
        break;

    case POLAR_VIEW_NWSE:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nwse), TRUE);
        break;

    case POLAR_VIEW_SENW:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (senw), TRUE);
        break;

    case POLAR_VIEW_SWNE:
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (swne), TRUE);
        break;

    default:
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Invalid chart orientation %d (using N/E/S/W)"),
                     __FILE__, __FUNCTION__, orient);
        orient = POLAR_VIEW_NESW;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nesw), TRUE);
        break;
    }

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}


/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void sat_pref_polar_view_cancel (GKeyFile *cfg)
{
    dirty = FALSE;
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void sat_pref_polar_view_ok     (GKeyFile *cfg)
{
    guint    rgba;
    guint16  alpha;
    GdkColor col;


    if (dirty) {
        if (cfg != NULL) {
            /* use g_key_file_set_xxx */

            /* orientation */
            g_key_file_set_integer (cfg,
                                    MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_ORIENTATION,
                                    orient);
            /* extra contents */
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_SHOW_QTH_INFO,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (qth)));
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_SHOW_NEXT_EVENT,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (next)));
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_SHOW_CURS_TRACK,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (curs)));
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xtick)));

            /* colours */
            gtk_color_button_get_color (GTK_COLOR_BUTTON (bgd), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (bgd));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg,MOD_CFG_POLAR_SECTION,MOD_CFG_POLAR_BGD_COL,rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (axis), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (axis));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg,MOD_CFG_POLAR_SECTION,MOD_CFG_POLAR_AXIS_COL,rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (tick), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (tick));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg,MOD_CFG_POLAR_SECTION,MOD_CFG_POLAR_TICK_COL,rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (sat), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (sat));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg,MOD_CFG_POLAR_SECTION,MOD_CFG_POLAR_SAT_COL,rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (ssat), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (ssat));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg,MOD_CFG_POLAR_SECTION,MOD_CFG_POLAR_SAT_SEL_COL,rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (info), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (info));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg,MOD_CFG_POLAR_SECTION,MOD_CFG_POLAR_INFO_COL,rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (track), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (track));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg,MOD_CFG_POLAR_SECTION,MOD_CFG_POLAR_TRACK_COL,rgba);

            /* misc */
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_SHOW_TRACK_AUTO,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (showtrack)));

        }
        else {
            /* use sat_cfg_set_xxx */

            /* orientation */
            sat_cfg_set_int (SAT_CFG_INT_POLAR_ORIENTATION, orient);

            /* extra contents */
            sat_cfg_set_bool (SAT_CFG_BOOL_POL_SHOW_QTH_INFO,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (qth)));
            sat_cfg_set_bool (SAT_CFG_BOOL_POL_SHOW_NEXT_EV,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (next)));
            sat_cfg_set_bool (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (curs)));
            sat_cfg_set_bool (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xtick)));

            /* colours */
            gtk_color_button_get_color (GTK_COLOR_BUTTON (bgd), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (bgd));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_POLAR_BGD_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (axis), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (axis));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_POLAR_AXIS_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (tick), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (tick));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_POLAR_TICK_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (sat), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (sat));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_POLAR_SAT_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (ssat), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (ssat));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_POLAR_SAT_SEL_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (info), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (info));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_POLAR_INFO_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (track), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (track));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_POLAR_TRACK_COL, rgba);

            /* misc */
            sat_cfg_set_bool (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (showtrack)));

        }

        dirty = FALSE;
    }
    else if (reset) {
        if (cfg != NULL) {
            /* use g_key_file_remove_key */
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_ORIENTATION,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_QTH_INFO,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_NEXT_EVENT,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_CURS_TRACK,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_BGD_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_AXIS_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_TICK_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SAT_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SAT_SEL_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_INFO_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_TRACK_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_POLAR_SECTION,
                                   MOD_CFG_POLAR_SHOW_TRACK_AUTO,
                                   NULL);

        }
        else {

            /* use sat_cfg_reset_xxx */
            /* orientation */
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_ORIENTATION);
            /* extra contents */
            sat_cfg_reset_bool (SAT_CFG_BOOL_POL_SHOW_QTH_INFO);
            sat_cfg_reset_bool (SAT_CFG_BOOL_POL_SHOW_NEXT_EV);
            sat_cfg_reset_bool (SAT_CFG_BOOL_POL_SHOW_CURS_TRACK);
            sat_cfg_reset_bool (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);

            /* colours */
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_BGD_COL);
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_AXIS_COL);
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_TICK_COL);
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_SAT_COL);
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_SAT_SEL_COL);
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_INFO_COL);
            sat_cfg_reset_int (SAT_CFG_INT_POLAR_TRACK_COL);

            /* misc */
            sat_cfg_reset_bool (SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO);
        }
        reset = FALSE;
    }
}

