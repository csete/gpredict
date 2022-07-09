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

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-event-list.h"
#include "gtk-event-list-popup.h"
#include "gtk-sat-data.h"
#include "mod-cfg-get-param.h"
#include "locator.h"
#include "orbit-tools.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-vis.h"
#include "sat-info.h"
#include "sgpsdp/sgp4sdp4.h"


#define EVENT_LIST_COL_DEF (EVENT_LIST_FLAG_NAME | EVENT_LIST_FLAG_AZ | EVENT_LIST_FLAG_EL | EVENT_LIST_FLAG_TIME)

/** Column titles indexed with column symb. refs. */
const gchar    *EVENT_LIST_COL_TITLE[EVENT_LIST_COL_NUMBER] = {
    N_("Satellite"),
    N_("Catnum"),
    N_("Az"),
    N_("El"),
    N_("Event"),
    N_("AOS/LOS"),
    N_("Decayed"),              /*should never be seen */
    N_("BOLD")                  /*should never be seen */
};

/** Column title hints indexed with column symb. refs. */
const gchar    *EVENT_LIST_COL_HINT[EVENT_LIST_COL_NUMBER] = {
    N_("Satellite Name"),
    N_("Catalogue Number"),
    N_("Azimuth"),
    N_("Elevation"),
    N_("Next event type (A: AOS, L: LOS)"),
    N_("Countdown until next event")
};

/* field alignments */
const gfloat    EVENT_LIST_COL_XALIGN[EVENT_LIST_COL_NUMBER] = {
    0.0,                        // name
    0.5,                        // catnum
    1.0,                        // az
    1.0,                        // el
    0.5,                        // event type
    1.0,                        // time
};

/* column head alignments */
const gfloat    EVENT_LIST_HEAD_XALIGN[EVENT_LIST_COL_NUMBER] = {
    0.0,                        // name
    0.5,                        // catnum
    0.5,                        // az
    0.5,                        // el
    0.5,                        // event type
    1.0,                        // time
};

static void     gtk_event_list_class_init(GtkEventListClass * class,
					  gpointer class_data);
static void     gtk_event_list_init(GtkEventList * list,
				    gpointer g_class);
static void     gtk_event_list_destroy(GtkWidget * widget);
static GtkTreeModel *create_and_fill_model(GHashTable * sats);
static void     event_list_add_satellites(gpointer key,
                                          gpointer value, gpointer user_data);
static gboolean event_list_update_sats(GtkTreeModel * model,
                                       GtkTreePath * path,
                                       GtkTreeIter * iter, gpointer data);

/* cell rendering related functions */
static void     check_and_set_cell_renderer(GtkTreeViewColumn * column,
                                            GtkCellRenderer * renderer,
                                            gint i);
static void     evtype_cell_data_function(GtkTreeViewColumn * col,
                                          GtkCellRenderer * renderer,
                                          GtkTreeModel * model,
                                          GtkTreeIter * iter, gpointer column);
static void     time_cell_data_function(GtkTreeViewColumn * col,
                                        GtkCellRenderer * renderer,
                                        GtkTreeModel * model,
                                        GtkTreeIter * iter, gpointer column);
static void     degree_cell_data_function(GtkTreeViewColumn * col,
                                          GtkCellRenderer * renderer,
                                          GtkTreeModel * model,
                                          GtkTreeIter * iter, gpointer column);
static gint     event_cell_compare_function(GtkTreeModel * model,
                                            GtkTreeIter * a,
                                            GtkTreeIter * b,
                                            gpointer user_data);

static gboolean popup_menu_cb(GtkWidget * treeview, gpointer list);
static gboolean button_press_cb(GtkWidget * treeview,
                                GdkEventButton * event, gpointer list);
static void     view_popup_menu(GtkWidget * treeview,
                                GdkEventButton * event, gpointer list);
static void     row_activated_cb(GtkTreeView * tree_view,
                                 GtkTreePath * path,
                                 GtkTreeViewColumn * column, gpointer list);

static GtkBoxClass *parent_class = NULL;

GType gtk_event_list_get_type()
{
    static GType    gtk_event_list_type = 0;

    if (!gtk_event_list_type)
    {
        static const GTypeInfo gtk_event_list_info = {
            sizeof(GtkEventListClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_event_list_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkEventList),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_event_list_init,
            NULL
        };

        gtk_event_list_type = g_type_register_static(GTK_TYPE_BOX,
                                                     "GtkEventList",
                                                     &gtk_event_list_info, 0);
    }

    return gtk_event_list_type;
}

static void gtk_event_list_class_init(GtkEventListClass * class,
				      gpointer class_data)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    (void)class_data;

    widget_class->destroy = gtk_event_list_destroy;
    parent_class = g_type_class_peek_parent(class);
}

static void gtk_event_list_init(GtkEventList * list,
				gpointer g_class)
{
    (void)list;
    (void)g_class;
}

static void gtk_event_list_destroy(GtkWidget * widget)
{
    GtkEventList   *evlist = GTK_EVENT_LIST(widget);

    g_key_file_set_integer(evlist->cfgdata, MOD_CFG_EVENT_LIST_SECTION,
                           MOD_CFG_EVENT_LIST_SORT_COLUMN,
                           evlist->sort_column);

    g_key_file_set_integer(evlist->cfgdata, MOD_CFG_EVENT_LIST_SECTION,
                           MOD_CFG_EVENT_LIST_SORT_ORDER, evlist->sort_order);

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

/**
 * Create a new GtkEventList widget.
 * @param cfgdata Pointer to the module configuration data.
 * @param sats Hash table containing the satellites tracked by the parent module.
 * @param qth Pointer to the QTH used by this module.
 * @param columns Visible columns (currently not in use).
 */
GtkWidget      *gtk_event_list_new(GKeyFile * cfgdata, GHashTable * sats,
                                   qth_t * qth, guint32 columns)
{
    GtkWidget      *widget;
    GtkEventList   *evlist;
    GtkTreeModel   *model, *filter, *sortable;
    guint           i;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    (void)columns;

    widget = g_object_new(GTK_TYPE_EVENT_LIST, NULL);
    evlist = GTK_EVENT_LIST(widget);

    evlist->update = gtk_event_list_update;

    evlist->cfgdata = cfgdata;
    evlist->sort_column = EVENT_LIST_COL_TIME;
    evlist->sort_order = GTK_SORT_ASCENDING;
    if (g_key_file_has_key(evlist->cfgdata, MOD_CFG_EVENT_LIST_SECTION,
                           MOD_CFG_EVENT_LIST_SORT_COLUMN, NULL))
    {
        evlist->sort_column = g_key_file_get_integer(evlist->cfgdata,
                                                     MOD_CFG_EVENT_LIST_SECTION,
                                                     MOD_CFG_EVENT_LIST_SORT_COLUMN,
                                                     NULL);
        if ((evlist->sort_column > EVENT_LIST_COL_NUMBER)
            || (evlist->sort_column < 0))
        {
            evlist->sort_column = EVENT_LIST_COL_TIME;
        }
    }
    if (g_key_file_has_key(evlist->cfgdata,
                           MOD_CFG_EVENT_LIST_SECTION,
                           MOD_CFG_EVENT_LIST_SORT_ORDER, NULL))
    {
        evlist->sort_order = g_key_file_get_integer(evlist->cfgdata,
                                                    MOD_CFG_EVENT_LIST_SECTION,
                                                    MOD_CFG_EVENT_LIST_SORT_ORDER,
                                                    NULL);
        if ((evlist->sort_order > 1) || (evlist->sort_order < 0))
        {
            evlist->sort_order = GTK_SORT_ASCENDING;
        }
    }

    evlist->satellites = sats;
    evlist->qth = qth;

    evlist->flags = EVENT_LIST_COL_DEF;
    evlist->treeview = gtk_tree_view_new();
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(evlist->treeview),
                                 GTK_TREE_VIEW_GRID_LINES_NONE);

    /* create treeview columns */
    for (i = 0; i < EVENT_LIST_COL_NUMBER; i++)
    {
        renderer = gtk_cell_renderer_text_new();
        g_object_set(G_OBJECT(renderer), "xalign", EVENT_LIST_COL_XALIGN[i],
                     NULL);

        column =
            gtk_tree_view_column_new_with_attributes(_
                                                     (EVENT_LIST_COL_TITLE[i]),
                                                     renderer, "text", i,
                                                     "weight",
                                                     EVENT_LIST_COL_BOLD,
                                                     NULL);

        gtk_tree_view_insert_column(GTK_TREE_VIEW(evlist->treeview),
                                    column, -1);
        gtk_tree_view_column_set_alignment(column, EVENT_LIST_HEAD_XALIGN[i]);
        gtk_tree_view_column_set_sort_column_id(column, i);
        check_and_set_cell_renderer(column, renderer, i);

        /* hide columns that have not been specified */
        if (!(evlist->flags & (1 << i)))
        {
            gtk_tree_view_column_set_visible(column, FALSE);
        }
    }

    /* create model and finalise treeview */
    model = create_and_fill_model(evlist->satellites);
    filter = gtk_tree_model_filter_new(model, NULL);
    sortable = gtk_tree_model_sort_new_with_model(filter);
    evlist->sortable = sortable;
    gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter),
                                             EVENT_LIST_COL_DECAY);
    gtk_tree_view_set_model(GTK_TREE_VIEW(evlist->treeview), sortable);

    /* The time sort function needs to be special */
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sortable),
                                    EVENT_LIST_COL_TIME,
                                    event_cell_compare_function, NULL, NULL);

    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable),
                                         evlist->sort_column,
                                         evlist->sort_order);

    g_object_unref(model);
    g_object_unref(filter);
    g_object_unref(sortable);

    g_signal_connect(evlist->treeview, "button-press-event",
                     G_CALLBACK(button_press_cb), widget);
    g_signal_connect(evlist->treeview, "popup-menu",
                     G_CALLBACK(popup_menu_cb), widget);
    g_signal_connect(evlist->treeview, "row-activated",
                     G_CALLBACK(row_activated_cb), widget);

    evlist->swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(evlist->swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(evlist->swin), evlist->treeview);

    gtk_box_pack_start(GTK_BOX(widget), evlist->swin, TRUE, TRUE, 0);
    gtk_widget_show_all(widget);

    return widget;
}

/** Create and file the tree model for the even list. */
static GtkTreeModel *create_and_fill_model(GHashTable * sats)
{
    GtkListStore   *liststore;

    liststore = gtk_list_store_new(EVENT_LIST_COL_NUMBER, G_TYPE_STRING,        // name
                                   G_TYPE_INT,  // catnum
                                   G_TYPE_DOUBLE,       // az
                                   G_TYPE_DOUBLE,       // el
                                   G_TYPE_BOOLEAN,      // TRUE if AOS, FALSE if LOS
                                   G_TYPE_DOUBLE,       // time
                                   G_TYPE_BOOLEAN,      // decayed 
                                   G_TYPE_INT); // bold for storing weight

    g_hash_table_foreach(sats, event_list_add_satellites, liststore);

    return GTK_TREE_MODEL(liststore);
}

/**
 * Add satellites. This function is a g_hash_table_foreach() callback.
 * @param key The key of the satellite in the hash table.
 * @param value Pointer to the satellite (sat_t structure) that should be added.
 * @param user_data Pointer to the GtkListStore where the satellite should be added
 *
 * This function is called by by the create_and_fill_models() function for adding
 * the satellites to the internal liststore.
 */
static void event_list_add_satellites(gpointer key, gpointer value,
                                      gpointer user_data)
{
    GtkListStore   *store = GTK_LIST_STORE(user_data);
    GtkTreeIter     item;
    sat_t          *sat = SAT(value);

    (void)key;

    gtk_list_store_append(store, &item);
    gtk_list_store_set(store, &item,
                       EVENT_LIST_COL_NAME, sat->nickname,
                       EVENT_LIST_COL_CATNUM, sat->tle.catnr,
                       EVENT_LIST_COL_AZ, sat->az,
                       EVENT_LIST_COL_EL, sat->el,
                       EVENT_LIST_COL_EVT, (sat->el >= 0) ? TRUE : FALSE,
                       EVENT_LIST_COL_TIME, 0.0,
                       EVENT_LIST_COL_DECAY, !decayed(sat), -1);
}

/** Update satellites */
void gtk_event_list_update(GtkWidget * widget)
{
    GtkTreeModel   *model;
    GtkEventList   *evlist = GTK_EVENT_LIST(widget);

    /* first, do some sanity checks */
    if ((evlist == NULL) || !IS_GTK_EVENT_LIST(evlist))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Invalid GtkEventList!"), __func__);
        return;
    }

    /* get and tranverse the model */
    model =
        gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER
                                        (gtk_tree_model_sort_get_model
                                         (GTK_TREE_MODEL_SORT
                                          (gtk_tree_view_get_model
                                           (GTK_TREE_VIEW
                                            (evlist->treeview))))));

    /* save the sort information */
    gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(evlist->sortable),
                                         &(evlist->sort_column),
                                         &(evlist->sort_order));

    /* update */
    gtk_tree_model_foreach(model, event_list_update_sats, evlist);

#if 0
    /* check refresh rate */
    if (evlist->counter < evlist->refresh)
    {
        evlist->counter++;
    }
    else
    {
        evlist->counter = 1;

        /* get and tranverse the model */
        model =
            gtk_tree_model_filter_get_model(gtk_tree_view_get_model
                                            (GTK_TREE_VIEW(evlist->treeview)));


        /* update */
        gtk_tree_model_foreach(model, event_list_update_sats, evlist);
    }
#endif
}

/** Update data in each column in a given row */
static gboolean event_list_update_sats(GtkTreeModel * model,
                                       GtkTreePath * path,
                                       GtkTreeIter * iter, gpointer data)
{
    GtkEventList   *evlist = GTK_EVENT_LIST(data);
    guint          *catnum;
    sat_t          *sat;
    gdouble         number, now;

    (void)path;

    /* get the catalogue number for this row
       then look it up in the hash table
     */
    catnum = g_new0(guint, 1);
    gtk_tree_model_get(model, iter, EVENT_LIST_COL_CATNUM, catnum, -1);
    sat = SAT(g_hash_table_lookup(evlist->satellites, catnum));

    if (sat == NULL)
    {
        /* satellite not tracked anymore => remove */
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Failed to get data for #%d."), __func__, *catnum);

        gtk_list_store_remove(GTK_LIST_STORE(model), iter);

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Satellite #%d removed from list."),
                    __func__, *catnum);
    }
    else
    {
        /* update data */
        now = evlist->tstamp;

        if (sat->el > 0.0)
        {
            if (sat->los > 0.0)
            {
                number = sat->los - now;
            }
            else
            {
                number = -1.0;  /* Sat is staionary or no event */
            }
        }
        else
        {
            if (sat->aos > 0.0)
            {
                number = sat->aos - now;
            }
            else
            {
                number = -1.0;  /* Sat is staionary or no event */
            }
        }

        /* store new data */
        gtk_list_store_set(GTK_LIST_STORE(model), iter,
                           EVENT_LIST_COL_AZ, sat->az,
                           EVENT_LIST_COL_EL, sat->el,
                           EVENT_LIST_COL_EVT, (sat->el >= 0) ? TRUE : FALSE,
                           EVENT_LIST_COL_TIME, number,
                           EVENT_LIST_COL_DECAY, !decayed(sat),
                           EVENT_LIST_COL_BOLD,
                           (sat->el >
                            0.0) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
                           -1);
    }

    g_free(catnum);

    /* Return value not documented what to return, but it seems that
       FALSE continues to next row while TRUE breaks
     */
    return FALSE;
}

/** Set cell renderer function. */
static void check_and_set_cell_renderer(GtkTreeViewColumn * column,
                                        GtkCellRenderer * renderer, gint i)
{
    switch (i)
    {
        /* Event type */
    case EVENT_LIST_COL_AZ:
    case EVENT_LIST_COL_EL:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                degree_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

        /* Event type */
    case EVENT_LIST_COL_EVT:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                evtype_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

        /* time countdown */
    case EVENT_LIST_COL_TIME:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                time_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    default:
        break;
    }
}

/**
 * Render column containing event type.
 *
 * Event type can be AOS or LOS depending on whether the satellite is within
 * range or not. AOS will rendern an "A", LOS will render an "L".
 */
static void evtype_cell_data_function(GtkTreeViewColumn * col,
                                      GtkCellRenderer * renderer,
                                      GtkTreeModel * model,
                                      GtkTreeIter * iter, gpointer column)
{
    (void)col;

    gboolean        value;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    /* get field value from cell */
    gtk_tree_model_get(model, iter, coli, &value, -1);

    if (value == TRUE)
    {
        buff = g_strdup(_("LOS"));
    }
    else
    {
        buff = g_strdup(_("AOS"));
    }

    /* render the cell */
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* AOS/LOS; convert julian date to string */
static void time_cell_data_function(GtkTreeViewColumn * col,
                                    GtkCellRenderer * renderer,
                                    GtkTreeModel * model,
                                    GtkTreeIter * iter, gpointer column)
{
    (void)col;

    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    guint           h, m, s;

    /* get cell data */
    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* format the time code */
    if (number < 0.0)
    {
        buff = g_strdup(_("Never"));
    }
    else
    {
        /* convert julian date to seconds */
        s = (guint) (number * 86400);

        /* extract hours */
        h = (guint) floor(s / 3600);
        s -= 3600 * h;

        /* extract minutes */
        m = (guint) floor(s / 60);
        s -= 60 * m;

        if (h > 0)
        {
            buff = g_strdup_printf("%02d:%02d:%02d", h, m, s);
        }
        else
        {
            buff = g_strdup_printf("%02d:%02d", m, s);
        }
    }

    /* render the cell */
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* general floats with 2 digits + degree char. Used for Az and El */
static void degree_cell_data_function(GtkTreeViewColumn * col,
                                      GtkCellRenderer * renderer,
                                      GtkTreeModel * model,
                                      GtkTreeIter * iter, gpointer column)
{
    (void)col;

    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    /* get the value */
    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf("%.2f\302\260", number);

    /* render column */
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/**
 * Function to compare to Event cells.
 * @param model Pointer to the GtkTreeModel.
 * @param a Pointer to the first element.
 * @param b Pointer to the second element.
 * @param user_data Always NULL (TBC).
 * @return See detailed description.
 *
 * This function is used by the SatList sort function to determine whether
 * AOS/LOS cell a is greater than b or not. The cells a and b contain the
 * time of the event in Julian days, thus the result can be computed by a
 * simple comparison between the two numbers contained in the cells.
 *
 * The function returns -1 if a < b; +1 if a > b; 0 otherwise.
 */
static gint event_cell_compare_function(GtkTreeModel * model,
                                        GtkTreeIter * a,
                                        GtkTreeIter * b, gpointer user_data)
{
    gint            result;
    gdouble         ta, tb;
    gint            sort_col = EVENT_LIST_COL_TIME;     /* hard coded as it is the only option */

    /* extracting from models was not working */
    /* with filter and sorting */

    (void)user_data;

    /* get a and b */
    gtk_tree_model_get(model, a, sort_col, &ta, -1);
    gtk_tree_model_get(model, b, sort_col, &tb, -1);

    if (ta < tb)
    {
        result = -1;
    }
    else if (ta > tb)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

/** Reload configuration */
void gtk_event_list_reconf(GtkWidget * widget, GKeyFile * cfgdat)
{
    (void)cfgdat;
    (void)widget;
    sat_log_log(SAT_LOG_LEVEL_WARN, _("%s: FIXME I am not implemented"));
}

/**
 * Manage "popup-menu" events.
 * @param treeview The tree view in the GtkSatList widget
 * @param list Pointer to the GtkSatList widget.
 *
 */
static gboolean popup_menu_cb(GtkWidget * treeview, gpointer list)
{
    /* if there is no selection, select the first row */

    view_popup_menu(treeview, NULL, list);

    return TRUE;                /* we handled this */
}

/**
 * Manage button press events.
 * @param treeview The tree view in the GtkSatList widget.
 * @param event The event received.
 * @param list Pointer to the GtkSatList widget.
 */
static gboolean button_press_cb(GtkWidget * treeview, GdkEventButton * event,
                                gpointer list)
{

    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        /* optional: select row if no row is selected or only
         *  one other row is selected (will only do something
         *  if you set a tree selection mode as described later
         *  in the tutorial) */
        if (1)
        {
            GtkTreeSelection *selection;

            selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

            /* Note: gtk_tree_selection_count_selected_rows() does not
             *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
            if (gtk_tree_selection_count_selected_rows(selection) <= 1)
            {
                GtkTreePath    *path;

                /* Get tree path for row that was clicked */
                if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                                  (gint) event->x,
                                                  (gint) event->y,
                                                  &path, NULL, NULL, NULL))
                {
                    gtk_tree_selection_unselect_all(selection);
                    gtk_tree_selection_select_path(selection, path);
                    gtk_tree_path_free(path);
                }
            }
        }                       /* end of optional bit */

        view_popup_menu(treeview, event, list);

        return TRUE;            /* we handled this */
    }

    return FALSE;               /* we did not handle this */
}

static void row_activated_cb(GtkTreeView * tree_view,
                             GtkTreePath * path, GtkTreeViewColumn * column,
                             gpointer list)
{
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    guint          *catnum;
    sat_t          *sat;

    (void)column;

    catnum = g_new0(guint, 1);
    model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, EVENT_LIST_COL_CATNUM, catnum, -1);

    sat = SAT(g_hash_table_lookup(GTK_EVENT_LIST(list)->satellites, catnum));

    if (sat == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d Failed to get data for %d."),
                    __FILE__, __LINE__, *catnum);
    }
    else
    {
        show_sat_info(sat, gtk_widget_get_toplevel(GTK_WIDGET(list)));
    }

    g_free(catnum);
}

static void view_popup_menu(GtkWidget * treeview, GdkEventButton * event,
                            gpointer list)
{
    GtkTreeSelection *selection;
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    guint          *catnum;
    sat_t          *sat;

    catnum = g_new0(guint, 1);

    /* get selected satellite */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter, EVENT_LIST_COL_CATNUM, catnum, -1);

        sat =
            SAT(g_hash_table_lookup(GTK_EVENT_LIST(list)->satellites, catnum));

        if (sat == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%d Failed to get data for %d."),
                        __FILE__, __LINE__, *catnum);
        }
        else
        {
            gtk_event_list_popup_exec(sat, GTK_EVENT_LIST(list)->qth, event,
                                      GTK_EVENT_LIST(list));
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: There is no selection; skip popup."),
                    __FILE__, __LINE__);
    }

    g_free(catnum);
}

/** Reload reference to satellites (e.g. after TLE update). */
void gtk_event_list_reload_sats(GtkWidget * evlist, GHashTable * sats)
{
    GTK_EVENT_LIST(evlist)->satellites = sats;
}

/** Select satellite. */
void gtk_event_list_select_sat(GtkWidget * widget, gint catnum)
{
    GtkEventList   *list;
    GtkTreeModel   *model;
    GtkTreeSelection *selection;
    GtkTreeIter     iter;
    gint            i, n;
    gint            sat;

    list = GTK_EVENT_LIST(widget);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(list->treeview));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list->treeview));

    /* iterate over the satellite list until a amtch is found */
    n = g_hash_table_size(list->satellites);
    for (i = 0; i < n; i++)
    {

        if (gtk_tree_model_iter_nth_child(model, &iter, NULL, i))
        {
            gtk_tree_model_get(model, &iter, EVENT_LIST_COL_CATNUM, &sat, -1);
            if (sat == catnum)
            {
                gtk_tree_selection_select_iter(selection, &iter);
                i = n;
            }
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: GtkEventList has not child with index %d"),
                        __func__, i);
        }
    }
}
