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
#include "sat-pref-refresh.h"


static GtkWidget *dataspin;   /* spin button for module refresh rate */
static GtkWidget *listspin;   /* spin button for list view */
static GtkWidget *mapspin;    /* spin button for map view */
static GtkWidget *polarspin;  /* spin button for polar view */
static GtkWidget *singlespin; /* spin button for single-sat view */

static gboolean dirty = FALSE;  /* used to check whether any changes have occurred */
static gboolean reset = FALSE;


static void spin_changed_cb     (GtkWidget *spinner, gpointer data);
static void create_reset_button (GKeyFile *cfg, GtkBox *vbox);
static void reset_cb            (GtkWidget *button, gpointer cfg);



/** \brief Create and initialise widgets for the refresh rates tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be laoded.
 */
GtkWidget *sat_pref_refresh_create (GKeyFile *cfg)
{
     GtkWidget *table;
     GtkWidget *vbox;
     GtkWidget *label;
     gint       val;

     dirty = FALSE;
     reset = FALSE;

     /* create table */
     table = gtk_table_new (6, 3, FALSE);
     gtk_table_set_row_spacings (GTK_TABLE (table), 10);
     gtk_table_set_col_spacings (GTK_TABLE (table), 5);

     /* data refresh */
     label = gtk_label_new (_("Refresh data every"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     dataspin = gtk_spin_button_new_with_range (100, 10000, 1);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (dataspin), 1, 100);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (dataspin), TRUE);
     gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (dataspin),
                            GTK_UPDATE_IF_VALID);
     if (cfg != NULL) {
          val = mod_cfg_get_int (cfg,
                           MOD_CFG_GLOBAL_SECTION,
                           MOD_CFG_TIMEOUT_KEY,
                           SAT_CFG_INT_MODULE_TIMEOUT);
     }
     else {
          val = sat_cfg_get_int (SAT_CFG_INT_MODULE_TIMEOUT);
     }
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (dataspin), val);
     g_signal_connect (G_OBJECT (dataspin), "value-changed",
                 G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), dataspin, 1, 2, 0, 1,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     label = gtk_label_new (_("[msec]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
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


     /* List View */
     label = gtk_label_new (_("Refresh list view every"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     listspin = gtk_spin_button_new_with_range (1, 50, 1);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (listspin), 1, 5);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (listspin), TRUE);
     gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (listspin),
                            GTK_UPDATE_IF_VALID);
     if (cfg != NULL) {
          val = mod_cfg_get_int (cfg,
                           MOD_CFG_LIST_SECTION,
                           MOD_CFG_LIST_REFRESH,
                           SAT_CFG_INT_LIST_REFRESH);
     }
     else {
          val = sat_cfg_get_int (SAT_CFG_INT_LIST_REFRESH);
     }
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (listspin), val);
     g_signal_connect (G_OBJECT (listspin), "value-changed",
                 G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), listspin, 1, 2, 2, 3,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     label = gtk_label_new (_("[cycle]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3,
                 GTK_FILL | GTK_EXPAND,
                 GTK_SHRINK,
                 0, 0);


     /* Map View */
     label = gtk_label_new (_("Refresh map view every"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     mapspin = gtk_spin_button_new_with_range (1, 50, 1);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (mapspin), 1, 5);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (mapspin), TRUE);
     gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (mapspin),
                            GTK_UPDATE_IF_VALID);
     if (cfg != NULL) {
          val = mod_cfg_get_int (cfg,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_REFRESH,
                           SAT_CFG_INT_MAP_REFRESH);
     }
     else {
          val = sat_cfg_get_int (SAT_CFG_INT_MAP_REFRESH);
     }
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (mapspin), val);
     g_signal_connect (G_OBJECT (mapspin), "value-changed",
                 G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), mapspin, 1, 2, 3, 4,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     label = gtk_label_new (_("[cycle]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 2, 3, 3, 4,
                 GTK_FILL | GTK_EXPAND,
                 GTK_SHRINK,
                 0, 0);


     /* Polar View */
     label = gtk_label_new (_("Refresh polar view every"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     polarspin = gtk_spin_button_new_with_range (1, 50, 1);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (polarspin), 1, 5);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (polarspin), TRUE);
     gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (polarspin),
                            GTK_UPDATE_IF_VALID);
     if (cfg != NULL) {
          val = mod_cfg_get_int (cfg,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_REFRESH,
                           SAT_CFG_INT_POLAR_REFRESH);
     }
     else {
          val = sat_cfg_get_int (SAT_CFG_INT_POLAR_REFRESH);
     }
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (polarspin), val);
     g_signal_connect (G_OBJECT (polarspin), "value-changed",
                 G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), polarspin, 1, 2, 4, 5,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     label = gtk_label_new (_("[cycle]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 2, 3, 4, 5,
                 GTK_FILL | GTK_EXPAND,
                 GTK_SHRINK,
                 0, 0);


     /* Single-Sat View */
     label = gtk_label_new (_("Refresh single-sat view every"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     singlespin = gtk_spin_button_new_with_range (1, 50, 1);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (singlespin), 1, 5);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (singlespin), TRUE);
     gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (singlespin),
                            GTK_UPDATE_IF_VALID);
     if (cfg != NULL) {
          val = mod_cfg_get_int (cfg,
                           MOD_CFG_SINGLE_SAT_SECTION,
                           MOD_CFG_SINGLE_SAT_REFRESH,
                           SAT_CFG_INT_SINGLE_SAT_REFRESH);
     }
     else {
          val = sat_cfg_get_int (SAT_CFG_INT_SINGLE_SAT_REFRESH);
     }
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (singlespin), val);
     g_signal_connect (G_OBJECT (singlespin), "value-changed",
                 G_CALLBACK (spin_changed_cb), NULL);
     gtk_table_attach (GTK_TABLE (table), singlespin, 1, 2, 5, 6,
                 GTK_FILL,
                 GTK_SHRINK,
                 0, 0);

     label = gtk_label_new (_("[cycle]"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 2, 3, 5, 6,
                 GTK_FILL | GTK_EXPAND,
                 GTK_SHRINK,
                 0, 0);


     /* create vertical box */
     vbox = gtk_vbox_new (FALSE, 0);
     gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);
     gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

     /* create RESET button */
     create_reset_button (cfg, GTK_BOX (vbox));
     

        return vbox;
}



/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void
sat_pref_refresh_cancel (GKeyFile *cfg)
{

     dirty = FALSE;
}



/** \brief User pressed OK. Any changes should be stored in config.
 */
void
sat_pref_refresh_ok     (GKeyFile *cfg)
{
     if (dirty) {

          if (cfg != NULL) {

               g_key_file_set_integer (cfg,
                              MOD_CFG_GLOBAL_SECTION,
                              MOD_CFG_TIMEOUT_KEY,
                              gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dataspin)));

               g_key_file_set_integer (cfg,
                              MOD_CFG_LIST_SECTION,
                              MOD_CFG_LIST_REFRESH,
                              gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (listspin)));

               g_key_file_set_integer (cfg,
                              MOD_CFG_MAP_SECTION,
                              MOD_CFG_MAP_REFRESH,
                              gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mapspin)));

               g_key_file_set_integer (cfg,
                              MOD_CFG_POLAR_SECTION,
                              MOD_CFG_POLAR_REFRESH,
                              gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (polarspin)));

               g_key_file_set_integer (cfg,
                              MOD_CFG_SINGLE_SAT_SECTION,
                              MOD_CFG_SINGLE_SAT_REFRESH,
                              gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (singlespin)));

          }
          else {

               sat_cfg_set_int (SAT_CFG_INT_MODULE_TIMEOUT,
                          gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dataspin)));

               sat_cfg_set_int (SAT_CFG_INT_LIST_REFRESH,
                          gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (listspin)));

               sat_cfg_set_int (SAT_CFG_INT_MAP_REFRESH,
                          gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mapspin)));

               sat_cfg_set_int (SAT_CFG_INT_POLAR_REFRESH,
                          gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (polarspin)));

               sat_cfg_set_int (SAT_CFG_INT_SINGLE_SAT_REFRESH,
                          gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (singlespin)));

          }

     }
     else if (reset) {
          /* we have to reset the values to global or default settings */
          if (cfg == NULL) {
               /* reset values in sat-cfg */
               sat_cfg_reset_int (SAT_CFG_INT_MODULE_TIMEOUT);
               sat_cfg_reset_int (SAT_CFG_INT_LIST_REFRESH);
               sat_cfg_reset_int (SAT_CFG_INT_MAP_REFRESH);
               sat_cfg_reset_int (SAT_CFG_INT_POLAR_REFRESH);
               sat_cfg_reset_int (SAT_CFG_INT_SINGLE_SAT_REFRESH);
          }
          else {
               /* remove keys */
               g_key_file_remove_key ((GKeyFile *)(cfg),
                                MOD_CFG_GLOBAL_SECTION,
                                MOD_CFG_TIMEOUT_KEY,
                                NULL);
               g_key_file_remove_key ((GKeyFile *)(cfg),
                                MOD_CFG_LIST_SECTION,
                                MOD_CFG_LIST_REFRESH,
                                NULL);
               g_key_file_remove_key ((GKeyFile *)(cfg),
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_REFRESH,
                                NULL);
               g_key_file_remove_key ((GKeyFile *)(cfg),
                                MOD_CFG_POLAR_SECTION,
                                MOD_CFG_POLAR_REFRESH,
                                NULL);
               g_key_file_remove_key ((GKeyFile *)(cfg),
                                MOD_CFG_SINGLE_SAT_SECTION,
                                MOD_CFG_SINGLE_SAT_REFRESH,
                                NULL);
          }
     }

     dirty = FALSE;
     reset = FALSE;
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
create_reset_button (GKeyFile *cfg, GtkBox *vbox)
{
     GtkWidget   *button;
     GtkWidget   *butbox;
     GtkTooltips *tips;


     button = gtk_button_new_with_label (_("Reset"));
     g_signal_connect (G_OBJECT (button), "clicked",
                 G_CALLBACK (reset_cb), cfg);

     tips = gtk_tooltips_new ();
     if (cfg == NULL) {
          gtk_tooltips_set_tip (tips, button,
                          _("Reset settings to the default values."),
                          NULL);
     }
     else {
          gtk_tooltips_set_tip (tips, button,
                          _("Reset module settings to the global values."),
                          NULL);
     }

     butbox = gtk_hbutton_box_new ();
     gtk_button_box_set_layout (GTK_BUTTON_BOX (butbox), GTK_BUTTONBOX_END);
     gtk_box_pack_end (GTK_BOX (butbox), button, FALSE, TRUE, 10);

     gtk_box_pack_end (vbox, butbox, FALSE, TRUE, 0);

}


/** \brief Reset settings.
 *  \param button The RESET button.
 *  \param cfg Pointer to the module config or NULL in global mode.
 *
 * This function is called when the user clicks on the RESET button. In global mode
 * (when cfg = NULL) the function will reset the settings to the efault values, while
 * in "local" mode (when cfg != NULL) the function will reset the module settings to
 * the global settings. This is done by removing the corresponding key from the GKeyFile.
 */
static void
reset_cb               (GtkWidget *button, gpointer cfg)
{
     gint val;


     /* views */
     if (cfg == NULL) {
          /* global mode, get defaults */
          val = sat_cfg_get_int_def (SAT_CFG_INT_MODULE_TIMEOUT);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (dataspin), val);
          val = sat_cfg_get_int_def (SAT_CFG_INT_LIST_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (listspin), val);
          val = sat_cfg_get_int_def (SAT_CFG_INT_MAP_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (mapspin), val);
          val = sat_cfg_get_int_def (SAT_CFG_INT_POLAR_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (polarspin), val);
          val = sat_cfg_get_int_def (SAT_CFG_INT_SINGLE_SAT_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (singlespin), val);
     }
     else {
          /* local mode, get global value */
          val = sat_cfg_get_int (SAT_CFG_INT_MODULE_TIMEOUT);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (dataspin), val);
          val = sat_cfg_get_int (SAT_CFG_INT_LIST_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (listspin), val);
          val = sat_cfg_get_int (SAT_CFG_INT_MAP_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (mapspin), val);
          val = sat_cfg_get_int (SAT_CFG_INT_POLAR_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (polarspin), val);
          val = sat_cfg_get_int (SAT_CFG_INT_SINGLE_SAT_REFRESH);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (singlespin), val);
     }


     /* reset flags */
     reset = TRUE;
     dirty = FALSE;
}
