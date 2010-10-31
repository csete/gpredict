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
#include "sat-pref-help.h"




/** \brief OS specific predefined browser strings.
 *
 * Leave public to allow access from other modules, too.
 *
 */
sat_help_t sat_help[BROWSER_TYPE_NUM] = {
     { "None", NULL},
#ifdef G_OS_UNIX
     { "Epiphany", "epiphany-browser --new-window %s"},
     { "Galeon",  "galeon --new-window %s"},
     { "Konqueror", "konqueror %s"},
#endif
     { "Firefox", "firefox -browser %s"},
     { "Mozilla", "mozilla %s"},
     { "Opera", "opera %s"},
#ifdef G_OS_WIN32
     { "Internet Explorer", "iexplorer %s"},
#endif
     { "Other...", NULL}
};


static GtkWidget *combo;
static GtkWidget *entry;

static gboolean dirty;


static void browser_changed_cb (GtkComboBox *cbox, gpointer data);


/** \brief Create and initialise widgets.
 *
 */
GtkWidget *sat_pref_help_create ()
{
     GtkWidget *vbox;    /* vbox containing the list part and the details part */
     GtkWidget *table;
     GtkWidget *label;
     guint      i;
     gint       idx;

     vbox = gtk_vbox_new (FALSE, 10);
     gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);

     /* header */
     label = gtk_label_new (NULL);
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_label_set_markup (GTK_LABEL (label), 
                     _("<b>Html Browser:</b>"));
     gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

     /* table for combo box and command string */
     table = gtk_table_new (2, 2, TRUE);
     gtk_table_set_col_spacings (GTK_TABLE (table), 10);
     gtk_table_set_row_spacings (GTK_TABLE (table), 10);
     gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
     
     /* browser type */
     label = gtk_label_new (_("Browser type:"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

     combo = gtk_combo_box_new_text ();
     for (i = 0; i < BROWSER_TYPE_NUM; i++)
          gtk_combo_box_append_text (GTK_COMBO_BOX (combo), sat_help[i].type);
     gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
     g_signal_connect (combo, "changed", G_CALLBACK (browser_changed_cb), NULL);

     /* command string */
     label = gtk_label_new (_("Command string:"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

     entry = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (entry), 100);
     gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
     
     /* get curreent browser */
     idx = sat_cfg_get_int (SAT_CFG_INT_WEB_BROWSER_TYPE);

     /* some sanity check before accessing the arrays ;-) */
     if ((idx < BROWSER_TYPE_NONE) || (idx >= BROWSER_TYPE_NUM)) {
          idx = BROWSER_TYPE_NONE;
     }

     gtk_combo_box_set_active (GTK_COMBO_BOX (combo), idx);
     if (idx == BROWSER_TYPE_OTHER)
          gtk_entry_set_text (GTK_ENTRY (entry),
                        sat_cfg_get_str (SAT_CFG_STR_WEB_BROWSER));
     

     /** FIXME */
     gtk_widget_set_sensitive (vbox, FALSE);


     dirty = FALSE;

     return vbox;
}


/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void
sat_pref_help_cancel ()
{
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void
sat_pref_help_ok     ()
{
     gint idx;

     if (dirty) {
          /* get and store browser type */
          idx = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
          sat_cfg_set_int (SAT_CFG_INT_WEB_BROWSER_TYPE, idx);

          /* if type is OTHER, store command, otherwise clear command */
          if (idx == BROWSER_TYPE_OTHER) {
               sat_cfg_set_str (SAT_CFG_STR_WEB_BROWSER,
                          gtk_entry_get_text (GTK_ENTRY (entry)));
          }
          else {
               sat_cfg_reset_str (SAT_CFG_STR_WEB_BROWSER);
          }

     }
     dirty = FALSE;
}


static void
browser_changed_cb (GtkComboBox *cbox, gpointer data)
{
     gint idx;

     idx = gtk_combo_box_get_active (cbox);

     if (idx == BROWSER_TYPE_OTHER) {
          gtk_widget_set_sensitive (entry, TRUE);
     }
     else {
          gtk_widget_set_sensitive (entry, FALSE);
          if (idx > BROWSER_TYPE_NONE) {
               gtk_entry_set_text (GTK_ENTRY (entry), sat_help[idx].cmd);
          }
     }

     dirty = TRUE;
}
