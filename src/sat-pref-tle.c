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

#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-tle.h"
#include "tle-update.h"


extern GtkWidget *window;       /* dialog window defined in sat-pref.c */

/* Update frequency widget */
static GtkWidget *freq;

/* auto update radio buttons */
static GtkWidget *warn, *autom;

/* internet updates */
static GtkWidget *proxy, *tle_url_view, *add_tle, *del_tle;

/* add new sats */
static GtkWidget *addnew;
static gboolean dirty = FALSE;
static gboolean reset = FALSE;


/*
 * Configuration values changed.
 *
 * This function is called when one of the configuration values are
 * modified. It sets the dirty flag to TRUE to indicate that the 
 * configuration values should be stored in case th euser presses
 * the OK button.
 */
static void value_changed_cb(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    dirty = TRUE;
}

/* Create widgets for auto-update configuration */
static void create_auto_update(GtkWidget * vbox)
{
    GtkWidget      *label;
    GtkWidget      *box;
    guint           i;

    /* auto update */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Auto-Update:</b>"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

    /* frequency */
    freq = gtk_combo_box_text_new();
    for (i = 0; i < TLE_AUTO_UPDATE_NUM; i++)
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(freq),
                                       tle_update_freq_to_str(i));
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(freq),
                             sat_cfg_get_int(SAT_CFG_INT_TLE_AUTO_UPD_FREQ));
    g_signal_connect(freq, "changed", G_CALLBACK(value_changed_cb), NULL);

    label = gtk_label_new(_("Check the age of TLE data:"));

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), freq, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, TRUE, 0);

    /* radio buttons selecting action */
    label = gtk_label_new(_("If TLEs are too old:"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
    warn = gtk_radio_button_new_with_label(NULL, _("Notify me"));
    gtk_box_pack_start(GTK_BOX(vbox), warn, FALSE, TRUE, 0);
    autom = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(warn),
                                                        _
                                                        ("Perform automatic update in the background"));
    gtk_box_pack_start(GTK_BOX(vbox), autom, FALSE, TRUE, 0);

    /* warn is selected automatically by default */
    if (sat_cfg_get_int(SAT_CFG_INT_TLE_AUTO_UPD_ACTION) ==
        TLE_AUTO_UPDATE_GOAHEAD)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autom), TRUE);
    }

    g_signal_connect(warn, "toggled", G_CALLBACK(value_changed_cb), NULL);
    g_signal_connect(autom, "toggled", G_CALLBACK(value_changed_cb), NULL);
}

/* Callback function called when an URL entry in the list has been edited */
static void url_edited_cb(GtkCellRendererText * renderer, gchar * path,
                          gchar * new_text, GtkListStore * tle_store)
{
    GtkTreeIter     iter;

    (void)renderer;

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(tle_store), &iter,
                                        path);
    gtk_list_store_set(tle_store, &iter, 0, new_text, -1);
    dirty = TRUE;
}

/* Add TLE button clicked */
static void add_tle_cb(GtkWidget * button, GtkListStore * tle_store)
{
    GtkTreeView    *tv = GTK_TREE_VIEW(tle_url_view);
    GtkTreePath    *path;
    GtkTreeIter     iter;
    GtkTreeSelection *selection;

    (void)button;

    gtk_list_store_append(tle_store, &iter);
    gtk_list_store_set(tle_store, &iter, 0, "http://server.com/file.txt", -1);

    /* activate the newly added row */
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(tle_store), &iter);
    selection = gtk_tree_view_get_selection(tv);
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_view_scroll_to_cell(tv, path, NULL, FALSE, 0.0, 0.0);

    /* TODO: Enable editing on row */

    dirty = TRUE;
}

/* Delete TLE button clicked */
static void del_tle_cb(GtkWidget * button, gpointer data)
{
    (void)button;
    (void)data;

    GtkTreeView    *tv = GTK_TREE_VIEW(tle_url_view);
    GtkTreeModel   *model = gtk_tree_view_get_model(tv);
    GtkTreeModel   *selmod;
    GtkTreeSelection *selection;
    GtkTreeIter     iter;

    /* if this is the only entry, tell user that it is not possible to delete */
    if (gtk_tree_model_iter_n_children(model, NULL) < 2)
    {
        GtkWidget      *dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        _("Can not delete TLE source!\n\n"
                                          "You should have at least one TLE source configured, "
                                          "otherwise Gpredict may not work correctly.\n\n"
                                          "If you want to modify the TLE source, click twice "
                                          "on the row to edit it."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    else
    {
        /* get selected row
           FIXME: do we really need to work with two models?
         */
        selection = gtk_tree_view_get_selection(tv);
        if (gtk_tree_selection_get_selected(selection, &selmod, &iter))
            gtk_list_store_remove(GTK_LIST_STORE(selmod), &iter);

        dirty = TRUE;
    }
}

static GtkWidget *create_tle_buttons(GtkListStore * tle_store)
{
    GtkWidget      *box;

    add_tle = gtk_button_new_with_label(_("Add TLE source"));
    gtk_widget_set_tooltip_text(add_tle,
                                _("Add a new TLE source to the list"));
    g_signal_connect(add_tle, "clicked", (GCallback) add_tle_cb, tle_store);

    del_tle = gtk_button_new_with_label(_("Delete TLE source"));
    gtk_widget_set_tooltip_text(del_tle, _("Delete the selected TLE source"));
    g_signal_connect(del_tle, "clicked", (GCallback) del_tle_cb, NULL);

    box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_START);
    gtk_container_add(GTK_CONTAINER(box), add_tle);
    gtk_container_add(GTK_CONTAINER(box), del_tle);

    return box;
}

/* Update TLE URL view with URLs from tle_url_str */
static void set_tle_urls(const gchar * tle_url_str)
{
    if (!tle_url_str)
        return;

    gchar         **tle_url_strv = g_strsplit(tle_url_str, ";", -1);

    if (g_strv_length(tle_url_strv) > 0)
    {
        GtkTreeIter     iter;
        GtkListStore   *tle_store;
        unsigned int    i;

        tle_store =
            GTK_LIST_STORE(gtk_tree_view_get_model
                           (GTK_TREE_VIEW(tle_url_view)));
        gtk_list_store_clear(tle_store);
        for (i = 0; i < g_strv_length(tle_url_strv); i++)
        {
            gtk_list_store_append(tle_store, &iter);
            gtk_list_store_set(tle_store, &iter, 0, tle_url_strv[i], -1);
        }
    }
    g_strfreev(tle_url_strv);
}

/* Create widgets for network update configuration */
static void create_network(GtkWidget * vbox)
{
    GtkWidget      *swin;
    GtkWidget      *table;
    GtkWidget      *label;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore   *tle_store;

    /* auto update */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         _("<b>Update from the Internet:</b>"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(table), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(table), 5);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);

    label = gtk_label_new(_("Proxy server:"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    proxy = gtk_entry_new();
    if (sat_cfg_get_str(SAT_CFG_STR_TLE_PROXY))
        gtk_entry_set_text(GTK_ENTRY(proxy),
                           sat_cfg_get_str(SAT_CFG_STR_TLE_PROXY));
    gtk_widget_set_tooltip_text(proxy,
                                _("Enter URL for local proxy server. e.g.\n"
                                  "http://my.proxy.com"));
    g_signal_connect(proxy, "changed", G_CALLBACK(value_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(table), proxy, 1, 0, 5, 1);

    /* URLs */
    label = gtk_label_new(_("TLE sources:"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);

    tle_store = gtk_list_store_new(1, G_TYPE_STRING);
    tle_url_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tle_store));
    g_object_unref(tle_store);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tle_url_view), FALSE);
    gtk_widget_set_tooltip_text(tle_url_view,
                                _
                                ("List of files with TLE data to retrieve from "
                                 "the Internet\nClick twice on a row to edit"));

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", G_CALLBACK(url_edited_cb), tle_store);
    column = gtk_tree_view_column_new_with_attributes("Invisible title :)",
                                                      renderer, "text", 0,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tle_url_view), column);

    /* set URL data */
    gchar          *tle_url_str = sat_cfg_get_str(SAT_CFG_STR_TLE_URLS);

    set_tle_urls(tle_url_str);
    g_free(tle_url_str);

    /* scrolled window to contain the URL list */
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swin), tle_url_view);
    gtk_grid_attach(GTK_GRID(table), swin, 1, 1, 5, 3);
    gtk_grid_attach(GTK_GRID(table), create_tle_buttons(tle_store),
                    1, 4, 4, 1);

    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
}

/* Create widgets for network update configuration */
static void create_misc(GtkWidget * vbox)
{
#define ADBUT_TEXT  N_("Add new satellites to local database")
#define ADBUT_TIP   N_("Note that selecting this option may cause previously " \
                       "deleted satellites to be re-added to the database.")

    addnew = gtk_check_button_new_with_label(ADBUT_TEXT);
    gtk_widget_set_tooltip_text(addnew, ADBUT_TIP);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(addnew),
                                 sat_cfg_get_bool(SAT_CFG_BOOL_TLE_ADD_NEW));
    g_signal_connect(addnew, "toggled", G_CALLBACK(value_changed_cb), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), addnew, FALSE, TRUE, 0);

}

/*
 * Reset settings.
 *
 * @param button The RESET button.
 * @param data User data (unused).
 *
 * This function is called when the user clicks on the RESET button. The function
 * will get the default values for the parameters and set the dirty and reset flags
 * appropriately. The reset will not have any effect if the user cancels the
 * dialog.
 */
static void reset_cb(GtkWidget * button, gpointer data)
{
    (void)button;
    (void)data;
    /* get defaults */

    /* update frequency */
    gtk_combo_box_set_active(GTK_COMBO_BOX(freq),
                             sat_cfg_get_int_def
                             (SAT_CFG_INT_TLE_AUTO_UPD_FREQ));

    /* action */
    if (sat_cfg_get_int_def(SAT_CFG_INT_TLE_AUTO_UPD_ACTION) ==
        TLE_AUTO_UPDATE_GOAHEAD)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autom), TRUE);
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(warn), TRUE);
    }


    /* proxy */
    gtk_entry_set_text(GTK_ENTRY(proxy), "");

    /* URLS */
    gchar          *tle_url_str = sat_cfg_get_str_def(SAT_CFG_STR_TLE_URLS);

    set_tle_urls(tle_url_str);
    g_free(tle_url_str);

    /* add new sats */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(addnew),
                                 sat_cfg_get_bool_def
                                 (SAT_CFG_BOOL_TLE_ADD_NEW));

    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}

/*
 * Create RESET button.
 *
 * @param cfg Config data or NULL in global mode.
 * @param vbox The container.
 *
 * This function creates and sets up the view selector combos.
 */
static void create_reset_button(GtkBox * vbox)
{
    GtkWidget      *button;
    GtkWidget      *butbox;

    button = gtk_button_new_with_label(_("Reset"));
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(reset_cb), NULL);

    gtk_widget_set_tooltip_text(button,
                                _("Reset settings to the default values."));

    butbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end(GTK_BOX(butbox), button, FALSE, TRUE, 10);

    gtk_box_pack_end(vbox, butbox, FALSE, TRUE, 0);
}

/* User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_tle_cancel()
{
    dirty = FALSE;
    reset = FALSE;
}

/* User pressed OK. Any changes should be stored in config. */
void sat_pref_tle_ok()
{
    if (dirty)
    {
        /* save settings */

        /* update frequency */
        sat_cfg_set_int(SAT_CFG_INT_TLE_AUTO_UPD_FREQ,
                        gtk_combo_box_get_active(GTK_COMBO_BOX(freq)));

        /* action to take */
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(autom)))
            sat_cfg_set_int(SAT_CFG_INT_TLE_AUTO_UPD_ACTION,
                            TLE_AUTO_UPDATE_GOAHEAD);
        else
            sat_cfg_set_int(SAT_CFG_INT_TLE_AUTO_UPD_ACTION,
                            TLE_AUTO_UPDATE_NOTIFY);

        /* proxy */
        if (gtk_entry_get_text_length(GTK_ENTRY(proxy)) > 0)
            sat_cfg_set_str(SAT_CFG_STR_TLE_PROXY,
                            gtk_entry_get_text(GTK_ENTRY(proxy)));
        else
            sat_cfg_reset_str(SAT_CFG_STR_TLE_PROXY);

        /* URLS */
        gsize           num;
        guint           i;
        gchar          *url, *buff, *all_urls = NULL;
        GtkTreeModel   *model;
        GtkTreeIter     iter;

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(tle_url_view));
        num = gtk_tree_model_iter_n_children(model, NULL);
        for (i = 0; i < num; i++)
        {
            if (gtk_tree_model_iter_nth_child(model, &iter, NULL, i))
            {
                gtk_tree_model_get(model, &iter, 0, &url, -1);
                if (i > 0)
                {
                    buff = g_strdup_printf("%s;%s", all_urls, url);
                    g_free(all_urls);
                }
                else
                {
                    buff = g_strdup_printf("%s", url);
                }
                all_urls = g_strdup(buff);
                g_free(buff);
                g_free(url);
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s:%s: Error fetching entry %d in URL list"),
                            __FILE__, __func__, i);
            }

        }
        sat_cfg_set_str(SAT_CFG_STR_TLE_URLS, all_urls);
        g_free(all_urls);

        dirty = FALSE;
    }
    else if (reset)
    {
        /* use sat_cfg_reset */
        sat_cfg_reset_int(SAT_CFG_INT_TLE_AUTO_UPD_FREQ);
        sat_cfg_reset_int(SAT_CFG_INT_TLE_AUTO_UPD_ACTION);
        sat_cfg_reset_str(SAT_CFG_STR_TLE_PROXY);
        sat_cfg_reset_str(SAT_CFG_STR_TLE_URLS);
        sat_cfg_reset_bool(SAT_CFG_BOOL_TLE_ADD_NEW);

        reset = FALSE;
    }
}

GtkWidget      *sat_pref_tle_create()
{
    GtkWidget      *vbox;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);

    create_auto_update(vbox);
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, TRUE, 0);
    create_network(vbox);
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, TRUE, 0);
    create_misc(vbox);

    create_reset_button(GTK_BOX(vbox));

    return vbox;
}
