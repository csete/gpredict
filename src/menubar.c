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
/** \defgroup menu The menubar
 *  \ingroup gui
 *
 */
/** \defgroup menuif Interface
 *  \ingroup menu
 *
 */
/** \defgroup menupriv Private
 *  \ingroup menu
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sat-pref.h"
#include "mod-cfg.h"
#include "sat-cfg.h"
#include "mod-mgr.h"
#include "sat-log.h"
#include "sat-log-browser.h"
#include "about.h"
#include "gtk-sat-module.h"
#include "gtk-sat-module-popup.h"
#include "gpredict-help.h"
#include "tle-update.h"
#include "compat.h"
#include "menubar.h"
#include "config-keys.h"
//#include "satellite-editor.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif


extern GtkWidget *app;


/* private function prototypes */
static void   menubar_new_mod_cb   (GtkWidget *widget, gpointer data);
static void   menubar_open_mod_cb  (GtkWidget *widget, gpointer data);
static void   menubar_message_log  (GtkWidget *widget, gpointer data);
static void   menubar_app_exit_cb  (GtkWidget *widget, gpointer data);
static void   menubar_freq_edit_cb (GtkWidget *widget, gpointer data);
static void   menubar_pref_cb      (GtkWidget *widget, gpointer data);
static void   menubar_tle_net_cb   (GtkWidget *widget, gpointer data);
static void   menubar_tle_local_cb (GtkWidget *widget, gpointer data);
static void   menubar_tle_manual_cb (GtkWidget *widget, gpointer data);
static void   menubar_window_cb    (GtkWidget *widget, gpointer data);
static void   menubar_predict_cb   (GtkWidget *widget, gpointer data);
static void   menubar_getting_started_cb (GtkWidget *widget, gpointer data);
static void   menubar_help_cb      (GtkWidget *widget, gpointer data);
static void   menubar_license_cb   (GtkWidget *widget, gpointer data);
static void   menubar_news_cb      (GtkWidget *widget, gpointer data);
static void   menubar_about_cb     (GtkWidget *widget, gpointer data);
static gchar *select_module        (void);
static void   create_module_window (GtkWidget *module);
static gint compare_func (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      userdata);

/** \brief Regular menu items.
 *  \ingroup menupriv
 */
static GtkActionEntry entries[] = {
    { "FileMenu", NULL, N_("_File") },
    { "EditMenu", NULL, N_("_Edit") },
    { "TleMenu", GTK_STOCK_REFRESH, N_("_Update TLE") },
    { "ToolsMenu", NULL, N_("_Tools") },
    { "HelpMenu", NULL, N_("_Help") },

    /* File menu */
    { "New", GTK_STOCK_NEW, N_("_New module"), "<control>N",
      N_("Create a new module"), G_CALLBACK (menubar_new_mod_cb) },
    { "Open", GTK_STOCK_OPEN, N_("_Open module"), "<control>O",
      N_("Open an existing module"), G_CALLBACK (menubar_open_mod_cb) },
    { "Log", GTK_STOCK_JUSTIFY_LEFT, "_Log browser", "<control>L",
      N_("Open the message log browser"), G_CALLBACK (menubar_message_log) },
    { "Exit", GTK_STOCK_QUIT, N_("E_xit"), "<control>Q",
      N_("Exit the program"), G_CALLBACK (menubar_app_exit_cb) },

    /* Edit menu */
    /*    { "Tle", GTK_STOCK_REFRESH, N_("Update TLE"), NULL,
        N_("Update Keplerian elements"), NULL},*/
    { "Net", GTK_STOCK_NETWORK, N_("From _network"), NULL,
      N_("Update Keplerian elements from a network server"),
      G_CALLBACK (menubar_tle_net_cb)},
    { "Local", GTK_STOCK_HARDDISK, N_("From l_ocal files"), NULL,
      N_("Update Keplerian elements from local files"),
      G_CALLBACK (menubar_tle_local_cb)},
    { "Man", GTK_STOCK_DND, N_("Using TLE _editor"), NULL,
      N_("Add or update Keplerian elements using the TLE editor"),
      G_CALLBACK (menubar_tle_manual_cb)},
    { "Freq", NULL, N_("_Transponders"), NULL,
      N_("Edit satellite transponder frequencies"),
      G_CALLBACK (menubar_freq_edit_cb)},
    { "Pref", GTK_STOCK_PREFERENCES, N_("_Preferences"), NULL,
      N_("Edit user preferences"), G_CALLBACK (menubar_pref_cb)},

    /* Tools menu */
    { "SatLab", NULL, N_("Satellite Editor"), NULL,
      N_("Open the satellite editor where you can manually edit orbital elements and other data"),
      G_CALLBACK (menubar_tle_manual_cb)},
    { "Window", NULL, N_("Comm Window"), NULL,
      N_("Predict windows between two observers"),
      G_CALLBACK (menubar_window_cb)},
    { "Predict", GTK_STOCK_DND_MULTIPLE, N_("Advanced Predict"), NULL,
      N_("Open advanced pass predictor"), G_CALLBACK (menubar_predict_cb)},

    /* Help menu */
    { "GettingStarted", GTK_STOCK_EXECUTE, N_("Getting Started"), NULL,
      N_("Show online user manual, Getting Started Section"),
      G_CALLBACK (menubar_getting_started_cb)},
    { "Help", GTK_STOCK_HELP, N_("Online help"), "F1",
      N_("Show online user manual"), G_CALLBACK (menubar_help_cb)},
    { "License", NULL, N_("_License"), NULL,
      N_("Show the Gpredict license"), G_CALLBACK (menubar_license_cb) },
    { "News", NULL, N_("_News"), NULL,
      N_("Show what's new in this release"), G_CALLBACK (menubar_news_cb) },
    { "About", GTK_STOCK_ABOUT, N_("_About Gpredict"), NULL,
      N_("Show about dialog"), G_CALLBACK (menubar_about_cb) },
};


/** \brief Menubar UI description.
 *  \ingroup menupriv
 */
static const char *menu_desc =
"<ui>"
"   <menubar name='GpredictMenu'>"
"      <menu action='FileMenu'>"
"         <menuitem action='New'/>"
"         <menuitem action='Open'/>"
"         <separator/>"
"         <menuitem action='Log'/>"
"         <separator/>"
"         <menuitem action='Exit'/>"
"      </menu>"
"      <menu action='EditMenu'>"
"         <menu action='TleMenu'>"
"            <menuitem action='Net'/>"
"            <menuitem action='Local'/>"
/*"            <menuitem action='Man'/>"*/
"         </menu>"
/* "         <menuitem action='Freq'/>" */
"         <separator/>"
"         <menuitem action='Pref'/>"
"      </menu>"
/*"      <menu action='ToolsMenu'>"
"         <menuitem action='SatLab'/>"
"         <separator/>"
"         <menuitem action='Window'/>"
"         <menuitem action='Predict'/>"
"      </menu>"*/
"      <menu action='HelpMenu'>"
/* "         <menuitem action='GettingStarted'/>" */
"         <menuitem action='Help'/>"
"         <separator/>"
"         <menuitem action='License'/>"
"         <menuitem action='News'/>"
"         <separator/>"
"         <menuitem action='About'/>"
"      </menu>"
"   </menubar>"
"</ui>";


/** \brief Create menubar.
 *  \param window The main application window.
 *  \return The menubar widget.
 *
 * This function creates and initializes the main menubar for gpredict.
 * It should be called from the main gui_create function.
 */
GtkWidget *
menubar_create (GtkWidget *window)
{
    GtkWidget      *menubar;
    GtkActionGroup *actgrp;
    GtkUIManager   *uimgr;
    GtkAccelGroup  *accgrp;
    GError         *error = NULL;
    //GtkWidget      *menuitem;
    //GtkWidget      *image;
    //gchar          *icon;
    gint           i;


    /* create action group */
    actgrp = gtk_action_group_new ("MenuActions");
    /* i18n */
       for (i=0; i<G_N_ELEMENTS (entries); i++) {
        if (entries[i].label)
            entries[i].label = _(entries[i].label);
        if (entries[i].tooltip)
            entries[i].tooltip = _(entries[i].tooltip);
    }

    gtk_action_group_add_actions (actgrp, entries, G_N_ELEMENTS (entries), NULL);

    /* create UI manager */
    uimgr = gtk_ui_manager_new ();
    gtk_ui_manager_insert_action_group (uimgr, actgrp, 0);

    /* accelerator group */
    accgrp = gtk_ui_manager_get_accel_group (uimgr);
    gtk_window_add_accel_group (GTK_WINDOW (window), accgrp);

    /* try to create UI from XML*/
    if (!gtk_ui_manager_add_ui_from_string (uimgr, menu_desc, -1, &error)) {
        g_print (_("Failed to build menubar: %s"), error->message);
        g_error_free (error);

        return NULL;
    }

    /* load custom icons */
/*    icon = icon_file_name ("gpredict-shuttle-small.png");
    image = gtk_image_new_from_file (icon);
    g_free (icon);
    menuitem = gtk_ui_manager_get_widget (uimgr, "/GpredictMenu/ToolsMenu/SatLab");
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);*/

    /* now, finally, get the menubar */
    menubar = gtk_ui_manager_get_widget (uimgr, "/GpredictMenu");

    return menubar;
}


/** \brief Create new module.
 *
 * This function first executes the mod-cfg editor. If the editor returns
 * the name of an existing .mod file it will create the corresponding module
 * and send it to the module manager.
 */
static void
menubar_new_mod_cb  (GtkWidget *widget, gpointer data)
{
    gchar *modnam = NULL;
    gchar *modfile;
    gchar *confdir;
    GtkWidget *module = NULL;


    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                 _("%s: Starting new module configurator..."),
                 __FUNCTION__);

    modnam = mod_cfg_new ();

    if (modnam) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: New module name is %s."),
                     __FUNCTION__, modnam);

        confdir = get_modules_dir ();
        modfile = g_strconcat (confdir, G_DIR_SEPARATOR_S,
                               modnam, ".mod", NULL);
        g_free (confdir);

        /* create new module */
        module = gtk_sat_module_new (modfile);

        if (module == NULL) {

            GtkWidget *dialog;

            dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                             GTK_DIALOG_MODAL |
                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_OK,
                                             _("Could not open %s. Please examine "\
                                               "the log messages for details."),
                                             modnam);

            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
        }
        else {
            mod_mgr_add_module (module, TRUE);
        }

        g_free (modnam);
        g_free (modfile);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: New module config cancelled."),
                     __FUNCTION__);
    }
}


static void
menubar_open_mod_cb  (GtkWidget *widget, gpointer data)
{
    gchar *modnam = NULL;
    gchar *modfile;
    gchar *confdir;
    GtkWidget *module = NULL;


    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                 _("%s: Open existing module..."),
                 __FUNCTION__);
    
    modnam = select_module ();

    if (modnam) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: Open module %s."),
                     __FUNCTION__, modnam);

        confdir = get_modules_dir ();
        modfile = g_strconcat (confdir, G_DIR_SEPARATOR_S,
                               modnam, ".mod", NULL);
        g_free (confdir);

        /* create new module */
        module = gtk_sat_module_new (modfile);

        if (module == NULL) {
            /* mod manager could not create the module */
            GtkWidget *dialog;

            dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                             GTK_DIALOG_MODAL |
                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_OK,
                                             _("Could not open %s. Please examine "\
                                               "the log messages for details."),
                                             modnam);

            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
        }
        else {

            /* if module state was window or user does not want to restore the
               state of the modules, pack the module into the notebook */
            if ((GTK_SAT_MODULE (module)->state == GTK_SAT_MOD_STATE_DOCKED) ||
                !sat_cfg_get_bool (SAT_CFG_BOOL_MOD_STATE)) {
                
                mod_mgr_add_module (module, TRUE);
                
            }
            else {
                mod_mgr_add_module (module, FALSE);
                create_module_window (module);
            }
        }

        g_free (modnam);
        g_free (modfile);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: Open module cancelled."),
                     __FUNCTION__);
    }
}


static void
menubar_message_log  (GtkWidget *widget, gpointer data)
{
    sat_log_browser_open ();
}


static void
menubar_app_exit_cb  (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy (app);
}


static void
menubar_freq_edit_cb (GtkWidget *widget, gpointer data)
{
}


static void
menubar_pref_cb      (GtkWidget *widget, gpointer data)
{
    sat_pref_run ();
}



/** \brief Update TLE from network.
 *  \param widget The menu item (unused).
 *  \param data User data (unused).
 *
 * This function is called when the user selects
 *      Edit -> Update TLE -> Update from network
 * in the menubar.
 * The function calls tle_update_from_network with silent flag FALSE.
 */
static void
menubar_tle_net_cb       (GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;          /* dialog window  */
    GtkWidget *label;           /* misc labels */
    GtkWidget *progress;        /* progress indicator */
    GtkWidget *label1,*label2;  /* activitity and stats labels */
    GtkWidget *box;


    
    /* create new dialog with progress indicator */
    dialog = gtk_dialog_new_with_buttons (_("TLE Update"),
                                          GTK_WINDOW (app),
                                          GTK_DIALOG_MODAL | 
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CLOSE,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    //gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 300);
    gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                       GTK_RESPONSE_ACCEPT,
                                       FALSE);


    /* create a vbox */
    box = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box), 20);
        
    /* add static label */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),
                          _("<b>Updating TLE files from network</b>"));
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

    /* activity label */
    label1 = gtk_label_new ("...");
    gtk_misc_set_alignment (GTK_MISC (label1), 0.5, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label1, FALSE, FALSE, 0);
                                  
    /* add progress bar */
    progress = gtk_progress_bar_new ();
    gtk_box_pack_start (GTK_BOX (box), progress, FALSE, FALSE, 10);

    /* statistics */
    label2 = gtk_label_new (_("Satellites updated:\t 0\n"\
                              "Satellites skipped:\t 0\n"\
                              "Missing Satellites:\t 0\n"));
    gtk_box_pack_start (GTK_BOX (box), label2, TRUE, TRUE, 0);        

    /* finalise dialog */
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), box);
    g_signal_connect_swapped (dialog,
                              "response", 
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);

    gtk_widget_show_all (dialog);

    /* Force the drawing queue to be processed otherwise the dialog
       may not appear before we enter the TLE updating func
       - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
    */
    while (g_main_context_iteration (NULL, FALSE));

    /* update TLE */
    tle_update_from_network (FALSE, progress, label1, label2);

    /* set progress bar to 100% */
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 1.0);

    gtk_label_set_text (GTK_LABEL (label1), _("Finished"));

    /* enable close button */
    gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                       GTK_RESPONSE_ACCEPT,
                                       TRUE);

    /* reload satellites */
    mod_mgr_reload_sats ();
}



/** \brief Update TLE from local files.
 *  \param widget The menu item (unused).
 *  \param data User data (unused).
 *
 * This function is called when the user selects
 *      Edit -> Update TLE -> From local files
 * in the menubar.
 *
 * First the function creates the GUI status indicator infrastructure
 * with the possibility to select a directory, then it calls the
 * tle_update_from_files with the corresponding parameters.
 *
 * Finally, the programs signals the module manager to reload the
 * satellites in each module.
 *
 * FIXME: fork as a thread?
 */
static void
menubar_tle_local_cb       (GtkWidget *widget, gpointer data)
{
    gchar     *dir;          /* selected directory */
    GtkWidget *dir_chooser;  /* directory chooser button */
    GtkWidget *dialog;       /* dialog window  */
    GtkWidget *label;        /* misc labels */
    GtkWidget *progress;     /* progress indicator */
    GtkWidget *label1,*label2;  /* activitity and stats labels */
    GtkWidget *box;
    gint       response;     /* dialog response */
    gboolean   doupdate = FALSE;


    /* get last used directory */
    dir = sat_cfg_get_str (SAT_CFG_STR_TLE_FILE_DIR);

    /* if there is no last used dir fall back to $HOME */
    if (dir == NULL) {
        dir = g_strdup (g_get_home_dir ());
    }

    /* create file chooser */
    dir_chooser = gtk_file_chooser_button_new (_("Select directory"),
                                               GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dir_chooser), dir);
    g_free (dir);

    /* create label */
    label = gtk_label_new (_("Select TLE directory:"));

    /* pack label and chooser into a hbox */
    box = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 5);
    gtk_box_pack_start (GTK_BOX (box), dir_chooser, TRUE, TRUE, 5);
    gtk_widget_show_all (box);

    /* create the dalog */
    dialog = gtk_dialog_new_with_buttons (_("Update TLE from files"),
                                          GTK_WINDOW (app),
                                          GTK_DIALOG_MODAL | 
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_REJECT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), box, TRUE, TRUE, 30);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    switch (response) {

    case GTK_RESPONSE_ACCEPT:
        /* set flag to indicate that we should do an update */
        doupdate = TRUE;
        break;

    default:
        doupdate = FALSE;
        break;
    }


    /* get directory before we destroy the dialog */
    dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dir_chooser));

    /* nuke the dialog */
    gtk_widget_destroy (dialog);

    if (doupdate) {

        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("%s: Running TLE update from %s"),
                     __FUNCTION__, dir);

        /* store last used TLE dir */
        sat_cfg_set_str (SAT_CFG_STR_TLE_FILE_DIR, dir);

        /* create new dialog with progress indicator */
        dialog = gtk_dialog_new_with_buttons (_("TLE Update"),
                                              GTK_WINDOW (app),
                                              GTK_DIALOG_MODAL | 
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CLOSE,
                                              GTK_RESPONSE_ACCEPT,
                                              NULL);
        //gtk_window_set_default_size (GTK_WINDOW (dialog), 400,250);
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           FALSE);

        /* create a vbox */
        box = gtk_vbox_new (FALSE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (box), 20);
        
        /* add static label */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
        gtk_label_set_markup (GTK_LABEL (label),
                              _("<b>Updating TLE files from files</b>"));
        gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

        /* activity label */
        label1 = gtk_label_new ("...");
        gtk_misc_set_alignment (GTK_MISC (label1), 0.5, 0.5);
        gtk_box_pack_start (GTK_BOX (box), label1, FALSE, FALSE, 0);
        
                                      
        /* add progress bar */
        progress = gtk_progress_bar_new ();
        gtk_box_pack_start (GTK_BOX (box), progress, FALSE, FALSE, 10);

        /* statistics */
        label2 = gtk_label_new (_("Satellites updated:\t 0\n"\
                                  "Satellites skipped:\t 0\n"\
                                  "Missing Satellites:\t 0\n"));
        gtk_box_pack_start (GTK_BOX (box), label2, TRUE, TRUE, 0);        

        /* finalise dialog */
        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), box);
        g_signal_connect_swapped (dialog,
                                  "response", 
                                  G_CALLBACK (gtk_widget_destroy),
                                  dialog);

        gtk_widget_show_all (dialog);

        /* Force the drawing queue to be processed otherwise the dialog
           may not appear before we enter the TLE updating func
           - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
        */
        while (g_main_context_iteration (NULL, FALSE));

        /* update TLE */
        tle_update_from_files (dir, NULL, FALSE, progress, label1, label2);

        /* set progress bar to 100% */
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 1.0);

        gtk_label_set_text (GTK_LABEL (label1), _("Finished"));

        /* enable close button */
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           TRUE);

    }

    if (dir)
        g_free (dir);

    /* reload satellites */
    mod_mgr_reload_sats ();
}


/** \brief Start Manual TLE editor. */
static void
menubar_tle_manual_cb       (GtkWidget *widget, gpointer data)
{
    //satellite_editor_run ();
}





static void
menubar_window_cb (GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;


    dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_OK,
                                     _("This function is still under development."));

    /* Destroy the dialog when the user responds to it (e.g. clicks a button) */
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);

    gtk_widget_show_all (dialog);
}


static void
menubar_predict_cb   (GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;


    dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_OK,
                                     _("This function is still under development."));

    /* Destroy the dialog when the user responds to it (e.g. clicks a button) */
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);

    gtk_widget_show_all (dialog);
}


static void
menubar_getting_started_cb (GtkWidget *widget, gpointer data)
{
    gpredict_help_launch (GPREDICT_HELP_GETTING_STARTED);
}

static void
menubar_help_cb (GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *button;

    dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_CLOSE,
                                     _("A comprehensive PDF user manual and \n"\
                                       "video tutorials are available from the \n"\
                                       "Gpredict website:"));

    button = gtk_link_button_new ("http://gpredict.oz9aec.net/documents.php");
    gtk_widget_show (button);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), button, FALSE, FALSE, 0);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

}


static void
menubar_license_cb (GtkWidget *widget, gpointer data)
{
    gpredict_help_show_txt ("COPYING");
}


static void
menubar_news_cb (GtkWidget *widget, gpointer data)
{
    gpredict_help_show_txt ("NEWS");
}


static void
menubar_about_cb (GtkWidget *widget, gpointer data)
{
    about_dialog_create ();
}



/** \brief Select an existing module.
 *
 * This function creates a dialog with a list of existing modules
 * from /homedir/.config/Gpredict/modules/ and lets the user select one
 * of them. The function will return the name of the selected module
 * without the .mod suffix.
 */
static gchar *
select_module        ()
{
    GtkWidget         *dialog;     /* the dialog window */
    GtkWidget         *modlist;    /* the treeview widget */
    GtkListStore      *liststore;  /* the list store data structure */
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter        item;       /* new item added to the list store */
    GtkTreeSelection  *selection;
    GtkTreeModel      *selmod;
     GtkTreeModel      *listtreemodel;
     GtkWidget         *swin;
    GDir              *dir = NULL;   /* directory handle */
    GError            *error = NULL; /* error flag and info */
    gchar             *dirname;      /* directory name */
    const gchar       *filename;     /* file name */
    gchar            **buffv;
    guint              count = 0;

    /* create and fill data model */
    liststore = gtk_list_store_new (1, G_TYPE_STRING);

    /* scan for .mod files in the user config directory and
       add the contents of each .mod file to the list store
    */
    dirname = get_modules_dir ();
    dir = g_dir_open (dirname, 0, &error);

    if (dir) {

        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%s: Scanning directory %s for modules."),
                     __FILE__, __FUNCTION__, dirname);

        while ((filename = g_dir_read_name (dir))) {

            if (g_str_has_suffix (filename, ".mod")) {

                /* strip extension and add to list */
                buffv = g_strsplit (filename, ".mod", 0);

                gtk_list_store_append (liststore, &item);
                gtk_list_store_set (liststore, &item,
                                    0, buffv[0],
                                    -1);

                g_strfreev (buffv);

                count++;
            }
        }
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to open module dir %s (%s)"),
                     __FILE__, __LINE__, dirname, error->message);
        g_clear_error (&error);
    }

    g_free (dirname);
    g_dir_close (dir);

    if (count < 1) {
        /* tell user that there are no modules, try "New" instead */
        dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_INFO,
                                         GTK_BUTTONS_OK,
                                         _("You do not have any modules "\
                                           "set up yet. Please use File->New "\
                                           "in order to create a module."));

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        return NULL;
    }

    /* create tree view */
    modlist = gtk_tree_view_new ();
     listtreemodel=GTK_TREE_MODEL (liststore);
     gtk_tree_view_set_model (GTK_TREE_VIEW (modlist), listtreemodel);
    g_object_unref (liststore);

     swin = gtk_scrolled_window_new(NULL,NULL);
     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
     /* sort the tree by name */
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (listtreemodel),
                                     0,
                                     compare_func,
                                     NULL,
                                     NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (listtreemodel),
                                          0,
                                          GTK_SORT_ASCENDING);


    /*** FIXME: Add g_stat info? */

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Module"), renderer,
                                                       "text", 0,
                                                       NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (modlist), column, -1);

    gtk_widget_show (modlist);

    /* create dialog */
    dialog = gtk_dialog_new_with_buttons (_("Select a module"),
                                          GTK_WINDOW (app),
                                          GTK_DIALOG_MODAL |
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          NULL);

    gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 200);
     gtk_container_add (GTK_CONTAINER (swin), modlist);
    gtk_widget_show (swin);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), swin);

    switch (gtk_dialog_run (GTK_DIALOG (dialog))) {

        /* user pressed OK */
    case GTK_RESPONSE_OK:

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (modlist));

        if (gtk_tree_selection_get_selected (selection, &selmod, &item)) {
            gtk_tree_model_get (selmod, &item,
                                0, &dirname,
                                -1);

            sat_log_log (SAT_LOG_LEVEL_DEBUG,
                         _("%s:%s: Selected module is: %s"),
                         __FILE__, __FUNCTION__, dirname);
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%s: No selection is list of modules."),
                         __FILE__, __FUNCTION__);

            dirname = NULL;
        }

        break;

        /* everything else is regarded as CANCEL */
    default:
        dirname = NULL;
        break;
    }

    gtk_widget_destroy (dialog);
    

    return dirname;
}



/** \brief Create a module window.
 *
 * This function is used to create a module window when opening modules
 * that should not be packed into the notebook.
 */
static void
create_module_window (GtkWidget *module)
{
    gint       w,h;
    gchar     *icon;      /* icon file name */
    gchar     *title;     /* window title */


    /* get stored size; use size from main window if size not explicitly stoed */
    if (g_key_file_has_key (GTK_SAT_MODULE (module)->cfgdata,
                            MOD_CFG_GLOBAL_SECTION,
                            MOD_CFG_WIN_WIDTH,
                            NULL)) {
        w = g_key_file_get_integer (GTK_SAT_MODULE (module)->cfgdata,
                                    MOD_CFG_GLOBAL_SECTION,
                                    MOD_CFG_WIN_WIDTH,
                                    NULL);
    }
    else {
        w = module->allocation.width;
    }
    if (g_key_file_has_key (GTK_SAT_MODULE (module)->cfgdata,
                            MOD_CFG_GLOBAL_SECTION,
                            MOD_CFG_WIN_HEIGHT,
                            NULL)) {
        h = g_key_file_get_integer (GTK_SAT_MODULE (module)->cfgdata,
                                    MOD_CFG_GLOBAL_SECTION,
                                    MOD_CFG_WIN_HEIGHT,
                                    NULL);
    }
    else {
        h = module->allocation.height;
    }
        
    /* increase reference count of module */
    //g_object_ref (module);

    /* we don't need the positions */
    //GTK_SAT_MODULE (module)->vpanedpos = -1;
    //GTK_SAT_MODULE (module)->hpanedpos = -1;

    /* undock from mod-mgr */
    //mod_mgr_undock_module (module);

    /* create window */
    GTK_SAT_MODULE (module)->win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    title = g_strconcat ("GPREDICT: ",
                         GTK_SAT_MODULE (module)->name,
                         " (", GTK_SAT_MODULE (module)->qth->name, ")",
                         NULL);
    gtk_window_set_title (GTK_WINDOW (GTK_SAT_MODULE (module)->win), title);
    g_free (title);
    gtk_window_set_default_size (GTK_WINDOW (GTK_SAT_MODULE (module)->win), w, h);
    g_signal_connect (G_OBJECT (GTK_SAT_MODULE (module)->win), "configure_event",
                      G_CALLBACK (module_window_config_cb), module);

    /* window icon */
    icon = icon_file_name ("gpredict-icon.png");
    if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
        gtk_window_set_icon_from_file (GTK_WINDOW (GTK_SAT_MODULE (module)->win), icon, NULL);
    }
    g_free (icon);

    /* move window to stored position if requested by configuration */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_MOD_WIN_POS) &&
        g_key_file_has_key (GTK_SAT_MODULE (module)->cfgdata,
                            MOD_CFG_GLOBAL_SECTION,
                            MOD_CFG_WIN_POS_X,
                            NULL) &&
        g_key_file_has_key (GTK_SAT_MODULE (module)->cfgdata,
                            MOD_CFG_GLOBAL_SECTION,
                            MOD_CFG_WIN_POS_Y,
                            NULL)) {
                
        gtk_window_move (GTK_WINDOW (GTK_SAT_MODULE (module)->win),
                         g_key_file_get_integer (GTK_SAT_MODULE (module)->cfgdata,
                                                 MOD_CFG_GLOBAL_SECTION,
                                                 MOD_CFG_WIN_POS_X, NULL),
                         g_key_file_get_integer (GTK_SAT_MODULE (module)->cfgdata,
                                                 MOD_CFG_GLOBAL_SECTION,
                                                 MOD_CFG_WIN_POS_Y,
                                                 NULL));

    }

    /* add module to window */
    gtk_container_add (GTK_CONTAINER (GTK_SAT_MODULE (module)->win), module);

    /* show window */
    gtk_widget_show_all (GTK_SAT_MODULE (module)->win);
        
    /* reparent time manager window if visible */
    if (GTK_SAT_MODULE (module)->tmgActive) {
        gtk_window_set_transient_for (GTK_WINDOW (GTK_SAT_MODULE (module)->tmgWin),
                                      GTK_WINDOW (GTK_SAT_MODULE (module)->win));
    }


}

static gint compare_func (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      userdata)
{
    gchar *sat1,*sat2;
    gint ret = 0;


    gtk_tree_model_get(model, a, 0, &sat1, -1);
    gtk_tree_model_get(model, b, 0, &sat2, -1);

    ret = g_ascii_strcasecmp (sat1, sat2);

    g_free (sat1);
    g_free (sat2);

    return ret;
}
