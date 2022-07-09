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

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "compat.h"
#include "gtk-sat-tree.h"
#include "sat-log.h"


static void     gtk_sat_tree_class_init(GtkSatTreeClass * class);
static void     gtk_sat_tree_init(GtkSatTree * sat_tree);
static void     gtk_sat_tree_destroy(GtkObject * object);
static GtkTreeModel *create_and_fill_model(guint flags);
static void     column_toggled(GtkCellRendererToggle * cell,
                               gchar * path_str, gpointer data);
static gint     scan_tle_file(const gchar * path,
                              GtkTreeStore * store, GtkTreeIter * node);
static gboolean check_and_select_sat(GtkTreeModel * model,
                                     GtkTreePath * path,
                                     GtkTreeIter * iter, gpointer data);
static gboolean uncheck_sat(GtkTreeModel * model,
                            GtkTreePath * path,
                            GtkTreeIter * iter, gpointer data);
static gint     compare_func(GtkTreeModel * model,
                             GtkTreeIter * a,
                             GtkTreeIter * b, gpointer userdata);
static void     expand_cb(GtkWidget * button, gpointer tree);
static void     collapse_cb(GtkWidget * button, gpointer tree);


static GtkVBoxClass *parent_class = NULL;


GType gtk_sat_tree_get_type()
{
    static GType    gtk_sat_tree_type = 0;

    if (!gtk_sat_tree_type)
    {
        static const GTypeInfo gtk_sat_tree_info = {
            sizeof(GtkSatTreeClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_sat_tree_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkSatTree),
            1,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_sat_tree_init,
            NULL
        };

        gtk_sat_tree_type = g_type_register_static(GTK_TYPE_VBOX,
                                                   "GtkSatTree",
                                                   &gtk_sat_tree_info, 0);
    }

    return gtk_sat_tree_type;
}

static void gtk_sat_tree_class_init(GtkSatTreeClass * class)
{
    GObjectClass   *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    gobject_class = G_OBJECT_CLASS(class);
    object_class = (GtkObjectClass *) class;
    widget_class = (GtkWidgetClass *) class;
    container_class = (GtkContainerClass *) class;

    parent_class = g_type_class_peek_parent(class);

    object_class->destroy = gtk_sat_tree_destroy;
}

static void gtk_sat_tree_init(GtkSatTree * sat_tree)
{
    (void)sat_tree;
}

static void gtk_sat_tree_destroy(GtkObject * object)
{
    GtkSatTree     *sat_tree = GTK_SAT_TREE(object);

    /* clear list of selected satellites */
    /* crashes on 2. instance: g_slist_free (sat_tree->selection); */
    guint           n, i;
    gpointer        data;

    n = g_slist_length(sat_tree->selection);

    for (i = 0; i < n; i++)
    {
        /* get the first element and delete it */
        data = g_slist_nth_data(sat_tree->selection, 0);
        sat_tree->selection = g_slist_remove(sat_tree->selection, data);
    }

    (*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

/**
 * Create a new GtkSatTree widget
 *
 * @param flags Flags indicating which columns should be visible
 *              (see gtk_sat_tree_flag_t)
 * @return A GtkSatTree widget.
 */
GtkWidget      *gtk_sat_tree_new(guint flags)
{
    GtkWidget      *widget;
    GtkSatTree     *sat_tree;
    GtkTreeModel   *model;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget      *hbox;
    GtkWidget      *expbut;
    GtkWidget      *colbut;

    if (!flags)
        flags = GTK_SAT_TREE_DEFAULT_FLAGS;

    widget = g_object_new(GTK_TYPE_SAT_TREE, NULL);
    sat_tree = GTK_SAT_TREE(widget);

    sat_tree->flags = flags;

    /* create list and model */
    sat_tree->tree = gtk_tree_view_new();
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(sat_tree->tree), TRUE);
    model = create_and_fill_model(flags);
    gtk_tree_view_set_model(GTK_TREE_VIEW(sat_tree->tree), model);
    g_object_unref(model);

    /* sort the tree by name */
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model),
                                    GTK_SAT_TREE_COL_NAME,
                                    compare_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
                                         GTK_SAT_TREE_COL_NAME,
                                         GTK_SORT_ASCENDING);

    /* create tree view columns */
    /* label column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Satellite"), renderer,
                                                      "text",
                                                      GTK_SAT_TREE_COL_NAME,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(sat_tree->tree), column, -1);
    if (!(flags & GTK_SAT_TREE_FLAG_NAME))
        gtk_tree_view_column_set_visible(column, FALSE);

    /* catalogue number */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Catnum"), renderer,
                                                      "text",
                                                      GTK_SAT_TREE_COL_CATNUM,
                                                      "visible",
                                                      GTK_SAT_TREE_COL_VIS,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(sat_tree->tree), column, -1);
    if (!(flags & GTK_SAT_TREE_FLAG_CATNUM))
        gtk_tree_view_column_set_visible(column, FALSE);

    /* epoch */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Epoch"), renderer,
                                                      "text",
                                                      GTK_SAT_TREE_COL_EPOCH,
                                                      "visible",
                                                      GTK_SAT_TREE_COL_VIS,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(sat_tree->tree), column, -1);
    if (!(flags & GTK_SAT_TREE_FLAG_EPOCH))
        gtk_tree_view_column_set_visible(column, FALSE);

    /* checkbox column */
    renderer = gtk_cell_renderer_toggle_new();
    sat_tree->handler_id = g_signal_connect(renderer, "toggled",
                                            G_CALLBACK(column_toggled),
                                            widget);

    column = gtk_tree_view_column_new_with_attributes(_("Selected"), renderer,
                                                      "active",
                                                      GTK_SAT_TREE_COL_SEL,
                                                      "visible",
                                                      GTK_SAT_TREE_COL_VIS,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sat_tree->tree), column);
    gtk_tree_view_column_set_alignment(column, 0.5);
    if (!(flags & GTK_SAT_TREE_FLAG_SEL))
        gtk_tree_view_column_set_visible(column, FALSE);


    /* scrolled window */
    GTK_SAT_TREE(widget)->swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
                                   (GTK_SAT_TREE(widget)->swin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(GTK_SAT_TREE(widget)->swin),
                      GTK_SAT_TREE(widget)->tree);

    //gtk_container_add (GTK_CONTAINER (widget), GTK_SAT_TREE (widget)->swin);
    gtk_box_pack_start(GTK_BOX(widget), GTK_SAT_TREE(widget)->swin, TRUE, TRUE,
                       0);

    /* expand and collabse buttons */
    expbut = gtk_button_new_with_label(_("Expand"));
    gtk_widget_set_tooltip_text(expbut,
                                _
                                ("Expand all nodes in the tree to make it searchable"));
    g_signal_connect(expbut, "clicked", G_CALLBACK(expand_cb), sat_tree);

    colbut = gtk_button_new_with_label(_("Collapse"));
    gtk_widget_set_tooltip_text(colbut, _("Collapse all nodes in the tree"));
    g_signal_connect(colbut, "clicked", G_CALLBACK(collapse_cb), sat_tree);

    hbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_START);
    gtk_box_pack_start(GTK_BOX(hbox), expbut, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), colbut, FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(widget), hbox, FALSE, FALSE, 5);

    gtk_widget_show_all(widget);

    /* initialise selection */
    GTK_SAT_TREE(widget)->selection = NULL;

    return widget;
}

/** FIXME: flags not needed here */
static GtkTreeModel *create_and_fill_model(guint flags)
{
    GtkTreeStore   *store;      /* the list store data structure */
    GtkTreeIter     node;       /* new top level node added to the tree store */
    GDir           *dir;
    gchar          *dirname;
    gchar          *path;
    gchar          *nodename;
    gchar         **buffv;
    const gchar    *fname;
    guint           num = 0;

    (void)flags;

    /* create a new tree store */
    store = gtk_tree_store_new(GTK_SAT_TREE_COL_NUM, G_TYPE_STRING,     // name
                               G_TYPE_INT,      // catnum
                               G_TYPE_STRING,   // epoch
                               G_TYPE_BOOLEAN,  // selected
                               G_TYPE_BOOLEAN   // visible
        );

    dirname = g_strconcat(g_get_home_dir(),
                          G_DIR_SEPARATOR_S, ".gpredict2",
                          G_DIR_SEPARATOR_S, "tle", NULL);

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%d: Directory is: %s"), __FILE__, __LINE__, dirname);

    dir = g_dir_open(dirname, 0, NULL);

    /* no tle files */
    if (!dir)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: No .tle files found in %s."),
                    __FILE__, __LINE__, dirname);

        g_free(dirname);

        return GTK_TREE_MODEL(store);;
    }

    /* Scan data directory for .tle files.
       For each file scan through the file and
       add entry to the tree.
     */
    while ((fname = g_dir_read_name(dir)))
    {

        if (g_str_has_suffix(fname, ".tle"))
        {

            buffv = g_strsplit(fname, ".tle", 0);
            nodename = g_strdup(buffv[0]);
            nodename[0] = g_ascii_toupper(nodename[0]);

            /* create a new top level node in the tree */
            gtk_tree_store_append(store, &node, NULL);
            gtk_tree_store_set(store, &node,
                               GTK_SAT_TREE_COL_NAME, nodename,
                               GTK_SAT_TREE_COL_VIS, FALSE, -1);

            /* build full path til file and sweep it for sats */
            path = g_strconcat(dirname, G_DIR_SEPARATOR_S, fname, NULL);

            num = scan_tle_file(path, store, &node);

            g_free(path);
            g_free(nodename);
            g_strfreev(buffv);

            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%d: Read %d sats from %s "),
                        __FILE__, __LINE__, num, fname);
        }
    }

    g_dir_close(dir);
    g_free(dirname);

    return GTK_TREE_MODEL(store);
}

/**
 * Scan .tle file and add satellites to GtkTreeStore.
 *
 * @param path Full path of the tle file.
 * @param store The GtkTreeStore to store the satellites into.
 * @param node The parent node for the satellites
 * @return The number of satellites that have been read into the tree.
 */
static gint scan_tle_file(const gchar * path, GtkTreeStore * store,
                          GtkTreeIter * node)
{
    guint           i = 0;
    guint           j;
    GIOChannel     *tlefile;
    GError         *error = NULL;
    GtkTreeIter     sat_iter;
    gchar          *line;
    gsize           length;
    gchar           catstr[6];

    gchar          *satnam;
    guint           catnum;

    /* open IO channel and read 3 lines at a time */
    tlefile = g_io_channel_new_file(path, "r", &error);

    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open %s (%s)"),
                    __FILE__, __LINE__, path, error->message);
        g_clear_error(&error);
    }
    else if (tlefile)
    {

        /*** FIXME: More error handling please */

        while (g_io_channel_read_line(tlefile, &line, &length, NULL, NULL) !=
               G_IO_STATUS_EOF)
        {

            /* satellite name can be found in the first line */
            satnam = g_strdup(line);
            g_strchomp(satnam);

            /* free allocated line */
            g_free(line);

            /* extract catnum from second line; index 2..6 */
            g_io_channel_read_line(tlefile, &line, &length, NULL, NULL);

            for (j = 2; j < 7; j++)
            {
                catstr[j - 2] = line[j];
            }
            catstr[5] = '\0';

            catnum = (guint) g_ascii_strtod(catstr, NULL);

            /* insert satnam and catnum */
            gtk_tree_store_append(store, &sat_iter, node);
            gtk_tree_store_set(store, &sat_iter,
                               GTK_SAT_TREE_COL_NAME, satnam,
                               GTK_SAT_TREE_COL_CATNUM, catnum,
                               GTK_SAT_TREE_COL_SEL, FALSE,
                               GTK_SAT_TREE_COL_VIS, TRUE, -1);

            g_free(satnam);
            g_free(line);

            /* read the third line */
            g_io_channel_read_line(tlefile, &line, &length, NULL, NULL);

            g_free(line);
            i++;
        }

        /* close IO channel; don't care about status */
        g_io_channel_shutdown(tlefile, TRUE, NULL);
        g_io_channel_unref(tlefile);
    }

    return i;
}

/**
 * Manage toggle signals.
 *
 * @param cell cell.
 * @param path_str Path string.
 * @param data Pointer to the GtkSatTree widget.
 *
 * This function is called when the user toggles the visibility for a column.
 * It will add or remove the toggled satellite from the list of selected sats.
 */
static void column_toggled(GtkCellRendererToggle * cell,
                           gchar * path_str, gpointer data)
{
    GtkSatTree     *sat_tree = GTK_SAT_TREE(data);
    GtkTreeModel   *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(sat_tree->tree));
    GtkTreePath    *path = gtk_tree_path_new_from_string(path_str);
    GtkTreeIter     iter;
    gboolean        toggle_item;
    guint           catnum;

    (void)cell;

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       GTK_SAT_TREE_COL_CATNUM, &catnum,
                       GTK_SAT_TREE_COL_SEL, &toggle_item, -1);

    /* do something with the value */
    toggle_item ^= 1;

    if (toggle_item)
    {

        /* only append if sat not already in list */
        if (!g_slist_find(sat_tree->selection, GUINT_TO_POINTER(catnum)))
        {
            sat_tree->selection = g_slist_append(sat_tree->selection,
                                                 GUINT_TO_POINTER(catnum));
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s:%d: Satellite %d selected."),
                        __FILE__, __LINE__, catnum);

            /* Scan the tree for other instances of this sat. For example is
               CUTE-1.7 present in both AMATEUR and CUBESAT.
               We will need access to both the sat_tree and the catnum in the
               foreach callback, so we attach catnum as data to the sat_tree
             */
            g_object_set_data(G_OBJECT(sat_tree), "tmp",
                              GUINT_TO_POINTER(catnum));

            /* find the satellite in the tree */
            gtk_tree_model_foreach(model, check_and_select_sat, sat_tree);

        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%d: Satellite %d already selected; skip..."),
                        __FILE__, __LINE__, catnum);
        }
    }
    else
    {
        sat_tree->selection = g_slist_remove(sat_tree->selection,
                                             GUINT_TO_POINTER(catnum));
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%d: Satellite %d de-selected."),
                    __FILE__, __LINE__, catnum);

        /* Scan the tree for other instances of this sat. For example is
           CUTE-1.7 present in both AMATEUR and CUBESAT.
           We will need access to both the sat_tree and the catnum in the
           foreach callback, so we attach catnum as data to the sat_tree
         */
        g_object_set_data(G_OBJECT(sat_tree), "tmp", GUINT_TO_POINTER(catnum));

        /* find the satellite in the tree */
        gtk_tree_model_foreach(model, uncheck_sat, sat_tree);
    }

    /* set new value */
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
                       GTK_SAT_TREE_COL_SEL, toggle_item, -1);

    gtk_tree_path_free(path);
}

/**
 * Select a satellite in the GtkSatTree.
 *
 * @param sat_tree The GtkSatTree widget.
 * @param catnum Catalogue number of satellite to be selected.
 */
void gtk_sat_tree_select(GtkSatTree * sat_tree, guint catnum)
{
    /* sanity check */
    if ((sat_tree == NULL) || !IS_GTK_SAT_TREE(sat_tree))
    {

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Invalid GtkSatTree!"), __func__);

        return;
    }

    if (!g_slist_find(sat_tree->selection, GUINT_TO_POINTER(catnum)))
    {

        GtkTreeModel   *model =
            gtk_tree_view_get_model(GTK_TREE_VIEW(sat_tree->tree));

        /* we will need access to both the sat_tree and the catnum in the
           foreach callback, so we attach catnum as data to the sat_tree
         */
        g_object_set_data(G_OBJECT(sat_tree), "tmp", GUINT_TO_POINTER(catnum));

        /* find the satellite in the tree */
        gtk_tree_model_foreach(model, check_and_select_sat, sat_tree);

    }
    else
    {
        /* else do nothing since the sat is already selected */
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Satellite %d already selected; skip..."),
                    __func__, catnum);
    }
}

/**
 * Foreach callback for checking and selecting a satellite.
 *
 * @param model The GtkTreeModel.
 * @param path The GtkTreePath of the current item.
 * @param iter The GtkTreeIter of the current item.
 * @param data Pointer to the GtkSatTree structure.
 * @return Always FALSE to let the for-each run to till end.
 *
 * This function is used as foreach-callback in the gtk_sat_tree_select function.
 * The purpoise of the function is to set the check box to chacked state and add
 * the satellite in question to the selection list. The catalogue number of the
 * satellite to be selected is attached as data to the GtkSatTree (key = tmp).
 *
 * The function is also used in the column_toggled callback function with the
 * purpose of locating and selecting other instances of the satellite than the
 * one, on which the user clicked on (meaning: some sats can be found in several
 * TLE file and we want to chak them all, not just the clicked instance).
 */
static gboolean check_and_select_sat(GtkTreeModel * model,
                                     GtkTreePath * path,
                                     GtkTreeIter * iter, gpointer data)
{
    GtkSatTree     *sat_tree = GTK_SAT_TREE(data);
    guint           cat1, cat2;

    (void)path;

    cat1 = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(data), "tmp"));
    gtk_tree_model_get(model, iter, GTK_SAT_TREE_COL_CATNUM, &cat2, -1);

    if (cat1 == cat2)
    {
        /* we have a match */
        gtk_tree_store_set(GTK_TREE_STORE(model), iter,
                           GTK_SAT_TREE_COL_SEL, TRUE, -1);

        /* only append if sat not already in list */
        if (!g_slist_find(sat_tree->selection, GUINT_TO_POINTER(cat1)))
        {
            sat_tree->selection = g_slist_append(sat_tree->selection,
                                                 GUINT_TO_POINTER(cat1));
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s:%d: Satellite %d selected."),
                        __FILE__, __LINE__, cat1);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%d: Satellite %d already selected; skip..."),
                        __FILE__, __LINE__, cat1);
        }

        /* If we return TRUE here, the foreach would terminate.
           We let it run to allow GtkSatTree to mark all instances
           of sat the satellite (some sats may be present in two or
           more .tle files.
         */
        //return TRUE;
    }

    /* continue in order to catch ALL instances of sat */
    return FALSE;
}

/**
 * Foreach callback for unchecking a satellite.
 *
 * @param model The GtkTreeModel.
 * @param path The GtkTreePath of the current item.
 * @param iter The GtkTreeIter of the current item.
 * @param data Pointer to the GtkSatTree structure.
 * @return Always FALSE to let the for-each run to till end.
 *
 * This function is very similar to the check_and_select callback except that it
 * is used only to uncheck a deselected satellite.
 */
static gboolean uncheck_sat(GtkTreeModel * model,
                            GtkTreePath * path,
                            GtkTreeIter * iter, gpointer data)
{
    guint           cat1, cat2;

    (void)path;

    cat1 = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(data), "tmp"));
    gtk_tree_model_get(model, iter, GTK_SAT_TREE_COL_CATNUM, &cat2, -1);

    if (cat1 == cat2)
    {
        /* we have a match */
        gtk_tree_store_set(GTK_TREE_STORE(model), iter,
                           GTK_SAT_TREE_COL_SEL, FALSE, -1);
    }

    /* continue in order to catch ALL instances of sat */
    return FALSE;
}

/**
 * Get list of selected satellites.
 *
 * @param sat_tree The GtkSatTree
 * @param size Return location for number of selected sats.
 * @return A newly allocated array containing the selected satellites or
 *         NULL if no satellites are selected.
 *
 * The returned array should be g_freed when no longer needed.
 */
guint          *gtk_sat_tree_get_selected(GtkSatTree * sat_tree, gsize * size)
{
    guint           i;
    gsize           s;
    guint          *ret;

    /* sanity check */
    if ((sat_tree == NULL) || !IS_GTK_SAT_TREE(sat_tree))
    {

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Invalid GtkSatTree!"), __func__);

        return NULL;
    }

    /* parameter are ok */
    s = g_slist_length(sat_tree->selection);

    if (s < 1)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: There are no satellites selected => NULL."),
                    __func__);

        *size = 0;

        return NULL;
    }

    ret = (guint *) g_try_malloc(s * sizeof(guint));

    for (i = 0; i < s; i++)
    {
        ret[i] = GPOINTER_TO_UINT(g_slist_nth_data(sat_tree->selection, i));
    }

    if (size != NULL)
        *size = s;

    return ret;
}

/**
 * Compare two rows of the GtkSatTree.
 *
 * @param model The tree model of the GtkSatTree.
 * @param a The first row.
 * @param b The second row.
 * @param userdata Not used.
 *
 * This function is used by the sorting algorithm to compare two rows of the
 * GtkSatTree widget. The unctions works by comparing the character strings
 * in the name column.
 */
static gint compare_func(GtkTreeModel * model,
                         GtkTreeIter * a, GtkTreeIter * b, gpointer userdata)
{
    gchar          *sat1, *sat2;
    gint            ret = 0;

    (void)userdata;

    gtk_tree_model_get(model, a, GTK_SAT_TREE_COL_NAME, &sat1, -1);
    gtk_tree_model_get(model, b, GTK_SAT_TREE_COL_NAME, &sat2, -1);

    ret = g_ascii_strcasecmp(sat1, sat2);

    g_free(sat1);
    g_free(sat2);

    return ret;
}

/**
 * Expand all nodes in the GtkSatTree.
 *
 * @param button The GtkButton that received the signal.
 * @param tree Pointer to the GtkSatTree widget.
 *
 * This function expands all rows in the tree view in order to make it
 * searchable.
 */
static void expand_cb(GtkWidget * button, gpointer tree)
{
    (void)button;

    gtk_tree_view_expand_all(GTK_TREE_VIEW(GTK_SAT_TREE(tree)->tree));
}

/**
 * Collapse all nodes in the GtkSatTree.
 *
 * @param button The GtkButton that received the signal.
 * @param tree Pointer to the GtkSatTree widget.
 *
 * This function collapses all rows in the tree view.
 */
static void collapse_cb(GtkWidget * button, gpointer tree)
{
    (void)button;

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(GTK_SAT_TREE(tree)->tree));
}
