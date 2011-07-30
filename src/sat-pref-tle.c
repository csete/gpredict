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
#include "tle-update.h"
#include "sat-cfg.h"
#include "sat-pref-tle.h"


/* Update frequency widget */
static GtkWidget *freq;

/* auto update radio buttons */
static GtkWidget *warn,*autom;

/* internet updates */
static GtkWidget *server,*proxy,*files;

/* add new sats */
static GtkWidget *addnew;


/* book keeping */
/* static tle_auto_upd_freq_t auto_freq = TLE_AUTO_UPDATE_NEVER; */
static gboolean dirty = FALSE;  /* used to check whether any changes have occurred */
static gboolean reset = FALSE;



static void create_auto_update  (GtkWidget *vbox);
static void create_network      (GtkWidget *vbox);
static void create_misc         (GtkWidget *vbox);

#if 0
static void create_local        (GtkWidget *vbox);
#endif

static void create_reset_button (GtkBox *vbox);
static void reset_cb            (GtkWidget *button, gpointer data);

static void value_changed_cb    (GtkWidget *widget, gpointer data);


/** \brief Create and initialise widgets for the locations TLE update tab.
 *
 *
 */
GtkWidget *sat_pref_tle_create ()
{

    GtkWidget   *vbox;

    /* vertical box */
    vbox = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);

    /* add contents */
    create_auto_update (vbox);
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 0);
    create_network (vbox);
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, TRUE, 0);
    create_misc (vbox);

#if 0
    create_local (vbox);
#endif

    /* create RESET button */
    create_reset_button (GTK_BOX (vbox));

    return vbox;
}


/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void
sat_pref_tle_cancel ()
{
    dirty = FALSE;
    reset = FALSE;
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void
sat_pref_tle_ok     ()
{
    if (dirty) {
        
        /* save settings */

        /* update frequency */
        sat_cfg_set_int (SAT_CFG_INT_TLE_AUTO_UPD_FREQ,
                            gtk_combo_box_get_active (GTK_COMBO_BOX (freq)));

        /* action to take */
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (autom)))
            sat_cfg_set_int (SAT_CFG_INT_TLE_AUTO_UPD_ACTION, TLE_AUTO_UPDATE_GOAHEAD);
        else
            sat_cfg_set_int (SAT_CFG_INT_TLE_AUTO_UPD_ACTION, TLE_AUTO_UPDATE_NOTIFY);

        /* server */
        sat_cfg_set_str (SAT_CFG_STR_TLE_SERVER,
                            gtk_entry_get_text (GTK_ENTRY (server)));

        /* proxy */
        sat_cfg_set_str (SAT_CFG_STR_TLE_PROXY,
                            gtk_entry_get_text (GTK_ENTRY (proxy)));

        /* files */
        sat_cfg_set_str (SAT_CFG_STR_TLE_FILES,
                            gtk_entry_get_text (GTK_ENTRY (files)));
        
        /* add new sats */
        sat_cfg_set_bool (SAT_CFG_BOOL_TLE_ADD_NEW,
                            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (addnew)));

        dirty = FALSE;
    }
    else if (reset) {

        /* use sat_cfg_reset */
        sat_cfg_reset_int (SAT_CFG_INT_TLE_AUTO_UPD_FREQ);
        sat_cfg_reset_int (SAT_CFG_INT_TLE_AUTO_UPD_ACTION);
        sat_cfg_reset_str (SAT_CFG_STR_TLE_SERVER);
        sat_cfg_reset_str (SAT_CFG_STR_TLE_PROXY);
        sat_cfg_reset_str (SAT_CFG_STR_TLE_FILES);
        sat_cfg_reset_bool (SAT_CFG_BOOL_TLE_ADD_NEW);

        reset = FALSE;
    }
}


/** \brief Create widgets for auto-update configuration */
static void
create_auto_update (GtkWidget *vbox)
{
    GtkWidget   *label;
    GtkWidget   *box;
    guint i;

    /* auto update */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b>Auto-Update:</b>"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

    /* frequency */
    freq = gtk_combo_box_new_text ();
    for (i = 0; i < TLE_AUTO_UPDATE_NUM; i++) {
        gtk_combo_box_append_text (GTK_COMBO_BOX (freq),
                                    tle_update_freq_to_str (i));
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (freq),
                                sat_cfg_get_int (SAT_CFG_INT_TLE_AUTO_UPD_FREQ));
    g_signal_connect (freq, "changed", G_CALLBACK (value_changed_cb), NULL);

    label = gtk_label_new (_("Check the age of TLE data:"));

    box = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), freq, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, TRUE, 0);

    /* radio buttons selecting action */
    label = gtk_label_new (_("If TLEs are too old:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
    warn = gtk_radio_button_new_with_label (NULL,
                                            _("Notify me"));
    gtk_box_pack_start (GTK_BOX (vbox), warn, FALSE, TRUE, 0);
    autom = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (warn),
                                                            _("Perform automatic update in the background"));
    gtk_box_pack_start (GTK_BOX (vbox), autom, FALSE, TRUE, 0);

    /* warn is selected automatically by default */
    if (sat_cfg_get_int (SAT_CFG_INT_TLE_AUTO_UPD_ACTION) == TLE_AUTO_UPDATE_GOAHEAD) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (autom), TRUE);
    }


    g_signal_connect (warn, "toggled", G_CALLBACK (value_changed_cb), NULL);
    g_signal_connect (autom, "toggled", G_CALLBACK (value_changed_cb), NULL);
}




/** \brief Create widgets for network update configuration */
static void
create_network (GtkWidget *vbox)
{
    GtkWidget   *label;
    GtkWidget   *table;

    /* auto update */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b>Update from the Internet:</b>"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

    /* create table */
    table = gtk_table_new (3, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);

    /* server */
    label = gtk_label_new (_("Remote server:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

    server = gtk_entry_new ();
    if (sat_cfg_get_str (SAT_CFG_STR_TLE_SERVER))
        gtk_entry_set_text (GTK_ENTRY (server), sat_cfg_get_str (SAT_CFG_STR_TLE_SERVER));

    gtk_widget_set_tooltip_text(server,_("Enter URL for remote server including directory, i.e.\n" \
                                         "protocol://servername/directory\n" \
                                         "Protocol can be both http and ftp."));

    g_signal_connect (server, "changed", G_CALLBACK (value_changed_cb), NULL);
    gtk_table_attach (GTK_TABLE (table), server, 1, 2, 0, 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    /* proxy */
    label = gtk_label_new (_("Proxy server:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

    proxy = gtk_entry_new ();
    if (sat_cfg_get_str (SAT_CFG_STR_TLE_PROXY))
        gtk_entry_set_text (GTK_ENTRY (proxy), sat_cfg_get_str (SAT_CFG_STR_TLE_PROXY));
    gtk_widget_set_tooltip_text(proxy,
                            _("Enter URL for local proxy server. e.g.\n"\
                            "http://my.proxy.com")); 
    g_signal_connect (proxy, "changed", G_CALLBACK (value_changed_cb), NULL);
    gtk_table_attach (GTK_TABLE (table), proxy, 1, 2, 1, 2,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    /* Files */
    label = gtk_label_new (_("Files to fetch:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

    files = gtk_entry_new ();
    if (sat_cfg_get_str (SAT_CFG_STR_TLE_FILES))
        gtk_entry_set_text (GTK_ENTRY (files), sat_cfg_get_str (SAT_CFG_STR_TLE_FILES));
    gtk_widget_set_tooltip_text(files,
                                _("Enter list of files to fetch from remote server.\n" \
                                  "The files should be separated with ; (semicolon)")); 
    g_signal_connect (files, "changed", G_CALLBACK (value_changed_cb), NULL);
    gtk_table_attach (GTK_TABLE (table), files, 1, 2, 2, 3,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    
    /* put table into vbox */
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
}


/** \brief Create widgets for network update configuration */
static void
create_misc (GtkWidget *vbox)
{
    addnew = gtk_check_button_new_with_label (_("Add new satellites to local database"));
    gtk_widget_set_tooltip_text(addnew,
                                _("Note that new satellites will be added to a group called Other"));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (addnew),
                                  sat_cfg_get_bool (SAT_CFG_BOOL_TLE_ADD_NEW));
    g_signal_connect (addnew, "toggled", G_CALLBACK (value_changed_cb), NULL);
    
    gtk_box_pack_start (GTK_BOX (vbox), addnew, FALSE, TRUE, 0);
    
}


#if 0
/** \brief Create widgets for local update configuration */
static void
create_local (GtkWidget *vbox)
{
    GtkWidget   *label;

    /* auto update */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b>Update from Local Files:</b>"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

}
#endif

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

    gtk_widget_set_tooltip_text(button,
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

    (void) button; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */
    /* get defaults */

    /* update frequency */
    gtk_combo_box_set_active (GTK_COMBO_BOX (freq),
                                sat_cfg_get_int_def (SAT_CFG_INT_TLE_AUTO_UPD_FREQ));

    /* action */
    if (sat_cfg_get_int_def (SAT_CFG_INT_TLE_AUTO_UPD_ACTION) == TLE_AUTO_UPDATE_GOAHEAD) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (autom), TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (warn), TRUE);
    }

    /* server */
    gtk_entry_set_text (GTK_ENTRY (server), sat_cfg_get_str_def (SAT_CFG_STR_TLE_SERVER));

    /* proxy */
    gtk_entry_set_text (GTK_ENTRY (proxy), "");

    /* files */
    gtk_entry_set_text (GTK_ENTRY (files), sat_cfg_get_str_def (SAT_CFG_STR_TLE_FILES));


    /* add new sats */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (addnew),
                                    sat_cfg_get_bool_def (SAT_CFG_BOOL_TLE_ADD_NEW));


    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}



/** \brief Configuration values changed.
 *
 * This function is called when one of the configuration values are
 * modified. It sets the dirty flag to TRUE to indicate that the 
 * configuration values should be stored in case th euser presses
 * the OK button.
 */
static void
value_changed_cb    (GtkWidget *widget, gpointer data)
{
    (void) widget; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */

    dirty = TRUE;
}

