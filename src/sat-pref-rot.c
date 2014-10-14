/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2010  Alexandru Csete, OZ9AEC.

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
#include <glib/gstdio.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "compat.h"
#include "rotor-conf.h"
#include "sat-pref-rot-data.h"
#include "sat-pref-rot-editor.h"
#include "sat-pref-rot.h"




extern GtkWidget *window; /* dialog window defined in sat-pref.c */


/* private function declarations */
static void          create_rot_list       (void);
static GtkTreeModel *create_and_fill_model (void);
static GtkWidget    *create_buttons        (void);

static void add_cb    (GtkWidget *button, gpointer data);
static void edit_cb   (GtkWidget *button, gpointer data);
static void delete_cb (GtkWidget *button, gpointer data);
static void row_activated_cb (GtkTreeView       *tree_view,
                              GtkTreePath       *path,
                              GtkTreeViewColumn *column,
                              gpointer           user_data);

static void render_angle (GtkTreeViewColumn *col,
                          GtkCellRenderer   *renderer,
                          GtkTreeModel      *model,
                          GtkTreeIter       *iter,
                          gpointer           column);

static void render_aztype (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           column);

/* global objects */
static GtkWidget *addbutton;
static GtkWidget *editbutton;
static GtkWidget *delbutton;
static GtkWidget *rotlist;


/** \brief Create and initialise widgets for the radios tab. */
GtkWidget *sat_pref_rot_create ()
{
    GtkWidget *vbox;    /* vbox containing the list part and the details part */
    GtkWidget *swin;


    vbox = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

    /* create rot list and pack into scrolled window */
    create_rot_list ();
    swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (swin), rotlist);

    gtk_box_pack_start (GTK_BOX (vbox), swin, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), create_buttons (), FALSE, FALSE, 0);

    return vbox;
}


/** \brief Create Radio configuration list widget.
 *
 */
static void create_rot_list ()
{
    GtkTreeModel      *model;
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;


    rotlist = gtk_tree_view_new ();
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (rotlist), TRUE);

    model = create_and_fill_model ();
    gtk_tree_view_set_model (GTK_TREE_VIEW (rotlist), model);
    g_object_unref (model);

    /* Conf name */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Config Name"), renderer,
                                                        "text", ROT_LIST_COL_NAME,
                                                        NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);

    /* Host */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Host"), renderer,
                                                       "text", ROT_LIST_COL_HOST,
                                                        NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);
    
    /* port */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Port"), renderer,
                                                       "text", ROT_LIST_COL_PORT,
                                                        NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);

    /* Az and el limits */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Min Az"), renderer,
                                                       "text", ROT_LIST_COL_MINAZ,
                                                       NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
                                             render_angle,
                                             GUINT_TO_POINTER(ROT_LIST_COL_MINAZ),
                                             NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Max Az"), renderer,
                                                        "text", ROT_LIST_COL_MAXAZ,
                                                        NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
                                             render_angle,
                                             GUINT_TO_POINTER(ROT_LIST_COL_MAXAZ),
                                             NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Min El"), renderer,
                                                        "text", ROT_LIST_COL_MINEL,
                                                        NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
                                             render_angle,
                                             GUINT_TO_POINTER(ROT_LIST_COL_MINEL),
                                             NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Max El"), renderer,
                                                    "text", ROT_LIST_COL_MAXEL,
                                                    NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
                                             render_angle,
                                             GUINT_TO_POINTER(ROT_LIST_COL_MAXEL),
                                             NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);
    
    /* Az type */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Azimuth Type"), renderer,
                                                      "text", ROT_LIST_COL_AZTYPE,
                                                       NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
                                             render_aztype,
                                             GUINT_TO_POINTER(ROT_LIST_COL_AZTYPE),
                                             NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (rotlist), column, -1);

    g_signal_connect (rotlist, "row-activated", G_CALLBACK (row_activated_cb), NULL);
}


/** \brief Create data storage for rotator configuration list. */
static GtkTreeModel *create_and_fill_model ()
{
    GtkListStore *liststore;    /* the list store data structure */
    GtkTreeIter  item;      /* new item added to the list store */
    GDir         *dir = NULL;   /* directory handle */
    GError       *error = NULL; /* error flag and info */
    gchar        *dirname;      /* directory name */
    gchar       **vbuff;
    const gchar  *filename;     /* file name */
    rotor_conf_t  conf;

    /* create a new list store */
    liststore = gtk_list_store_new (ROT_LIST_COL_NUM,
                                    G_TYPE_STRING,    // name
                                    G_TYPE_STRING,    // host
                                    G_TYPE_INT,       // port
                                    G_TYPE_DOUBLE,    // Min Az
                                    G_TYPE_DOUBLE,    // Max Az
                                    G_TYPE_DOUBLE,    // Min El
                                    G_TYPE_DOUBLE,    // Max El
                                    G_TYPE_INT        // Az type
                                   );
     gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE(liststore),ROT_LIST_COL_NAME,GTK_SORT_ASCENDING);
    /* open configuration directory */
    dirname = get_hwconf_dir ();
    
    dir = g_dir_open (dirname, 0, &error);
    if (dir) {
        /* read each .rot file */
        while ((filename = g_dir_read_name (dir))) {
            
            if (g_str_has_suffix (filename, ".rot")) {
                
                vbuff = g_strsplit (filename, ".rot", 0);
                conf.name = g_strdup (vbuff[0]);
                g_strfreev (vbuff);
                if (rotor_conf_read (&conf)) {
                    /* insert conf into liststore */
                    gtk_list_store_append (liststore, &item);
                    gtk_list_store_set (liststore, &item,
                                        ROT_LIST_COL_NAME, conf.name,
                                        ROT_LIST_COL_HOST, conf.host,
                                        ROT_LIST_COL_PORT, conf.port,
                                        ROT_LIST_COL_MINAZ, conf.minaz,
                                        ROT_LIST_COL_MAXAZ, conf.maxaz,
                                        ROT_LIST_COL_MINEL, conf.minel,
                                        ROT_LIST_COL_MAXEL, conf.maxel,
                                        ROT_LIST_COL_AZTYPE, conf.aztype,
                                        -1);
                    
                    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                                 _("%s:%d: Read %s"),
                                 __FILE__, __LINE__, filename);
                    /* clean up memory */
                    if (conf.name)
                        g_free (conf.name);
                    
                    if (conf.host)
                        g_free (conf.host);
                    
                }
                else {
                    /* there was an error */
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                 _("%s:%d: Failed to read %s"),
                                 __FILE__, __LINE__, conf.name);
                    
                    g_free (conf.name);
                }
            }
        }
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to open hwconf dir (%s)"),
                       __FILE__, __LINE__, error->message);
        g_clear_error (&error);
    }

    g_free (dirname);
    g_dir_close (dir);

    return GTK_TREE_MODEL (liststore);
}


/** \brief Create buttons.
 *  \return A button box containing the buttons.
 *
 * This function creates and initialises the three buttons below the rot list.
 * The treeview widget is needed by the buttons when they are activated.
 *
 */
static GtkWidget *create_buttons (void)
{
    GtkWidget *box;

    /* add button */
    addbutton = gpredict_hstock_button (GTK_STOCK_ADD,
                                        _("Add New"),
                                        _("Add a new rotator to the list"));
    g_signal_connect (addbutton, "clicked", G_CALLBACK (add_cb), NULL);

    /* edit button */
    editbutton = gpredict_hstock_button (GTK_STOCK_EDIT,
                                         _("Edit"),
                                         _("Edit the currently selected rotator"));
    g_signal_connect (editbutton, "clicked", G_CALLBACK (edit_cb), NULL);

    /* delete button; don't forget to delete file.... */
    delbutton = gpredict_hstock_button (GTK_STOCK_DELETE,
                                        _("Delete"),
                                        _("Delete the selected rotator"));
    g_signal_connect (delbutton, "clicked", G_CALLBACK (delete_cb), NULL);

    /* vertical button box */
    box = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (box), GTK_BUTTONBOX_START);

    gtk_container_add (GTK_CONTAINER (box), addbutton);
    gtk_container_add (GTK_CONTAINER (box), editbutton);
    gtk_container_add (GTK_CONTAINER (box), delbutton);


    return box;
}



/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void sat_pref_rot_cancel ()
{
}


/** \brief User pressed OK. Any changes should be stored in config.
 * 
 * First, all .rot files are deleted, whereafter the rotator configurations in
 * the rotlist are saved one by one.
 */
void sat_pref_rot_ok     ()
{
    GDir         *dir = NULL;   /* directory handle */
    GError       *error = NULL; /* error flag and info */
    gchar        *buff,*dirname;
    const gchar  *filename;
    GtkTreeIter   iter;      /* new item added to the list store */
    GtkTreeModel *model;
    guint         i,n;
    
    rotor_conf_t conf = {
        .name  = NULL,
        .host = NULL,
        .port  = 4533,
        .minaz = 0,
        .maxaz = 360,
        .minel = 0,
        .maxel = 90,
        .aztype = ROT_AZ_TYPE_360,
    };

    
    /* delete all .rot files */
    dirname = get_hwconf_dir ();
    
    dir = g_dir_open (dirname, 0, &error);
    if (dir) {
        /* read each .rot file */
        while ((filename = g_dir_read_name (dir))) {

            if (g_str_has_suffix (filename, ".rot")) {

                buff = g_strconcat (dirname, G_DIR_SEPARATOR_S, filename, NULL);
                g_remove (buff);
                g_free (buff);
            }
        }
    }

    g_free (dirname);
    g_dir_close (dir);
    
    /* create new .rot files for the radios in the rotlist */
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (rotlist));
    n = gtk_tree_model_iter_n_children (model, NULL);
    for (i = 0; i < n; i++) {
        
        /* get radio conf */
        if (gtk_tree_model_iter_nth_child (model, &iter, NULL, i)) {
        
            /* store conf */
            gtk_tree_model_get (model, &iter,
                                ROT_LIST_COL_NAME, &conf.name,
                                ROT_LIST_COL_HOST, &conf.host,
                                ROT_LIST_COL_PORT, &conf.port,
                                ROT_LIST_COL_MINAZ, &conf.minaz,
                                ROT_LIST_COL_MAXAZ, &conf.maxaz,
                                ROT_LIST_COL_MINEL, &conf.minel,
                                ROT_LIST_COL_MAXEL, &conf.maxel,
                                ROT_LIST_COL_AZTYPE, &conf.aztype,
                                -1);
            rotor_conf_save (&conf);
        
            /* free conf buffer */
            if (conf.name)
                g_free (conf.name);
            
            if (conf.host)
                g_free (conf.host);
            
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Failed to get ROT %s"),
                           __func__, i);
        }
    }
    
}


/** \brief Add a new rotor configuration
 * \param button Pointer to the Add button.
 * \param data User data (null).
 * 
 * This function executes the rotor configuration editor.
 */
static void add_cb    (GtkWidget *button, gpointer data)
{
    GtkTreeIter  item;      /* new item added to the list store */
    GtkListStore *liststore;
    
    (void) button; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */
    
    rotor_conf_t conf = {
        .name  = NULL,
        .host  = NULL,
        .port  = 4533,
        .minaz = 0,
        .maxaz = 360,
        .minel = 0,
        .maxel = 90,
        .aztype = ROT_AZ_TYPE_360,
    };
    
    /* run rot conf editor */
    sat_pref_rot_editor_run (&conf);
    
    /* add new rot to the list */
    if (conf.name != NULL) {
        liststore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (rotlist)));
        gtk_list_store_append (liststore, &item);
        gtk_list_store_set (liststore, &item,
                            ROT_LIST_COL_NAME, conf.name,
                            ROT_LIST_COL_HOST, conf.host,
                            ROT_LIST_COL_PORT, conf.port,
                            ROT_LIST_COL_MINAZ, conf.minaz,
                            ROT_LIST_COL_MAXAZ, conf.maxaz,
                            ROT_LIST_COL_MINEL, conf.minel,
                            ROT_LIST_COL_MAXEL, conf.maxel,
                            ROT_LIST_COL_AZTYPE, conf.aztype,
                            -1);
        
        g_free (conf.name);
        
        if (conf.host != NULL)
            g_free (conf.host);
        
    }
}


/** \brief Add a new rotor configuration
 * \param button Pointer to the Add button.
 * \param data User data (null).
 * 
 * This function executes the rotor configuration editor.
 * 
 */
static void edit_cb   (GtkWidget *button, gpointer data)
{
    GtkTreeModel     *model = gtk_tree_view_get_model (GTK_TREE_VIEW(rotlist));
    GtkTreeModel     *selmod;
    GtkTreeSelection *selection;
    GtkTreeIter       iter;
    
    (void) button; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */

    rotor_conf_t conf = {
        .name  = NULL,
        .host  = NULL,
        .port  = 4533,
        .minaz = 0,
        .maxaz = 360,
        .minel = 0,
        .maxel = 90,
        .aztype = ROT_AZ_TYPE_360,
    };

    
    /* If there are no entries, we have a bug since the button should 
    have been disabled. */
    if (gtk_tree_model_iter_n_children (model, NULL) < 1) {

        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Edit button should have been disabled."),
                       __FILE__, __func__);
        //gtk_widget_set_sensitive (button, FALSE);
        
        return;
    }
    
    /* get selected row
    FIXME: do we really need to work with two models?
    */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (rotlist));
    if (gtk_tree_selection_get_selected(selection, &selmod, &iter)) {
        gtk_tree_model_get (model, &iter,
                            ROT_LIST_COL_NAME, &conf.name,
                            ROT_LIST_COL_HOST, &conf.host,
                            ROT_LIST_COL_PORT, &conf.port,
                            ROT_LIST_COL_MINAZ, &conf.minaz,
                            ROT_LIST_COL_MAXAZ, &conf.maxaz,
                            ROT_LIST_COL_MINEL, &conf.minel,
                            ROT_LIST_COL_MAXEL, &conf.maxel,
                            ROT_LIST_COL_AZTYPE, &conf.aztype,
                            -1);

    }
    else {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                        _("Select the rotator you want to edit\n"\
                                          "and try again!"));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        return;
    }

    /* run radio configuration editor */
    sat_pref_rot_editor_run (&conf);
    
    /* apply changes */
    if (conf.name != NULL) {
        gtk_list_store_set (GTK_LIST_STORE(model), &iter,
                            ROT_LIST_COL_NAME, conf.name,
                            ROT_LIST_COL_HOST, conf.host,
                            ROT_LIST_COL_PORT, conf.port,
                            ROT_LIST_COL_MINAZ, conf.minaz,
                            ROT_LIST_COL_MAXAZ, conf.maxaz,
                            ROT_LIST_COL_MINEL, conf.minel,
                            ROT_LIST_COL_MAXEL, conf.maxel,
                            ROT_LIST_COL_AZTYPE, conf.aztype,
                            -1);
        
    }

    /* clean up memory */
    if (conf.name)
        g_free (conf.name);
        
    if (conf.host != NULL)
        g_free (conf.host);
        

}


/** \brief Delete selected rotator configuration
 *
 * This function is called when the user clicks the Delete button.
 * 
 */
static void delete_cb (GtkWidget *button, gpointer data)
{
    GtkTreeModel     *model = gtk_tree_view_get_model (GTK_TREE_VIEW(rotlist));
    GtkTreeSelection *selection;
    GtkTreeIter       iter;

    (void) button; /* avoid unused parameter compiler warning */
    (void) data; /* avoid unused parameter compiler warning */

    /* If there are no entries, we have a bug since the button should 
       have been disabled. */
    if (gtk_tree_model_iter_n_children (model, NULL) < 1) {

        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Delete button should have been disabled."),
                     __FILE__, __func__);
        //gtk_widget_set_sensitive (button, FALSE);
        
        return;
    }
    
    /* get selected row */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (rotlist));
    if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        gtk_list_store_remove (GTK_LIST_STORE(model), &iter);
    }
    else {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         _("Select the rotator you want to delete\n"\
                                           "and try again!"));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

    }
}

static void row_activated_cb (GtkTreeView       *tree_view,
                              GtkTreePath       *path,
                              GtkTreeViewColumn *column,
                              gpointer           user_data)
{
    (void) tree_view; /* avoid unused parameter compiler warning */
    (void) path; /* avoid unused parameter compiler warning */
    (void) column; /* avoid unused parameter compiler warning */
    (void) user_data; /* avoid unused parameter compiler warning */

    edit_cb (editbutton, NULL);
}

/** \brief Render an angle.
 * \param col Pointer to the tree view column.
 * \param renderer Pointer to the renderer.
 * \param model Pointer to the tree model.
 * \param iter Pointer to the tree iterator.
 * \param column The column number in the model.
 * 
 */
static void render_angle (GtkTreeViewColumn *col,
                          GtkCellRenderer   *renderer,
                          GtkTreeModel      *model,
                          GtkTreeIter       *iter,
                          gpointer           column)
{
    gdouble   number;
    guint   coli = GPOINTER_TO_UINT (column);
    gchar  *text;
    
    (void) col; /* avoid unused parameter compiler warning */

    gtk_tree_model_get (model, iter, coli, &number, -1);

    text = g_strdup_printf ("%.0f\302\260", number);
    g_object_set (renderer, "text", text, NULL);
    g_free (text);
}


/** \brief Render the azimuth type.
 * \param col Pointer to the tree view column.
 * \param renderer Pointer to the renderer.
 * \param model Pointer to the tree model.
 * \param iter Pointer to the tree iterator.
 * \param column The column number in the model.
 * 
 */
static void render_aztype (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           column)
{
    gint   number;
    guint   coli = GPOINTER_TO_UINT (column);
    gchar  *text;
    
    (void) col; /* avoid unused parameter compiler warning */
    
    gtk_tree_model_get (model, iter, coli, &number, -1);

    switch (number) {
        case ROT_AZ_TYPE_360:
            text = g_strdup_printf ("0\302\260 \342\206\222 180\302\260 \342\206\222 360\302\260");
            break;
            
        case ROT_AZ_TYPE_180:
            text = g_strdup_printf ("-180\302\260 \342\206\222 0\302\260 \342\206\222 +180\302\260");
            break;
            
        default:
            text = g_strdup_printf (_("Unknown (%d)"), number);
            break;
    }
    
    g_object_set (renderer, "text", text, NULL);
    g_free (text);
}

