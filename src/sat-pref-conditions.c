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

static void spin_changed_cb     (GtkWidget *spinner, gpointer data);
static void create_reset_button (GtkBox *vbox);
static void reset_cb            (GtkWidget *button, gpointer data);


/** \brief Create and initialise widgets for the radios tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be laoded.
 */
GtkWidget *sat_pref_conditions_create ()
{
     GtkWidget   *table;
     GtkWidget   *label;
     GtkTooltips *tips;
     GtkWidget   *vbox;


     dirty = FALSE;
     reset = FALSE;

     table = gtk_table_new (14, 3, FALSE);
     gtk_table_set_row_spacings (GTK_TABLE (table), 10);
     gtk_table_set_col_spacings (GTK_TABLE (table), 5);

     /* minimum elevation */
     label = gtk_label_new (_("Minimum elevation"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           0, 1, 0, 1,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     minel = gtk_spin_button_new_with_range (0, 90, 1);
     tips = gtk_tooltips_new ();
     gtk_tooltips_set_tip (tips, minel,
                                _("Elevation threshold for passes.\n"\
                                   "Passes with maximum elevation below this limit "\
                                   "will be omitted"),
                                NULL);
     gtk_spin_button_set_digits (GTK_SPIN_BUTTON (minel), 0);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (minel), TRUE);
     gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (minel), FALSE);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (minel),
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_MIN_EL));
     g_signal_connect (G_OBJECT (minel), "value-changed",
                           G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), minel,
                           1, 2, 0, 1,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     label = gtk_label_new (_("[deg]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           2, 3, 0, 1,
                           GTK_FILL | GTK_EXPAND,
                           GTK_SHRINK,
                           0, 0);
     
     /* separator */
     gtk_table_attach (GTK_TABLE (table),
                           gtk_hseparator_new (),
                           0, 3, 1, 2,
                           GTK_FILL | GTK_EXPAND,
                           GTK_SHRINK,
                           0, 0);

     label = gtk_label_new (NULL);
     gtk_label_set_markup (GTK_LABEL (label), _("<b>Multiple Passes:</b>"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           0, 1, 2, 3,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);

     /* number of passes */
     label = gtk_label_new (_("Number of passes to predict"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           0, 1, 3, 4,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     numpass = gtk_spin_button_new_with_range (5, 50, 1);
     tips = gtk_tooltips_new ();
     gtk_tooltips_set_tip (tips, numpass,
                                _("The maximum number of passes to predict."),
                                NULL);
     gtk_spin_button_set_digits (GTK_SPIN_BUTTON (numpass), 0);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (numpass), TRUE);
     gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (numpass), FALSE);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (numpass),
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_NUM_PASS));
     g_signal_connect (G_OBJECT (numpass), "value-changed",
                           G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), numpass,
                           1, 2, 3, 4,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);

     /* lookahead */
     label = gtk_label_new (_("Passes should occur within"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           0, 1, 4, 5,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     lookahead = gtk_spin_button_new_with_range (1, 14, 1);
     tips = gtk_tooltips_new ();
     gtk_tooltips_set_tip (tips, lookahead,
                                _("Only passes that occur within the specified "\
                                   "number of days will be shown."),
                                NULL);
     gtk_spin_button_set_digits (GTK_SPIN_BUTTON (lookahead), 0);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (lookahead), TRUE);
     gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (lookahead), FALSE);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (lookahead),
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD));
     g_signal_connect (G_OBJECT (lookahead), "value-changed",
                           G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), lookahead,
                           1, 2, 4, 5,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     label = gtk_label_new (_("[days]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           2, 3, 4, 5,
                           GTK_FILL | GTK_EXPAND,
                           GTK_SHRINK,
                           0, 0);
     
     /* separator */
     gtk_table_attach (GTK_TABLE (table),
                           gtk_hseparator_new (),
                           0, 3, 5, 6,
                           GTK_FILL | GTK_EXPAND,
                           GTK_SHRINK,
                           0, 0);


     label = gtk_label_new (NULL);
     gtk_label_set_markup (GTK_LABEL (label), _("<b>Pass Details:</b>"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           0, 1, 6, 7,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);

     /* time resolution */
     label = gtk_label_new (_("Time resolution"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           0, 1, 7, 8,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     res = gtk_spin_button_new_with_range (1, 600, 1);
     tips = gtk_tooltips_new ();
     gtk_tooltips_set_tip (tips, res,
                                _("Gpredict will try to show the pass details "\
                                   "with the specified time resolution."),
                                NULL);
     gtk_spin_button_set_digits (GTK_SPIN_BUTTON (res), 0);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (res), TRUE);
     gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (res), FALSE);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (res),
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_RESOLUTION));
     g_signal_connect (G_OBJECT (res), "value-changed",
                           G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), res,
                           1, 2, 7, 8,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     label = gtk_label_new (_("[sec]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           2, 3, 7, 8,
                           GTK_FILL | GTK_EXPAND,
                           GTK_SHRINK,
                           0, 0);

     /* number of entries */
     label = gtk_label_new (_("Number of entries"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label,
                           0, 1, 8, 9,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);
     nument = gtk_spin_button_new_with_range (10, 200, 1);
     tips = gtk_tooltips_new ();
     gtk_tooltips_set_tip (tips, nument,
                                _("Gpredict will try to keep the number of rows "\
                                   "in the detailed prediction within this limit."),
                                NULL);
     gtk_spin_button_set_digits (GTK_SPIN_BUTTON (nument), 0);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (nument), TRUE);
     gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (nument), FALSE);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (nument),
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_NUM_ENTRIES));
     g_signal_connect (G_OBJECT (nument), "value-changed",
                           G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), nument,
                           1, 2, 8, 9,
                           GTK_FILL,
                           GTK_SHRINK,
                           0, 0);

    /* separator */
    gtk_table_attach (GTK_TABLE (table),
                      gtk_hseparator_new (),
                      0, 3, 9, 10,
                      GTK_FILL | GTK_EXPAND,
                      GTK_SHRINK,
                      0, 0);

    /* satellite visibility */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b>Satellite Visibility:</b>"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, 10, 11,
                      GTK_FILL,
                      GTK_SHRINK,
                      0, 0);

    /* twilight threshold */
    label = gtk_label_new (_("Twilight threshold"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      0, 1, 11, 12,
                      GTK_FILL,
                      GTK_SHRINK,
                      0, 0);
    twspin = gtk_spin_button_new_with_range (-18, 0, 1);
    gtk_widget_set_tooltip_text (twspin,
                          _("Satellites are only considered visible if the elevation "\
                            "of the Sun is below the specified threshold.\n"\
                            "  \342\200\242 Astronomical: -18\302\260 to -12\302\260\n"\
                            "  \342\200\242 Nautical: -12\302\260 to -6\302\260\n"\
                            "  \342\200\242 Civil: -6\302\260 to 0\302\260"));
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (twspin), 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (twspin), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (twspin), FALSE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (twspin),
                               sat_cfg_get_int (SAT_CFG_INT_PRED_TWILIGHT_THLD));
    g_signal_connect (G_OBJECT (twspin), "value-changed",
                      G_CALLBACK (spin_changed_cb), NULL);
    gtk_table_attach (GTK_TABLE (table), twspin,
                      1, 2, 11, 12,
                      GTK_FILL,
                      GTK_SHRINK,
                      0, 0);
    label = gtk_label_new (_("[deg]"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label,
                      2, 3, 11, 12,
                      GTK_FILL | GTK_EXPAND,
                      GTK_SHRINK,
                      0, 0);
    
    /* separator */
    gtk_table_attach (GTK_TABLE (table),
                      gtk_hseparator_new (),
                      0, 3, 12, 13,
                      GTK_FILL | GTK_EXPAND,
                      GTK_SHRINK,
                      0, 0);

    /* T0 for predictions */
    tzero = gtk_check_button_new_with_label (_("Always use real time for pass predictions"));
    gtk_widget_set_tooltip_text (tzero,
                                 _("Check this box if you want Gpredict to always use "\
                                   "the current (real) time as starting time when predicting "\
                                   "future satellite passes.\n\n"\
                                   "If you leave the box unchecked and the time controller is "\
                                   "active, Gpredict will use the time from the time controller "\
                                   "as starting time for predicting satellite passes."));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tzero),
                                  sat_cfg_get_bool (SAT_CFG_BOOL_PRED_USE_REAL_T0));
    g_signal_connect (G_OBJECT (tzero), "toggled",
                      G_CALLBACK (spin_changed_cb), NULL);
     
    gtk_table_attach (GTK_TABLE (table), tzero, 0, 3, 13, 14,
                      GTK_FILL | GTK_EXPAND, GTK_SHRINK,
                      0, 0);
    
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
sat_pref_conditions_cancel ()
{
     dirty = FALSE;
     reset = FALSE;
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void
sat_pref_conditions_ok     ()
{
     if (dirty) {
          sat_cfg_set_int (SAT_CFG_INT_PRED_MIN_EL,
                               gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (minel)));
          sat_cfg_set_int (SAT_CFG_INT_PRED_NUM_PASS,
                               gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (numpass)));
          sat_cfg_set_int (SAT_CFG_INT_PRED_LOOK_AHEAD,
                               gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (lookahead)));
          sat_cfg_set_int (SAT_CFG_INT_PRED_RESOLUTION,
                               gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (res)));
          sat_cfg_set_int (SAT_CFG_INT_PRED_NUM_ENTRIES,
                               gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (nument)));
        sat_cfg_set_int (SAT_CFG_INT_PRED_TWILIGHT_THLD,
                         gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (twspin)));
        sat_cfg_set_bool (SAT_CFG_BOOL_PRED_USE_REAL_T0,
                          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tzero)));

          dirty = FALSE;
     }
     else if (reset) {
          sat_cfg_reset_int (SAT_CFG_INT_PRED_MIN_EL);
          sat_cfg_reset_int (SAT_CFG_INT_PRED_NUM_PASS);
          sat_cfg_reset_int (SAT_CFG_INT_PRED_LOOK_AHEAD);
          sat_cfg_reset_int (SAT_CFG_INT_PRED_RESOLUTION);
          sat_cfg_reset_int (SAT_CFG_INT_PRED_NUM_ENTRIES);
        sat_cfg_reset_int (SAT_CFG_INT_PRED_TWILIGHT_THLD);
        sat_cfg_reset_bool (SAT_CFG_BOOL_PRED_USE_REAL_T0);

          reset = FALSE;
     }
}


static void
spin_changed_cb (GtkWidget *spinner, gpointer data)
{
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
     GtkTooltips *tips;


     button = gtk_button_new_with_label (_("Reset"));
     g_signal_connect (G_OBJECT (button), "clicked",
                           G_CALLBACK (reset_cb), NULL);

     tips = gtk_tooltips_new ();
     gtk_tooltips_set_tip (tips, button,
                                _("Reset settings to the default values."),
                                NULL);

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

     /* get defaults */
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (minel),
                                      sat_cfg_get_int_def (SAT_CFG_INT_PRED_MIN_EL));
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (numpass),
                                      sat_cfg_get_int_def (SAT_CFG_INT_PRED_NUM_PASS));
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (lookahead),
                                      sat_cfg_get_int_def (SAT_CFG_INT_PRED_LOOK_AHEAD));
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (res),
                                      sat_cfg_get_int_def (SAT_CFG_INT_PRED_RESOLUTION));
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (nument),
                                      sat_cfg_get_int_def (SAT_CFG_INT_PRED_NUM_ENTRIES));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (twspin),
                               sat_cfg_get_int_def (SAT_CFG_INT_PRED_TWILIGHT_THLD));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tzero),
                                  sat_cfg_get_bool_def (SAT_CFG_BOOL_PRED_USE_REAL_T0));

     /* reset flags */
     reset = TRUE;
     dirty = FALSE;
}
