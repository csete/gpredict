/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.
 
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

#include "about.h"
#include "compat.h"
#include "config-keys.h"
#include "gpredict-help.h"
#include "gpredict-utils.h"
#include "gtk-sat-module.h"
#include "gtk-sat-module-popup.h"
#include "menubar.h"
#include "mod-cfg.h"
#include "mod-mgr.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-log-browser.h"
#include "sat-pref.h"
#include "tle-update.h"
#include "trsp-update.h"


extern GtkWidget *app;

/* Create new module */
static void menubar_new_mod_cb(GtkWidget * widget, gpointer data)
{
    gchar          *modnam = NULL;
    gchar          *modfile;
    gchar          *confdir;
    GtkWidget      *module = NULL;

    (void)widget;
    (void)data;

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Starting new module configurator..."), __func__);

    modnam = mod_cfg_new();

    if (modnam)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: New module name is %s."),
                    __func__, modnam);

        confdir = get_modules_dir();
        modfile =
            g_strconcat(confdir, G_DIR_SEPARATOR_S, modnam, ".mod", NULL);
        g_free(confdir);

        /* create new module */
        module = gtk_sat_module_new(modfile);

        if (module == NULL)
        {
            GtkWidget      *dialog;

            dialog = gtk_message_dialog_new(GTK_WINDOW(app),
                                            GTK_DIALOG_MODAL |
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Could not open %s. "
                                              "Please examine the log messages "
                                              "for details."), modnam);

            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
        else
        {
            mod_mgr_add_module(module, TRUE);
        }

        g_free(modnam);
        g_free(modfile);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: New module config cancelled."),
                    __func__);
    }
}

/*
 * Create a module window.
 *
 * This function is used to create a module window when opening modules
 * that should not be packed into the notebook.
 */
static void create_module_window(GtkWidget * module)
{
    gint            w, h;
    gchar          *icon;       /* icon file name */
    gchar          *title;      /* window title */
    GtkAllocation   aloc;

    gtk_widget_get_allocation(module, &aloc);
    /* get stored size; use size from main window if size not explicitly stoed */
    if (g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                           MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_WIDTH, NULL))
    {
        w = g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                   MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_WIDTH,
                                   NULL);
    }
    else
    {
        w = aloc.width;
    }
    if (g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                           MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_HEIGHT, NULL))
    {
        h = g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                   MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_HEIGHT,
                                   NULL);
    }
    else
    {
        h = aloc.height;
    }

    /* create window */
    GTK_SAT_MODULE(module)->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    title = g_strconcat(_("GPREDICT: "),
                        GTK_SAT_MODULE(module)->name,
                        " (", GTK_SAT_MODULE(module)->qth->name, ")", NULL);
    gtk_window_set_title(GTK_WINDOW(GTK_SAT_MODULE(module)->win), title);
    g_free(title);
    gtk_window_set_default_size(GTK_WINDOW(GTK_SAT_MODULE(module)->win), w, h);
    g_signal_connect(G_OBJECT(GTK_SAT_MODULE(module)->win), "configure_event",
                     G_CALLBACK(module_window_config_cb), module);

    /* window icon */
    icon = icon_file_name("gpredict-icon.png");
    if (g_file_test(icon, G_FILE_TEST_EXISTS))
    {
        gtk_window_set_icon_from_file(GTK_WINDOW(GTK_SAT_MODULE(module)->win),
                                      icon, NULL);
    }
    g_free(icon);

    /* move window to stored position if requested by configuration */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_MOD_WIN_POS) &&
        g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                           MOD_CFG_GLOBAL_SECTION,
                           MOD_CFG_WIN_POS_X,
                           NULL) &&
        g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                           MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_POS_Y, NULL))
    {

        gtk_window_move(GTK_WINDOW(GTK_SAT_MODULE(module)->win),
                        g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                               MOD_CFG_GLOBAL_SECTION,
                                               MOD_CFG_WIN_POS_X, NULL),
                        g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                               MOD_CFG_GLOBAL_SECTION,
                                               MOD_CFG_WIN_POS_Y, NULL));

    }

    /* add module to window */
    gtk_container_add(GTK_CONTAINER(GTK_SAT_MODULE(module)->win), module);

    /* show window */
    gtk_widget_show_all(GTK_SAT_MODULE(module)->win);

    /* reparent time manager window if visible */
    if (GTK_SAT_MODULE(module)->tmgActive)
    {
        gtk_window_set_transient_for(GTK_WINDOW
                                     (GTK_SAT_MODULE(module)->tmgWin),
                                     GTK_WINDOW(GTK_SAT_MODULE(module)->win));
    }
}

/*
 * Manage row activated (double click) event in the module selector window.
 * 
 * This event handler is triggered when the user double clicks on a row in the
 * "Open module" dialog window. This function will simply emit GTK_RESPONSE_OK
 * which will be interpreted as if th euser clicked on the OK button.
 */
static void select_module_row_activated_cb(GtkTreeView * tree_view,
                                           GtkTreePath * path,
                                           GtkTreeViewColumn * column,
                                           gpointer data)
{
    GtkDialog      *dialog = GTK_DIALOG(data);

    (void)tree_view;
    (void)path;
    (void)column;

    gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

static gint compare_func(GtkTreeModel * model, GtkTreeIter * a,
                         GtkTreeIter * b, gpointer userdata)
{
    gchar          *sat1, *sat2;
    gint            ret = 0;

    (void)userdata;             /* avoid unused parameter compiler warning */

    gtk_tree_model_get(model, a, 0, &sat1, -1);
    gtk_tree_model_get(model, b, 0, &sat2, -1);

    ret = gpredict_strcmp(sat1, sat2);

    g_free(sat1);
    g_free(sat2);

    return ret;
}

static gchar   *select_module()
{
    GtkWidget      *dialog;     /* the dialog window */
    GtkWidget      *modlist;    /* the treeview widget */
    GtkListStore   *liststore;  /* the list store data structure */
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter     item;       /* new item added to the list store */
    GtkTreeSelection *selection;
    GtkTreeModel   *selmod;
    GtkTreeModel   *listtreemodel;
    GtkWidget      *swin;
    GDir           *dir = NULL; /* directory handle */
    GError         *error = NULL;       /* error flag and info */
    gchar          *dirname;    /* directory name */
    const gchar    *filename;   /* file name */
    gchar         **buffv;
    guint           count = 0;

    /* create and fill data model */
    liststore = gtk_list_store_new(1, G_TYPE_STRING);

    /* scan for .mod files in the user config directory and
       add the contents of each .mod file to the list store
     */
    dirname = get_modules_dir();
    dir = g_dir_open(dirname, 0, &error);

    if (dir)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%s: Scanning directory %s for modules."),
                    __FILE__, __func__, dirname);

        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".mod"))
            {
                /* strip extension and add to list */
                buffv = g_strsplit(filename, ".mod", 0);

                gtk_list_store_append(liststore, &item);
                gtk_list_store_set(liststore, &item, 0, buffv[0], -1);

                g_strfreev(buffv);

                count++;
            }
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open module dir %s (%s)"),
                    __FILE__, __LINE__, dirname, error->message);
        g_clear_error(&error);
    }

    g_free(dirname);
    g_dir_close(dir);

    if (count < 1)
    {
        /* tell user that there are no modules, try "New" instead */
        dialog = gtk_message_dialog_new(GTK_WINDOW(app),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_INFO,
                                        GTK_BUTTONS_OK,
                                        _("You do not have any modules "
                                          "set up yet. Please use File->New "
                                          "in order to create a module."));

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return NULL;
    }

    /* create tree view */
    modlist = gtk_tree_view_new();
    listtreemodel = GTK_TREE_MODEL(liststore);
    gtk_tree_view_set_model(GTK_TREE_VIEW(modlist), listtreemodel);
    g_object_unref(liststore);
    /* connecting row activated signal postponed so that we can attach dialog window */

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    /* sort the tree by name */
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(listtreemodel), 0,
                                    compare_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(listtreemodel),
                                         0, GTK_SORT_ASCENDING);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Module"), renderer,
                                                      "text", 0, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(modlist), column, -1);
    gtk_widget_show(modlist);

    /* create dialog */
    dialog = gtk_dialog_new_with_buttons(_("Select a module"),
                                         GTK_WINDOW(app),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_OK", GTK_RESPONSE_OK, NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 200);
    gtk_container_add(GTK_CONTAINER(swin), modlist);
    gtk_widget_show(swin);
    gtk_box_pack_start(GTK_BOX
                       (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), swin,
                       TRUE, TRUE, 0);

    /* double clicking in list will open clicked module */
    g_signal_connect(modlist, "row-activated",
                     G_CALLBACK(select_module_row_activated_cb), dialog);

    switch (gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        /* user pressed OK */
    case GTK_RESPONSE_OK:
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(modlist));

        if (gtk_tree_selection_get_selected(selection, &selmod, &item))
        {
            gtk_tree_model_get(selmod, &item, 0, &dirname, -1);
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s:%s: Selected module is: %s"),
                        __FILE__, __func__, dirname);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%s: No selection is list of modules."),
                        __FILE__, __func__);
            dirname = NULL;
        }
        break;

        /* everything else is regarded as CANCEL */
    default:
        dirname = NULL;
        break;
    }

    gtk_widget_destroy(dialog);

    return dirname;
}

static void menubar_open_mod_cb(GtkWidget * widget, gpointer data)
{
    gchar          *modnam = NULL;
    gchar          *modfile;
    gchar          *confdir;
    GtkWidget      *module = NULL;

    (void)widget;
    (void)data;

    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Open existing module..."),
                __func__);

    modnam = select_module();

    if (modnam)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Open module %s."), __func__,
                    modnam);

        confdir = get_modules_dir();
        modfile =
            g_strconcat(confdir, G_DIR_SEPARATOR_S, modnam, ".mod", NULL);
        g_free(confdir);

        /* create new module */
        module = gtk_sat_module_new(modfile);

        if (module == NULL)
        {
            /* mod manager could not create the module */
            GtkWidget      *dialog;

            dialog = gtk_message_dialog_new(GTK_WINDOW(app),
                                            GTK_DIALOG_MODAL |
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Could not open %s. "
                                              "Please examine the log "
                                              "messages for details."),
                                            modnam);

            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
        else
        {
            /* if module state was window or user does not want to restore the
               state of the modules, pack the module into the notebook */
            if ((GTK_SAT_MODULE(module)->state == GTK_SAT_MOD_STATE_DOCKED) ||
                !sat_cfg_get_bool(SAT_CFG_BOOL_MOD_STATE))
            {
                mod_mgr_add_module(module, TRUE);
            }
            else
            {
                mod_mgr_add_module(module, FALSE);
                create_module_window(module);
            }
        }

        g_free(modnam);
        g_free(modfile);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Open module cancelled."),
                    __func__);
    }
}

static void menubar_message_log(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    sat_log_browser_open();
}

static void menubar_app_exit_cb(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    gtk_widget_destroy(app);
}

static void menubar_pref_cb(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    sat_pref_run();
}

/* Update Frequency information from Network */
static void menubar_trsp_net_cb(GtkWidget * widget, gpointer data)
{

    GtkWidget      *dialog;
    GtkWidget      *label;
    GtkWidget      *progress;
    GtkWidget      *label1, *label2;
    GtkWidget      *box;

    (void)widget;
    (void)data;

    /* create new dialog with progress indicator */
    dialog = gtk_dialog_new_with_buttons(_("Transponder Update"),
                                         GTK_WINDOW(app),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Close", GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT,
                                      FALSE);
    /* create a vbox */
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(box), 20);

    /* add static label */
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.5, "yalign", 0.5, NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         _("<b>Updating transponder files from network</b>"));
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    /* activity label */
    label1 = gtk_label_new("...");
    g_object_set(label1, "xalign", 0.5, "yalign", 0.5, NULL);
    gtk_box_pack_start(GTK_BOX(box), label1, FALSE, FALSE, 0);

    /* add progress bar */
    progress = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(box), progress, FALSE, FALSE, 10);

    /* statistics */
    label2 = gtk_label_new(_("\n"));
    gtk_box_pack_start(GTK_BOX(box), label2, TRUE, TRUE, 0);

    /* finalise dialog */
    gtk_container_add(GTK_CONTAINER
                      (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box);
    g_signal_connect_swapped(dialog, "response",
                             G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_widget_show_all(dialog);

    /* Force the drawing queue to be processed otherwise the dialog
       may not appear before we enter the TRSP updating func
       - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
     */
    while (g_main_context_iteration(NULL, FALSE));

    /* update TRSP */
    trsp_update_from_network(FALSE, progress, label1, label2);

    /* set progress bar to 100% */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 1.0);

    gtk_label_set_text(GTK_LABEL(label1), _("Finished"));

    /* enable close button */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT,
                                      TRUE);

    /* reload satellites */
    mod_mgr_reload_sats();
}

/* Update TLE from network */
static void menubar_tle_net_cb(GtkWidget * widget, gpointer data)
{
    GtkWidget      *dialog;
    GtkWidget      *label;
    GtkWidget      *progress;
    GtkWidget      *label1, *label2;
    GtkWidget      *box;

    (void)widget;
    (void)data;

    /* create new dialog with progress indicator */
    dialog = gtk_dialog_new_with_buttons(_("TLE Update"),
                                         GTK_WINDOW(app),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Close", GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT,
                                      FALSE);

    /* create a vbox */
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(box), 20);

    /* add static label */
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.5, "yalign", 0.5, NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         _("<b>Updating TLE files from network</b>"));
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    /* activity label */
    label1 = gtk_label_new("...");
    g_object_set(label1, "xalign", 0.5, "yalign", 0.5, NULL);
    gtk_box_pack_start(GTK_BOX(box), label1, FALSE, FALSE, 0);

    /* add progress bar */
    progress = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(box), progress, FALSE, FALSE, 10);

    /* statistics */
    label2 = gtk_label_new(_("Satellites updated:\t 0\n"
                             "Satellites skipped:\t 0\n"
                             "Missing Satellites:\t 0\n"));
    gtk_box_pack_start(GTK_BOX(box), label2, TRUE, TRUE, 0);

    /* finalise dialog */
    gtk_container_add(GTK_CONTAINER
                      (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box);
    g_signal_connect_swapped(dialog, "response",
                             G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_widget_show_all(dialog);

    /* Force the drawing queue to be processed otherwise the dialog
       may not appear before we enter the TLE updating func
       - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
     */
    while (g_main_context_iteration(NULL, FALSE));

    /* update TLE */
    tle_update_from_network(FALSE, progress, label1, label2);

    /* set progress bar to 100% */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 1.0);

    gtk_label_set_text(GTK_LABEL(label1), _("Finished"));

    /* enable close button */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT,
                                      TRUE);

    /* reload satellites */
    mod_mgr_reload_sats();
}

/* Update TLE from local files */
static void menubar_tle_local_cb(GtkWidget * widget, gpointer data)
{
    gchar          *dir;        /* selected directory */
    GtkWidget      *dir_chooser;        /* directory chooser button */
    GtkWidget      *dialog;     /* dialog window  */
    GtkWidget      *label;      /* misc labels */
    GtkWidget      *progress;   /* progress indicator */
    GtkWidget      *label1, *label2;    /* activitity and stats labels */
    GtkWidget      *box;
    gint            response;   /* dialog response */
    gboolean        doupdate = FALSE;

    (void)widget;
    (void)data;

    /* get last used directory */
    dir = sat_cfg_get_str(SAT_CFG_STR_TLE_FILE_DIR);

    /* if there is no last used dir fall back to $HOME */
    if (dir == NULL)
    {
        dir = g_strdup(g_get_home_dir());
    }

    /* create file chooser */
    dir_chooser = gtk_file_chooser_button_new(_("Select directory"),
                                              GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dir_chooser), dir);
    g_free(dir);

    /* create label */
    label = gtk_label_new(_("Select TLE directory:"));

    /* pack label and chooser into a hbox */
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(box), dir_chooser, TRUE, TRUE, 5);
    gtk_widget_show_all(box);

    /* create the dalog */
    dialog = gtk_dialog_new_with_buttons(_("Update TLE from files"),
                                         GTK_WINDOW(app),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         "_OK", GTK_RESPONSE_ACCEPT, NULL);
    gtk_box_pack_start(GTK_BOX
                       (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box,
                       TRUE, TRUE, 30);

    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT)
        doupdate = TRUE;
    else
        doupdate = FALSE;

    /* get directory before we destroy the dialog */
    dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dir_chooser));

    /* nuke the dialog */
    gtk_widget_destroy(dialog);

    if (doupdate)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Running TLE update from %s"),
                    __func__, dir);

        /* store last used TLE dir */
        sat_cfg_set_str(SAT_CFG_STR_TLE_FILE_DIR, dir);

        /* create new dialog with progress indicator */
        dialog = gtk_dialog_new_with_buttons(_("TLE Update"),
                                             GTK_WINDOW(app),
                                             GTK_DIALOG_MODAL |
                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                             "_Close", GTK_RESPONSE_ACCEPT,
                                             NULL);
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_ACCEPT, FALSE);

        /* create a vbox */
        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(box), 20);

        /* add static label */
        label = gtk_label_new(NULL);
        g_object_set(label, "xalign", 0.5, "yalign", 0.5, NULL);
        gtk_label_set_markup(GTK_LABEL(label),
                             _("<b>Updating TLE files from files</b>"));
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

        /* activity label */
        label1 = gtk_label_new("...");
        g_object_set(label1, "xalign", 0.5, "yalign", 0.5, NULL);
        gtk_box_pack_start(GTK_BOX(box), label1, FALSE, FALSE, 0);

        /* add progress bar */
        progress = gtk_progress_bar_new();
        gtk_box_pack_start(GTK_BOX(box), progress, FALSE, FALSE, 10);

        /* statistics */
        label2 = gtk_label_new(_("Satellites updated:\t 0\n"
                                 "Satellites skipped:\t 0\n"
                                 "Missing Satellites:\t 0\n"));
        gtk_box_pack_start(GTK_BOX(box), label2, TRUE, TRUE, 0);

        /* finalise dialog */
        gtk_container_add(GTK_CONTAINER
                          (gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                          box);
        g_signal_connect_swapped(dialog, "response",
                                 G_CALLBACK(gtk_widget_destroy), dialog);

        gtk_widget_show_all(dialog);

        /* Force the drawing queue to be processed otherwise the dialog
           may not appear before we enter the TLE updating func
           - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
         */
        while (g_main_context_iteration(NULL, FALSE));

        /* update TLE */
        tle_update_from_files(dir, NULL, FALSE, progress, label1, label2);

        /* set progress bar to 100% */
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 1.0);

        gtk_label_set_text(GTK_LABEL(label1), _("Finished"));

        /* enable close button */
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_ACCEPT, TRUE);
    }

    if (dir)
        g_free(dir);

    /* reload satellites */
    mod_mgr_reload_sats();
}

static void menubar_help_cb(GtkWidget * widget, gpointer data)
{
    GtkWidget      *dialog;
    GtkWidget      *button;

    (void)widget;
    (void)data;

    dialog = gtk_message_dialog_new(GTK_WINDOW(app),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_CLOSE,
                                    _("A comprehensive PDF user manual and \n"
                                      "video tutorials are available from the \n"
                                      "Gpredict website:"));

    button = gtk_link_button_new("http://gpredict.oz9aec.net/documents.php");
    gtk_widget_show(button);

    gtk_box_pack_start(GTK_BOX
                       (gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       button, FALSE, FALSE, 0);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void menubar_license_cb(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    gpredict_help_show_txt("COPYING");
}

static void menubar_news_cb(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    gpredict_help_show_txt("NEWS");
}

static void menubar_about_cb(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    about_dialog_create();
}

#define ACCEL_PATH_QUIT g_intern_static_string("<Gpredict>/File/Quit")

static void create_file_menu_items(GtkMenuShell * menu,
                                   GtkAccelGroup * accel_group)
{
    GtkWidget      *menu_item;

    if (menu == NULL)
        return;

    menu_item = gtk_menu_item_new_with_mnemonic(_("_New module"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_new_mod_cb),
                     NULL);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_KEY_n,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("_Open module"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_open_mod_cb),
                     NULL);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_KEY_o,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("_Log browser"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_message_log),
                     NULL);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_KEY_l,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("_Quit"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_app_exit_cb),
                     NULL);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_KEY_q,
                               GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(menu, menu_item);
}

static void create_edit_menu_items(GtkMenuShell * menu)
{
    GtkWidget      *menu_item;

    if (menu == NULL)
        return;

    menu_item = gtk_menu_item_new_with_mnemonic(_("Update TLE data from _network"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_tle_net_cb),
                     NULL);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("Update TLE data from local _files"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_tle_local_cb),
                     NULL);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("Update transponder data"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_trsp_net_cb),
                     NULL);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("_Preferences"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_pref_cb), NULL);
    gtk_menu_shell_append(menu, menu_item);
}

static void create_help_menu_items(GtkMenuShell * menu,
                                   GtkAccelGroup * accel_group)
{
    GtkWidget      *menu_item;

    if (menu == NULL)
        return;

    menu_item = gtk_menu_item_new_with_mnemonic(_("_Online help"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_help_cb), NULL);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group,
                               GDK_KEY_F1, 0, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("_License"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_license_cb),
                     NULL);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("_News"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_news_cb), NULL);
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(menu, menu_item);

    menu_item = gtk_menu_item_new_with_mnemonic(_("_About Gpredict"));
    g_signal_connect(menu_item, "activate", G_CALLBACK(menubar_about_cb),
                     NULL);
    gtk_menu_shell_append(menu, menu_item);
}

GtkWidget      *menubar_create(GtkWidget * window)
{
    GtkWidget      *menubar;
    GtkWidget      *submenu;
    GtkWidget      *submenu_item;
    GtkAccelGroup  *accel_group;

    (void)window;

    menubar = gtk_menu_bar_new();
    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    submenu = gtk_menu_new();
    submenu_item = gtk_menu_item_new_with_mnemonic(_("_File"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), submenu_item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenu_item), submenu);
    create_file_menu_items(GTK_MENU_SHELL(submenu), accel_group);

    submenu = gtk_menu_new();
    submenu_item = gtk_menu_item_new_with_mnemonic(_("_Edit"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), submenu_item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenu_item), submenu);
    create_edit_menu_items(GTK_MENU_SHELL(submenu));

    submenu = gtk_menu_new();
    submenu_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), submenu_item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenu_item), submenu);
    create_help_menu_items(GTK_MENU_SHELL(submenu), accel_group);

    return menubar;
}
