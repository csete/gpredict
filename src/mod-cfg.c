/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-selector.h"
#include "mod-cfg.h"
#include "qth-editor.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-modules.h"


extern GtkWidget *app;

/* private widgets */
static GtkWidget *namew;        /* GtkEntry widget for module name */
static GtkWidget *locw;         /* GtkComboBox for location selection */
static GtkWidget *satlist;      /* list of selected satellites */


static gint qth_name_compare(const gchar * a, const gchar * b)
{
    return (gpredict_strcmp(a, b));
}

/* warn if module is about to be overwritten; returns TRUE if users says OK */
static gboolean overwrite_existing_module(void)
{
    gboolean        overwrite;
    GtkWidget      *warn;

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Already have module %s. Ask user to confirm..."),
                __func__, gtk_entry_get_text(GTK_ENTRY(namew)));

    warn = gtk_message_dialog_new(GTK_WINDOW(app),
                                  GTK_DIALOG_MODAL |
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                  _("There is already a module called %s.\n"
                                    "Do you want to overwrite this module?"),
                                  gtk_entry_get_text(GTK_ENTRY(namew)));

    switch (gtk_dialog_run(GTK_DIALOG(warn)))
    {
    case GTK_RESPONSE_YES:
        overwrite = TRUE;
        break;
    default:
        overwrite = FALSE;
        break;
    }
    gtk_widget_destroy(warn);

    return overwrite;
}


/* Add a satellite to the list of selected satellites */
static void add_selected_sat(GtkListStore * store, gint catnum)
{
    gint            i, sats = 0;
    GtkTreeIter     iter;
    GtkTreeIter     node;       /* new top level node added to the tree store */
    gint            catnr;
    gboolean        found = FALSE;
    sat_t           sat;

    /* check if the satellite is already in the list */

    /* get number of satellites already in list */
    sats = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);

    /* loop over list entries and check their catnums */
    for (i = 0; i < sats; i++)
    {
        if (gtk_tree_model_iter_nth_child
            (GTK_TREE_MODEL(store), &iter, NULL, i))
        {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                               GTK_SAT_SELECTOR_COL_CATNUM, &catnr, -1);
            if (catnum == catnr)
            {
                found = TRUE;
            }
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%s: Could not fetch entry %d in satellite list"),
                        __FILE__, __func__, i);
        }

    }

    if (found)
        return;

    /* if we have made it so far, satellite is not in list */

    /* Get satellite data */
    if (gtk_sat_data_read_sat(catnum, &sat))
    {
        /* error */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Error reading satellite %d."),
                    __FILE__, __func__, catnum);
    }
    else
    {
        /* insert satellite into liststore */
        gtk_list_store_append(store, &node);
        gtk_list_store_set(store, &node,
                           GTK_SAT_SELECTOR_COL_NAME, sat.nickname,
                           GTK_SAT_SELECTOR_COL_CATNUM, catnum,
                           GTK_SAT_SELECTOR_COL_EPOCH, sat.jul_epoch, -1);
        g_free(sat.name);
        g_free(sat.nickname);
    }
}

/* Signal handler for "->" button signals */
static void addbut_clicked_cb(GtkButton * button, GtkSatSelector * selector)
{
    GtkListStore   *store;
    gint            catnum = 0;
    gchar          *name;
    gdouble         epoch;

    (void)button;

    /* get the selected row in the treeview */
    gtk_sat_selector_get_selected(selector, &catnum, &name, &epoch);

    if (catnum)
    {
        /* Add satellite to selected list */
        store =
            GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(satlist)));
        add_selected_sat(store, catnum);
        /*tell the sat_selector to hide that satellite */
        gtk_sat_selector_mark_selected(selector, catnum);
    }
}

/* Signal handler for "<-" button signals */
static void delbut_clicked_cb(GtkButton * button, GtkSatSelector * selector)
{
    GtkTreeSelection *selection;
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    gboolean        haveselection = FALSE;      /* this flag is set to TRUE if there is a selection */
    gint            catnr;

    (void)button;

    /* get the selected row in the treeview */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(satlist));
    haveselection = gtk_tree_selection_get_selected(selection, &model, &iter);

    if (haveselection)
    {
        gtk_tree_model_get(model, &iter, GTK_SAT_SELECTOR_COL_CATNUM, &catnr,
                           -1);
        /*tell the sat_selector it can show that satellite again */
        gtk_sat_selector_mark_unselected(selector, catnr);
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
}

static void sat_activated_cb(GtkSatSelector * selector, gint catnr,
                             gpointer data)
{
    GtkListStore   *store;

    (void)data;

    /* Add satellite to selected list */
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(satlist)));
    add_selected_sat(store, catnr);
    /*tell the sat_selector it can hide that satellite */
    gtk_sat_selector_mark_selected(selector, catnr);
}


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

static void add_qth_cb(GtkWidget * button, gpointer data)
{
    GtkWindow      *parent = GTK_WINDOW(data);
    GtkResponseType response;
    qth_t           qth;

    (void)button;               /* avoid unused parameter compiler warning */

    qth.name = NULL;
    qth.loc = NULL;
    qth.desc = NULL;
    qth.wx = NULL;
    qth.qra = NULL;
    qth.data = NULL;

    response = qth_editor_run(&qth, parent);

    if (response == GTK_RESPONSE_OK)
    {
        gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(locw), qth.name);
        gtk_combo_box_set_active(GTK_COMBO_BOX(locw), 0);

        /* clean up */
        g_free(qth.name);
        if (qth.loc != NULL)
            g_free(qth.loc);
        if (qth.desc != NULL)
            g_free(qth.desc);
        if (qth.wx != NULL)
            g_free(qth.wx);
        if (qth.qra != NULL)
            g_free(qth.qra);
    }
}

static GtkWidget *create_loc_selector(GKeyFile * cfgdata)
{
    GtkWidget      *combo;
    GDir           *dir = NULL;
    GError         *error = NULL;
    gchar          *dirname;
    const gchar    *filename;
    gchar          *defqth = NULL;
    gchar          *defqthshort = NULL;
    gchar         **buffv;
    gint            idx = -1;
    gint            count = 0;
    GSList         *qths = NULL;
    gchar          *qthname;
    gint            i, n;

    combo = gtk_combo_box_text_new();

    /* get qth file name from cfgdata; if cfg data has no QTH
       set defqth = "** DEFAULT **"
     */

    if (g_key_file_has_key
        (cfgdata, MOD_CFG_GLOBAL_SECTION, MOD_CFG_QTH_FILE_KEY, NULL))
    {
        defqth = g_key_file_get_string(cfgdata,
                                       MOD_CFG_GLOBAL_SECTION,
                                       MOD_CFG_QTH_FILE_KEY, &error);
        buffv = g_strsplit(defqth, ".qth", 0);
        defqthshort = g_strdup(buffv[0]);

        g_strfreev(buffv);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Module has no QTH; use default."), __func__);

        defqth = g_strdup(_("** DEFAULT **"));
        defqthshort = g_strdup(defqth);
    }


    /* scan for .qth files in the user config directory and
       add the contents of each .qth file to the list store
     */
    dirname = get_user_conf_dir();
    dir = g_dir_open(dirname, 0, &error);

    if (dir)
    {
        while ((filename = g_dir_read_name(dir)))
        {
            /*create a sorted list then use it to load the combo box */
            if (g_str_has_suffix(filename, ".qth"))
            {

                buffv = g_strsplit(filename, ".qth", 0);
                qths =
                    g_slist_insert_sorted(qths, g_strdup(buffv[0]),
                                          (GCompareFunc) qth_name_compare);
                g_strfreev(buffv);
            }
        }
        n = g_slist_length(qths);
        for (i = 0; i < n; i++)
        {
            qthname = g_slist_nth_data(qths, i);
            if (qthname)
            {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo),
                                               qthname);

                /* is this the QTH for this module? */
                /* comparison uses short name full filename */
                if (!g_ascii_strcasecmp(defqthshort, qthname))
                {
                    idx = count;
                }
                g_free(qthname);
                count++;
            }
        }
        g_slist_free(qths);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open user cfg dir %s (%s)"),
                    __FILE__, __LINE__, dirname, error->message);
        g_clear_error(&error);
    }

    /* finally, add "** DEFAULT **" string; secting this will
       clear the MOD_CFG_QTH_FILE_KEY module configuration
       key ensuring that the module will use the default QTH
     */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo),
                                   _("** DEFAULT **"));

    /* select the qth of this module;
       if idx == -1 we should select the "** DEFAULT **"
       string corresponding to index = count
     */
    if (idx == -1)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), count);
    }
    else
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), idx);
    }

    g_free(defqth);
    g_free(defqthshort);
    g_free(dirname);
    g_dir_close(dir);

    return combo;
}

/**
 * Compare two rows of the GtkSatSelector.
 *
 * @param model The tree model of the GtkSatSelector.
 * @param a The first row.
 * @param b The second row.
 * @param userdata Not used.
 *
 * This function is used by the sorting algorithm to compare two rows of the
 * GtkSatSelector widget. The unctions works by comparing the character strings
 * in the name column.
 */
static gint compare_func(GtkTreeModel * model,
                         GtkTreeIter * a, GtkTreeIter * b, gpointer userdata)
{
    gchar          *sat1, *sat2;
    gint            catnr1, catnr2;
    gint            ret = 0;

    (void)userdata;

    gtk_tree_model_get(model, a, GTK_SAT_SELECTOR_COL_NAME, &sat1, -1);
    gtk_tree_model_get(model, b, GTK_SAT_SELECTOR_COL_NAME, &sat2, -1);

    ret = gpredict_strcmp(sat1, sat2);

    if (ret == 0)
    {
        gtk_tree_model_get(model, a, GTK_SAT_SELECTOR_COL_CATNUM, &catnr1, -1);
        gtk_tree_model_get(model, b, GTK_SAT_SELECTOR_COL_CATNUM, &catnr2, -1);
        if (catnr1 < catnr2)
            ret = -1;
        else if (catnr1 > catnr2)
            ret = 1;
        else
            /* never supposed to happen */
            ret = 0;
    }

    g_free(sat1);
    g_free(sat2);

    return ret;
}


/**
 * Signal handler for managing satellite selection.
 *
 * @param view Pointer to the GtkTreeView object.
 * @param path The path of the row that was activated.
 * @param column The column where the activation occured.
 * @param data Pointer to the GtkSatselector widget.
 *
 * This function is called when the user double clicks on a satellite in the
 * list of satellites. The double clicked satellite is removed from the list.
 */
static void row_activated_cb(GtkTreeView * view, GtkTreePath * path,
                             GtkTreeViewColumn * column, gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    gboolean        haveselection = FALSE;      /* this flag is set to TRUE if there is a selection */
    gint            catnr;
    GtkSatSelector *selector = GTK_SAT_SELECTOR(data);

    (void)path;
    (void)column;

    /* get the selected row in the treeview */
    selection = gtk_tree_view_get_selection(view);
    haveselection = gtk_tree_selection_get_selected(selection, &model, &iter);

    if (haveselection)
    {
        gtk_tree_model_get(model, &iter, GTK_SAT_SELECTOR_COL_CATNUM, &catnr,
                           -1);
        /*tell the sat_selector it can show that satellite again */
        gtk_sat_selector_mark_unselected(selector, catnr);
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
}

/**
 * Create the list containing the selected satellites.
 *
 * @param new Flag indicating whether the module config window is for a new module.
 * @returns A newly created GtkTreeView widget.
 *
 */
static GtkWidget *create_selected_sats_list(GKeyFile * cfgdata, gboolean new,
                                            GtkSatSelector * selector)
{
    GtkWidget      *satlist;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore   *store;

    satlist = gtk_tree_view_new();
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Selected Satellites"),
                                                      renderer, "text",
                                                      GTK_SAT_SELECTOR_COL_NAME,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(satlist), column, -1);
    gtk_tree_view_column_set_visible(column, TRUE);

    /* catalogue number */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Catnum"), renderer,
                                                      "text",
                                                      GTK_SAT_SELECTOR_COL_CATNUM,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(satlist), column, -1);
    gtk_tree_view_column_set_visible(column, TRUE);

    /* epoch */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Epoch"), renderer,
                                                      "text",
                                                      GTK_SAT_SELECTOR_COL_EPOCH,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(satlist), column, -1);
    gtk_tree_view_column_set_visible(column, FALSE);

    /* "row-activated" signal is used to catch double click events, which means
       a satellite has been selected. This will cause the satellite to be deleted */
    g_signal_connect(GTK_TREE_VIEW(satlist), "row-activated",
                     G_CALLBACK(row_activated_cb), selector);

    /* create the model */
    store = gtk_list_store_new(GTK_SAT_SELECTOR_COL_NUM, G_TYPE_STRING, // name
                               G_TYPE_INT,      // catnum
                               G_TYPE_DOUBLE,   // epoch
                               G_TYPE_BOOLEAN   // selected 
        );

    /* sort the list by name */
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),
                                    GTK_SAT_SELECTOR_COL_NAME,
                                    compare_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
                                         GTK_SAT_SELECTOR_COL_NAME,
                                         GTK_SORT_ASCENDING);

    /* If we are editing an existing module, load the satellites into the list */
    if (!new)
    {
        gint           *sats = NULL;
        gsize           length;
        GError         *error = NULL;
        guint           i;

        sats = g_key_file_get_integer_list(cfgdata,
                                           MOD_CFG_GLOBAL_SECTION,
                                           MOD_CFG_SATS_KEY, &length, &error);

        if (error != NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Failed to get list of satellites (%s)"),
                        __func__, error->message);

            g_clear_error(&error);

            /* GLib API says nothing about the contents in case of error */
            if (sats)
            {
                g_free(sats);
            }

        }
        else
        {
            for (i = 0; i < length; i++)
            {
                add_selected_sat(store, sats[i]);
                gtk_sat_selector_mark_selected(selector, sats[i]);
            }
            g_free(sats);
        }

    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(satlist), GTK_TREE_MODEL(store));
    g_object_unref(store);

    return satlist;
}

static GtkWidget *mod_cfg_editor_create(const gchar * modname,
                                        GKeyFile * cfgdata,
                                        GtkWidget * toplevel)
{
    GtkWidget      *dialog;
    GtkWidget      *add;
    GtkWidget      *grid;
    GtkWidget      *label;
    GtkWidget      *swin;
    GtkWidget      *addbut, *delbut;
    GtkWidget      *vbox;
    gchar          *strbuf;
    GtkWidget      *frame;

    gboolean        new = (modname != NULL) ? FALSE : TRUE;

    if (new)
        strbuf = g_strdup(_("Create new module"));
    else
        strbuf = g_strdup(_("Edit module"));

    dialog = gtk_dialog_new_with_buttons(strbuf, GTK_WINDOW(toplevel),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Properties", GTK_RESPONSE_HELP,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_OK", GTK_RESPONSE_OK, NULL);
    g_free(strbuf);

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    /* window icon */
    strbuf = icon_file_name("gpredict-sat-pref.png");
    if (g_file_test(strbuf, G_FILE_TEST_EXISTS))
        gtk_window_set_icon_from_file(GTK_WINDOW(dialog), strbuf, NULL);
    g_free(strbuf);

    /* module name */
    namew = gtk_entry_new();

    if (!new)
    {
        /* set module name and make entry insensitive */
        gtk_entry_set_text(GTK_ENTRY(namew), modname);
        gtk_widget_set_sensitive(namew, FALSE);
    }
    else
    {
        /* connect changed signal to text-checker */
        gtk_entry_set_max_length(GTK_ENTRY(namew), 25);
        gtk_widget_set_tooltip_text(namew,
                                    _("Enter a short name for this module.\n"
                                      "Allowed characters: 0..9, a..z, A..Z, - and _"));
        /*new tooltip api does not allow for private 
           _("The name will be used to identify the module "             \
           "and it is also used a file name for saving the data."        \
           "Max length is 25 characters."));
         */
        /* attach changed signal so that we can enable OK button when
           a proper name has been entered
           oh, btw. disable OK button to begin with....
         */
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_OK, FALSE);
        g_signal_connect(namew, "changed", G_CALLBACK(name_changed), dialog);

    }

    /* ground station selector */
    locw = create_loc_selector(cfgdata);
    gtk_widget_set_tooltip_text(locw,
                                _("Select a ground station for this module."));

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);

    label = gtk_label_new(_("Module name"));
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), namew, 2, 0, 4, 1);

    label = gtk_label_new(_("Ground station"));
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), locw, 2, 1, 2, 1);

    /* add button */
    add = gtk_button_new_with_label(_("Add"));
    gtk_widget_set_tooltip_text(add, _("Add a new ground station"));
    g_signal_connect(add, "clicked", G_CALLBACK(add_qth_cb), dialog);
    gtk_grid_attach(GTK_GRID(grid), add, 4, 1, 1, 1);

    vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 10);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Satellites</b>"));
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);

    /* satellite selector */
    GtkWidget      *selector = gtk_sat_selector_new(0);

    g_signal_connect(selector, "sat-activated",
                     G_CALLBACK(sat_activated_cb), NULL);

    /* list of selected satellites */
    satlist = create_selected_sats_list(cfgdata, new,
                                        GTK_SAT_SELECTOR(selector));
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swin), satlist);

    /* Add and Delete buttons */
    addbut = gtk_button_new_with_label(" --> ");
    gtk_widget_set_tooltip_text(addbut, _("Add satellite to list"));
    g_signal_connect(addbut, "clicked", G_CALLBACK(addbut_clicked_cb),
                     selector);

    delbut = gtk_button_new_with_label(" <-- ");
    gtk_widget_set_tooltip_text(delbut, _("Delete satellite from list"));
    g_signal_connect(delbut, "clicked", G_CALLBACK(delbut_clicked_cb),
                     selector);

    /* quick sat selector tutorial label */
    label = gtk_label_new(_("Double click on a satellite "
                            "to move it to the other box."));
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    g_object_set(label, "xalign", 0.0f, "yalign", 1.0f, NULL);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), swin);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_attach(GTK_GRID(grid), selector, 0, 0, 4, 8);
    gtk_grid_attach(GTK_GRID(grid), addbut, 4, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), delbut, 4, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label, 5, 0, 3, 2);
    gtk_grid_attach(GTK_GRID(grid), frame, 5, 2, 4, 6);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 10);

    gtk_widget_show_all(vbox);

    return dialog;
}

static void mod_cfg_apply(GKeyFile * cfgdata)
{
    gsize           num;
    guint           i;
    guint           catnum;
    gchar          *satstr = NULL;
    gchar          *buff;
    GtkTreeModel   *model;
    GtkTreeIter     iter;

    /* store location */
    buff = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(locw));

    /* is buff == "** DEFAULT **" clear the configuration key
       otherwise store the filename
     */
    if (!g_ascii_strcasecmp(buff, _("** DEFAULT **")))
    {
        g_key_file_remove_key(cfgdata,
                              MOD_CFG_GLOBAL_SECTION,
                              MOD_CFG_QTH_FILE_KEY, NULL);
    }
    else
    {
        satstr = g_strconcat(buff, ".qth", NULL);
        g_key_file_set_string(cfgdata,
                              MOD_CFG_GLOBAL_SECTION,
                              MOD_CFG_QTH_FILE_KEY, satstr);
        g_free(satstr);
    }

    g_free(buff);

    /* get number of satellites already in list */
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(satlist));
    num = gtk_tree_model_iter_n_children(model, NULL);

    /* loop over list entries and check their catnums */
    for (i = 0; i < num; i++)
    {
        if (gtk_tree_model_iter_nth_child(model, &iter, NULL, i))
        {
            gtk_tree_model_get(model, &iter,
                               GTK_SAT_SELECTOR_COL_CATNUM, &catnum, -1);
            if (i)
            {
                buff = g_strdup_printf("%s;%d", satstr, catnum);
                g_free(satstr);
            }
            else
            {
                buff = g_strdup_printf("%d", catnum);
            }
            satstr = g_strdup(buff);
            g_free(buff);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%s: Could not fetch entry %d in satellite list"),
                        __FILE__, __func__, i);
        }
    }

    g_key_file_set_string(cfgdata,
                          MOD_CFG_GLOBAL_SECTION, MOD_CFG_SATS_KEY, satstr);
    g_free(satstr);

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Applied changes to %s."),
                __func__, gtk_entry_get_text(GTK_ENTRY(namew)));
}

/**
 * Edit advanced settings.
 *
 * @param parent The parent mod-cfg dialog.
 * @param cfgdata The GKeyFile structure containing module config.
 *
 * This function creates a dialog window containing a notebook with the
 * relevant sat-pref config modules, i.e. those which are relevant for
 * configuring GtkSatModules.
 */
static void edit_advanced_settings(GtkDialog * parent, GKeyFile * cfgdata)
{
    GtkWidget      *dialog;
    GtkWidget      *contents;
    gchar          *icon;       /* window icon file name */

    dialog = gtk_dialog_new_with_buttons(_("Module properties"),
                                         GTK_WINDOW(parent),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         "_OK", GTK_RESPONSE_ACCEPT, NULL);

    /* window icon */
    icon = icon_file_name("gpredict-sat-pref.png");
    if (g_file_test(icon, G_FILE_TEST_EXISTS))
        gtk_window_set_icon_from_file(GTK_WINDOW(dialog), icon, NULL);

    contents = sat_pref_modules_create(cfgdata);
    gtk_widget_show_all(contents);

    gtk_box_pack_start(GTK_BOX
                       (gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       contents, TRUE, TRUE, 0);

    /* execute dialog */
    switch (gtk_dialog_run(GTK_DIALOG(dialog)))
    {
    case GTK_RESPONSE_ACCEPT:
        sat_pref_modules_ok(cfgdata);
        break;

    default:
        sat_pref_modules_cancel(cfgdata);
        break;
    }

    gtk_widget_destroy(dialog);
}

/**
 * Create a new module.
 *
 * This function creates a new module. The name of the module is
 * returned  and it should be freed when no longer needed.
 */
gchar          *mod_cfg_new()
{
    GtkWidget      *dialog;
    GKeyFile       *cfgdata;
    gchar          *name = NULL;
    gint            response;
    mod_cfg_status_t status;
    gboolean        finished = FALSE;
    gsize           num = 0;


    /* create cfg data */
    cfgdata = g_key_file_new();

    dialog = mod_cfg_editor_create(NULL, cfgdata, app);
    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 400);

    while (!finished)
    {
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        switch (response)
        {
            /* user pressed OK */
        case GTK_RESPONSE_OK:
            num = gtk_tree_model_iter_n_children(gtk_tree_view_get_model
                                                 (GTK_TREE_VIEW(satlist)),
                                                 NULL);
            if (num > 0)
            {
                /* we have at least one sat selected */
                gchar          *filename, *confdir;
                gboolean        save = TRUE;

                /* check if there is already a module with this name */
                confdir = get_modules_dir();
                filename = g_strconcat(confdir, G_DIR_SEPARATOR_S,
                                       gtk_entry_get_text(GTK_ENTRY(namew)),
                                       ".mod", NULL);

                if (g_file_test(filename, G_FILE_TEST_EXISTS))
                    save = overwrite_existing_module();

                g_free(confdir);
                g_free(filename);

                if (save)
                {
                    name = g_strdup(gtk_entry_get_text(GTK_ENTRY(namew)));
                    mod_cfg_apply(cfgdata);
                    status = mod_cfg_save(name, cfgdata);

                    if (status != MOD_CFG_OK)
                    {
                        /* send an error message */
                        sat_log_log(SAT_LOG_LEVEL_ERROR,
                                    _
                                    ("%s: Error while saving module data (%d)."),
                                    __func__, status);
                    }

                    finished = TRUE;
                }
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s: User tried to create module with no sats."),
                            __func__);

                /* tell user to behave nicely */
                GtkWidget      *errdialog;

                errdialog = gtk_message_dialog_new(NULL,
                                                   //GTK_WINDOW (app),
                                                   GTK_DIALOG_MODAL |
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   _
                                                   ("Please select at least one satellite "
                                                    "from the list."));
                gtk_dialog_run(GTK_DIALOG(errdialog));
                gtk_widget_destroy(errdialog);
            }
            break;

            /* bring up properties editor */
        case GTK_RESPONSE_HELP:
            edit_advanced_settings(GTK_DIALOG(dialog), cfgdata);
            finished = FALSE;
            break;

            /* everything else is regarded CANCEL */
        default:
            finished = TRUE;
            break;
        }
    }

    /* clean up */
    g_key_file_free(cfgdata);
    gtk_widget_destroy(dialog);

    return name;
}

/**
 * Edit configuration for an existing module.
 * 
 * @param modname The name of the module to edit.
 * @param cfgdata Configuration data for the module.
 * @param toplevel Pointer to the toplevel window.
 *
 * This function allows the user to edit the configuration
 * of the module specified by modname. The changes are stored
 * in the cfgdata configuration structure but are not saved
 * to disc; that has to be done separately using the
 * mod_cfg_save function. 
 *
 */
mod_cfg_status_t mod_cfg_edit(gchar * modname, GKeyFile * cfgdata,
                              GtkWidget * toplevel)
{
    GtkWidget      *dialog;
    gint            response;
    gboolean        finished = FALSE;
    gsize           num = 0;
    mod_cfg_status_t status = MOD_CFG_CANCEL;

    dialog = mod_cfg_editor_create(modname, cfgdata, toplevel);
    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 400);

    while (!finished)
    {
        response = gtk_dialog_run(GTK_DIALOG(dialog));

        switch (response)
        {
            /* user pressed OK */
        case GTK_RESPONSE_OK:

            /* check that user has selected at least one satellite */
            num =
                gtk_tree_model_iter_n_children(gtk_tree_view_get_model
                                               (GTK_TREE_VIEW(satlist)), NULL);

            if (num > 0)
            {

                /* we have at least one sat selected */
                mod_cfg_apply(cfgdata);
                finished = TRUE;
                status = MOD_CFG_OK;
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s: User tried to create module with no sats."),
                            __func__);

                /* tell user not to do that */
                GtkWidget      *errdialog;

                errdialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
                                                   GTK_DIALOG_MODAL |
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   _
                                                   ("Please select at least one satellite "
                                                    "from the list."));
                gtk_dialog_run(GTK_DIALOG(errdialog));
                gtk_widget_destroy(errdialog);
            }

            break;

            /* bring up properties editor */
        case GTK_RESPONSE_HELP:
            edit_advanced_settings(GTK_DIALOG(dialog), cfgdata);
            finished = FALSE;
            break;

            /* everything else is regarded CANCEL */
        default:
            finished = TRUE;
            break;
        }
    }

    /* clean up */
    gtk_widget_destroy(dialog);

    return status;
}

mod_cfg_status_t mod_cfg_save(gchar * modname, GKeyFile * cfgdata)
{
    gchar          *filename;   /* file name */
    gchar          *confdir;
    gboolean        err;

    if (!modname)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Attempt to save data to empty file name."),
                    __func__);

        return MOD_CFG_ERROR;
    }
    if (!cfgdata)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Attempt to save NULL data."), __func__);

        return MOD_CFG_ERROR;
    }

    /* create file and write data stream */
    confdir = get_modules_dir();
    filename = g_strconcat(confdir, G_DIR_SEPARATOR_S, modname, ".mod", NULL);
    g_free(confdir);

    err = gpredict_save_key_file(cfgdata, filename);
    g_free(filename);
    if (err)
        return MOD_CFG_ERROR;

    return MOD_CFG_OK;
}

mod_cfg_status_t mod_cfg_delete(gchar * modname, gboolean needcfm)
{
    (void)modname;
    (void)needcfm;

    return MOD_CFG_CANCEL;
}
