/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2008  Alexandru Csete, OZ9AEC.

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
/** \brief Satellite selector.
 *
 * FIXME: add search/lookup function
 * FIXME: epoch not implemented
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "gtk-sat-data.h"
#include "compat.h"
#include "gtk-sat-selector.h"







static void gtk_sat_selector_class_init (GtkSatSelectorClass *class);
static void gtk_sat_selector_init       (GtkSatSelector *selector);
static void gtk_sat_selector_destroy    (GtkObject *object);

static void create_and_fill_models      (GtkSatSelector *selector);

static gint compare_func (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      userdata);

static GtkVBoxClass *parent_class = NULL;


GType gtk_sat_selector_get_type ()
{
    static GType gtk_sat_selector_type = 0;

    if (!gtk_sat_selector_type)
    {
        static const GTypeInfo gtk_sat_selector_info =
        {
            sizeof (GtkSatSelectorClass),
            NULL,  /* base_init */
            NULL,  /* base_finalize */
            (GClassInitFunc) gtk_sat_selector_class_init,
            NULL,  /* class_finalize */
            NULL,  /* class_data */
            sizeof (GtkSatSelector),
            1,     /* n_preallocs */
            (GInstanceInitFunc) gtk_sat_selector_init,
        };

        gtk_sat_selector_type = g_type_register_static (GTK_TYPE_VBOX,
                                                        "GtkSatSelector",
                                                        &gtk_sat_selector_info,
                                                        0);
    }

    return gtk_sat_selector_type;
}


static void gtk_sat_selector_class_init (GtkSatSelectorClass *class)
{
    GObjectClass      *gobject_class;
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    gobject_class   = G_OBJECT_CLASS (class);
    object_class    = (GtkObjectClass*) class;
    widget_class    = (GtkWidgetClass*) class;
    container_class = (GtkContainerClass*) class;

    parent_class = g_type_class_peek_parent (class);

    object_class->destroy = gtk_sat_selector_destroy;

}


/** \brief Initialise satellite selector widget */
static void gtk_sat_selector_init (GtkSatSelector *selector)
{
    selector->models = NULL;
}



/** \brief Clean up memory before destroying satellite selector widget */
static void gtk_sat_selector_destroy (GtkObject *object)
{
    GtkSatSelector *selector = GTK_SAT_SELECTOR (object);

    /* clear list of selected satellites */
    /* crashes on 2. instance: g_slist_free (sat_tree->selection); */
    guint n,i;
    gpointer data;

    n = g_slist_length (selector->models);

    for (i = 0; i < n; i++) {
        /* get the first element and delete it */
        data = g_slist_nth_data (selector->models, 0);
        selector->models = g_slist_remove (selector->models, data);
    }


    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



/** \brief Create a new GtkSatSelector widget
 *  \param flags Flags indicating which columns should be visible
 *               (see gtk_sat_selector_flag_t)
 *  \return A GtkSatSelector widget.
 */
GtkWidget *gtk_sat_selector_new (guint flags)
{
    GtkWidget          *widget;
    GtkSatSelector     *selector;
    GtkTreeModel       *model;
    GtkCellRenderer    *renderer;
    GtkTreeViewColumn  *column;
    GtkWidget          *hbox;
    GtkTooltips        *tips;
    GtkWidget          *expbut;
    GtkWidget          *colbut;


    if (!flags)
        flags = GTK_SAT_SELECTOR_DEFAULT_FLAGS;

    widget = g_object_new (GTK_TYPE_SAT_SELECTOR, NULL);
    selector = GTK_SAT_SELECTOR (widget);

    selector->flags = flags;

    /* create list and model */
    selector->tree = gtk_tree_view_new ();
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (selector->tree), TRUE);
    create_and_fill_models (selector);
    model = GTK_TREE_MODEL (g_slist_nth_data (selector->models, 0));
    gtk_tree_view_set_model (GTK_TREE_VIEW (selector->tree), model);
    g_object_unref (model);

    /* sort the tree by name */
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
                                     GTK_SAT_SELECTOR_COL_NAME,
                                     compare_func,
                                     NULL,
                                     NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
                                          GTK_SAT_SELECTOR_COL_NAME,
                                          GTK_SORT_ASCENDING);

    /* create tree view columns */
    /* label column */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Satellite"), renderer,
                                                       "text", GTK_SAT_SELECTOR_COL_NAME,
                                                       NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (selector->tree), column, -1);
    if (!(flags & GTK_SAT_SELECTOR_FLAG_NAME))
        gtk_tree_view_column_set_visible (column, FALSE);

    /* catalogue number */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Catnum"), renderer,
                                                       "text", GTK_SAT_SELECTOR_COL_CATNUM,
                                                       NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (selector->tree), column, -1);
    if (!(flags & GTK_SAT_SELECTOR_FLAG_CATNUM))
        gtk_tree_view_column_set_visible (column, FALSE);

    /* epoch */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Epoch"), renderer,
                                                       "text", GTK_SAT_SELECTOR_COL_EPOCH,
                                                       NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (selector->tree), column, -1);
    if (!(flags & GTK_SAT_SELECTOR_FLAG_EPOCH))
        gtk_tree_view_column_set_visible (column, FALSE);


    /* scrolled window */
    GTK_SAT_SELECTOR (widget)->swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (GTK_SAT_SELECTOR (widget)->swin),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (GTK_SAT_SELECTOR (widget)->swin),
                       GTK_SAT_SELECTOR (widget)->tree);

    //gtk_container_add (GTK_CONTAINER (widget), GTK_SAT_TREE (widget)->swin);
    gtk_box_pack_start (GTK_BOX (widget), GTK_SAT_SELECTOR (widget)->swin, TRUE, TRUE, 0);


    gtk_widget_show_all (widget);


    /* initialise selection */
    //GTK_SAT_TREE (widget)->selection = NULL;


    return widget;
}



/** FIXME: flags not needed here */
static void create_and_fill_models (GtkSatSelector *selector)
{
    GtkListStore *store;    /* the list store data structure */
    GtkTreeIter   node;     /* new top level node added to the tree store */
    GDir         *dir;
    gchar        *dirname;
    sat_t         sat;
    gint          catnum;

    gchar        *path;
    gchar        *nodename;
    gchar       **buffv;
    const gchar  *fname;

    guint         num = 0;




    /* load all satellites into selector->models[0] */
    store = gtk_list_store_new (GTK_SAT_SELECTOR_COL_NUM,
                                G_TYPE_STRING,    // name
                                G_TYPE_INT,       // catnum
                                G_TYPE_STRING     // epoch
                                );
    selector->models = g_slist_append (selector->models, store);

    dirname = get_satdata_dir ();
    dir = g_dir_open (dirname, 0, NULL);
    if (!dir) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Failed to open satdata directory %s."),
                     __FILE__, __FUNCTION__, dirname);

        g_free (dirname);

        return;
    }

    /* Scan data directory for .tle files.
       For each file scan through the file and
       add entry to the tree.
        */
    while ((fname = g_dir_read_name (dir))) {

        if (g_strrstr (fname, ".sat")) {

            buffv = g_strsplit (fname, ".", 0);
            catnum = (gint) g_ascii_strtoll (buffv[0], NULL, 0);

            if (gtk_sat_data_read_sat (catnum, &sat)) {
                /* error */
            }
            else {
                /* read satellite */

                gtk_list_store_append (store, &node);
                gtk_list_store_set (store, &node,
                                    GTK_SAT_SELECTOR_COL_NAME, sat.nickname,
                                    GTK_SAT_SELECTOR_COL_CATNUM, catnum,
                                    -1);

                g_free (sat.name);
                g_free (sat.nickname);
                num++;
            }

            g_strfreev (buffv);
        }
    }
    sat_log_log (SAT_LOG_LEVEL_MSG,
                 _("%s:%s: Read %d satellites into MAIN group."),
                 __FILE__, __FUNCTION__, num);

    /* load satellites from each .cat file into selector->models[i] */
    g_dir_rewind (dir);
    while ((fname = g_dir_read_name (dir))) {
        if (g_strrstr (fname, ".cat")) {

        }
    }

    g_dir_close (dir);
    g_free (dirname);


#if 0
    /* create a new tree store */
    store = gtk_tree_store_new (GTK_SAT_SELECTOR_COL_NUM,
                                G_TYPE_STRING,    // name
                                G_TYPE_INT,       // catnum
                                G_TYPE_STRING     // epoch
                                );


    dirname = g_strconcat (g_get_home_dir (),
                           G_DIR_SEPARATOR_S, ".gpredict2",
                           G_DIR_SEPARATOR_S, "tle",
                           NULL);


    /* debug message */
    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                 _("%s:%d: Directory is: %s"),
                 __FILE__, __LINE__, dirname);

    dir = g_dir_open (dirname, 0, NULL);

    /* no tle files */
    if (!dir) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: No .tle files found in %s."),
                     __FILE__, __LINE__, dirname);

        g_free (dirname);

        return GTK_TREE_MODEL (store);;
    }

    /* Scan data directory for .tle files.
           For each file scan through the file and
           add entry to the tree.
        */
    while ((fname = g_dir_read_name (dir))) {

        if (g_strrstr (fname, ".tle")) {

            buffv = g_strsplit (fname, ".tle", 0);
            nodename = g_strdup (buffv[0]);
            nodename[0] = g_ascii_toupper (nodename[0]);

            /* create a new top level node in the tree */
            gtk_tree_store_append (store, &node, NULL);
            gtk_tree_store_set (store, &node,
                                GTK_SAT_SELECTOR_COL_NAME, nodename,
                                -1);

            /* build full path til file and sweep it for sats */
            path = g_strconcat (dirname, G_DIR_SEPARATOR_S,
                                fname, NULL);

            num = scan_tle_file (path, store, &node);

            g_free (path);
            g_free (nodename);
            g_strfreev (buffv);

            sat_log_log (SAT_LOG_LEVEL_MSG,
                         _("%s:%d: Read %d sats from %s "),
                         __FILE__, __LINE__, num, fname);
        }

    }

    g_dir_close (dir);
    g_free (dirname);
#endif


}








/** \brief Compare two rows of the GtkSatSelector.
 *  \param model The tree model of the GtkSatSelector.
 *  \param a The first row.
 *  \param b The second row.
 *  \param userdata Not used.
 *
 * This function is used by the sorting algorithm to compare two rows of the
 * GtkSatSelector widget. The unctions works by comparing the character strings
 * in the name column.
 */
static gint compare_func (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      userdata)
{
    gchar *sat1,*sat2;
    gint ret = 0;


    gtk_tree_model_get(model, a, GTK_SAT_SELECTOR_COL_NAME, &sat1, -1);
    gtk_tree_model_get(model, b, GTK_SAT_SELECTOR_COL_NAME, &sat2, -1);

    ret = g_ascii_strcasecmp (sat1, sat2);

    g_free (sat1);
    g_free (sat2);

    return ret;
}

