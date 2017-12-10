/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete
           Charles Suprin

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
/*
 * Module manager.
 *
 * The module manager is responsible for the management of opened modules.
 * It consist of a GtkNoteBook container where the modules are placed initially.
 *
 * The module manager is initialised with the mod_mgr_create function, which will
 * create the notebook widget and re-open the modules that have been open when
 * gpredict has been quit last time.
 *
 * To add additional modules the mod_mgr_add_module function should be used. This
 * function takes a fully initialised GtkSatModule (FIXME: cast to GtkWidget) and
 * a boolean flag indicating whether the module should be docked into the notebook
 * or not. Please note, that if a module is added with dock=FALSE, the caller will
 * have the responsibility of creating a proper container window for the module.
 *
 * Finally, when gpredict is about to exit, the state of the module manager can be
 * saved by calling the mod_mgr_save_state. This will save a list of open modules
 * so that they can be restored next time gpredict is re-opened.
 *
 * The mod-mgr maintains an internal GSList with references to the opened modules.
 * This allows the mod-mgr to know about both docked and undocked modules.
 *
 */
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "compat.h"
#include "gtk-sat-module.h"
#include "gtk-sat-module-popup.h"
#include "mod-cfg.h"
#include "mod-mgr.h"
#include "sat-cfg.h"
#include "sat-log.h"

extern GtkWidget *app;

/* List of modules, docked and undocked */
static GSList  *modules = NULL;


/* The notebook widget for docked modules */
static GtkWidget *nbook = NULL;


static void     update_window_title(void);
static void     switch_page_cb(GtkNotebook * notebook,
                               gpointer * page,
                               guint page_num, gpointer user_data);

static void     create_module_window(GtkWidget * module);


GtkWidget      *mod_mgr_create(void)
{
    gchar          *openmods = NULL;
    gchar         **mods;
    gint            count, i;
    GtkWidget      *module;
    gchar          *modfile;
    gchar          *confdir;
    gint            page;

    nbook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(nbook), TRUE);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(nbook));
    g_signal_connect(G_OBJECT(nbook), "switch-page",
                     G_CALLBACK(switch_page_cb), NULL);

    openmods = sat_cfg_get_str(SAT_CFG_STR_OPEN_MODULES);
    page = sat_cfg_get_int(SAT_CFG_INT_MODULE_CURRENT_PAGE);

    if (openmods)
    {
        mods = g_strsplit(openmods, ";", 0);
        count = g_strv_length(mods);

        for (i = 0; i < count; i++)
        {

            confdir = get_modules_dir();
            modfile = g_strconcat(confdir, G_DIR_SEPARATOR_S,
                                  mods[i], ".mod", NULL);
            g_free(confdir);
            module = gtk_sat_module_new(modfile);

            if (IS_GTK_SAT_MODULE(module))
            {

                /* if module state was window or user does not want to restore the
                   state of the modules, pack the module into the notebook */
                if ((GTK_SAT_MODULE(module)->state == GTK_SAT_MOD_STATE_DOCKED)
                    || !sat_cfg_get_bool(SAT_CFG_BOOL_MOD_STATE))
                {
                    mod_mgr_add_module(module, TRUE);
                }
                else
                {
                    mod_mgr_add_module(module, FALSE);
                    create_module_window(module);
                }
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: Failed to restore %s"), __func__, mods[i]);

                /* try to smartly handle disappearing modules */
                page--;
            }

            g_free(modfile);

        }

        /* set to the page open when gpredict was closed */
        if (page >= 0)
            gtk_notebook_set_current_page(GTK_NOTEBOOK(nbook), page);

        g_strfreev(mods);
        g_free(openmods);

        /* disable tabs if only one page in notebook */
        if ((gtk_notebook_get_n_pages(GTK_NOTEBOOK(nbook))) == 1)
        {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), FALSE);
        }
        else
        {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), TRUE);
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: No modules have to be restored."), __func__);
    }

    return nbook;
}

/* Register a new module in the mod-mgr. If the dock flag is true the module is
 * added to the mod-mgr notebook, otherwise it will be up to the caller to
 * create a proper container.
 */
gint mod_mgr_add_module(GtkWidget * module, gboolean dock)
{
    gint            retcode = 0;
    gint            page;


    if (module)
    {

        /* add module to internal list */
        modules = g_slist_append(modules, module);

        if (dock)
        {
            /* add module to notebook if state = DOCKED */
            page = gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                                            module,
                                            gtk_label_new(GTK_SAT_MODULE
                                                          (module)->name));

            /* allow nmodule to be dragged to different position */
            gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(nbook), module,
                                             TRUE);

            gtk_notebook_set_current_page(GTK_NOTEBOOK(nbook), page);

            /* send message to logger */
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Added %s to module manager (page %d)."),
                        __func__, GTK_SAT_MODULE(module)->name, page);
        }
        else
        {
            /* send message to logger */
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Added %s to module manager (NOT DOCKED)."),
                        __func__, GTK_SAT_MODULE(module)->name);
        }
        retcode = 0;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s seems to be NULL"),
                    __func__, GTK_SAT_MODULE(module)->name);
        retcode = 1;
    }

    /* disable tabs if only one page in notebook */
    if ((gtk_notebook_get_n_pages(GTK_NOTEBOOK(nbook))) == 1)
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), FALSE);
    }
    else
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), TRUE);
    }

    update_window_title();

    return retcode;
}

gint mod_mgr_remove_module(GtkWidget * module)
{
    gint            page;
    gint            retcode = 0;

    /* remove from notebook */
    if (GTK_SAT_MODULE(module)->state == GTK_SAT_MOD_STATE_DOCKED)
    {
        /* get page number for this module */
        page = gtk_notebook_page_num(GTK_NOTEBOOK(nbook), module);

        if (page == -1)
        {
            /* this is some kind of bug (inconsistency between internal states) */
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _
                        ("%s: Could not find child in notebook. This may hurt..."),
                        __func__);

            retcode = 1;
        }
        else
        {
            gtk_notebook_remove_page(GTK_NOTEBOOK(nbook), page);

            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Removed child from notebook page %d."),
                        __func__, page);

            retcode = 0;
        }
    }

    modules = g_slist_remove(modules, module);

    /* undocked modules will have to destroy themselves
       because of their parent window
     */

    /* disable tabs if only one page in notebook */
    if ((gtk_notebook_get_n_pages(GTK_NOTEBOOK(nbook))) == 1)
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), FALSE);
    }
    else
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), TRUE);
    }

    /* update window title */
    update_window_title();

    return retcode;
}

/*
 * Save state of module manager.
 *
 * This function saves the state of the module manager. Currently, this consists
 * of saving the list of open modules. If no modules are open, the function saves
 * a NULL-list, indication that the corresponding configuration key should be
 * removed.
 */
void mod_mgr_save_state()
{
    guint           num;
    guint           i;
    GtkWidget      *module;
    gchar          *mods = NULL;
    gchar          *buff;
    gint            page;


    if (!nbook)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Attempt to save state but mod-mgr is NULL?"),
                    __func__);
        return;
    }

    num = g_slist_length(modules);
    if (num == 0)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: No modules need to save state."), __func__);

        sat_cfg_set_str(SAT_CFG_STR_OPEN_MODULES, NULL);

        return;
    }

    for (i = 0; i < num; i++)
    {
        module = GTK_WIDGET(g_slist_nth_data(modules, i));

        /* save state of the module */
        mod_cfg_save(GTK_SAT_MODULE(module)->name,
                     GTK_SAT_MODULE(module)->cfgdata);

        if (i == 0)
        {
            buff = g_strdup(GTK_SAT_MODULE(module)->name);
        }
        else
        {
            buff = g_strconcat(mods, ";", GTK_SAT_MODULE(module)->name, NULL);
            g_free(mods);
        }

        mods = g_strdup(buff);
        g_free(buff);
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Stored %s"),
                    __func__, GTK_SAT_MODULE(module)->name);
    }

    /* store the currently open page number */
    page = gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook));

    sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Saved states for %d modules."),
                __func__, num);

    sat_cfg_set_str(SAT_CFG_STR_OPEN_MODULES, mods);
    sat_cfg_set_int(SAT_CFG_INT_MODULE_CURRENT_PAGE, page);

    g_free(mods);
}

gboolean mod_mgr_mod_is_visible(GtkWidget * module)
{
    gint            page;
    gboolean        retcode = TRUE;

    /* get page number for this module */
    page = gtk_notebook_page_num(GTK_NOTEBOOK(nbook), module);

    if (page != -1)
    {
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook)) == page)
        {
            retcode = TRUE;
        }
        else
        {
            retcode = FALSE;
        }
    }
    else
    {
        retcode = FALSE;
    }

    return retcode;
}

/*
 * Dock a module into the notebook.
 *
 * This function inserts the module into the notebook but does not add it
 * to the list of modules, since it should already be there.
 *
 * The function does some sanity checks to ensure the the module actually
 * is in the internal list of modules and also that the module is not
 * already present in the notebook. If any of these checks fail, the function
 * will send an error message and try to recover.
 *
 * The function does not modify the internal state of the module, module->state,
 * that is up to the module itself.
 */
gint mod_mgr_dock_module(GtkWidget * module)
{
    gint            retcode = 0;
    gint            page;

    if (!g_slist_find(modules, module))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s not found in list. Trying to recover."),
                    __func__, GTK_SAT_MODULE(module)->name);
        modules = g_slist_append(modules, module);
    }

    page = gtk_notebook_page_num(GTK_NOTEBOOK(nbook), module);
    if (page != -1)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s already in notebook!"),
                    __func__, GTK_SAT_MODULE(module)->name);
        retcode = 1;
    }
    else
    {
        /* add module to notebook */
        page = gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                                        module,
                                        gtk_label_new(GTK_SAT_MODULE(module)->
                                                      name));

        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Docked %s into notebook (page %d)"),
                    __func__, GTK_SAT_MODULE(module)->name, page);

        retcode = 0;
    }

    /* disable tabs if only one page in notebook */
    if ((gtk_notebook_get_n_pages(GTK_NOTEBOOK(nbook))) == 1)
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), FALSE);
    }
    else
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), TRUE);
    }

    /* update window title */
    update_window_title();

    return retcode;
}

/*
 * Undock module from notebook
 *
 * This function removes module from the notebook without removing it from
 * the internal list of modules.
 *
 * The function does some sanity checks to ensure that the module actually
 * exists in the mod-mgr, if not it will add module to the internal list
 * and raise a warning.
 *
 * The function does not modify the internal state of the module, module->state,
 * that is up to the module itself.
 *
 * \note The module itself is responsible for temporarily incrementing the
 *       reference count of the widget in order to avoid destruction when
 *       removing from the notebook.
 */
gint mod_mgr_undock_module(GtkWidget * module)
{
    gint            retcode = 0;
    gint            page;

    if (!g_slist_find(modules, module))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s not found in list. Trying to recover."),
                    __func__, GTK_SAT_MODULE(module)->name);
        modules = g_slist_append(modules, module);
    }

    page = gtk_notebook_page_num(GTK_NOTEBOOK(nbook), module);
    if (page == -1)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s does not seem to be docked!"),
                    __func__, GTK_SAT_MODULE(module)->name);
        retcode = 1;
    }
    else
    {

        gtk_notebook_remove_page(GTK_NOTEBOOK(nbook), page);

        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Removed %s from notebook page %d."),
                    __func__, GTK_SAT_MODULE(module)->name, page);

        retcode = 0;
    }

    /* disable tabs if only one page in notebook */
    if ((gtk_notebook_get_n_pages(GTK_NOTEBOOK(nbook))) == 1)
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), FALSE);
    }
    else
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), TRUE);
    }

    /* update window title */
    update_window_title();

    return retcode;
}

static void update_window_title()
{
    gint            pgn, num;
    GtkWidget      *pg;
    gchar          *title;

    /* get number of pages */
    num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(nbook));

    if (num == 0)
    {
        gtk_window_set_title(GTK_WINDOW(app), _("Gpredict: (none)"));
    }
    else
    {
        pgn = gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook));
        pg = gtk_notebook_get_nth_page(GTK_NOTEBOOK(nbook), pgn);
        title = g_strdup_printf(_("Gpredict: %s"),
                                gtk_notebook_get_tab_label_text(GTK_NOTEBOOK
                                                                (nbook), pg));
        gtk_window_set_title(GTK_WINDOW(app), title);
        g_free(title);
    }
}

static void switch_page_cb(GtkNotebook * notebook,
                           gpointer * page, guint page_num, gpointer user_data)
{
    GtkWidget      *pg;
    gchar          *title;

    (void)notebook;
    (void)page;
    (void)user_data;

    pg = gtk_notebook_get_nth_page(GTK_NOTEBOOK(nbook), page_num);
    title = g_strdup_printf(_("Gpredict: %s"),
                            gtk_notebook_get_tab_label_text(GTK_NOTEBOOK
                                                            (nbook), pg));
    gtk_window_set_title(GTK_WINDOW(app), title);
    g_free(title);
}

void mod_mgr_reload_sats()
{
    guint           num;
    guint           i;
    GtkSatModule   *mod;

    if (!nbook)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Attempt to reload sats but mod-mgr is NULL?"),
                    __func__);
        return;
    }

    num = g_slist_length(modules);
    if (num == 0)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: No modules need to reload sats."), __func__);
        return;
    }

    /* for each module in the GSList execute sat_module_reload_sats() */
    for (i = 0; i < num; i++)
    {
        mod = GTK_SAT_MODULE(g_slist_nth_data(modules, i));
        gtk_sat_module_reload_sats(mod);
    }
}

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
                                   MOD_CFG_GLOBAL_SECTION,
                                   MOD_CFG_WIN_WIDTH, NULL);
    }
    else
    {
        w = aloc.width;
    }
    if (g_key_file_has_key(GTK_SAT_MODULE(module)->cfgdata,
                           MOD_CFG_GLOBAL_SECTION, MOD_CFG_WIN_HEIGHT, NULL))
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
    //g_object_ref (module);

    /* we don't need the positions */
    //GTK_SAT_MODULE (module)->vpanedpos = -1;
    //GTK_SAT_MODULE (module)->hpanedpos = -1;

    /* undock from mod-mgr */
    //mod_mgr_undock_module (module);

    /* create window */
    GTK_SAT_MODULE(module)->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    title = g_strconcat(_("Gpredict: "),
                        GTK_SAT_MODULE(module)->name,
                        " (", GTK_SAT_MODULE(module)->qth->name, ")", NULL);
    gtk_window_set_title(GTK_WINDOW(GTK_SAT_MODULE(module)->win), title);
    g_free(title);
    gtk_window_set_default_size(GTK_WINDOW(GTK_SAT_MODULE(module)->win), w, h);
    g_signal_connect(G_OBJECT(GTK_SAT_MODULE(module)->win), "configure_event",
                     G_CALLBACK(module_window_config_cb), module);

    icon = logo_file_name("gpredict_icon_color.svg");
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

    gtk_container_add(GTK_CONTAINER(GTK_SAT_MODULE(module)->win), module);
    gtk_widget_show_all(GTK_SAT_MODULE(module)->win);

    /* reparent time manager window if visible */
    if (GTK_SAT_MODULE(module)->tmgActive)
    {
        gtk_window_set_transient_for(GTK_WINDOW
                                     (GTK_SAT_MODULE(module)->tmgWin),
                                     GTK_WINDOW(GTK_SAT_MODULE(module)->win));
    }
}
