/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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
/** \brief Satellite List Widget.
 *
 * More info...
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-event-list.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "mod-cfg-get-param.h"
#include "gtk-sat-data.h"
#include "gpredict-utils.h"
#include "locator.h"
#include "sat-vis.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif

/*** FIXME */
#include "gtk-sat-list-popup.h"



/** \brief Column titles indexed with column symb. refs. */
const gchar *EVENT_LIST_COL_TITLE[EVENT_LIST_COL_NUMBER] = {
    N_("Sat"),
    N_("Catnum"),
    N_("A/L"),
    N_("Time")
};


/** \brief Column title hints indexed with column symb. refs. */
const gchar *EVENT_LIST_COL_HINT[EVENT_LIST_COL_NUMBER] = {
    N_("Satellite Name"),
    N_("Catalogue Number"),
    N_("Next event type (A: AOS, L: LOS)"),
    N_("Countdown until next event")
};

const gfloat EVENT_LIST_COL_XALIGN[EVENT_LIST_COL_NUMBER] = {
    0.0, // name
    0.5, // catnum
    0.5, // event type
    0.5, // time
};

static void          gtk_event_list_class_init (GtkEventListClass *class);
static void          gtk_event_list_init       (GtkEventList      *list);
static void          gtk_event_list_destroy    (GtkObject       *object);
static GtkTreeModel *create_and_fill_model   (GHashTable      *sats);
static void          event_list_add_satellites (gpointer key,
                                                gpointer value,
                                                gpointer user_data);
static gboolean      event_list_update_sats    (GtkTreeModel *model,
                                                GtkTreePath  *path,
                                                GtkTreeIter  *iter,
                                                gpointer      data);

/* cell rendering related functions */
static void          check_and_set_cell_renderer (GtkTreeViewColumn *column,
                                                  GtkCellRenderer   *renderer,
                                                  gint               i);

static void          evtype_cell_data_function (GtkTreeViewColumn *col,
                                                GtkCellRenderer   *renderer,
                                                GtkTreeModel      *model,
                                                GtkTreeIter       *iter,
                                                gpointer           column);

static void          time_cell_data_function (GtkTreeViewColumn *col,
                                              GtkCellRenderer   *renderer,
                                              GtkTreeModel      *model,
                                              GtkTreeIter       *iter,
                                              gpointer           column);

static gint event_cell_compare_function (GtkTreeModel *model,
                                         GtkTreeIter  *a,
                                         GtkTreeIter  *b,
                                         gpointer user_data);


static gboolean   popup_menu_cb   (GtkWidget *treeview,
                                   gpointer list);

static gboolean   button_press_cb (GtkWidget *treeview,
                                   GdkEventButton *event,
                                   gpointer list);

static void       view_popup_menu (GtkWidget *treeview,
                                   GdkEventButton *event,
                                   gpointer list);


static GtkVBoxClass *parent_class = NULL;


GType gtk_event_list_get_type ()
{
    static GType gtk_event_list_type = 0;

    if (!gtk_event_list_type)
        {
            static const GTypeInfo gtk_event_list_info =
                {
                    sizeof (GtkEventListClass),
                    NULL,  /* base_init */
                    NULL,  /* base_finalize */
                    (GClassInitFunc) gtk_event_list_class_init,
                    NULL,  /* class_finalize */
                    NULL,  /* class_data */
                    sizeof (GtkEventList),
                    5,     /* n_preallocs */
                    (GInstanceInitFunc) gtk_event_list_init,
                };

            gtk_event_list_type = g_type_register_static (GTK_TYPE_VBOX,
                                                          "GtkEventList",
                                                          &gtk_event_list_info,
                                                          0);
        }

    return gtk_event_list_type;
}


static void gtk_event_list_class_init (GtkEventListClass *class)
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

    object_class->destroy = gtk_event_list_destroy;
 
}


static void gtk_event_list_init (GtkEventList *list)
{

}


static void gtk_event_list_destroy (GtkObject *object)
{
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/** \brief Create a new GtkEventList widget.
  * \param cfgdata Pointer to the module configuration data.
  * \param sats Hash table containing the satellites tracked by the parent module.
  * \param qth Pointer to the QTH used by this module.
  * \param columns Visible columns (currently not in use).
  *
  */
GtkWidget *gtk_event_list_new (GKeyFile *cfgdata, GHashTable *sats, qth_t *qth, guint32 columns)
{
    GtkWidget    *widget;
    GtkEventList *evlist;
    GtkTreeModel *model;
    guint         i;
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;


    widget = g_object_new (GTK_TYPE_EVENT_LIST, NULL);
    evlist = GTK_EVENT_LIST (widget);

    evlist->update = gtk_event_list_update;

    /* Read configuration data. */
    /* ... */

    evlist->satellites = sats;
    evlist->qth = qth;


    /* initialise column flags */
    evlist->flags = 0;
/*    if (columns > 0)
        evlist->flags = columns;
    else
        evlist->flags = mod_cfg_get_int (cfgdata,
                                         MOD_CFG_EVLIST_SECTION,
                                         MOD_CFG_EVLIST_COLUMNS,
                                         SAT_CFG_INT_EVLIST_COLUMNS);
                                         */
    
    /* get refresh rate and cycle counter */
/*    evlist->refresh = mod_cfg_get_int (cfgdata,
                                       MOD_CFG_EVLIST_SECTION,
                                       MOD_CFG_EVLIST_REFRESH,
                                       SAT_CFG_INT_EVLIST_REFRESH);
                                       */
    evlist->refresh = 1000;

    evlist->counter = 1;

    /* create the tree view and add columns */
    evlist->treeview = gtk_tree_view_new ();

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (evlist->treeview), FALSE);

    /* create treeview columns */
    for (i = 0; i < EVENT_LIST_COL_NUMBER; i++) {

        renderer = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (renderer), "xalign", EVENT_LIST_COL_XALIGN[i], NULL);

        column = gtk_tree_view_column_new_with_attributes (_(EVENT_LIST_COL_TITLE[i]),
                                                           renderer,
                                                           "text", i,
                                                           NULL);
        gtk_tree_view_insert_column (GTK_TREE_VIEW (evlist->treeview),
                                     column, -1);

        /* only aligns the headers */
        gtk_tree_view_column_set_alignment (column, 0.5);

        /* set sort id */
        gtk_tree_view_column_set_sort_column_id (column, i);

        /* set cell data function; allows to format data before rendering */
        check_and_set_cell_renderer (column, renderer, i);

        /* hide columns that have not been specified */
        /*if (!(evlist->flags & (1 << i))) {
            gtk_tree_view_column_set_visible (column, FALSE);
        }*/
        
    }

    /* create model and finalise treeview */
    model = create_and_fill_model (evlist->satellites);
    gtk_tree_view_set_model (GTK_TREE_VIEW (evlist->treeview), model);

    /* The time sort function needs to be special */
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
                                     EVENT_LIST_COL_TIME,
                                     event_cell_compare_function,
                                     NULL, NULL);

    /* initial sorting criteria */
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
                                          EVENT_LIST_COL_TIME,
                                          GTK_SORT_ASCENDING),

    g_object_unref (model);

/*    g_signal_connect (evlist->treeview, "button-press-event",
                      G_CALLBACK (button_press_cb), widget);
    g_signal_connect (evlist->treeview, "popup-menu",
                      G_CALLBACK (popup_menu_cb), widget);*/

    evlist->swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (evlist->swin),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (evlist->swin), evlist->treeview);

    gtk_container_add (GTK_CONTAINER (widget), evlist->swin);
    gtk_widget_show_all (widget);

    return widget;
}


/** \brief Create and file the tree model for the even list. */
static GtkTreeModel *create_and_fill_model   (GHashTable      *sats)
{
    GtkListStore *liststore;


    liststore = gtk_list_store_new (SAT_LIST_COL_NUMBER,
                                    G_TYPE_STRING,     // name
                                    G_TYPE_INT,        // catnum
                                    G_TYPE_BOOLEAN,    // TRUE if AOS, FALSE if LOS
                                    G_TYPE_DOUBLE);    // time

    /* add each satellite from hash table */
    g_hash_table_foreach (sats, event_list_add_satellites, liststore);


    return GTK_TREE_MODEL (liststore);
}


/** \brief Add satellites. This function is a g_hash_table_foreach() callback.
  * \param key The key of the satellite in the hash table.
  * \param value Pointer to the satellite (sat_t structure) that should be added.
  * \param user_data Pointer to the GtkListStore where the satellite should be added
  *
  * This function is called by by the create_and_fill_models() function for adding
  * the satellites to the internal liststore.
  */
static void event_list_add_satellites (gpointer key, gpointer value, gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter   item;
    sat_t        *sat = SAT (value);


    gtk_list_store_append (store, &item);
    gtk_list_store_set (store, &item,
                        EVENT_LIST_COL_NAME, sat->nickname,
                        EVENT_LIST_COL_CATNUM, sat->tle.catnr,
                        EVENT_LIST_COL_EVT, (sat->el >= 0) ? TRUE : FALSE,
                        EVENT_LIST_COL_TIME, 0.0,
                        -1);    
}



/** \brief Update satellites */
void gtk_event_list_update          (GtkWidget *widget)
{
    GtkTreeModel *model;
    GtkEventList   *evlist = GTK_EVENT_LIST (widget);


    /* first, do some sanity checks */
    if ((evlist == NULL) || !IS_GTK_EVENT_LIST (evlist)) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s: Invalid GtkEventList!"),
                     __FUNCTION__);
    }


    /* check refresh rate */
    if (evlist->counter < evlist->refresh) {
        evlist->counter++;
    }
    else {
        evlist->counter = 1;

        /* get and tranverse the model */
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (evlist->treeview));

        /* update */
        gtk_tree_model_foreach (model, event_list_update_sats, evlist);
    }
}


/** \brief Update data in each column in a given row */
static gboolean event_list_update_sats (GtkTreeModel *model,
                                        GtkTreePath  *path,
                                        GtkTreeIter  *iter,
                                        gpointer      data)
{
    GtkEventList *satlist = GTK_EVENT_LIST (data);
    guint      *catnum;
    sat_t      *sat;
    gchar      *buff;
    gint        retcode;


    /* get the catalogue number for this row
       then look it up in the hash table
    */
    catnum = g_new0 (guint, 1);
    gtk_tree_model_get (model, iter, EVENT_LIST_COL_CATNUM, catnum, -1);
    sat = SAT (g_hash_table_lookup (satlist->satellites, catnum));

    if (sat == NULL) {
        /* satellite not tracked anymore => remove */
        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("%s: Failed to get data for #%d."),
                     __FUNCTION__, *catnum);

        gtk_list_store_remove (GTK_LIST_STORE (model), iter);

        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s: Satellite #%d removed from list."),
                     __FUNCTION__, *catnum);
    }
    else {


        /**** FIXME *****/
        /* update data */

        /* store new data */
/*        gtk_list_store_set (GTK_LIST_STORE (model), iter,
                            SAT_LIST_COL_AZ, sat->az,
                            SAT_LIST_COL_EL, sat->el,
                            SAT_LIST_COL_RANGE, sat->range,
                            SAT_LIST_COL_RANGE_RATE, sat->range_rate,
                            SAT_LIST_COL_LAT, sat->ssplat,
                            SAT_LIST_COL_LON, sat->ssplon,
                            SAT_LIST_COL_FOOTPRINT, sat->footprint,
                            SAT_LIST_COL_ALT, sat->alt,
                            SAT_LIST_COL_VEL, sat->velo,
                            SAT_LIST_COL_MA, sat->ma,
                            SAT_LIST_COL_PHASE, sat->phase,
                            SAT_LIST_COL_ORBIT, sat->orbit,
                            -1);
*/


    }

    g_free (catnum);

    /* Return value not documented what to return, but it seems that
       FALSE continues to next row while TRUE breaks
    */
    return FALSE;
}



/** \brief Set cell renderer function. */
static void check_and_set_cell_renderer (GtkTreeViewColumn *column,
                                         GtkCellRenderer   *renderer,
                                         gint               i)
{

    switch (i) {

        /* Event type */
    case EVENT_LIST_COL_EVT:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 evtype_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

        /* time countdown */
    case EVENT_LIST_COL_TIME:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 time_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

    default:
        break;

    }

}


/** \brief Render column containg event type.
  *
  * Event type can be AOS or LOS depending on whether the satellite is within
  * range or not. AOS will rendern an "A", LOS will render an "L".
  */
static void evtype_cell_data_function (GtkTreeViewColumn *col,
                                       GtkCellRenderer   *renderer,
                                       GtkTreeModel      *model,
                                       GtkTreeIter       *iter,
                                       gpointer           column)
{
    gboolean  value;
    gchar    *buff;
    guint     coli = GPOINTER_TO_UINT (column);


    /* get field value from cell */
    gtk_tree_model_get (model, iter, coli, &value, -1);


    if (value = TRUE) {
        buff = g_strdup ("L");
    }
    else {
        buff = g_strdup ("A");
    }

    /* render the cell */
    g_object_set (renderer, "text", buff, NULL);
    g_free (buff);
}



/* AOS/LOS; convert julian date to string */
static void time_cell_data_function (GtkTreeViewColumn *col,
                                     GtkCellRenderer   *renderer,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           column)
{
    gdouble    number;
    gchar     *buff;
    guint      coli = GPOINTER_TO_UINT (column);
    time_t     t;
    guint size;


    /* get cell data */
    gtk_tree_model_get (model, iter, coli, &number, -1);

    /* format the time code */
    buff = g_strdup_printf ("%.6f", number);

    /* render the cell */
    g_object_set (renderer, "text", buff, NULL);
    g_free (buff);

}


/** \brief Function to compare to Event cells.
  * \param model Pointer to the GtkTreeModel.
  * \param a Pointer to the first element.
  * \param b Pointer to the second element.
  * \param user_data Always NULL (TBC).
  * \return See detailed description.
  *
  * This function is used by the SatList sort function to determine whether
  * AOS/LOS cell a is greater than b or not. The cells a and b contain the
  * time of the event in Julian days, thus the result can be computed by a
  * simple comparison between the two numbers contained in the cells.
  *
  * The function returns -1 if a < b; +1 if a > b; 0 otherwise.
  */
static gint event_cell_compare_function (GtkTreeModel *model,
                                         GtkTreeIter  *a,
                                         GtkTreeIter  *b,
                                         gpointer user_data)
{
    gint result;
    gdouble ta,tb;
    gint sort_col;
    GtkSortType sort_type;


    /* Since this function is used for both AOS and LOS columns,
       we need to get the sort column */
    gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
                                          &sort_col,
                                          &sort_type);

    /* get a and b */
    gtk_tree_model_get (model, a, sort_col, &ta, -1);
    gtk_tree_model_get (model, b, sort_col, &tb, -1);

    if (ta < tb) {
        result = -1;
    }
    else if (ta > tb) {
        result = 1;
    }
    else {
        result = 0;
    }

    return result;
}


/** \brief Reload configuration */
void gtk_event_list_reconf (GtkWidget *widget, GKeyFile *cfgdat)
{
    sat_log_log (SAT_LOG_LEVEL_WARN, _("%s: FIXME I am not implemented"));
}



/** \brief Manage "popup-menu" events.
 *  \param treeview The tree view in the GtkSatList widget
 *  \param list Pointer to the GtkSatList widget.
 *
 */
static gboolean popup_menu_cb (GtkWidget *treeview, gpointer list)
{

    /* if there is no selection, select the first row */

    view_popup_menu (treeview, NULL, list);

    return TRUE; /* we handled this */
}


/** \brief Manage button press events.
 *  \param treeview The tree view in the GtkSatList widget.
 *  \param event The event received.
 *  \param list Pointer to the GtkSatList widget.
 *
 */
static gboolean button_press_cb (GtkWidget *treeview, GdkEventButton *event, gpointer list)
{

    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {

        /* optional: select row if no row is selected or only
         *  one other row is selected (will only do something
         *  if you set a tree selection mode as described later
         *  in the tutorial) */
        if (1) {
            GtkTreeSelection *selection;

            selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

            /* Note: gtk_tree_selection_count_selected_rows() does not
             *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
            if (gtk_tree_selection_count_selected_rows (selection)  <= 1) {
                GtkTreePath *path;

                /* Get tree path for row that was clicked */
                if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
                                                   (gint) event->x,
                                                   (gint) event->y,
                                                   &path, NULL, NULL, NULL)) {
                    gtk_tree_selection_unselect_all (selection);
                    gtk_tree_selection_select_path (selection, path);
                    gtk_tree_path_free (path);
                }
            }
        } /* end of optional bit */

        view_popup_menu (treeview, event, list);

        return TRUE; /* we handled this */
    }

    return FALSE; /* we did not handle this */
}


static void view_popup_menu (GtkWidget *treeview, GdkEventButton *event, gpointer list)
{
    GtkTreeSelection *selection;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    guint            *catnum;
    sat_t            *sat;

    catnum = g_new0 (guint, 1);

    /* get selected satellite */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    if (gtk_tree_selection_get_selected (selection, &model, &iter))  {


        gtk_tree_model_get (model, &iter,
                            EVENT_LIST_COL_CATNUM, catnum,
                            -1);

        sat = SAT (g_hash_table_lookup (GTK_SAT_LIST (list)->satellites, catnum));

        if (sat == NULL) {
            sat_log_log (SAT_LOG_LEVEL_MSG,
                         _("%s:%d Failed to get data for %d."),
                         __FILE__, __LINE__, catnum);

        }
        else {
            gtk_sat_list_popup_exec (sat, GTK_SAT_LIST (list)->qth, event,
                                     GTK_SAT_LIST (list));
        }


    }
    else {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s:%d: There is no selection; skip popup."),
                     __FILE__, __LINE__);
    }

    g_free (catnum);
}





/** \brief Reload reference to satellites (e.g. after TLE update). */
void
gtk_event_list_reload_sats (GtkWidget *evlist, GHashTable *sats)
{
    GTK_EVENT_LIST (evlist)->satellites = sats;
}
