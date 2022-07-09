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

#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-sky-at-glance.h"


static GtkWidget *timesp;       /* spin button for number of hours */
static GtkWidget *col1, *col2, *col3, *col4, *col5;
static GtkWidget *col6, *col7, *col8, *col9, *col10;

static gboolean dirty = FALSE;  /* used to check whether any changes have occurred */
static gboolean reset = FALSE;

void sat_pref_sky_at_glance_cancel()
{
    dirty = FALSE;
    reset = FALSE;
}

void sat_pref_sky_at_glance_ok()
{
    GdkRGBA         gdk_rgba;
    guint           rgb;

    if (dirty)
    {
        /* values have changed; store new values */
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_TIME,
                        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                         (timesp)));

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col1), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_01, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col2), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_02, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col3), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_03, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col4), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_04, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col5), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_05, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col6), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_06, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col7), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_07, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col8), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_08, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col9), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_09, rgb);

        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(col10), &gdk_rgba);
        rgb = rgb_to_cfg(&gdk_rgba);
        sat_cfg_set_int(SAT_CFG_INT_SKYATGL_COL_10, rgb);
    }
    else if (reset)
    {
        /* values haven't changed since last reset */
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_TIME);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_01);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_02);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_03);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_04);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_05);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_06);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_07);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_08);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_09);
        sat_cfg_reset_int(SAT_CFG_INT_SKYATGL_COL_10);

        /* FIXME: sats */
    }
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
 * @param data User data (unused).
 *
 * This function is called when the user clicks on the RESET button. The function
 * will get the default values for the parameters and set the dirty and reset flags
 * appropriately. The reset will not have any effect if the user cancels the
 * dialog.
 */
static void reset_cb(GtkWidget * button, gpointer data)
{
    guint           rgb;
    GdkRGBA         gdk_rgba;

    (void)data;
    (void)button;

    /* get defaults */

    /* hours */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(timesp),
                              sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_TIME));

    /* satellites */

    /* colours */
    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_01);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col1), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_02);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col2), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_03);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col3), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_04);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col4), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_05);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col5), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_06);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col6), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_07);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col7), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_08);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col8), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_09);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col9), &gdk_rgba);

    rgb = sat_cfg_get_int_def(SAT_CFG_INT_SKYATGL_COL_10);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col10), &gdk_rgba);

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
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
 * Create RESET button.
 *
 * @param cfg Config data or NULL in global mode.
 * @param vbox The container.
 *
 * This function creates and sets up the view selector combos.
 */
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

/**
 * Create and initialise widgets for the sky-at-glance tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be loaded.
 */
GtkWidget      *sat_pref_sky_at_glance_create()
{
    GtkWidget      *table;
    GtkWidget      *label;
    GtkWidget      *vbox;
    GdkRGBA         gdk_rgba;
    guint           rgb;        /* 0xRRGGBB encoded colour */
    guint           y;

    dirty = FALSE;
    reset = FALSE;

    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(table), FALSE);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);
    gtk_grid_set_row_spacing(GTK_GRID(table), 5);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Time:</b>"));
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    /* number of hours */
    label = gtk_label_new(_("Find and show passes that occur within"));
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 1, 2, 1);
    timesp = gtk_spin_button_new_with_range(1, 24, 1);
    gtk_widget_set_tooltip_text(timesp,
                                _
                                ("The passes shown on the Sky at a Glance chart\n"
                                 "will begin within this number of hours."));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(timesp), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(timesp), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(timesp), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(timesp),
                              sat_cfg_get_int(SAT_CFG_INT_SKYATGL_TIME));
    g_signal_connect(G_OBJECT(timesp), "value-changed",
                     G_CALLBACK(spin_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), timesp, 2, 1, 1, 1);
    label = gtk_label_new(_("hours"));
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 3, 1, 1, 1);

    /* separator */
    gtk_grid_attach(GTK_GRID(table),
                    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 2, 5, 1);

    y = 3;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Colours:</b>"));
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, y, 1, 1);

    /* colour 1 */
    label = gtk_label_new(_("Satellite 1: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, y + 1, 1, 1);
    col1 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col1), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col1), _("Select colour 1"));
    gtk_grid_attach(GTK_GRID(table), col1, 1, y + 1, 1, 1);
    gtk_widget_set_tooltip_text(col1, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_01);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col1), &gdk_rgba);
    g_signal_connect(col1, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 2 */
    label = gtk_label_new(_("Satellite 2: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, y + 2, 1, 1);
    col2 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col2), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col1), _("Select colour 2"));
    gtk_grid_attach(GTK_GRID(table), col2, 1, y + 2, 1, 1);
    gtk_widget_set_tooltip_text(col2, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_02);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col2), &gdk_rgba);
    g_signal_connect(col2, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 3 */
    label = gtk_label_new(_("Satellite 3: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, y + 3, 1, 1);
    col3 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col3), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col3), _("Select colour 3"));
    gtk_grid_attach(GTK_GRID(table), col3, 1, y + 3, 1, 1);
    gtk_widget_set_tooltip_text(col3, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_03);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col3), &gdk_rgba);
    g_signal_connect(col3, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 4 */
    label = gtk_label_new(_("Satellite 4: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, y + 4, 1, 1);
    col4 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col4), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col4), _("Select colour 4"));
    gtk_grid_attach(GTK_GRID(table), col4, 1, y + 4, 1, 1);
    gtk_widget_set_tooltip_text(col4, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_04);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col4), &gdk_rgba);
    g_signal_connect(col4, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 5 */
    label = gtk_label_new(_("Satellite 5: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, y + 5, 1, 1);
    col5 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col5), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col5), _("Select colour 5"));
    gtk_grid_attach(GTK_GRID(table), col5, 1, y + 5, 1, 1);
    gtk_widget_set_tooltip_text(col5, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_05);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col5), &gdk_rgba);
    g_signal_connect(col5, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 6 */
    label = gtk_label_new(_("Satellite 6: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, y + 1, 1, 1);
    col6 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col6), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col6), _("Select colour 6"));
    gtk_grid_attach(GTK_GRID(table), col6, 3, y + 1, 1, 1);
    gtk_widget_set_tooltip_text(col6, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_06);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col6), &gdk_rgba);
    g_signal_connect(col6, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 7 */
    label = gtk_label_new(_("Satellite 7: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, y + 2, 1, 1);
    col7 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col7), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col7), _("Select colour 7"));
    gtk_grid_attach(GTK_GRID(table), col7, 3, y + 2, 1, 1);
    gtk_widget_set_tooltip_text(col7, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_07);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col7), &gdk_rgba);
    g_signal_connect(col7, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 8 */
    label = gtk_label_new(_("Satellite 8: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, y + 3, 1, 1);
    col8 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col8), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col8), _("Select colour 8"));
    gtk_grid_attach(GTK_GRID(table), col8, 3, y + 3, 1, 1);
    gtk_widget_set_tooltip_text(col8, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_08);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col8), &gdk_rgba);
    g_signal_connect(col8, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 9 */
    label = gtk_label_new(_("Satellite 9: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, y + 4, 1, 1);
    col9 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col9), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col9), _("Select colour 9"));
    gtk_grid_attach(GTK_GRID(table), col9, 3, y + 4, 1, 1);
    gtk_widget_set_tooltip_text(col9, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_09);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col9), &gdk_rgba);
    g_signal_connect(col9, "color-set", G_CALLBACK(colour_changed), NULL);

    /* colour 10 */
    label = gtk_label_new(_("Satellite 10: "));
    g_object_set(label, "xalign", 1.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, y + 5, 1, 1);
    col10 = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(col10), FALSE);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(col10), _("Select colour 10"));
    gtk_grid_attach(GTK_GRID(table), col10, 3, y + 5, 1, 1);
    gtk_widget_set_tooltip_text(col10, _("Click to select a colour"));
    rgb = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_10);
    rgb_from_cfg(rgb, &gdk_rgba);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(col10), &gdk_rgba);
    g_signal_connect(col10, "color-set", G_CALLBACK(colour_changed), NULL);

    /* create vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

    /* create RESET button */
    create_reset_button(GTK_BOX(vbox));

    return vbox;
}
