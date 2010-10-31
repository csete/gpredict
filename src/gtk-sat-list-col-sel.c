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
/** \brief SatList column selector.
 *
 * More info...
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gtk-sat-list.h"
#include "gtk-sat-list-col-sel.h"



/* defined in gtk-sat-list.c; we use them for labels */
extern const gchar *SAT_LIST_COL_HINT[];



static void gtk_sat_list_col_sel_class_init (GtkSatListColSelClass *class);
static void gtk_sat_list_col_sel_init       (GtkSatListColSel      *sel);
static void gtk_sat_list_col_sel_destroy    (GtkObject             *object);

static GtkTreeModel *create_and_fill_model (guint32 flags);
static void          column_toggled        (GtkCellRendererToggle *cell,
                             gchar                 *path_str,
                             gpointer               data);

static gboolean set_col (GtkTreeModel *model,
                GtkTreePath *path,
                GtkTreeIter *iter,
                gpointer data);


static GtkVBoxClass *parent_class = NULL;


GType
gtk_sat_list_col_sel_get_type ()
{
     static GType gtk_sat_list_col_sel_type = 0;

     if (!gtk_sat_list_col_sel_type)
     {
          static const GTypeInfo gtk_sat_list_col_sel_info =
               {
                    sizeof (GtkSatListColSelClass),
                    NULL,  /* base_init */
                    NULL,  /* base_finalize */
                    (GClassInitFunc) gtk_sat_list_col_sel_class_init,
                    NULL,  /* class_finalize */
                    NULL,  /* class_data */
                    sizeof (GtkSatListColSel),
                    5,     /* n_preallocs */
                    (GInstanceInitFunc) gtk_sat_list_col_sel_init,
               };

          gtk_sat_list_col_sel_type = g_type_register_static (GTK_TYPE_VBOX,
                                            "GtkSatListColSel",
                                            &gtk_sat_list_col_sel_info,
                                            0);
     }

     return gtk_sat_list_col_sel_type;
}


static void
gtk_sat_list_col_sel_class_init (GtkSatListColSelClass *class)
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

     object_class->destroy = gtk_sat_list_col_sel_destroy;
 
}



static void
gtk_sat_list_col_sel_init (GtkSatListColSel *list)
{

}

static void
gtk_sat_list_col_sel_destroy (GtkObject *object)
{
     //GtkSatListColSel *sel = GTK_SAT_LIST_COL_SEL (object);

     (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



GtkWidget *
gtk_sat_list_col_sel_new (guint32 flags)
{
     GtkWidget         *widget;
     GtkSatListColSel  *sel;
     GtkTreeModel      *model;
     GtkCellRenderer   *renderer;
     GtkTreeViewColumn *column;


     widget = g_object_new (GTK_TYPE_SAT_LIST_COL_SEL, NULL);
     sel = GTK_SAT_LIST_COL_SEL (widget);

     sel->flags = flags;

     /* create list and model */
     sel->list = gtk_tree_view_new ();
     model = create_and_fill_model (flags);
     gtk_tree_view_set_model (GTK_TREE_VIEW (sel->list), model);
     g_object_unref (model);

     /* create tree view columns */
     /* label column */
     renderer = gtk_cell_renderer_text_new ();
     column = gtk_tree_view_column_new_with_attributes (_("Column Name"), renderer,
                                      "text", 0, NULL);
     gtk_tree_view_insert_column (GTK_TREE_VIEW (sel->list), column, -1);

     /* checkbox column */
     renderer = gtk_cell_renderer_toggle_new ();
     sel->handler_id = g_signal_connect (renderer, "toggled",
                             G_CALLBACK (column_toggled), widget);

     column = gtk_tree_view_column_new_with_attributes (_("Visible"), renderer,
                                      "active", 1, NULL);
     gtk_tree_view_append_column (GTK_TREE_VIEW (sel->list), column);
     gtk_tree_view_column_set_alignment (column, 0.5);

     /* invisible row number */
     renderer = gtk_cell_renderer_text_new ();
     column = gtk_tree_view_column_new_with_attributes (_("Row"), renderer,
                                      "text", 2, NULL);

     gtk_tree_view_append_column (GTK_TREE_VIEW (sel->list), column);
     gtk_tree_view_column_set_visible (column, FALSE);
     
     /* this is discouraged but looks cool in this case */
     gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sel->list), TRUE);

     /* scrolled window */
     GTK_SAT_LIST_COL_SEL (widget)->swin = gtk_scrolled_window_new (NULL, NULL);
     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (GTK_SAT_LIST_COL_SEL (widget)->swin),
                         GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

     gtk_container_add (GTK_CONTAINER (GTK_SAT_LIST_COL_SEL (widget)->swin),
                  GTK_SAT_LIST_COL_SEL (widget)->list);

     gtk_container_add (GTK_CONTAINER (widget), GTK_SAT_LIST_COL_SEL (widget)->swin);
     gtk_widget_show_all (widget);


     return widget;
}

static GtkTreeModel *
create_and_fill_model (guint32 flags)
{
     GtkListStore *liststore;    /* the list store data structure */
     GtkTreeIter  item;       /* new item added to the list store */
     guint i;
     gboolean checked;

     /* create a new list store */
     liststore = gtk_list_store_new (3,
                         G_TYPE_STRING,    // column label
                         G_TYPE_BOOLEAN,   // visible checkbox
                         G_TYPE_INT        // row number
                         );


     for (i = 0; i < SAT_LIST_COL_NUMBER; i++) {

          checked = (flags & (1 << i)) ? TRUE : FALSE;

          gtk_list_store_append (liststore, &item);
          gtk_list_store_set (liststore, &item,
                        0, SAT_LIST_COL_HINT[i],
                        1, checked,
                        2, i,
                        -1);

     }


     return GTK_TREE_MODEL (liststore);
}



guint32
gtk_sat_list_col_sel_get_flags (GtkSatListColSel *sel)
{

     g_return_val_if_fail ((sel != NULL) || !IS_GTK_SAT_LIST_COL_SEL (sel), 0);

     return sel->flags;
}


void
gtk_sat_list_col_sel_set_flags (GtkSatListColSel *sel, guint32 flags)
{
     GtkTreeModel *liststore;    /* the list store data structure */


     g_return_if_fail ((sel != NULL) || !IS_GTK_SAT_LIST_COL_SEL (sel));

     sel->flags = flags;


     liststore = gtk_tree_view_get_model (GTK_TREE_VIEW (sel->list));
     gtk_tree_model_foreach (liststore, set_col, sel);
}


static gboolean
set_col (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
     GtkSatListColSel *sel = GTK_SAT_LIST_COL_SEL (data);
     guint i;
     gboolean checked;

     /* get row number */
     gtk_tree_model_get (model, iter, 2, &i, -1);

     /* get flag */
     checked = (sel->flags & (1 << i)) ? TRUE : FALSE;

     /* store flag */
     gtk_list_store_set (GTK_LIST_STORE (model), iter, 1, checked, -1);

     return FALSE;
}





/** \brief Manage toggle signals.
 *  \param cell cell.
 *  \param path_str Path string.
 *  \param data Pointer to the GtkSatListColSel widget.
 *
 * This function is called when the user toggles the visibility for a column.
 */
static void
column_toggled        (GtkCellRendererToggle *cell,
                 gchar                 *path_str,
                 gpointer               data)
{
     GtkSatListColSel *sel = GTK_SAT_LIST_COL_SEL (data);
     GtkTreeModel *model;
     GtkTreeIter   iter;
     GtkTreePath  *path = gtk_tree_path_new_from_string (path_str);
     gboolean      checked;
     gint          row;


     /* block toggle signals while we mess with the check boxes */
     g_signal_handler_block (cell, sel->handler_id);

     /* get toggled iter */
     model = gtk_tree_view_get_model (GTK_TREE_VIEW (sel->list));
     gtk_tree_model_get_iter (model, &iter, path);
     gtk_tree_model_get (model, &iter,
                   1, &checked,
                   2, &row,
                   -1);

     /* reverse status */
     checked = !checked;

     gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                   1, checked,
                   -1);

     if (checked) {
          /* turn bit ON */
          sel->flags |= (1 << row);
     }
     else {
          /* turn bit OFF */
          sel->flags &= ~(1 << row);
     }

     /* clean up */
     gtk_tree_path_free (path);

     /* unblock toggle signals */
     g_signal_handler_unblock (cell, sel->handler_id);

}
