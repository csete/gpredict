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
#include "mod-cfg-get-param.h"
#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-map.h"
#include "sat-pref-map-view.h"
#include "map-selector.h"
#include "compat.h"


/* map thumbnail */
static GtkWidget *thumb;
static gchar     *mapf = NULL;

/* content selectors */
static GtkWidget *qth,*next,*curs,*grid;

/* colour selectors */
static GtkWidget *qthc,*gridc,*tickc;
static GtkWidget *satc,*ssatc,*trackc;
static GtkWidget *covc,*infofg,*infobg;
static GtkWidget *terminator, *globe_shadow;
static GtkWidget *shadow;

/* ground track orbit number selector */
static GtkWidget *orbit;

/* map center spin box */
static GtkWidget *center;

/* misc bookkeeping */
static gboolean dirty = FALSE;
static gboolean reset = FALSE;


static void create_map_selector     (GKeyFile *cfg, GtkBox *vbox);
static void create_bool_selectors   (GKeyFile *cfg, GtkBox *vbox);
static void create_colour_selectors (GKeyFile *cfg, GtkBox *vbox);
static void create_orbit_selector   (GKeyFile *cfg, GtkBox *vbox);
static void create_center_selector  (GKeyFile *cfg, GtkBox *vbox);
static void create_reset_button     (GKeyFile *cfg, GtkBox *vbox);

/* private function: callbacks */
static void content_changed (GtkToggleButton *but, gpointer data);
static void colour_changed  (GtkWidget *but, gpointer data);
static void orbit_changed   (GtkWidget *spin, gpointer data);
static void center_changed   (GtkWidget *spin, gpointer data);
static void reset_cb        (GtkWidget *button, gpointer cfg);

static void select_map_cb   (GtkWidget *button, gpointer data);
static void update_map_icon (void);
static gboolean shadow_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer data);



/** \brief Create and initialise widgets for the map preferences tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be laoded.
 */
GtkWidget *sat_pref_map_view_create (GKeyFile *cfg)
{
    GtkWidget *vbox;
    GtkWidget *swin;

    /* create vertical box */
    vbox = gtk_vbox_new (FALSE, 2); // !!!
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);

    /* create the components */
    create_map_selector (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_bool_selectors (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_colour_selectors (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_orbit_selector (cfg, GTK_BOX (vbox));
    create_center_selector (cfg, GTK_BOX (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 10);
    create_reset_button (cfg, GTK_BOX (vbox));

    reset = FALSE;
    dirty = FALSE;

    /* pack vbox into a scrolled window to reduce vertical size of the window */
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), vbox);

    return swin;

}


/** \brief Create map selector widget. */
static void create_map_selector  (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget *button;
    GtkWidget *label;
    GtkWidget *table;
    gchar     *mapfile;
    GdkPixbuf *obuf,*sbuf;


    /* create header */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),
                          _("<b>Background Map:</b>"));
    gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);

    /* create a table to put the map preview and select button in.
       using a simple hbox won't do it because the button would have
       the same height as the map preview
     */
    table = gtk_table_new (3, 2, TRUE);
    gtk_box_pack_start (vbox, table, FALSE, FALSE, 0);

    /* load map file into a pixbuf */
    if (cfg != NULL) {
        mapf = mod_cfg_get_str (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_FILE,
                                SAT_CFG_STR_MAP_FILE);
    }
    else {
        mapf = sat_cfg_get_str (SAT_CFG_STR_MAP_FILE);
    }
    if (g_path_is_absolute (mapf)) {
        /* map is user specific, ie. in USER_CONF_DIR/maps/ */
        mapfile = g_strdup (mapf);
    }
    else {
        /* build complete path */
        mapfile = map_file_name (mapf);
    }
    obuf = gdk_pixbuf_new_from_file (mapfile, NULL);
    g_free (mapfile);

    if (obuf != NULL) {

        /* scale the pixbuf */
        sbuf = gdk_pixbuf_scale_simple (obuf, 100, 50, GDK_INTERP_HYPER);
        g_object_unref (obuf);

        /* create a GtkImage from the pixbuf */
        thumb = gtk_image_new_from_pixbuf (sbuf);
        g_object_unref (sbuf);
    }
    else {
        thumb = gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
    }

    gtk_table_attach (GTK_TABLE (table), thumb, 0, 1, 0, 3,
                      GTK_FILL, GTK_FILL, 0, 0);

    /* select button */
    button = gtk_button_new_with_label (_("Select map"));
    gtk_widget_set_tooltip_text (button,
                          _("Click to select a map"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (select_map_cb), NULL);
    gtk_table_attach (GTK_TABLE (table), button, 1, 2, 1, 2,
                      GTK_FILL, GTK_FILL, 10, 0);



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
    gtk_widget_set_tooltip_text (qth,
                          _("Show location information on the map"));
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_MAP_SECTION,
                                                        MOD_CFG_MAP_SHOW_QTH_INFO,
                                                        SAT_CFG_BOOL_MAP_SHOW_QTH_INFO));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_QTH_INFO));
    }
    g_signal_connect (qth, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), qth, FALSE, TRUE, 0);

    /* Next Event */
    next = gtk_check_button_new_with_label (_("Next Event"));
    gtk_widget_set_tooltip_text (next,
                          _("Show which satellite comes up next and at what time"));
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_MAP_SECTION,
                                                        MOD_CFG_MAP_SHOW_NEXT_EVENT,
                                                        SAT_CFG_BOOL_MAP_SHOW_NEXT_EV));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_NEXT_EV));
    }
    g_signal_connect (next, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), next, FALSE, TRUE, 0);

    /* Cursor position */
    curs = gtk_check_button_new_with_label (_("Cursor Position"));
    gtk_widget_set_tooltip_text (curs,
                          _("Show the latitude and longitude of the mouse pointer"));
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_MAP_SECTION,
                                                        MOD_CFG_MAP_SHOW_CURS_TRACK,
                                                        SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK));
    }
    g_signal_connect (curs, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), curs, FALSE, TRUE, 0);


    /* Grid */
    grid = gtk_check_button_new_with_label (_("Grid Lines"));
    gtk_widget_set_tooltip_text (grid,
                          _("Show horizontal and vertical grid lines"));
    if (cfg != NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (grid),
                                      mod_cfg_get_bool (cfg,
                                                        MOD_CFG_MAP_SECTION,
                                                        MOD_CFG_MAP_SHOW_GRID,
                                                        SAT_CFG_BOOL_MAP_SHOW_GRID));
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (grid),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_GRID));
    }
    g_signal_connect (grid, "toggled", G_CALLBACK (content_changed), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), grid, FALSE, TRUE, 0);

}


/** \brief Create colour selector widgets.
 *  \param cfg The module configuration or NULL in global mode.
 *  \param vbox The container box in which the widgets should be packed into.
 *
 * This function creates the widgets for selecting colours for the plot background,
 * axes, tick labels, satellites, track, and info text.
 */
static void create_colour_selectors (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget   *label;
    GtkWidget   *table;
    guint        rgba;   /* RRGGBBAA encoded colour */
    guint16      alpha;  /* alpha channel 16 bits */
    GdkColor     col;    /* GdkColor colour representation */


    /* create header */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),  _("<b>Colours:</b>"));
    gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);

    /* container */
    table = gtk_table_new (4, 6, TRUE);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (table), 3);
    gtk_box_pack_start (vbox, table, FALSE, TRUE, 0);


    /* background */
    label = gtk_label_new (_("Ground Station:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      GTK_FILL, GTK_FILL, 0, 0);
    qthc = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (qthc), TRUE);
    gtk_table_attach (GTK_TABLE (table), qthc, 1, 2, 0, 1,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (qthc,
                          _("Click to select a colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_QTH_COL,
                                SAT_CFG_INT_MAP_QTH_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_QTH_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (qthc), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (qthc), alpha);
    g_signal_connect (qthc, "color-set", G_CALLBACK (colour_changed), NULL);

    /* Grid in case it is enabled */
    label = gtk_label_new (_("Grid:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                      GTK_FILL, GTK_FILL, 0, 0);
    gridc = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (gridc), TRUE);
    gtk_table_attach (GTK_TABLE (table), gridc, 3, 4, 0, 1,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (gridc,
                          _("Click to select the grid colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_GRID_COL,
                                SAT_CFG_INT_MAP_GRID_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_GRID_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (gridc), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (gridc), alpha);
    g_signal_connect (gridc, "color-set", G_CALLBACK (colour_changed), NULL);

    /* tick labels */
    label = gtk_label_new (_("Tick Labels:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 4, 5, 0, 1,
                      GTK_FILL, GTK_FILL, 0, 0);
    tickc = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (tickc), TRUE);
    gtk_table_attach (GTK_TABLE (table), tickc, 5, 6, 0, 1,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (tickc,
                          _("Click to select the colour for tick labels"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_TICK_COL,
                                SAT_CFG_INT_MAP_TICK_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_TICK_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (tickc), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (tickc), alpha);
    g_signal_connect (tickc, "color-set", G_CALLBACK (colour_changed), NULL);

    /* satellite */
    label = gtk_label_new (_("Satellite:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    satc = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (satc), TRUE);
    gtk_table_attach (GTK_TABLE (table), satc, 1, 2, 1, 2,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (satc,
                          _("Click to select satellite colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_SAT_COL,
                                SAT_CFG_INT_MAP_SAT_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_SAT_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (satc), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (satc), alpha);
    g_signal_connect (satc, "color-set", G_CALLBACK (colour_changed), NULL);

    /* selected satellite */
    label = gtk_label_new (_("Selected Sat.:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    ssatc = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (ssatc), TRUE);
    gtk_table_attach (GTK_TABLE (table), ssatc, 3, 4, 1, 2,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (ssatc,
                          _("Click to select colour for selected satellites"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_SAT_SEL_COL,
                                SAT_CFG_INT_MAP_SAT_SEL_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_SAT_SEL_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (ssatc), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (ssatc), alpha);
    g_signal_connect (ssatc, "color-set", G_CALLBACK (colour_changed), NULL);

    /* tack */
    label = gtk_label_new (_("Ground Track:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    trackc = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (trackc), TRUE);
    gtk_table_attach (GTK_TABLE (table), trackc, 5, 6, 1, 2,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (trackc,
                          _("Click to select ground track colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_TRACK_COL,
                                SAT_CFG_INT_MAP_TRACK_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_TRACK_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (trackc), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (trackc), alpha);
    g_signal_connect (trackc, "color-set", G_CALLBACK (colour_changed), NULL);

    /* coverage */
    label = gtk_label_new (_("Area Coverage:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                      GTK_FILL, GTK_FILL, 0, 0);
    covc = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (covc), TRUE);
    gtk_table_attach (GTK_TABLE (table), covc, 1, 2, 2, 3,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (covc,
                          _("Colour for coverage Area (make it transparent)"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_SAT_COV_COL,
                                SAT_CFG_INT_MAP_SAT_COV_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_SAT_COV_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (covc), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (covc), alpha);
    g_signal_connect (covc, "color-set", G_CALLBACK (colour_changed), NULL);

    /* Info foreground */
    label = gtk_label_new (_("Info Text FG:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3,
                      GTK_FILL, GTK_FILL, 0, 0);
    infofg = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (infofg), TRUE);
    gtk_table_attach (GTK_TABLE (table), infofg, 3, 4, 2, 3,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (infofg,
                          _("Click to select info text foreground colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_INFO_COL,
                                SAT_CFG_INT_MAP_INFO_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_INFO_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (infofg), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (infofg), alpha);
    g_signal_connect (infofg, "color-set", G_CALLBACK (colour_changed), NULL);


    /* Info background */
    label = gtk_label_new (_("Info Text BG:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 4, 5, 2, 3,
                      GTK_FILL, GTK_FILL, 0, 0);
    infobg = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (infobg), TRUE);
    gtk_table_attach (GTK_TABLE (table), infobg, 5, 6, 2, 3,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (infobg,
                          _("Click to select info text background colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_INFO_BGD_COL,
                                SAT_CFG_INT_MAP_INFO_BGD_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_INFO_BGD_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (infobg), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (infobg), alpha);
    g_signal_connect (infobg, "color-set", G_CALLBACK (colour_changed), NULL);

    /* Solar terminator */
    label = gtk_label_new (_("Solar terminator:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                      GTK_FILL, GTK_FILL, 0, 0);
    terminator = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (terminator), TRUE);
    gtk_table_attach (GTK_TABLE (table), terminator, 1, 2, 3, 4,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (terminator,
                                 _("Click to select terminator colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_TERMINATOR_COL,
                                SAT_CFG_INT_MAP_TERMINATOR_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_TERMINATOR_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (terminator), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (terminator), alpha);
    g_signal_connect (terminator, "color-set", G_CALLBACK (colour_changed), NULL);

    /* Earth shadow */
    label = gtk_label_new (_("Global shadow:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 2, 3, 3, 4,
                      GTK_FILL, GTK_FILL, 0, 0);
    globe_shadow = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (globe_shadow), TRUE);
    gtk_table_attach (GTK_TABLE (table), globe_shadow, 3, 4, 3, 4,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (globe_shadow,
                                 _("Click to select Earth shadow colour"));
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_GLOBAL_SHADOW_COL,
                                SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL);
    }
    rgba2gdk (rgba, &col, &alpha);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (globe_shadow), &col);
    gtk_color_button_set_alpha (GTK_COLOR_BUTTON (globe_shadow), alpha);
    g_signal_connect (globe_shadow, "color-set", G_CALLBACK (colour_changed), NULL);

    /* Shadow */
    label = gtk_label_new (_("Shadow:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                      GTK_FILL, GTK_FILL, 0, 0);
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<i>Transparent</i>"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 1, 2, 4, 5,
                      GTK_FILL, GTK_FILL, 0, 0);
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<i>Strong</i>"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 5, 6, 4, 5,
                      GTK_FILL, GTK_FILL, 0, 0);
    shadow = gtk_hscale_new_with_range (0, 255, 1);
    gtk_scale_set_draw_value (GTK_SCALE (shadow), FALSE);
    if (cfg != NULL) {
        rgba = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_SHADOW_ALPHA,
                                SAT_CFG_INT_MAP_SHADOW_ALPHA);
    }
    else {
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_SHADOW_ALPHA);
    }
    gtk_range_set_value (GTK_RANGE (shadow), rgba);
    gtk_widget_set_tooltip_text (shadow,
                                 _("Specifies how strong the shadow under the satellite marker should be. "\
                                   "The shadow improves the visibility of the satellites where the colour of "\
                                   "the background is light, e.g. the South Pole.\n\n"\
                                   "Transparent corresponds to no shadow."));
    gtk_table_attach (GTK_TABLE (table), shadow, 2, 5, 4, 5,
                      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
    g_signal_connect (shadow, "value-changed", G_CALLBACK (shadow_changed), NULL);


}


/** \brief Create orbit number selector widget.
 *  \param cfg The module configuration or NULL in global mode.
 *  \param vbox The container box in which the widgets should be packed into.
 *
 * This function creates the widgets for selecting the number of orbit to show
 * the satellite ground track for.
 *
 */
static void create_orbit_selector   (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget   *label;
    GtkWidget   *hbox;
    gint         onum;

    hbox = gtk_hbox_new (FALSE, 10);
    gtk_box_pack_start (vbox, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Display ground track for"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    orbit = gtk_spin_button_new_with_range (1, 10, 1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (orbit), 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (orbit), TRUE);

    if (cfg != NULL) {
        onum = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_TRACK_NUM,
                                SAT_CFG_INT_MAP_TRACK_NUM);
    }
    else {
        onum = sat_cfg_get_int (SAT_CFG_INT_MAP_TRACK_NUM);
    }
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (orbit), onum);
    g_signal_connect (G_OBJECT (orbit), "value-changed",
                      G_CALLBACK (orbit_changed), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), orbit, FALSE, FALSE, 0);

    label = gtk_label_new (_("orbit(s)"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

}


/** \brief Create map center selector widget.
 *  \param cfg The module configuration or NULL in global mode.
 *  \param vbox The container box in which the widgets should be packed into.
 *
 * This function creates the widgets for selecting the longitude around which
 * to center the map.
 *
 */
static void create_center_selector(GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget   *label;
    GtkWidget   *hbox;
    gint         clon;

    hbox = gtk_hbox_new (FALSE, 10);
    gtk_box_pack_start (vbox, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Center map at longitude"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    center = gtk_spin_button_new_with_range(-180, 180, 1);
    gtk_widget_set_tooltip_text(center, _("Select longitude. West is negative."));
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (center), 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (center), TRUE);

    if (cfg != NULL) {
        clon = mod_cfg_get_int (cfg,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_CENTER,
                                SAT_CFG_INT_MAP_CENTER);
    }
    else {
        clon = sat_cfg_get_int (SAT_CFG_INT_MAP_CENTER);
    }
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (center), clon);
    g_signal_connect (G_OBJECT (center), "value-changed",
                      G_CALLBACK (center_changed), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), center, FALSE, FALSE, 0);

    label = gtk_label_new (_("[deg]"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
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




/** \brief Manage check-box actions.
 *  \param but The check-button that has been toggled.
 *  \param daya User data (always NULL).
 *
 * We don't need to do anything but set the dirty flag since the values can
 * always be obtained from the global widgets.
 */
static void content_changed    (GtkToggleButton *but, gpointer data)
{
    (void) but; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */

    dirty = TRUE;
}


/** \brief Manage color and font changes.
 *  \param but The color/font picker button that received the signal.
 *  \param data User data (always NULL).
 *
 * We don't need to do anything but set the dirty flag since the values can
 * always be obtained from the global widgets.
 */
static void colour_changed (GtkWidget *but, gpointer data)
{
    (void) but; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */
    
    dirty = TRUE;
}


static void orbit_changed (GtkWidget *spin, gpointer data)
{
    (void) spin; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */
    
    dirty = TRUE;
}

static void center_changed (GtkWidget *spin, gpointer data)
{
    (void) spin; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */
    
    dirty = TRUE;
}

static gboolean shadow_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer data)
{
    (void) range; /* avoid unused parameter compiler warning */
    (void) scroll; /* avoid unused parameter compiler warning */
    (void) value; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */

    dirty = TRUE;

    /* prevent other signal handlers from being executed */
    return TRUE;
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
static void reset_cb (GtkWidget *button, gpointer cfg)
{
    GdkColor col;
    guint16  alpha;
    guint    rgba;

    (void) button; /* avoid unused parameter compiler warning */

    if (cfg == NULL) {
        /* global mode, get defaults */

        /* background map */
        g_free (mapf);
        mapf = sat_cfg_get_str_def (SAT_CFG_STR_MAP_FILE);
        update_map_icon ();

        /* extra contents */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_MAP_SHOW_QTH_INFO));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_MAP_SHOW_NEXT_EV));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (grid),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_MAP_SHOW_GRID));

        /* colours */
        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_QTH_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (qthc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (qthc), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_GRID_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (gridc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (gridc), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_TICK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (tickc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (tickc), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_SAT_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (satc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (satc), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_SAT_SEL_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (ssatc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (ssatc), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_TRACK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (trackc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (trackc), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_SAT_COV_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (covc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (covc), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_INFO_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (infofg), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (infofg), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_INFO_BGD_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (infobg), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (infobg), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_TERMINATOR_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (terminator), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (terminator), alpha);

        rgba = sat_cfg_get_int_def (SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (globe_shadow), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (globe_shadow), alpha);

        /* shadow */
        gtk_range_set_value (GTK_RANGE (shadow), sat_cfg_get_int_def (SAT_CFG_INT_MAP_SHADOW_ALPHA));

        /* ground track orbits */
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (orbit),
                                   sat_cfg_get_int_def (SAT_CFG_INT_MAP_TRACK_NUM));

        /* center longitude */
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (center),
                                   sat_cfg_get_int_def (SAT_CFG_INT_MAP_CENTER));
    }
    else {
        /* local mode, get global value */

        /* background map */
        g_free (mapf);
        mapf = sat_cfg_get_str (SAT_CFG_STR_MAP_FILE);
        update_map_icon ();

        /* extra contents */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (qth),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_QTH_INFO));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (next),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_NEXT_EV));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (curs),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (grid),
                                      sat_cfg_get_bool (SAT_CFG_BOOL_MAP_SHOW_GRID));

        /* colours */
        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_QTH_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (qthc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (qthc), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_GRID_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (gridc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (gridc), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_TICK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (tickc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (tickc), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_SAT_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (satc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (satc), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_SAT_SEL_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (ssatc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (ssatc), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_TRACK_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (trackc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (trackc), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_SAT_COV_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (covc), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (covc), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_INFO_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (infofg), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (infofg), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_INFO_BGD_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (infobg), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (infobg), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_TERMINATOR_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (terminator), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (terminator), alpha);

        rgba = sat_cfg_get_int (SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL);
        rgba2gdk (rgba, &col, &alpha);
        gtk_color_button_set_color (GTK_COLOR_BUTTON (globe_shadow), &col);
        gtk_color_button_set_alpha (GTK_COLOR_BUTTON (globe_shadow), alpha);

        /* shadow */
        gtk_range_set_value (GTK_RANGE (shadow), sat_cfg_get_int (SAT_CFG_INT_MAP_SHADOW_ALPHA));

        /* ground track orbits */
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (orbit),
                                   sat_cfg_get_int (SAT_CFG_INT_MAP_TRACK_NUM));

        /* center longitude */
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (center),
                                   sat_cfg_get_int (SAT_CFG_INT_MAP_CENTER));
    }

    /* map file */

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}



/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void sat_pref_map_view_cancel (GKeyFile *cfg)
{
    (void) cfg; /* avoid unused parameter compiler warning */

    dirty = FALSE;

    g_free (mapf);
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void sat_pref_map_view_ok     (GKeyFile *cfg)
{
    guint    rgba;
    guint16  alpha;
    GdkColor col;


    if (dirty) {
        if (cfg != NULL) {
            /* local config use g_key_file_set_xxx */

            /* background map */
            g_key_file_set_string (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_FILE,
                                   mapf);

            /* extra contents */
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_MAP_SECTION,
                                    MOD_CFG_MAP_SHOW_QTH_INFO,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (qth)));
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_MAP_SECTION,
                                    MOD_CFG_MAP_SHOW_NEXT_EVENT,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (next)));
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_MAP_SECTION,
                                    MOD_CFG_MAP_SHOW_CURS_TRACK,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (curs)));
            g_key_file_set_boolean (cfg,
                                    MOD_CFG_MAP_SECTION,
                                    MOD_CFG_MAP_SHOW_GRID,
                                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (grid)));

            /* colours */
            gtk_color_button_get_color (GTK_COLOR_BUTTON (qthc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (qthc));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_QTH_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (gridc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (gridc));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_GRID_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (tickc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (tickc));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_TICK_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (satc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (satc));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_SAT_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (ssatc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (ssatc));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_SAT_SEL_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (trackc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (trackc));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_TRACK_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (covc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (covc));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_SAT_COV_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (infofg), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (infofg));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_INFO_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (infobg), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (infobg));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_INFO_BGD_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (terminator), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (terminator));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_TERMINATOR_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (globe_shadow), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (globe_shadow));
            gdk2rgba (&col, alpha, &rgba);
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_GLOBAL_SHADOW_COL, rgba);

            /* shadow */
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_SHADOW_ALPHA,
                                    (gint) gtk_range_get_value (GTK_RANGE (shadow)));

            /* orbit */
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_TRACK_NUM,
                                    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (orbit)));

            /* map center */
            g_key_file_set_integer (cfg, MOD_CFG_MAP_SECTION, MOD_CFG_MAP_CENTER,
                                    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (center)));
        }
        else {
            /* use sat_cfg_set_xxx */

            /* background map */
            sat_cfg_set_str (SAT_CFG_STR_MAP_FILE, mapf);

            /* extra contents */
            sat_cfg_set_bool (SAT_CFG_BOOL_MAP_SHOW_QTH_INFO,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (qth)));
            sat_cfg_set_bool (SAT_CFG_BOOL_MAP_SHOW_NEXT_EV,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (next)));
            sat_cfg_set_bool (SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (curs)));
            sat_cfg_set_bool (SAT_CFG_BOOL_MAP_SHOW_GRID,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (grid)));


            /* colours */
            gtk_color_button_get_color (GTK_COLOR_BUTTON (qthc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (qthc));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_QTH_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (gridc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (gridc));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_GRID_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (tickc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (tickc));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_TICK_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (satc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (satc));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_SAT_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (ssatc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (ssatc));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_SAT_SEL_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (trackc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (trackc));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_TRACK_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (covc), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (covc));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_SAT_COV_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (infofg), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (infofg));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_INFO_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (infobg), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (infobg));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_INFO_BGD_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (terminator), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (terminator));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_TERMINATOR_COL, rgba);

            gtk_color_button_get_color (GTK_COLOR_BUTTON (globe_shadow), &col);
            alpha = gtk_color_button_get_alpha (GTK_COLOR_BUTTON (globe_shadow));
            gdk2rgba (&col, alpha, &rgba);
            sat_cfg_set_int (SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL, rgba);

            /* shadow */
            sat_cfg_set_int (SAT_CFG_INT_MAP_SHADOW_ALPHA,
                             (gint) gtk_range_get_value (GTK_RANGE (shadow)));

            /* orbit */
            sat_cfg_set_int (SAT_CFG_INT_MAP_TRACK_NUM,
                             gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (orbit)));

            /* map center */
            sat_cfg_set_int (SAT_CFG_INT_MAP_CENTER,
                             gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (center)));
        }

        dirty = FALSE;
    }

    else if (reset) {
        if (cfg != NULL) {
            /* use g_key_file_remove_key */
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_FILE,
                                   NULL);

            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SHOW_QTH_INFO,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SHOW_NEXT_EVENT,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SHOW_CURS_TRACK,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SHOW_GRID,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_QTH_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_GRID_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_TICK_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SAT_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SAT_SEL_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_TRACK_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SAT_COV_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_INFO_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_INFO_BGD_COL,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_SHADOW_ALPHA,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_TRACK_NUM,
                                   NULL);
            g_key_file_remove_key (cfg,
                                   MOD_CFG_MAP_SECTION,
                                   MOD_CFG_MAP_CENTER,
                                   NULL);
        }
        else {
            /* use sat_cfg_reset_xxx */

            /* background map */
            sat_cfg_reset_str (SAT_CFG_STR_MAP_FILE);

            /* extra contents */
            sat_cfg_reset_bool (SAT_CFG_BOOL_MAP_SHOW_QTH_INFO);
            sat_cfg_reset_bool (SAT_CFG_BOOL_MAP_SHOW_NEXT_EV);
            sat_cfg_reset_bool (SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK);
            sat_cfg_reset_bool (SAT_CFG_BOOL_MAP_SHOW_GRID);

            /* colours */
            sat_cfg_reset_int (SAT_CFG_INT_MAP_QTH_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_GRID_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_TICK_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_SAT_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_SAT_SEL_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_TRACK_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_SAT_COV_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_INFO_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_INFO_BGD_COL);
            sat_cfg_reset_int (SAT_CFG_INT_MAP_SHADOW_ALPHA);

            /* orbit */
            sat_cfg_reset_int (SAT_CFG_INT_MAP_TRACK_NUM);

            /* map center */
            sat_cfg_reset_int (SAT_CFG_INT_MAP_CENTER);
        }
        reset = FALSE;
    }

    g_free (mapf);
}



/** \brief Select a map
 *  \param button Pointer to the "Select Map" button
 *  \param data User data (always NULL)
 *
 * This function is called when the user clicks on the "Select Map"
 * button. The function calls the select_map function which will
 * return a newly allocated string containing the name of the newly
 * selected map, or NULL if the action was cancelled.
 *
 */
static void select_map_cb   (GtkWidget *button, gpointer data)
{
    gchar   *mapfile;
    
    (void) button; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */

    /* execute map selector */
    mapfile = select_map (mapf);

    if (mapfile) {
        /* store new map name */
        g_free (mapf);
        mapf = g_strdup (mapfile);
        g_free (mapfile);
        dirty = TRUE;

        /* update map preview */
        update_map_icon ();
    }
}



static void update_map_icon ()
{
    gchar   *mapfile;
    GdkPixbuf *obuf,*sbuf;


    if (g_path_is_absolute (mapf)) {
        /* map is user specific, ie. in USER_CONF_DIR/maps/ */
        mapfile = g_strdup (mapf);
    }
    else {
        /* build complete path */
        mapfile = map_file_name (mapf);
    }
    obuf = gdk_pixbuf_new_from_file (mapfile, NULL);
    g_free (mapfile);

    if (obuf != NULL) {

        /* scale the pixbuf */
        sbuf = gdk_pixbuf_scale_simple (obuf, 100, 50, GDK_INTERP_HYPER);
        g_object_unref (obuf);

        /* update the GtkImage from the pixbuf */
        gtk_image_clear (GTK_IMAGE (thumb));
        gtk_image_set_from_pixbuf (GTK_IMAGE (thumb), sbuf);
        g_object_unref (sbuf);
    }
    else {
        gtk_image_clear (GTK_IMAGE (thumb));
        gtk_image_set_from_stock (GTK_IMAGE (thumb),
                                  GTK_STOCK_MISSING_IMAGE,
                                  GTK_ICON_SIZE_LARGE_TOOLBAR);
    }

}



