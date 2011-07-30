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
#include "sat-pass-dialogs.h"
#include "sat-cfg.h"
#include "sat-pref-multi-pass.h"



/** \brief First row where checkboxes are placed */
#define Y0 1

/** \brief Number of columns in the table */
#define COLUMNS 2


static GtkWidget *check[MULTI_PASS_COL_NUMBER];
static guint startflags;
static guint flags;
static gboolean dirty = FALSE;
static gboolean reset = FALSE;


extern const gchar *MULTI_PASS_COL_HINT[];


static void toggle_cb           (GtkToggleButton *toggle, gpointer data);
static void create_reset_button (GtkBox *vbox);
static void reset_cb            (GtkWidget *button, gpointer data);



/** \brief Create and initialise widgets for the radios tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be laoded.
 */
GtkWidget *sat_pref_multi_pass_create ()
{
     GtkWidget *table;
     GtkWidget *label;
     GtkWidget *vbox;
     guint      i;



     /* create the table */
     table = gtk_table_new ((MULTI_PASS_COL_NUMBER+1)/COLUMNS + 1, COLUMNS, TRUE);
     gtk_table_set_row_spacings (GTK_TABLE (table), 10);
     gtk_table_set_col_spacings (GTK_TABLE (table), 5);

     /* create header */
     label = gtk_label_new (NULL);
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_label_set_markup (GTK_LABEL (label), 
                     _("<b>Visible Columns:</b>"));

     gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
                 GTK_FILL,  GTK_SHRINK, 0, 0);

     /* get visible column flags */
     flags = sat_cfg_get_int (SAT_CFG_INT_PRED_MULTI_COL);

     for (i = 0; i < MULTI_PASS_COL_NUMBER; i++) {

          check[i] = gtk_check_button_new_with_label (MULTI_PASS_COL_HINT[i]);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check[i]),
                               flags & (1 << i));

          gtk_table_attach (GTK_TABLE (table), check[i],
                      i % COLUMNS, (i % COLUMNS) + 1,
                      Y0 + i / COLUMNS, Y0 + i / COLUMNS + 1,
                      GTK_FILL,  GTK_SHRINK, 0, 0);

          g_signal_connect (check[i], "toggled",
                      G_CALLBACK (toggle_cb),
                      GUINT_TO_POINTER (i));

     }


     /* create vertical box */
     vbox = gtk_vbox_new (FALSE, 0);
     gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);
     gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

     /* create RESET button */
     create_reset_button (GTK_BOX (vbox));

     startflags = flags;
     dirty = FALSE;
     reset = FALSE;


     return vbox;
}


/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void
sat_pref_multi_pass_cancel ()
{
     dirty = FALSE;
     reset = FALSE;
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void
sat_pref_multi_pass_ok     ()
{

     if (dirty) {
          sat_cfg_set_int (SAT_CFG_INT_PRED_MULTI_COL, flags);
          dirty = FALSE;
     }
     else if (reset) {
          sat_cfg_reset_int (SAT_CFG_INT_PRED_MULTI_COL);
          reset = FALSE;
     }

}


static void
toggle_cb  (GtkToggleButton *toggle, gpointer data)
{


     if (gtk_toggle_button_get_active (toggle)) {

          flags |= (1 << GPOINTER_TO_UINT (data));
     }
     else {
          flags &= ~(1 << GPOINTER_TO_UINT (data));
     }
     
     /* clear dirty flag if we are back where we started */
     dirty = (flags != startflags);
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
     guint i;
     
     (void) button; /* avoid unused parameter compiler warning */
     (void) data; /* avoid unused parameter compiler warning */
     
     
     /* get defaults */
     flags = sat_cfg_get_int_def (SAT_CFG_INT_PRED_MULTI_COL);

     for (i = 0; i < MULTI_PASS_COL_NUMBER; i++) {

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check[i]),
                               flags & (1 << i));

     }

     /* reset flags */
     reset = TRUE;
     dirty = FALSE;
}
