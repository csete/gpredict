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

#include <errno.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <string.h>		// strerror

#include "compat.h"
#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-rig-ctrl.h"
#include "gtk-rot-ctrl.h"
#include "gtk-sat-module.h"
#include "gtk-sat-module-popup.h"
#include "gtk-sat-module-tmg.h"
#include "gtk-sky-glance.h"
#include "mod-mgr.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"


extern GtkWidget *app;          /* in main.c */

/**
 * Configure module.
 *
 * This function is called when the user selects the configure
 * menu item in the GtkSatModule popup menu. It is a simple
 * wrapper for gtk_sat_module_config_cb
 *
 */
static void config_cb(GtkWidget * menuitem, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);

    if (module->rigctrlwin || module->rotctrlwin)
    {
        GtkWidget      *dialog;

        /* FIXME: should offer option to close controllers */
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        _
                                        ("A module can not be configured while the "
                                         "radio or rotator controller is active.\n\n"
                                         "Please close the radio and rotator controllers "
                                         "and try again."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    else
    {
        gtk_sat_module_config_cb(menuitem, data);
    }
}

/**
 * Manage name changes.
 *
 * This function is called when the contents of the name entry changes.
 * The primary purpose of this function is to check whether the char length
 * of the name is greater than zero, if yes enable the OK button of the dialog.
 */
static void name_changed(GtkWidget * widget, gpointer data)
{
    const gchar    *text;
    gchar          *entry, *end, *j;
    gint            len, pos;
    GtkWidget      *dialog = GTK_WIDGET(data);

    /* step 1: ensure that only valid characters are entered
       (stolen from xlog, tnx pg4i)
     */
    entry = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
    if ((len = g_utf8_strlen(entry, -1)) > 0)
    {
        end = entry + g_utf8_strlen(entry, -1);
        for (j = entry; j < end; ++j)
        {
            if (!gpredict_legal_char(*j))
            {
                gdk_display_beep(gdk_display_get_default());
                pos = gtk_editable_get_position(GTK_EDITABLE(widget));
                gtk_editable_delete_text(GTK_EDITABLE(widget), pos, pos + 1);
            }
        }
    }

    /* step 2: if name seems all right, enable OK button */
    text = gtk_entry_get_text(GTK_ENTRY(widget));

    if (g_utf8_strlen(text, -1) > 0)
    {
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_OK, TRUE);
    }
    else
    {
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_OK, FALSE);
    }
}

/**
 * Clone module.
 *
 * This function is called when the user selects the clone
 * menu item in the GtkSatModule popup menu. the function creates
 * a dialog in which the user is asked for a new module name.
 * When a valid module name is available and the user clicks on OK,
 * an exact copy of the currwent module is created.
 * By default, the nes module will be opened but the user has the
 * possibility to override this in the dialog window.
 */
static void clone_cb(GtkWidget * menuitem, gpointer data)
{
    GtkWidget      *dialog;
    GtkWidget      *entry;
    GtkWidget      *label;
    GtkWidget      *toggle;
    GtkWidget      *vbox;
    GtkAllocation   aloc;
    guint           response;
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    GtkSatModule   *newmod;
    gchar          *source, *target;
    gchar          *icon;       /* icon file name */
    gchar          *title;      /* window title */

    (void)menuitem;

    dialog = gtk_dialog_new_with_buttons(_("Clone Module"),
                                         GTK_WINDOW(gtk_widget_get_toplevel
                                                    (GTK_WIDGET(module))),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_OK", GTK_RESPONSE_OK,
                                         NULL);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    /* label */
    label = gtk_label_new(_("Name of new module:"));
    vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    /* name entry */
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 25);
    gtk_entry_set_text(GTK_ENTRY(entry), module->name);
    gtk_widget_set_tooltip_text(entry,
                                _("Enter a short name for this module.\n"
                                  "Allowed characters: 0..9, a..z, A..Z, - and _"));

    /*not sure what to do with the old private tip the new api does not like them
       _("The name will be used to identify the module "                 \
       "and it is also used a file name for saving the data."\
       "Max length is 25 characters."));
     */
    /* attach changed signal so that we can enable OK button when
       a proper name has been entered
       oh, btw. disable OK button to begin with....
     */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                      GTK_RESPONSE_OK, FALSE);
    g_signal_connect(entry, "changed", G_CALLBACK(name_changed), dialog);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);


    /* check button */
    toggle = gtk_check_button_new_with_label(_("Open module when created"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE);
    gtk_widget_set_tooltip_text(toggle,
                                _("If checked, the new module will be opened "
                                  "after it has been created"));
    gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 20);


    gtk_widget_show_all(vbox);

    /* run dialog */
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (response)
    {

    case GTK_RESPONSE_OK:
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d: Cloning %s => %s"),
                    __FILE__, __LINE__, module->name,
                    gtk_entry_get_text(GTK_ENTRY(entry)));

        /* build full file names */
        gchar          *moddir = get_modules_dir();

        source = g_strconcat(moddir, G_DIR_SEPARATOR_S,
                             module->name, ".mod", NULL);
        target = g_strconcat(moddir, G_DIR_SEPARATOR_S,
                             gtk_entry_get_text(GTK_ENTRY(entry)), ".mod",
                             NULL);
        g_free(moddir);

        /* copy file */
        if (gpredict_file_copy(source, target))
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%d: Failed to clone %s."),
                        __FILE__, __LINE__, module->name);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%d: Successfully cloned %s."),
                        __FILE__, __LINE__, module->name);

            /* open module if requested */
            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
            {

                newmod = GTK_SAT_MODULE(gtk_sat_module_new(target));
                newmod->state = module->state;

                if (newmod->state == GTK_SAT_MOD_STATE_DOCKED)
                {

                    /* add to module manager */
                    mod_mgr_add_module(GTK_WIDGET(newmod), TRUE);

                }
                else
                {
                    /* add to module manager */
                    mod_mgr_add_module(GTK_WIDGET(newmod), FALSE);

                    /* create window */
                    newmod->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                    gtk_window_set_title(GTK_WINDOW(newmod->win),
                                         newmod->name);
                    title = g_strconcat(_("GPREDICT: "),
                                        newmod->name,
                                        " (", newmod->qth->name, ")", NULL);
                    gtk_window_set_title(GTK_WINDOW(newmod->win), title);
                    g_free(title);

                    /* use size of source module */
                    gtk_widget_get_allocation(GTK_WIDGET(module), &aloc);
                    gtk_window_set_default_size(GTK_WINDOW(newmod->win),
                                                aloc.width, aloc.height);

                    g_signal_connect(G_OBJECT(newmod->win), "configure_event",
                                     G_CALLBACK(module_window_config_cb),
                                     newmod);

                    /* add module to window */
                    gtk_container_add(GTK_CONTAINER(newmod->win),
                                      GTK_WIDGET(newmod));

                    /* window icon */
                    icon = icon_file_name("gpredict-icon.png");
                    if (g_file_test(icon, G_FILE_TEST_EXISTS))
                    {
                        gtk_window_set_icon_from_file(GTK_WINDOW(newmod->win),
                                                      icon, NULL);
                    }
                    g_free(icon);

                    /* show window */
                    gtk_widget_show_all(newmod->win);

                }
            }
        }

        /* clean up */
        g_free(source);
        g_free(target);

        break;

    case GTK_RESPONSE_CANCEL:
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d: Cloning cancelled by user."),
                    __FILE__, __LINE__);
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d: Cloning interrupted."), __FILE__, __LINE__);
        break;
    }

    gtk_widget_destroy(dialog);
}

/**
 * Toggle dockig state.
 *
 * This function is called when the user selects the (Un)Dock menu
 * item in the GtkSatModule popup menu. If the current module state
 * is DOCKED, the module will be undocked and moved into it's own,
 * GtkWindow. If the current module state is WINDOW or FULLSCREEN,
 * the module will be docked.
 *
 * The text of the menu item will be changed corresponding to the
 * action that has been performed.
 */
static void docking_state_cb(GtkWidget * menuitem, gpointer data)
{
    GtkWidget      *module = GTK_WIDGET(data);
    GtkAllocation   aloc;
    gint            w, h;
    gchar          *icon;       /* icon file name */
    gchar          *title;      /* window title */

    (void)menuitem;

    gtk_widget_get_allocation(module, &aloc);

    switch (GTK_SAT_MODULE(module)->state)
    {
    case GTK_SAT_MOD_STATE_DOCKED:
        /* get stored size; use size from main window if size not explicitly stoed */
        if (g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION,
                               MOD_CFG_WIN_WIDTH, NULL))
        {
            w = g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                       MOD_CFG_GLOBAL_SECTION,
                                       MOD_CFG_WIN_WIDTH, NULL);
        }
        else
        {
            w = aloc.width;
        }
        if (g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION,
                               MOD_CFG_WIN_HEIGHT, NULL))
        {
            h = g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                       MOD_CFG_GLOBAL_SECTION,
                                       MOD_CFG_WIN_HEIGHT, NULL);
        }
        else
        {
            h = aloc.height;
        }

        /* increase reference count of module */
        g_object_ref(module);

        /* we don't need the positions */
        //GTK_SAT_MODULE (module)->vpanedpos = -1;
        //GTK_SAT_MODULE (module)->hpanedpos = -1;

        /* undock from mod-mgr */
        mod_mgr_undock_module(module);

        /* create window */
        GTK_SAT_MODULE(module)->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        title = g_strconcat(_("GPREDICT: "),
                            GTK_SAT_MODULE(module)->name,
                            " (", GTK_SAT_MODULE(module)->qth->name, ")",
                            NULL);
        gtk_window_set_title(GTK_WINDOW(GTK_SAT_MODULE(module)->win), title);
        g_free(title);
        gtk_window_set_default_size(GTK_WINDOW(GTK_SAT_MODULE(module)->win), w,
                                    h);
        g_signal_connect(G_OBJECT(GTK_SAT_MODULE(module)->win),
                         "configure_event",
                         G_CALLBACK(module_window_config_cb), module);

        /* window icon */
        icon = icon_file_name("gpredict-icon.png");
        if (g_file_test(icon, G_FILE_TEST_EXISTS))
        {
            gtk_window_set_icon_from_file(GTK_WINDOW
                                          (GTK_SAT_MODULE(module)->win), icon,
                                          NULL);
        }
        g_free(icon);

        /* move window to stored position if requested by configuration */
        if (sat_cfg_get_bool(SAT_CFG_BOOL_MOD_WIN_POS) &&
            g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION,
                               MOD_CFG_WIN_POS_X,
                               NULL) &&
            g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION,
                               MOD_CFG_WIN_POS_Y, NULL))
        {

            gtk_window_move(GTK_WINDOW(GTK_SAT_MODULE(module)->win),
                            g_key_file_get_integer(GTK_SAT_MODULE
                                                   (module)->cfgdata,
                                                   MOD_CFG_GLOBAL_SECTION,
                                                   MOD_CFG_WIN_POS_X, NULL),
                            g_key_file_get_integer(GTK_SAT_MODULE
                                                   (module)->cfgdata,
                                                   MOD_CFG_GLOBAL_SECTION,
                                                   MOD_CFG_WIN_POS_Y, NULL));
        }

        /* add module to window */
        gtk_container_add(GTK_CONTAINER(GTK_SAT_MODULE(module)->win), module);

        /* change internal state */
        GTK_SAT_MODULE(module)->state = GTK_SAT_MOD_STATE_WINDOW;

        /* store new state in configuration */
        g_key_file_set_integer(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION,
                               MOD_CFG_STATE, GTK_SAT_MOD_STATE_WINDOW);

        /* decrease reference count of module */
        g_object_unref(module);

        /* show window */
        gtk_widget_show_all(GTK_SAT_MODULE(module)->win);

        /* reparent time manager window if visible */
        if (GTK_SAT_MODULE(module)->tmgActive)
        {
            gtk_window_set_transient_for(GTK_WINDOW
                                         (GTK_SAT_MODULE(module)->tmgWin),
                                         GTK_WINDOW(GTK_SAT_MODULE
                                                    (module)->win));
        }

        break;

    case GTK_SAT_MOD_STATE_WINDOW:
    case GTK_SAT_MOD_STATE_FULLSCREEN:
        /* increase referene count */
        g_object_ref(module);

        /* reparent time manager window if visible */
        if (GTK_SAT_MODULE(module)->tmgActive)
        {
            gtk_window_set_transient_for(GTK_WINDOW
                                         (GTK_SAT_MODULE(module)->tmgWin),
                                         GTK_WINDOW(app));
        }

        /* remove module from window, destroy window */
        gtk_container_remove(GTK_CONTAINER(GTK_SAT_MODULE(module)->win),
                             module);
        gtk_widget_destroy(GTK_SAT_MODULE(module)->win);
        GTK_SAT_MODULE(module)->win = NULL;

        /* dock into mod-mgr */
        mod_mgr_dock_module(module);

        /* change internal state */
        GTK_SAT_MODULE(module)->state = GTK_SAT_MOD_STATE_DOCKED;

        /* store new state in configuration */
        g_key_file_set_integer(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION,
                               MOD_CFG_STATE, GTK_SAT_MOD_STATE_DOCKED);

        /* decrease reference count of module */
        g_object_unref(module);

        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Unknown module state: %d"),
                    __FILE__, __LINE__, GTK_SAT_MODULE(module)->state);
        break;
    }
}

/**
 * Toggle screen state.
 *
 * This function is intended to toggle between FULLSCREEN
 * and WINDOW state.
 */
static void screen_state_cb(GtkWidget * menuitem, gpointer data)
{
    GtkWidget      *module = GTK_WIDGET(data);
    gint            w, h;
    gchar          *icon;       /* icon file name */
    gchar          *title;      /* window title */

    (void)menuitem;

    switch (GTK_SAT_MODULE(module)->state)
    {
    case GTK_SAT_MOD_STATE_DOCKED:

        /* increase reference count of module */
        g_object_ref(module);

        /* undock from mod-mgr */
        mod_mgr_undock_module(module);

        /* create window */
        GTK_SAT_MODULE(module)->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        title = g_strconcat(_("GPREDICT: "),
                            GTK_SAT_MODULE(module)->name,
                            " (", GTK_SAT_MODULE(module)->qth->name, ")",
                            NULL);
        gtk_window_set_title(GTK_WINDOW(GTK_SAT_MODULE(module)->win), title);
        g_free(title);

        /* window icon */
        icon = icon_file_name("gpredict-icon.png");
        if (g_file_test(icon, G_FILE_TEST_EXISTS))
        {
            gtk_window_set_icon_from_file(GTK_WINDOW
                                          (GTK_SAT_MODULE(module)->win), icon,
                                          NULL);
        }
        g_free(icon);

        /* add module to window */
        gtk_container_add(GTK_CONTAINER(GTK_SAT_MODULE(module)->win), module);

        /* change internal state */
        GTK_SAT_MODULE(module)->state = GTK_SAT_MOD_STATE_FULLSCREEN;

        /* decrease reference count of module */
        g_object_unref(module);

        gtk_window_fullscreen(GTK_WINDOW(GTK_SAT_MODULE(module)->win));

        /* show window */
        gtk_widget_show_all(GTK_SAT_MODULE(module)->win);

        /* reparent time manager window if visible */
        if (GTK_SAT_MODULE(module)->tmgActive)
        {
            gtk_window_set_transient_for(GTK_WINDOW
                                         (GTK_SAT_MODULE(module)->tmgWin),
                                         GTK_WINDOW(GTK_SAT_MODULE
                                                    (module)->win));
        }
        break;

    case GTK_SAT_MOD_STATE_WINDOW:
        /* change internal state */
        GTK_SAT_MODULE(module)->state = GTK_SAT_MOD_STATE_FULLSCREEN;
        gtk_window_fullscreen(GTK_WINDOW(GTK_SAT_MODULE(module)->win));
        gtk_window_set_default_size(GTK_WINDOW(GTK_SAT_MODULE(module)->win),
                                    800, 600);
        break;

    case GTK_SAT_MOD_STATE_FULLSCREEN:
        /* change internal state */
        GTK_SAT_MODULE(module)->state = GTK_SAT_MOD_STATE_WINDOW;
        gtk_window_unfullscreen(GTK_WINDOW(GTK_SAT_MODULE(module)->win));

        /* get stored size; use some standard size if not explicitly specified */
        if (g_key_file_has_key
            (GTK_SAT_MODULE(module)->cfgdata, MOD_CFG_GLOBAL_SECTION,
             MOD_CFG_WIN_WIDTH, NULL))
        {
            w = g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                       MOD_CFG_GLOBAL_SECTION,
                                       MOD_CFG_WIN_WIDTH, NULL);
        }
        else
        {
            w = 800;
        }
        if (g_key_file_has_key
            (GTK_SAT_MODULE(module)->cfgdata, MOD_CFG_GLOBAL_SECTION,
             MOD_CFG_WIN_HEIGHT, NULL))
        {
            h = g_key_file_get_integer(GTK_SAT_MODULE(module)->cfgdata,
                                       MOD_CFG_GLOBAL_SECTION,
                                       MOD_CFG_WIN_HEIGHT, NULL);
        }
        else
        {
            h = 600;
        }
        gtk_window_set_default_size(GTK_WINDOW(GTK_SAT_MODULE(module)->win), w,
                                    h);

        /* move window to stored position if requested by configuration */
        if (sat_cfg_get_bool(SAT_CFG_BOOL_MOD_WIN_POS) &&
            g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_POS_X, NULL)
            && g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                                  MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_POS_Y,
                                  NULL))
        {

            gtk_window_move(GTK_WINDOW(GTK_SAT_MODULE(module)->win),
                            g_key_file_get_integer(GTK_SAT_MODULE
                                                   (module)->cfgdata,
                                                   MOD_CFG_GLOBAL_SECTION,
                                                   MOD_CFG_WIN_POS_X, NULL),
                            g_key_file_get_integer(GTK_SAT_MODULE
                                                   (module)->cfgdata,
                                                   MOD_CFG_GLOBAL_SECTION,
                                                   MOD_CFG_WIN_POS_Y, NULL));
        }

        /* store new state in configuration */
        g_key_file_set_integer(GTK_SAT_MODULE(module)->cfgdata,
                               MOD_CFG_GLOBAL_SECTION, MOD_CFG_STATE,
                               GTK_SAT_MOD_STATE_WINDOW);
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Unknown module state: %d"),
                    __FILE__, __LINE__, GTK_SAT_MODULE(module)->state);
        break;
    }
}

/**
 * New satellite selected.
 *
 * @param data Pointer to the GtkSatModule widget
 * 
 * This menu item is activated when a new satellite is selected in the 
 * "Select satellite" submenu of the module pop-up. This will trigger a call
 * to the select_sat() fuinction of the module, which in turn will call the
 * select_sat() function of each child view.
 * 
 * The catalog number of the selected satellite is attached to the menu item
 */
static void sat_selected_cb(GtkWidget * menuitem, gpointer data)
{
    gint            catnum;
    GtkSatModule   *module;

    catnum = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "catnum"));
    module = GTK_SAT_MODULE(data);
    gtk_sat_module_select_sat(module, catnum);
}

/** Ensure that deleted top-level windows are destroyed */
static gint window_delete(GtkWidget * widget, GdkEvent * event, gpointer data)
{
    (void)widget;
    (void)event;
    (void)data;

    return FALSE;
}

/**
 * Destroy sky at glance window.
 *
 * @param window Pointer to the sky at glance window.
 * @param data Pointer to the GtkSatModule to which this widget is attached.
 *
 * This function is called automatically when the window is destroyed.
 */
static void destroy_skg(GtkWidget * window, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);

    (void)window;

    module->skgwin = NULL;
    module->skg = NULL;
}

/**
 * Invoke Sky-at-glance.
 *
 * This function is a shortcut to the sky at glance function
 * in that it will make the predictions with the satellites
 * tracked in the current module.
 */
static void sky_at_glance_cb(GtkWidget * menuitem, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    gchar          *buff;

    (void)menuitem;

    /* if module is busy wait until done then go on */
    g_mutex_lock(&module->busy);
    if (module->skgwin != NULL)
    {
        /* there is already a sky at glance for this module */
        gtk_window_present(GTK_WINDOW(module->skgwin));
        g_mutex_unlock(&module->busy);

        return;
    }

    /* create window */
    module->skgwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    buff = g_strdup_printf(_("The sky at a glance (%s)"), module->name);
    gtk_window_set_title(GTK_WINDOW(module->skgwin), buff);
    g_free(buff);
    g_signal_connect(G_OBJECT(module->skgwin), "delete_event",
                     G_CALLBACK(window_delete), NULL);
    g_signal_connect(G_OBJECT(module->skgwin), "destroy",
                     G_CALLBACK(destroy_skg), module);

    /* window icon */
    buff = icon_file_name("gpredict-planner.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(module->skgwin), buff, NULL);
    g_free(buff);

    /* create sky at a glance widget */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_PRED_USE_REAL_T0))
    {
        module->skg = gtk_sky_glance_new(module->satellites, module->qth, 0.0);
    }
    else
    {
        module->skg = gtk_sky_glance_new(module->satellites, module->qth,
                                         module->tmgCdnum);
    }

    /* store time at which GtkSkyGlance has been created */
    module->lastSkgUpd = module->tmgCdnum;

    gtk_container_set_border_width(GTK_CONTAINER(module->skgwin), 4);
    gtk_container_add(GTK_CONTAINER(module->skgwin), module->skg);

    gtk_widget_show_all(module->skgwin);

    g_mutex_unlock(&module->busy);
}

/** Open time manager. */
static void tmgr_cb(GtkWidget * menuitem, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);

    (void)menuitem;

    tmg_create(module);
}

/**
 * Destroy radio control window.
 *
 * @param window Pointer to the radio control window.
 * @param data Pointer to the GtkSatModule to which this controller is attached.
 *
 * This function is called automatically when the window is destroyed.
 */
static void destroy_rigctrl(GtkWidget * window, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);

    (void)window;               /* avoid unused parameter compiler warning */

    module->rigctrlwin = NULL;
    module->rigctrl = NULL;
}

/**
 * Open Radio control window.
 *
 * @param menuitem The menuitem that was selected.
 * @param data Pointer the GtkSatModule.
 */
static void rigctrl_cb(GtkWidget * menuitem, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    gchar          *buff;

    (void)menuitem;

    if (module->rigctrlwin != NULL)
    {
        /* there is already a radio controller for this module */
        gtk_window_present(GTK_WINDOW(module->rigctrlwin));
        return;
    }

    module->rigctrl = gtk_rig_ctrl_new(module);

    if (module->rigctrl == NULL)
    {
        /* gtk_rig_ctrl_new returned NULL becasue no radios are configured */
        GtkWidget      *dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(app),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                        _("You have no radio configuration!\n"
                                          "Please configure a radio first."));
        g_signal_connect_swapped(dialog, "response",
                                 G_CALLBACK(gtk_widget_destroy), dialog);
        gtk_window_set_title(GTK_WINDOW(dialog), _("ERROR"));
        gtk_widget_show_all(dialog);

        return;
    }

    /* create a window */
    module->rigctrlwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    buff = g_strdup_printf(_("Gpredict Radio Control: %s"), module->name);
    gtk_window_set_title(GTK_WINDOW(module->rigctrlwin), buff);
    g_free(buff);
    g_signal_connect(G_OBJECT(module->rigctrlwin), "delete_event",
                     G_CALLBACK(window_delete), NULL);
    g_signal_connect(G_OBJECT(module->rigctrlwin), "destroy",
                     G_CALLBACK(destroy_rigctrl), module);

    /* window icon */
    buff = icon_file_name("gpredict-oscilloscope.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(module->rigctrlwin), buff, NULL);
    g_free(buff);

    gtk_container_add(GTK_CONTAINER(module->rigctrlwin), module->rigctrl);

    gtk_widget_show_all(module->rigctrlwin);
}

/**
 * Destroy rotator control window.
 *
 * @param window Pointer to the rotator control window.
 * @param data Pointer to the GtkSatModule to which this controller is attached.
 * 
 * This function is called automatically when the window is destroyed.
 */
static void destroy_rotctrl(GtkWidget * window, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);

    (void)window;

    module->rotctrlwin = NULL;
    module->rotctrl = NULL;
}

/**
 * Open antenna rotator control window.
 *
 * @param menuitem The menuitem that was selected.
 * @param data Pointer the GtkSatModule.
 */
static void rotctrl_cb(GtkWidget * menuitem, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    gchar          *buff;

    (void)menuitem;

    if (module->rotctrlwin != NULL)
    {
        /* there is already a roto controller for this module */
        gtk_window_present(GTK_WINDOW(module->rotctrlwin));
        return;
    }

    module->rotctrl = gtk_rot_ctrl_new(module);

    if (module->rotctrl == NULL)
    {
        /* gtk_rot_ctrl_new returned NULL becasue no rotators are configured */
        GtkWidget      *dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(app),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                        _
                                        ("You have no rotator configuration!\n"
                                         "Please configure an antenna rotator first."));
        g_signal_connect_swapped(dialog, "response",
                                 G_CALLBACK(gtk_widget_destroy), dialog);
        gtk_window_set_title(GTK_WINDOW(dialog), _("ERROR"));
        gtk_widget_show_all(dialog);

        return;
    }

    /* create a window */
    module->rotctrlwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    buff = g_strdup_printf(_("Gpredict Rotator Control: %s"), module->name);
    gtk_window_set_title(GTK_WINDOW(module->rotctrlwin), buff);
    g_free(buff);
    g_signal_connect(G_OBJECT(module->rotctrlwin), "delete_event",
                     G_CALLBACK(window_delete), module);
    g_signal_connect(G_OBJECT(module->rotctrlwin), "destroy",
                     G_CALLBACK(destroy_rotctrl), module);

    /* window icon */
    buff = icon_file_name("gpredict-antenna.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(module->rotctrlwin), buff, NULL);
    g_free(buff);

    gtk_container_add(GTK_CONTAINER(module->rotctrlwin), module->rotctrl);

    gtk_widget_show_all(module->rotctrlwin);
}

/** Autotrack activated */
static void autotrack_cb(GtkCheckMenuItem * menuitem, gpointer data)
{
    GTK_SAT_MODULE(data)->autotrack = gtk_check_menu_item_get_active(menuitem);
}

/**
 * Close module.
 *
 * This function is called when the user selects the close menu
 * item in the GtkSatModule popup menu. It is simply a wrapper
 * for gtk_sat_module_close_cb, which will close the current module.
 */
static void close_cb(GtkWidget * menuitem, gpointer data)
{
    gtk_sat_module_close_cb(menuitem, data);
}

/**
 * Close and permanently delete module.
 *
 * This function is called when the user selects the delete menu
 * item in the GtkSatModule popup menu.
 */
static void delete_cb(GtkWidget * menuitem, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    GtkWidget      *toplevel;
    GtkWidget      *dialog;
    int             result = 0;
    gint            response;
    gchar          *file;
    gchar          *moddir;

	toplevel = gtk_widget_get_toplevel(GTK_WIDGET(data));
    moddir = get_modules_dir();
    file = g_strconcat(moddir, G_DIR_SEPARATOR_S, module->name, ".mod", NULL);
    g_free(moddir);

    /* ask user to confirm removal */
    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(toplevel),
                                                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                                _("This operation will permanently delete <b>%s</b> "\
                                                  "from the disk.\nDo you you want to proceed?"),
                                                module->name);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    switch (response)
    {
    case GTK_RESPONSE_YES:
		gtk_sat_module_close_cb(menuitem, data);
		result = g_remove(file);
        if (result)
        {
			
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("Failed to delete %s: %s"),
                        file, strerror(errno));
            dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(toplevel),
                                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                        _("An error occurred deleting <b>%s</b>:\n%s"),
                                                        module->name, strerror(errno));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, _("%s deleted"), file);
        }
        break;
    default:
        break;
    }

    g_free(file);
}

/**
 * Snoop window position and size when main window receives configure event.
 *
 * @param widget Pointer to the module window.
 * @param event  Pointer to the event structure.
 * @param data   Pointer to user data, in this case the module.
 *
 * This function is used to trap configure events in order to store the current
 * position and size of the module window.
 *
 * @note unfortunately GdkEventConfigure ignores the window gravity, while
 *       the only way we have of setting the position doesn't. We have to
 *       call get_position because it does pay attention to the gravity.
 *
 * @note The logic in the code has been borrowed from gaim/pidgin http://pidgin.im/
 *
 */
gboolean module_window_config_cb(GtkWidget * widget, GdkEventConfigure * event,
                                 gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    gint            x, y, w, h;

    /* data is only useful when window is visible */
    if (gtk_widget_get_visible(widget))
        gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
    else
        return FALSE;           /* carry on normally */

#ifdef G_OS_WIN32
    /* Workaround for GTK+ bug # 169811 - "configure_event" is fired
       when the window is being maximized */
    if (gdk_window_get_state(gtk_widget_get_window(widget)) & GDK_WINDOW_STATE_MAXIMIZED)
    {
        return FALSE;
    }
#endif

    /* don't save off-screen positioning */
    /* gtk_menu_popup got deprecated in 3.22, first available in Ubuntu 18.04 */
#if GTK_MINOR_VERSION < 22
    w = gdk_screen_width();
    h = gdk_screen_height();
#else
    GdkRectangle    work_area;
    gdk_monitor_get_workarea(gdk_display_get_primary_monitor(gdk_display_get_default()),
                             &work_area);
    w = work_area.width;
    h = work_area.height;
#endif

    if (x < 0 || y < 0 || x + event->width > w || y + event->height > h)
    {
        return FALSE;
    }

    /* store the position and size */
    g_key_file_set_integer(module->cfgdata, MOD_CFG_GLOBAL_SECTION,
                           MOD_CFG_WIN_POS_X, x);
    g_key_file_set_integer(module->cfgdata, MOD_CFG_GLOBAL_SECTION,
                           MOD_CFG_WIN_POS_Y, y);
    g_key_file_set_integer(module->cfgdata, MOD_CFG_GLOBAL_SECTION,
                           MOD_CFG_WIN_WIDTH, event->width);
    g_key_file_set_integer(module->cfgdata, MOD_CFG_GLOBAL_SECTION,
                           MOD_CFG_WIN_HEIGHT, event->height);

    /* continue to handle event normally */
    return FALSE;
}

static gint sat_nickname_compare(const sat_t * a, const sat_t * b)
{
    return gpredict_strcmp(a->nickname, b->nickname);
}

/**
 * Create and run GtkSatModule popup menu.
 *
 * @param module The module that should have the popup menu attached to it.
 *
 * This function ctreates and executes a popup menu that is related to a
 * GtkSatModule widget. The module must be a valid GtkSatModule, since it makes
 * no sense whatsoever to have this kind of popup menu without a GtkSatModule
 * parent.
 *
 */
void gtk_sat_module_popup(GtkSatModule * module)
{
    GtkWidget      *menu;       /* The pop-up menu */
    GtkWidget      *satsubmenu; /* Satellite selection submenu */
    GtkWidget      *menuitem;   /* Widget used to create the menu items */
    GList          *sats;
    sat_t          *sat;
    guint           i, n;


    if ((module == NULL) || !IS_GTK_SAT_MODULE(module))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: %s called with NULL parameter!"),
                    __FILE__, __LINE__, __func__);
        return;
    }

    menu = gtk_menu_new();

    if (module->state == GTK_SAT_MOD_STATE_DOCKED)
    {
        menuitem = gtk_menu_item_new_with_label(_("Detach module"));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(docking_state_cb), module);
    }
    else
    {
        menuitem = gtk_menu_item_new_with_label(_("Attach module"));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(docking_state_cb), module);
    }

    if (module->state == GTK_SAT_MOD_STATE_FULLSCREEN)
    {
        menuitem = gtk_menu_item_new_with_label(_("Exit full screen"));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(screen_state_cb), module);
    }
    else
    {
        menuitem = gtk_menu_item_new_with_label(_("Full screen"));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(screen_state_cb), module);
    }

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* autotrack */
    menuitem = gtk_check_menu_item_new_with_label(_("Autotrack"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                   module->autotrack);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(autotrack_cb), module);

    /* select satellite submenu */
    menuitem = gtk_menu_item_new_with_label(_("Select satellite"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    satsubmenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), satsubmenu);

    sats = g_hash_table_get_values(module->satellites);
    sats = g_list_sort(sats, (GCompareFunc) sat_nickname_compare);

    n = g_list_length(sats);
    for (i = 0; i < n; i++)
    {
        sat = SAT(g_list_nth_data(sats, i));
        menuitem = gtk_menu_item_new_with_label(sat->nickname);
        g_object_set_data(G_OBJECT(menuitem), "catnum",
                          GINT_TO_POINTER(sat->tle.catnr));
        g_signal_connect(menuitem, "activate", G_CALLBACK(sat_selected_cb),
                         module);
        gtk_menu_shell_append(GTK_MENU_SHELL(satsubmenu), menuitem);
    }

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* sky at a glance */
    menuitem = gtk_menu_item_new_with_label(_("Sky at a glance"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate",
                     G_CALLBACK(sky_at_glance_cb), module);

    /* time manager */
    menuitem = gtk_menu_item_new_with_label(_("Time Controller"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(tmgr_cb), module);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* Radio Control */
    menuitem = gtk_menu_item_new_with_label(_("Radio Control"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(rigctrl_cb), module);

    /* Antenna Control */
    menuitem = gtk_menu_item_new_with_label(_("Antenna Control"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(rotctrl_cb), module);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* configure */
    menuitem = gtk_menu_item_new_with_label(_("Configure"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(config_cb), module);

    /* clone */
    menuitem = gtk_menu_item_new_with_label(_("Clone..."));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(clone_cb), module);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* delete module */
    menuitem = gtk_menu_item_new_with_label(_("Delete"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(delete_cb), module);

    /* close */
    menuitem = gtk_menu_item_new_with_label(_("Close"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect(menuitem, "activate", G_CALLBACK(close_cb), module);

    gtk_widget_show_all(menu);

    /* gtk_menu_popup got deprecated in 3.22, first available in Ubuntu 18.04 */
#if GTK_MINOR_VERSION < 22
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   0, gdk_event_get_time(NULL));
#else
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
#endif
}
