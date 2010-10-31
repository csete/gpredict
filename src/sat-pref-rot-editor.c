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

/** \brief Edit rotator configuration.
 *
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "rotor-conf.h"
#include "sat-pref-rot-editor.h"



extern GtkWidget *window; /* dialog window defined in sat-pref.c */



/* private widgets */
static GtkWidget *dialog;   /* dialog window */
static GtkWidget *name;     /* Configuration name */
static GtkWidget *host;     /* host name or IP */
static GtkWidget *port;     /* port number */
static GtkWidget *aztype;
static GtkWidget *minaz;
static GtkWidget *maxaz;
static GtkWidget *minel;
static GtkWidget *maxel;


static GtkWidget    *create_editor_widgets (rotor_conf_t *conf);
static void          update_widgets        (rotor_conf_t *conf);
static void          clear_widgets         (void);
static gboolean      apply_changes         (rotor_conf_t *conf);
static void          name_changed          (GtkWidget *widget, gpointer data);
static void          aztype_changed_cb     (GtkComboBox *box, gpointer data);


/** \brief Add or edit a rotor configuration.
 * \param conf Pointer to a rotator configuration.
 *
 * If conf->name is not NULL the widgets will be populated with the data.
 */
void
sat_pref_rot_editor_run (rotor_conf_t *conf)
{
     gint       response;
     gboolean   finished = FALSE;


     /* crate dialog and add contents */
     dialog = gtk_dialog_new_with_buttons (_("Edit rotator configuration"),
                                                    GTK_WINDOW (window),
                                                    GTK_DIALOG_MODAL |
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CLEAR,
                                                    GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OK,
                                                    GTK_RESPONSE_OK,
                                                    NULL);

     /* disable OK button to begin with */
     gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                                GTK_RESPONSE_OK,
                                                FALSE);

     gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                            create_editor_widgets (conf));

     /* this hacky-thing is to keep the dialog running in case the
        CLEAR button is plressed. OK and CANCEL will exit the loop
     */
     while (!finished) {

          response = gtk_dialog_run (GTK_DIALOG (dialog));

          switch (response) {

               /* OK */
          case GTK_RESPONSE_OK:
               if (apply_changes (conf)) {
                    finished = TRUE;
               }
               else {
                    finished = FALSE;
               }
               break;

               /* CLEAR */
          case GTK_RESPONSE_REJECT:
               clear_widgets ();
               break;

               /* Everything else is considered CANCEL */
          default:
               finished = TRUE;
               break;
          }
     }

     gtk_widget_destroy (dialog);
}


/** \brief Create and initialise widgets */
static GtkWidget *
create_editor_widgets (rotor_conf_t *conf)
{
     GtkWidget    *table;
     GtkWidget    *label;


     table = gtk_table_new (7, 4, FALSE);
     gtk_container_set_border_width (GTK_CONTAINER (table), 5);
     gtk_table_set_col_spacings (GTK_TABLE (table), 5);
     gtk_table_set_row_spacings (GTK_TABLE (table), 5);

     /* Config name */
     label = gtk_label_new (_("Name"));
     gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
     
     name = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (name), 25);
    gtk_widget_set_tooltip_text (name,
                                 _("Enter a short name for this configuration, e.g. ROTOR-1.\n"\
                                   "Allowed characters: 0..9, a..z, A..Z, - and _"));
     gtk_table_attach_defaults (GTK_TABLE (table), name, 1, 4, 0, 1);

     /* attach changed signal so that we can enable OK button when
        a proper name has been entered
     */
     g_signal_connect (name, "changed", G_CALLBACK (name_changed), NULL);

     /* Host */
     label = gtk_label_new (_("Host"));
     gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
    
    host = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (host), 50);
    gtk_entry_set_text (GTK_ENTRY (host), "localhost");
    gtk_widget_set_tooltip_text (host,
                                 _("Enter the host where rigctld is running. "\
                                 "You can use both host name and IP address, "\
                                 "e.g. 192.168.1.100\n\n"\
                                 "If gpredict and rotctld are running on the "\
                                 "same computer, use localhost"));
    gtk_table_attach_defaults (GTK_TABLE (table), host, 1, 4, 1, 2);
     
    /* port */
    label = gtk_label_new (_("Port"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
    
    port = gtk_spin_button_new_with_range (1024, 65535, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (port), 4533); 
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (port), 0);
    gtk_widget_set_tooltip_text (port,
                                 _("Enter the port number where rotctld is listening. Default is 4533."));
    gtk_table_attach_defaults (GTK_TABLE (table), port, 1, 2, 2, 3);

    gtk_table_attach_defaults (GTK_TABLE (table), gtk_hseparator_new(), 0, 4, 3, 4);
    
    /* Az-type */
    label = gtk_label_new (_("Az type"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
    
    aztype = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (aztype),
                               "0\302\260 \342\206\222 180\302\260 \342\206\222 360\302\260");
    gtk_combo_box_append_text (GTK_COMBO_BOX (aztype),
                               "-180\302\260 \342\206\222 0\302\260 \342\206\222 +180\302\260");
    gtk_combo_box_set_active (GTK_COMBO_BOX (aztype), 0);
    gtk_widget_set_tooltip_text (aztype,
                                 _("Select your azimuth range here. Note that gpredict assumes "
                                  "that 0\302\260 is at North and + direction is clockwise for "\
                                  "both types"));
    gtk_table_attach_defaults (GTK_TABLE (table), aztype, 1, 3, 4, 5);
    g_signal_connect (G_OBJECT (aztype), "changed", G_CALLBACK (aztype_changed_cb), NULL);
    
    /* Az and El limits */
    label = gtk_label_new (_(" Min Az"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 5, 6);
    minaz = gtk_spin_button_new_with_range (-200, 100, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (minaz), 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (minaz), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (minaz), FALSE);
    gtk_table_attach_defaults (GTK_TABLE (table), minaz, 1, 2, 5, 6);
    
    label = gtk_label_new (_(" Max Az"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 5, 6);
    maxaz = gtk_spin_button_new_with_range (0, 450, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxaz), 360);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (maxaz), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (maxaz), FALSE);
    gtk_table_attach_defaults (GTK_TABLE (table), maxaz, 3, 4, 5, 6);
    
    label = gtk_label_new (_(" Min El"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 6, 7);
    minel = gtk_spin_button_new_with_range (-10, 180, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (minel), 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (minel), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (minel), FALSE);
    gtk_table_attach_defaults (GTK_TABLE (table), minel, 1, 2, 6, 7);
    
    label = gtk_label_new (_(" Max El"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 6, 7);
    maxel = gtk_spin_button_new_with_range (-10, 180, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxel), 90);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (maxel), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (maxel), FALSE);
    gtk_table_attach_defaults (GTK_TABLE (table), maxel, 3, 4, 6, 7);
    
    if (conf->name != NULL)
          update_widgets (conf);

     gtk_widget_show_all (table);

     return table;
}


/** \brief Update widgets from the currently selected row in the treeview
 */
static void
update_widgets (rotor_conf_t *conf)
{
    
    /* configuration name */
    gtk_entry_set_text (GTK_ENTRY (name), conf->name);
    
    /* host */
    if (conf->host)
        gtk_entry_set_text (GTK_ENTRY (host), conf->host);
    
    /* port */
    if (conf->port > 1023)
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (port), conf->port);
    else
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (port), 4533); /* hamlib default? */
    
    gtk_combo_box_set_active (GTK_COMBO_BOX (aztype), conf->aztype);
    
    /* az and el limits */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (minaz), conf->minaz);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxaz), conf->maxaz);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (minel), conf->minel);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxel), conf->maxel);
    

}



/** \brief Clear the contents of all widgets.
 *
 * This function is usually called when the user clicks on the CLEAR button
 *
 */
static void
clear_widgets () 
{
    gtk_entry_set_text (GTK_ENTRY (name), "");
    gtk_entry_set_text (GTK_ENTRY (host), "localhost");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (port), 4533); /* hamlib default? */
    gtk_combo_box_set_active (GTK_COMBO_BOX (aztype), ROT_AZ_TYPE_360);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (minaz), 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxaz), 360);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (minel), 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxel), 90);
}


/** \brief Apply changes.
 *  \return TRUE if things are ok, FALSE otherwise.
 *
 * This function is usually called when the user clicks the OK button.
 */
static gboolean
apply_changes         (rotor_conf_t *conf)
{

    
    /* name */
    if (conf->name)
        g_free (conf->name);

    conf->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (name)));
    
    /* host */
    if (conf->host)
        g_free (conf->host);
    
    conf->host = g_strdup (gtk_entry_get_text (GTK_ENTRY (host)));

    /* port */
    conf->port = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (port));

    /* az type */
    conf->aztype = gtk_combo_box_get_active (GTK_COMBO_BOX (aztype));
    
    /* az and el ranges */
    conf->minaz = gtk_spin_button_get_value (GTK_SPIN_BUTTON (minaz));
    conf->maxaz = gtk_spin_button_get_value (GTK_SPIN_BUTTON (maxaz));
    conf->minel = gtk_spin_button_get_value (GTK_SPIN_BUTTON (minel));
    conf->maxel = gtk_spin_button_get_value (GTK_SPIN_BUTTON (maxel));
    
     return TRUE;
}



/** \brief Manage name changes.
 *
 * This function is called when the contents of the name entry changes.
 * The primary purpose of this function is to check whether the char length
 * of the name is greater than zero, if yes enable the OK button of the dialog.
 */
static void
name_changed          (GtkWidget *widget, gpointer data)
{
     const gchar *text;
     gchar *entry, *end, *j;
     gint len, pos;


     /* step 1: ensure that only valid characters are entered
        (stolen from xlog, tnx pg4i)
     */
     entry = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
     if ((len = g_utf8_strlen (entry, -1)) > 0)
          {
               end = entry + g_utf8_strlen (entry, -1);
               for (j = entry; j < end; ++j)
                    {
                         switch (*j)
                              {
                              case '0' ... '9':
                              case 'a' ... 'z':
                              case 'A' ... 'Z':
                              case '-':
                              case '_':
                                   break;
                              default:
                                   gdk_beep ();
                                   pos = gtk_editable_get_position (GTK_EDITABLE (widget));
                                   gtk_editable_delete_text (GTK_EDITABLE (widget),
                                                                   pos, pos+1);
                                   break;
                              }
                    }
          }


     /* step 2: if name seems all right, enable OK button */
     text = gtk_entry_get_text (GTK_ENTRY (widget));

     if (g_utf8_strlen (text, -1) > 0) {
          gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                                     GTK_RESPONSE_OK,
                                                     TRUE);
     }
     else {
          gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                                     GTK_RESPONSE_OK,
                                                     FALSE);
     }
}



static void aztype_changed_cb     (GtkComboBox *box, gpointer data)
{
    gint type = gtk_combo_box_get_active (box);
    
    switch (type) {
        
        case ROT_AZ_TYPE_360:
            gtk_spin_button_set_value (GTK_SPIN_BUTTON (minaz), 0.0);
            gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxaz), 360.0);
            break;
            
        case ROT_AZ_TYPE_180:
            gtk_spin_button_set_value (GTK_SPIN_BUTTON (minaz), -180.0);
            gtk_spin_button_set_value (GTK_SPIN_BUTTON (maxaz), +180.0);
            break;
            
        default:
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%s: Invalid AZ rotator type."),
                           __FILE__, __FUNCTION__);
            break;
    }

}
