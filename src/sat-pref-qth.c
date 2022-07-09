/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>
           Charles Suprin  <hamaa1vs@gmail.com>

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
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gpredict-utils.h"
#include "locator.h"
#include "qth-data.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-qth.h"
#include "sat-pref-qth-data.h"
#include "sat-pref-qth-editor.h"


extern GtkWidget *window;       /* dialog window defined in sat-pref.c */
static GtkWidget *addbutton;
static GtkWidget *editbutton;
static GtkWidget *delbutton;
static gulong   handler_id;

static GtkWidget *qthlist;      /* we need access to this from the ok and cancel functions */

/* private function prototypes */
static GtkWidget *create_qth_list(void);
static GtkTreeModel *create_and_fill_model(void);
static guint    read_qth_file(GtkListStore * liststore, gchar * filename);
static GtkWidget *create_buttons(GtkTreeView * qthlist);
static void     default_toggled(GtkCellRendererToggle * cell,
                                gchar * path_str, gpointer data);

static gboolean clear_default_flags(GtkTreeModel * model,
                                    GtkTreePath * path,
                                    GtkTreeIter * iter, gpointer defqth);

static void     float_cell_data_function(GtkTreeViewColumn * col,
                                         GtkCellRenderer * renderer,
                                         GtkTreeModel * model,
                                         GtkTreeIter * iter, gpointer column);

static void     add_cb(GtkWidget * button, gpointer data);
static void     edit_cb(GtkWidget * button, gpointer data);
static void     delete_cb(GtkWidget * button, gpointer data);
static void     row_activated_cb(GtkTreeView * tree_view,
                                 GtkTreePath * path,
                                 GtkTreeViewColumn * column,
                                 gpointer user_data);

/* static gboolean check_and_set_default_qth (GtkTreeModel *model, */
/*                                                      GtkTreePath  *path, */
/*                                                      GtkTreeIter  *iter, */
/*                                                      gpointer      data); */

static gboolean save_qth(GtkTreeModel * model,
                         GtkTreePath * path,
                         GtkTreeIter * iter, gpointer data);

static void     delete_location_files(void);


static gboolean convert_qth_altitude(GtkTreeModel * model,
                                     GtkTreePath * path,
                                     GtkTreeIter * iter, gpointer data);


/**
 * @brief Create and initialise widgets for the locations prefs tab.
 *
 * The QTH tab consists of two main parts: A list showing the configured
 * locations (.qth files in USER_CONF_DIR/ and three buttons beside the list:
 *    Add      Add a new location to the list
 *    Edit     Edit selected location
 *    Delete   Delete selected location
 *
 * @note This module uses the gtk-sat-data infrastructure to read and write
 *       .qth files.
 *
 * Add button always active.
 * The other buttons are active only if a row is selected.
 *
 */
GtkWidget      *sat_pref_qth_create()
{
    GtkWidget      *vbox;       /* vbox containing the list part and the details part */
    GtkWidget      *swin;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* create qth list and pack into scrolled window */
    qthlist = create_qth_list();
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), qthlist);

    gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),
                       create_buttons(GTK_TREE_VIEW(qthlist)),
                       FALSE, FALSE, 0);

    return vbox;
}

/** User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_qth_cancel()
{
}

/** User pressed OK. Any changes should be stored in config. */
void sat_pref_qth_ok()
{
    delete_location_files();

    gtk_tree_model_foreach(gtk_tree_view_get_model(GTK_TREE_VIEW(qthlist)),
                           save_qth, NULL);
}

/** Create QTH list widgets. */
static GtkWidget *create_qth_list()
{
    GtkTreeModel   *model;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    qthlist = gtk_tree_view_new();

    model = create_and_fill_model();
    gtk_tree_view_set_model(GTK_TREE_VIEW(qthlist), model);
    g_object_unref(model);

    /* name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer,
                                                      "text",
                                                      QTH_LIST_COL_NAME, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);

    /* location column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Location"), renderer,
                                                      "text", QTH_LIST_COL_LOC,
                                                      NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);

    /* lat column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Lat"), renderer,
                                                      "text", QTH_LIST_COL_LAT,
                                                      NULL);
    gtk_tree_view_column_set_alignment(column, 0.5);
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            float_cell_data_function,
                                            GUINT_TO_POINTER(QTH_LIST_COL_LAT),
                                            NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);

    /* lon column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Lon"), renderer,
                                                      "text", QTH_LIST_COL_LON,
                                                      NULL);
    gtk_tree_view_column_set_alignment(column, 0.5);
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            float_cell_data_function,
                                            GUINT_TO_POINTER(QTH_LIST_COL_LON),
                                            NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);

    /* alt column */
    renderer = gtk_cell_renderer_text_new();
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
    {
        column = gtk_tree_view_column_new_with_attributes(_("Alt\n(ft)"),
                                                          renderer,
                                                          "text",
                                                          QTH_LIST_COL_ALT,
                                                          NULL);
    }
    else
    {
        column =
            gtk_tree_view_column_new_with_attributes(_("Alt\n(m)"), renderer,
                                                     "text", QTH_LIST_COL_ALT,
                                                     NULL);
    }
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);
    gtk_tree_view_column_set_alignment(column, 0.5);

    /* locator */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("QRA"), renderer,
                                                      "text", QTH_LIST_COL_QRA,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);
    gtk_tree_view_column_set_alignment(column, 0.5);

    /* weather station */
    /*      renderer = gtk_cell_renderer_text_new (); */
    /*      column = gtk_tree_view_column_new_with_attributes (_("WX"), renderer, */
    /*                                       "text", QTH_LIST_COL_WX, */
    /*                                       NULL); */
    /*      gtk_tree_view_insert_column (GTK_TREE_VIEW (qthlist), column, -1); */
    /*      gtk_tree_view_column_set_alignment (column, 0.5); */

    /* default */
    renderer = gtk_cell_renderer_toggle_new();
    handler_id = g_signal_connect(renderer, "toggled",
                                  G_CALLBACK(default_toggled), model);

    column = gtk_tree_view_column_new_with_attributes(_("Default"), renderer,
                                                      "active",
                                                      QTH_LIST_COL_DEF, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(qthlist), column);
    gtk_tree_view_column_set_alignment(column, 0.5);

    g_signal_connect(qthlist, "row-activated", G_CALLBACK(row_activated_cb),
                     NULL);

#ifdef HAS_LIBGPS
    /* GPSD enabled */
    /*server */
    renderer = gtk_cell_renderer_text_new();
    column =
        gtk_tree_view_column_new_with_attributes(_("GPSD\nServer"), renderer,
                                                 "text",
                                                 QTH_LIST_COL_GPSD_SERVER,
                                                 NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);
    gtk_tree_view_column_set_alignment(column, 0.5);
    /*port */
    renderer = gtk_cell_renderer_text_new();
    column =
        gtk_tree_view_column_new_with_attributes(_("GPSD\nPort"), renderer,
                                                 "text",
                                                 QTH_LIST_COL_GPSD_PORT, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);
    gtk_tree_view_column_set_alignment(column, 0.5);

    /*type */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("QTH\nType"), renderer,
                                                      "text",
                                                      QTH_LIST_COL_TYPE, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(qthlist), column, -1);
    gtk_tree_view_column_set_alignment(column, 0.5);
#endif

    return qthlist;
}

/**
 * Create data storage for QTH list.
 *
 * This function creates the data storage necessary for the
 * list view. The newly created tree model is populated with
 * data from the .qth files in the users config directory.
 * The individual .qth files are read by the read_qth_file
 * function.
 */
static GtkTreeModel *create_and_fill_model()
{
    GtkListStore   *liststore;  /* the list store data structure */
    GDir           *dir = NULL; /* directory handle */
    GError         *error = NULL;       /* error flag and info */
    gchar          *dirname;    /* directory name */
    const gchar    *filename;   /* file name */
    gchar          *buff;

    /* create a new list store */
    liststore = gtk_list_store_new(QTH_LIST_COL_NUM, G_TYPE_STRING,     // QTH name
                                   G_TYPE_STRING,       // Location
                                   G_TYPE_STRING,       // Description
                                   G_TYPE_DOUBLE,       // Latitude
                                   G_TYPE_DOUBLE,       // Longitude
                                   G_TYPE_INT,  // Altitude
                                   G_TYPE_STRING,       // QRA locator
                                   G_TYPE_STRING,       // Weather station
                                   G_TYPE_BOOLEAN,      // Default
                                   G_TYPE_INT,  //type
                                   G_TYPE_STRING,       //server
                                   G_TYPE_INT   //port
        );

    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(liststore),
                                         QTH_LIST_COL_NAME,
                                         GTK_SORT_ASCENDING);
    /* scan for .qth files in the user config directory and
       add the contents of each .qth file to the list store
     */
    dirname = get_user_conf_dir();
    dir = g_dir_open(dirname, 0, &error);

    if (dir)
    {
        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".qth"))
            {
                buff = g_strconcat(dirname, G_DIR_SEPARATOR_S, filename, NULL);

                /* read qth file */
                if (read_qth_file(liststore, buff))
                {
                    /* send debug message */
                    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                _("%s:%d: Read QTH data from %s."),
                                __FILE__, __LINE__, filename);
                }
                else
                {
                    /* error reading the file */
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s:%d: Error reading %s (see prev msg)"),
                                __FILE__, __LINE__, filename);
                }
                g_free(buff);
            }
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open user cfg dir (%s)"),
                    __FILE__, __LINE__, error->message);
        g_clear_error(&error);
    }

    g_free(dirname);
    g_dir_close(dir);

    return GTK_TREE_MODEL(liststore);
}

/**
 * Read QTH file and add data to list store.
 * @param liststore The GtkListStore where the data should be stored.
 * @param filename The full name of the qth file.
 * @return 1 if read is successful, 0 if an error occurs.
 *
 * The function uses the gtk-sat-data infrastructure to read the qth
 * data from the specified file.
 *
 * There is a little challenge here. First, we want to read the data from
 * the .qth files and store them in the list store. To do this we use a
 * qth_t structure, which can be populated using gtk_sat_data_read_qth.
 * Then, when the configuration is finished and the user presses "OK", we
 * want to write all the data back to the .qth files. To do that, we create
 * an up-to-date qth_t data structure and pass it to the gtk_sat_data_write_qth
 * function, which will take care of updating the GKeyFile data structure and
 * writing the contents to the .qth file.
 */
static guint read_qth_file(GtkListStore * liststore, gchar * filename)
{
    GtkTreeIter     item;       /* new item added to the list store */
    qth_t          *qth;        /* qth data structure */
    gchar          *defqth;
    gboolean        is_default = FALSE;
    gchar          *fname;
    gint            dispalt;    /* displayed altitude */

    if ((qth = g_try_new0(qth_t, 1)) == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to allocate memory!\n"),
                    __FILE__, __LINE__);

        return FALSE;
    }

    /* read data from file */
    if (!qth_data_read(filename, qth))
    {
        g_free(qth);
        return FALSE;
    }

    /* calculate QRA locator */
    gint            retcode;

    qth->qra = g_malloc(7);
    retcode = longlat2locator(qth->lon, qth->lat, qth->qra, 3);

    if (retcode != RIG_OK)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Could not convert (%.2f,%.2f) to QRA."),
                    __FILE__, __LINE__, qth->lat, qth->lon);
        qth->qra[0] = '\0';
    }
    else
    {
        qth->qra[6] = '\0';
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%d: QRA locator is %s"),
                    __FILE__, __LINE__, qth->qra);
    }

    /* is this the default qth? */
    defqth = sat_cfg_get_str(SAT_CFG_STR_DEF_QTH);

    if (g_str_has_suffix(filename, defqth))
    {
        is_default = TRUE;

        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d: This appears to be the default QTH."),
                    __FILE__, __LINE__);
    }

    g_free(defqth);

    /* check wehter we are using imperial or metric system;
       in case of imperial we have to convert altitude from
       meters to feet.
       note: the internat data are always kept in metric and
       only the displayed numbers are converted. Therefore,
       we use a dedicated var 'dispalt'
     */
    /* NO, ONLY DISPLAY WIDGETS WILL BE AFFECTED BY THIS SETTING */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
    {
        dispalt = (gint) M_TO_FT(qth->alt);
    }
    else
    {
        dispalt = (gint) qth->alt;
    }

    /* strip file name; we don't need the whole path */
    fname = g_path_get_basename(filename);

    /* we now have all necessary data in the qth_t structure;
       add the data to the list store */
    gtk_list_store_append(liststore, &item);
    gtk_list_store_set(liststore, &item,
                       QTH_LIST_COL_NAME, qth->name,
                       QTH_LIST_COL_LOC, qth->loc,
                       QTH_LIST_COL_DESC, qth->desc,
                       QTH_LIST_COL_LAT, qth->lat,
                       QTH_LIST_COL_LON, qth->lon,
                       QTH_LIST_COL_ALT, dispalt,
                       QTH_LIST_COL_QRA, qth->qra,
                       QTH_LIST_COL_WX, qth->wx,
                       QTH_LIST_COL_DEF, is_default,
                       QTH_LIST_COL_TYPE, qth->type,
                       QTH_LIST_COL_GPSD_SERVER, qth->gpsd_server,
                       QTH_LIST_COL_GPSD_PORT, qth->gpsd_port, -1);

    g_free(fname);

    /* we are finished with this qth, free it */
    qth_data_free(qth);

    return TRUE;
}

/**
 * Create buttons.
 * @param qthlist The GtkTreeView widget containing the qth data.
 * @return A button box containing the buttons.
 *
 * This function creates and initialises the three buttons below the qth list.
 * The treeview widget is needed by the buttons when they are activated.
 */
static GtkWidget *create_buttons(GtkTreeView * qthlist)
{
    GtkWidget      *box;

    /* add button */
    addbutton = gtk_button_new_with_label(_("Add new"));
    gtk_widget_set_tooltip_text(addbutton,
                                _("Add a new ground station to the list"));
    g_signal_connect(addbutton, "clicked", G_CALLBACK(add_cb), qthlist);

    /* edit button */
    editbutton = gtk_button_new_with_label(_("Edit"));
    gtk_widget_set_tooltip_text(editbutton,
                                _("Edit the selected ground station"));
    g_signal_connect(editbutton, "clicked", G_CALLBACK(edit_cb), qthlist);

    /* delete button; don't forget to delete file.... */
    delbutton = gtk_button_new_with_label(_("Delete"));
    gtk_widget_set_tooltip_text(delbutton,
                                _("Delete selected ground station"));
    g_signal_connect(delbutton, "clicked", G_CALLBACK(delete_cb), qthlist);

    /* vertical button box */
    box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_START);

    gtk_container_add(GTK_CONTAINER(box), addbutton);
    gtk_container_add(GTK_CONTAINER(box), editbutton);
    gtk_container_add(GTK_CONTAINER(box), delbutton);

    return box;
}

static void add_cb(GtkWidget * button, gpointer data)
{
    GtkTreeView    *qthlist = GTK_TREE_VIEW(data);

    (void)button;

    sat_pref_qth_editor_run(qthlist, TRUE);
}

static void row_activated_cb(GtkTreeView * tree_view,
                             GtkTreePath * path,
                             GtkTreeViewColumn * column, gpointer user_data)
{
    (void)path;
    (void)column;
    (void)user_data;

    sat_pref_qth_editor_run(tree_view, FALSE);
}

static void edit_cb(GtkWidget * button, gpointer data)
{
    GtkTreeView    *qthlist = GTK_TREE_VIEW(data);

    (void)button;

    sat_pref_qth_editor_run(qthlist, FALSE);
}

/**
 * Delete selected location
 *
 * This function is called when the user clicks the DELETE button.
 * If there are more than one locations defined in the list, the function
 * will delete the selected one. If the deleted location used to be the default
 * location, the first entry in the list will be selected as new default location.
 *
 * @note This function only deletes from the QTH list, not from disk. If the user
 *       eventually presses OK, all QTH files will be removed before the new data
 *       is rewritten to disk, thus the deleted entry will not be saved again. On
 *       the other hand, if user clicks cancel, the delete process will be undone.
 */
static void delete_cb(GtkWidget * button, gpointer data)
{
    GtkTreeView    *qthlist = GTK_TREE_VIEW(data);
    GtkTreeModel   *model = gtk_tree_view_get_model(qthlist);
    GtkTreeModel   *selmod;
    GtkTreeSelection *selection;
    GtkTreeIter     iter;

    (void)button;

    /* if this is the only entry, tell user that it is not
       possible to delete
     */
    if (gtk_tree_model_iter_n_children(model, NULL) < 2)
    {

        GtkWidget      *dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        _("Can not delete ground station!\n\n"
                                          "You need to have at least one ground\n"
                                          "station set up, otherwise gpredict may\n"
                                          "not work properly."));

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    else
    {
        /* get selected row
           FIXME: do we really need to work with two models?
         */
        selection = gtk_tree_view_get_selection(qthlist);
        if (gtk_tree_selection_get_selected(selection, &selmod, &iter))
        {

            gboolean        neednewdef = FALSE;
            gchar          *buff;

            gtk_tree_model_get(selmod, &iter,
                               QTH_LIST_COL_DEF, &neednewdef,
                               QTH_LIST_COL_NAME, &buff, -1);

            g_free(buff);

            /* delete qth entry from list */
            gtk_list_store_remove(GTK_LIST_STORE(selmod), &iter);


            /* if the selected qth entry is the default one
               select the first entry in the list as the
               new default qth.
             */
            if (neednewdef)
            {
                if (gtk_tree_model_get_iter_first(model, &iter))
                {
                    gtk_list_store_set(GTK_LIST_STORE(model),
                                       &iter, QTH_LIST_COL_DEF, TRUE, -1);
                }
                else
                {
                    /* huh? no more entries, this is a bug! */
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s:%d: Empty ground station list!"),
                                __FILE__, __LINE__);
                }
            }
        }
    }
}

/**
 * Handle toggle events on "Default" check box
 * @param cell The item that received the signal.
 * @param path_str Path string.
 * @param data Pointer to user data (list store).
 *
 * This function is called when the user clicks on "Default" check box
 * indicating that a new default location has been selected. If the
 * clicked check box has been un-checked the action is ignored, because
 * we need a default location. If the clicked check box has been checked,
 * the default flag of the checked QTH is set to TRUE, while the flag is
 * cleared for all the other QTH's.
 */
static void default_toggled(GtkCellRendererToggle * cell, gchar * path_str,
                            gpointer data)
{
    GtkTreeModel   *model = (GtkTreeModel *) data;
    GtkTreeIter     iter;
    GtkTreePath    *path = gtk_tree_path_new_from_string(path_str);
    gboolean        fixed;
    gchar          *defqth;

    /* block toggle signals while we mess with the check boxes */
    g_signal_handler_block(cell, handler_id);

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, QTH_LIST_COL_DEF, &fixed, -1);

    if (fixed)
    {
        /* do nothing except sending a message */
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d: Default QTH can not be cleared! "
                      "Select another QTH to change default."),
                    __FILE__, __LINE__);
    }
    else
    {
        /* make this qth new default */
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           QTH_LIST_COL_DEF, TRUE, -1);

        /* copy file name of new default QTH to a string buffer */
        gtk_tree_model_get(model, &iter, QTH_LIST_COL_NAME, &defqth, -1);

        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d: New default QTH is %s.qth."),
                    __FILE__, __LINE__, defqth);

        /* clear the default flag for the other qth */
        gtk_tree_model_foreach(model, clear_default_flags, defqth);

        g_free(defqth);

    }

    /* clean up */
    gtk_tree_path_free(path);

    /* unblock toggle signals */
    g_signal_handler_unblock(cell, handler_id);
}

/**
 * Clear default flag for all qth, except defqth.
 * @param model The GtkTreeModel.
 * @param path  The GtkTreePath.
 * @param iter  The GtkTreeIter of the current item.
 * @param defqth The file name of the new default which should not be cleared.
 *
 * This function is called for each QTH entry in the QTH list with the purpose of
 * clearing the default flag. This happens when the user selects a new QTH to be
 * the default QTH and the other ones need to be cleared.
 */
static gboolean clear_default_flags(GtkTreeModel * model, GtkTreePath * path,
                                    GtkTreeIter * iter, gpointer defqth)
{
    gchar          *thisqth;

    (void)path;

    gtk_tree_model_get(model, iter, QTH_LIST_COL_NAME, &thisqth, -1);

    /* clear flag if this is not the new default QTH */
    if (g_ascii_strcasecmp(thisqth, (gchar *) defqth))
    {
        gtk_list_store_set(GTK_LIST_STORE(model), iter,
                           QTH_LIST_COL_DEF, FALSE, -1);

        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%d: Clearing default flag for %s."),
                    __FILE__, __LINE__, thisqth);
    }

    g_free(thisqth);

    return FALSE;
}

/* render column containing float
   by using this instead of the default data function, we can
   control the number of decimals and display the coordinates in a
   fancy way, including degree sign and NWSE suffixes.

   Please note that this function only affects how the numbers are
   displayed (rendered), the tree_store will still contain the
   original floating point numbers. Very cool!
*/
static void float_cell_data_function(GtkTreeViewColumn * col,
                                     GtkCellRenderer * renderer,
                                     GtkTreeModel * model,
                                     GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);
    gchar           hmf = ' ';

    (void)col;

    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* check whether configuration requests the use
       of N, S, E and W instead of signs
     */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW))
    {
        if (coli == QTH_LIST_COL_LAT)
        {
            if (number < 0.0)
            {
                number *= -1.0;
                hmf = 'S';
            }
            else
            {
                hmf = 'N';
            }
        }
        else if (coli == QTH_LIST_COL_LON)
        {
            if (number < 0.0)
            {
                number *= -1.0;
                hmf = 'W';
            }
            else
            {
                hmf = 'E';
            }
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%d: Invalid column: %d"),
                        __FILE__, __LINE__, coli);
            hmf = '?';
        }
    }

    /* format the number */
    buff = g_strdup_printf("%.4f\302\260%c", number, hmf);
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/** Save a row from the QTH list (called by the save function) */
static gboolean save_qth(GtkTreeModel * model, GtkTreePath * path,
                         GtkTreeIter * iter, gpointer data)
{
    qth_t           qth;
    gboolean        def = FALSE;
    gchar          *filename, *confdir;
    gchar          *buff;

    (void)path;
    (void)data;

    gtk_tree_model_get(model, iter,
                       QTH_LIST_COL_DEF, &def,
                       QTH_LIST_COL_NAME, &qth.name,
                       QTH_LIST_COL_LOC, &qth.loc,
                       QTH_LIST_COL_DESC, &qth.desc,
                       QTH_LIST_COL_LAT, &qth.lat,
                       QTH_LIST_COL_LON, &qth.lon,
                       QTH_LIST_COL_ALT, &qth.alt,
                       QTH_LIST_COL_WX, &qth.wx,
                       QTH_LIST_COL_TYPE, &qth.type,
                       QTH_LIST_COL_GPSD_SERVER, &qth.gpsd_server,
                       QTH_LIST_COL_GPSD_PORT, &qth.gpsd_port, -1);

    confdir = get_user_conf_dir();
    filename = g_strconcat(confdir, G_DIR_SEPARATOR_S, qth.name, ".qth", NULL);
    g_free(confdir);

    /* check wehter we are using imperial or metric system;
       in case of imperial we have to convert altitude from
       feet to meters before saving.
     */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
    {
        qth.alt = (guint) FT_TO_M(qth.alt);
    }

    if (qth_data_save(filename, &qth))
    {
        /* saved ok, go on check whether qth is default */
        if (def)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%d: %s appears to be default QTH"),
                        __FILE__, __LINE__, qth.name);

            buff = g_path_get_basename(filename);
            sat_cfg_set_str(SAT_CFG_STR_DEF_QTH, buff);
            g_free(buff);
        }
    }

    g_free(filename);
    g_free(qth.name);
    g_free(qth.loc);
    g_free(qth.desc);
    g_free(qth.wx);

    return FALSE;
}

/**
 * Remove .qth files.
 *
 * This function is used to remove any existing .qth file
 * before storing the data from the QTH list.
 */
static void delete_location_files()
{
    GDir           *dir = NULL; /* directory handle */
    GError         *error = NULL;       /* error flag and info */
    gchar          *dirname;    /* directory name */
    const gchar    *filename;   /* file name */
    gchar          *buff;

    /* scan for .qth files in the user config directory and
       add the contents of each .qth file to the list store
     */
    dirname = get_user_conf_dir();
    dir = g_dir_open(dirname, 0, &error);

    if (dir)
    {
        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".qth"))
            {
                buff = g_strconcat(dirname, G_DIR_SEPARATOR_S, filename, NULL);

                /* remove file */
                if (g_remove(buff))
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s:%d: Failed to remove %s"),
                                __FILE__, __LINE__, filename);
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                _("%s:%d: Removed %s"),
                                __FILE__, __LINE__, filename);
                }

                g_free(buff);
            }
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open user cfg dir (%s)"),
                    __FILE__, __LINE__, error->message);
        g_clear_error(&error);

    }

    g_free(dirname);
    g_dir_close(dir);
}

/**
 * Manage measurement system changes.
 *
 * This function should be called when the user changes between the
 * metric and imperial systems. When that happens, we need to convert
 * location altitudeas right away, otherwise the altitudes in the qth
 * list will keep their old values in the new system.
 *
 * If imperial = TRUE the new system is imperial and so we have to use
 * M_TO_FT.
 */
void sat_pref_qth_sys_changed(gboolean imperial)
{
    gtk_tree_model_foreach(gtk_tree_view_get_model(GTK_TREE_VIEW(qthlist)),
                           convert_qth_altitude, GUINT_TO_POINTER(imperial));
}

/**
 * Convert altitude of the specified QTH entry.
 *
 * The data contain pointer to a boolean indicating whether the new
 * system is imperial (TRUE) or metric (FALSE)
 */
static          gboolean
convert_qth_altitude(GtkTreeModel * model,
                     GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
    gint            alti;
    GtkTreeViewColumn *column;
    gchar          *title;

    (void)path;

    /* first, get the current altitude and other data */
    gtk_tree_model_get(model, iter, QTH_LIST_COL_ALT, &alti, -1);
    column = gtk_tree_view_get_column(GTK_TREE_VIEW(qthlist), 3);

    if (GPOINTER_TO_UINT(data))
    {
        /* new sys is imperial */
        alti = M_TO_FT(alti);
        title = g_strdup(_("Alt (ft)"));
    }
    else
    {
        /* new sys is metric */
        alti = FT_TO_M(alti);
        title = g_strdup(_("Alt (m)"));
    }

    /* store new value */
    gtk_list_store_set(GTK_LIST_STORE(model), iter,
                       QTH_LIST_COL_ALT, alti, -1);
    /* update column title */
    gtk_tree_view_column_set_title(column, title);
    g_free(title);

    return FALSE;
}
