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
#include "sat-pref-sky-at-glance.h"



static GtkWidget *timesp;  /* spin button for number of hours */
static GtkWidget *col1,*col2,*col3,*col4,*col5;
static GtkWidget *col6,*col7,*col8,*col9,*col10;

static gboolean dirty = FALSE;  /* used to check whether any changes have occurred */
static gboolean reset = FALSE;

static void spin_changed_cb     (GtkWidget *spinner, gpointer data);
static void create_reset_button (GtkBox *vbox);
static void reset_cb            (GtkWidget *button, gpointer data);
static void colour_changed      (GtkWidget *but, gpointer data);



/** \brief Create and initialise widgets for the sky-at-glance tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be laoded.
 */
GtkWidget *sat_pref_sky_at_glance_create ()
{
    GtkWidget   *table;
    GtkWidget   *label;
    GtkWidget   *vbox;
    GdkColor     col;
    guint        rgb;   /* 0xRRGGBB encoded colour */
    guint        y;



    dirty = FALSE;
    reset = FALSE;

    table = gtk_table_new (16, 5, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);


    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b>Time:</b>"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, 0, 1,
                      GTK_FILL,
                      GTK_SHRINK,
                      0, 0);

    /* number of hours */
    label = gtk_label_new (_("Find and show passes that occur within"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, 1, 2,
                      GTK_FILL,
                      GTK_SHRINK,
                      0, 0);
    timesp = gtk_spin_button_new_with_range (1, 24, 1);
    gtk_widget_set_tooltip_text (timesp,
                          _("The passes shown on the Sky at a Glance chart\n"\
                            "will begin within this number of hours."));
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (timesp), 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (timesp), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (timesp), FALSE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (timesp),
                               sat_cfg_get_int (SAT_CFG_INT_SKYATGL_TIME));
    g_signal_connect (G_OBJECT (timesp), "value-changed",
                      G_CALLBACK (spin_changed_cb), NULL);
    gtk_table_attach (GTK_TABLE (table), timesp,
                      1, 2, 1, 2,
                      GTK_SHRINK, GTK_SHRINK,
                      0, 0);
    label = gtk_label_new (_("hours"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      2, 3, 1, 2,
                      GTK_FILL,
                      GTK_SHRINK,
                      0, 0);
    
    /* separator */
    gtk_table_attach (GTK_TABLE (table),
                      gtk_hseparator_new (),
                      0, 5, 2, 3,
                      GTK_FILL | GTK_EXPAND,
                      GTK_SHRINK,
                      0, 0);



    y = 3;

    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b>Colours:</b>"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, y, y+1,
                      GTK_FILL,
                      GTK_SHRINK,
                      0, 0);

    /* colour 1 */
    label = gtk_label_new (_("Colour for satellite 1: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, y+1, y+2,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col1 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col1), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col1), _("Select colour 1"));
    gtk_table_attach (GTK_TABLE (table), col1,
                      1, 2, y+1, y+2,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text ( col1,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_01);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col1), &col);
    g_signal_connect (col1, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 2 */
    label = gtk_label_new (_("Colour for satellite 2: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, y+2, y+3,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col2 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col2), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col1), _("Select colour 2"));
    gtk_table_attach (GTK_TABLE (table), col2,
                      1, 2, y+2, y+3,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text ( col2,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_02);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col2), &col);
    g_signal_connect (col2, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 3 */
    label = gtk_label_new (_("Colour for satellite 3: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, y+3, y+4,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col3 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col3), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col3), _("Select colour 3"));
    gtk_table_attach (GTK_TABLE (table), col3,
                      1, 2, y+3, y+4,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col3,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_03);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col3), &col);
    g_signal_connect (col3, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 4 */
    label = gtk_label_new (_("Colour for satellite 4: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, y+4, y+5,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col4 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col4), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col4), _("Select colour 4"));
    gtk_table_attach (GTK_TABLE (table), col4,
                      1, 2, y+4, y+5,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col4,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_04);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col4), &col);
    g_signal_connect (col4, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 5 */
    label = gtk_label_new (_("Colour for satellite 5: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, y+5, y+6,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col5 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col5), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col5), _("Select colour 5"));
    gtk_table_attach (GTK_TABLE (table), col5,
                      1, 2, y+5, y+6,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col5,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_05);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col5), &col);
    g_signal_connect (col5, "color-set", G_CALLBACK (colour_changed), NULL);


    /* colour 6 */
    label = gtk_label_new (_("Colour for satellite 6: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      3, 4, y+1, y+2,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col6 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col6), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col6), _("Select colour 6"));
    gtk_table_attach (GTK_TABLE (table), col6,
                      4, 5, y+1, y+2,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col6,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_06);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col6), &col);
    g_signal_connect (col6, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 7 */
    label = gtk_label_new (_("Colour for satellite 7: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      3, 4, y+2, y+3,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col7 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col7), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col7), _("Select colour 7"));
    gtk_table_attach (GTK_TABLE (table), col7,
                      4, 5, y+2, y+3,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col7,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_07);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col7), &col);
    g_signal_connect (col7, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 8 */
    label = gtk_label_new (_("Colour for satellite 8: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      3, 4, y+3, y+4,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col8 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col8), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col8), _("Select colour 8"));
    gtk_table_attach (GTK_TABLE (table), col8,
                      4, 5, y+3, y+4,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col8,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_08);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col8), &col);
    g_signal_connect (col8, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 9 */
    label = gtk_label_new (_("Colour for satellite 9: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      3, 4, y+4, y+5,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col9 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col9), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col9), _("Select colour 9"));
    gtk_table_attach (GTK_TABLE (table), col9,
                      4, 5, y+4, y+5,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col9,
                                 _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_09);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col9), &col);
    g_signal_connect (col9, "color-set", G_CALLBACK (colour_changed), NULL);

    /* colour 10 */
    label = gtk_label_new (_("Colour for satellite 10: "));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      3, 4, y+5, y+6,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    col10 = gtk_color_button_new ();
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (col10), FALSE);
    gtk_color_button_set_title (GTK_COLOR_BUTTON (col10), _("Select colour 10"));
    gtk_table_attach (GTK_TABLE (table), col10,
                      4, 5, y+5, y+6,
                      GTK_FILL , GTK_FILL, 0, 0);
    gtk_widget_set_tooltip_text (col10,
                          _("Click to select a colour"));
    rgb = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_10);

    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col10), &col);
    g_signal_connect (col10, "color-set", G_CALLBACK (colour_changed), NULL);




    /* create vertical box */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

    /* create RESET button */
    create_reset_button (GTK_BOX (vbox));


    return vbox;

}


/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void
sat_pref_sky_at_glance_cancel ()
{
    dirty = FALSE;
    reset = FALSE;
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void
sat_pref_sky_at_glance_ok     ()
{
    GdkColor col;
    guint    rgb;


    if (dirty) {
        /* values have changed; store new values */
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_TIME,
                         gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (timesp)));

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col1), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_01, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col2), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_02, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col3), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_03, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col4), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_04, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col5), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_05, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col6), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_06, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col7), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_07, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col8), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_08, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col9), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_09, rgb);

        gtk_color_button_get_color (GTK_COLOR_BUTTON (col10), &col);
        gdk2rgb (&col, &rgb);
        sat_cfg_set_int (SAT_CFG_INT_SKYATGL_COL_10, rgb);


    }
    else if (reset) {
        /* values haven't changed since last reset */
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_TIME);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_01);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_02);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_03);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_04);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_05);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_06);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_07);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_08);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_09);
        sat_cfg_reset_int (SAT_CFG_INT_SKYATGL_COL_10);

        /* FIXME: sats */
    }
}


static void
spin_changed_cb (GtkWidget *spinner, gpointer data)
{
    (void) spinner; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */

    dirty = TRUE;
}






/** \brief Create RESET button.
 *  \param cfg Config data or NULL in global mode.
 *  \param vbox The container.
 *
 * This function creates and sets up the view selector combos.
 */
static void
create_reset_button (GtkBox *vbox)
{
    GtkWidget   *button;
    GtkWidget   *butbox;

    button = gtk_button_new_with_label (_("Reset"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (reset_cb), NULL);

    gtk_widget_set_tooltip_text (button,
                                 _("Reset settings to the default values."));

    butbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end (GTK_BOX (butbox), button, FALSE, TRUE, 10);

    gtk_box_pack_end (vbox, butbox, FALSE, TRUE, 0);

}


/** \brief Reset settings.
 *  \param button The RESET button.
 *  \param data User data (unused).
 *
 * This function is called when the user clicks on the RESET button. The function
 * will get the default values for the parameters and set the dirty and reset flags
 * apropriately. The reset will not have any effect if the user cancels the
 * dialog.
 */
static void
reset_cb               (GtkWidget *button, gpointer data)
{
    guint    rgb;
    GdkColor col;

    (void) data; /* avoid unused parameter compiler warning */
    (void) button; /* avoid unused parameter compiler warning */

    /* get defaults */

    /* hours */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (timesp),
                               sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_TIME));

    /* satellites */

    /* colours */
    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_01);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col1), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_02);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col2), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_03);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col3), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_04);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col4), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_05);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col5), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_06);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col6), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_07);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col7), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_08);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col8), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_09);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col9), &col);

    rgb = sat_cfg_get_int_def (SAT_CFG_INT_SKYATGL_COL_10);
    rgb2gdk (rgb, &col);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (col10), &col);

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}


/** \brief Manage color and font changes.
 *  \param but The color/font picker button that received the signal.
 *  \param data User data (always NULL).
 *
 * We don't need to do anything but set the dirty flag since the values can
 * always be obtained from the global widgets.
 */
static void
colour_changed     (GtkWidget *but, gpointer data)
{
    (void) but; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */
    
    dirty = TRUE;
}


