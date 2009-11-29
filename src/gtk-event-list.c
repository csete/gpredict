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
    if ((evlist == NULL) || !IS_GTK_EVENT_LIST (satlist)) {
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
        gtk_tree_model_foreach (model, event_list_update_sats, satlist);
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

        /**** UPDATED UNTIL HERE *****/
        /* store new data */
        gtk_list_store_set (GTK_LIST_STORE (model), iter,
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

        if (satlist->flags & SAT_LIST_FLAG_NEXT_EVENT) {
            gdouble    number;
            gchar      buff[TIME_FORMAT_MAX_LENGTH];
            gchar     *tfstr;
            gchar     *fmtstr;
            gchar     *alstr;
            time_t     t;
            guint      size;


            if (sat->aos > sat->los) {
                /* next event is LOS */
                number = sat->los;
                alstr = g_strdup ("LOS: ");
            }
            else {
                /* next event is AOS */
                number = sat->aos;
                alstr = g_strdup ("AOS: ");
            }
    
            if (number == 0.0) {
                gtk_list_store_set (GTK_LIST_STORE (model), iter,
                                    SAT_LIST_COL_NEXT_EVENT, "--- N/A ---",
                                    -1);
            }
            else {

                /* convert julian date to struct tm */
                t = (number - 2440587.5)*86400.;

                /* format the number */
                tfstr = sat_cfg_get_str (SAT_CFG_STR_TIME_FORMAT);
                fmtstr = g_strconcat (alstr, tfstr, NULL);
                g_free (tfstr);

                /* format either local time or UTC depending on check box */
                if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
                    size = strftime (buff, TIME_FORMAT_MAX_LENGTH,
                                     fmtstr, localtime (&t));
                else
                    size = strftime (buff, TIME_FORMAT_MAX_LENGTH,
                                     fmtstr, gmtime (&t));
        
                if (size == 0)
                    /* size > MAX_LENGTH */
                    buff[TIME_FORMAT_MAX_LENGTH-1] = '\0';

                gtk_list_store_set (GTK_LIST_STORE (model), iter,
                                    SAT_LIST_COL_NEXT_EVENT, buff,
                                    -1);


                g_free (fmtstr);
            }

            g_free (alstr);
            
        }

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

        /* general float with 2 dec. precision
           no extra format besides a degree char
        */
    case SAT_LIST_COL_AZ:
    case SAT_LIST_COL_EL:
    case SAT_LIST_COL_RA:
    case SAT_LIST_COL_DEC:
    case SAT_LIST_COL_MA:
    case SAT_LIST_COL_PHASE:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 degree_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

        /* LAT/LON format */
    case SAT_LIST_COL_LAT:
    case SAT_LIST_COL_LON:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 latlon_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

        /* distances and velocities */
    case SAT_LIST_COL_RANGE:
    case SAT_LIST_COL_ALT:
    case SAT_LIST_COL_FOOTPRINT:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 distance_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

    case SAT_LIST_COL_VEL:
    case SAT_LIST_COL_RANGE_RATE:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 range_rate_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

    case SAT_LIST_COL_DOPPLER:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 float_to_int_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

    case SAT_LIST_COL_DELAY:
    case SAT_LIST_COL_LOSS:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 two_dec_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

    case SAT_LIST_COL_AOS:
    case SAT_LIST_COL_LOS:
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 event_cell_data_function,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
        break;

    default:
        break;

    }

}


/* render column containg lat/lon
   by using this instead of the default data function, we can
   control the number of decimals and display the coordinates in a
   fancy way, including degree sign and NWSE suffixes.

   Please note that this function only affects how the numbers are
   displayed (rendered), the tree_store will still contain the
   original flaoting point numbers. Very cool!
*/
static void
latlon_cell_data_function (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           column)
{
    gdouble   number = 0.0;
    gchar   *buff;
    guint    coli = GPOINTER_TO_UINT (column);
    gchar    hmf = ' ';

    gtk_tree_model_get (model, iter, coli, &number, -1);

    /* check whether configuration requests the use
       of N, S, E and W instead of signs
    */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_NSEW)) {

        if (coli == SAT_LIST_COL_LAT) {
            if (number < 0.00) {
                number = -number;
                hmf = 'S';
            }
            else {
                hmf = 'N';
            }
        }
        else if (coli == SAT_LIST_COL_LON) {
            if (number < 0.00) {
                number = -number;
                hmf = 'W';
            }
            else {
                hmf = 'E';
            }
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%d: Invalid column: %d"),
                         __FILE__, __LINE__,
                         coli);
            hmf = '?';
        }
    }

    /* format the number */
    buff = g_strdup_printf ("%.2f\302\260%c", number, hmf);
    g_object_set (renderer,
                  "text", buff,
                  NULL);
    g_free (buff);
}


/* general floats with 2 digits + degree char */
static void
degree_cell_data_function (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           column)
{
    gdouble    number;
    gchar     *buff;
    guint      coli = GPOINTER_TO_UINT (column);

    gtk_tree_model_get (model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf ("%.2f\302\260", number);
    g_object_set (renderer,
                  "text", buff,
                  NULL);
    g_free (buff);
}


/* distance and velocity, 0 decimal digits */
static void
distance_cell_data_function (GtkTreeViewColumn *col,
                             GtkCellRenderer   *renderer,
                             GtkTreeModel      *model,
                             GtkTreeIter       *iter,
                             gpointer           column)
{
    gdouble    number;
    gchar     *buff;
    guint      coli = GPOINTER_TO_UINT (column);

    gtk_tree_model_get (model, iter, coli, &number, -1);

    /* convert distance to miles? */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_IMPERIAL)) {
        number = KM_TO_MI(number);
    }

    /* format the number */
    buff = g_strdup_printf ("%.0f", number);
    g_object_set (renderer,
                  "text", buff,
                  NULL);
    g_free (buff);
}

/* range rate is special, because we may need to convert to miles
   and want 2-3 decimal digits.
*/
static void
range_rate_cell_data_function (GtkTreeViewColumn *col,
                               GtkCellRenderer   *renderer,
                               GtkTreeModel      *model,
                               GtkTreeIter       *iter,
                               gpointer           column)
{
    gdouble    number;
    gchar     *buff;
    guint      coli = GPOINTER_TO_UINT (column);

    gtk_tree_model_get (model, iter, coli, &number, -1);

    /* convert distance to miles? */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_IMPERIAL)) {
        number = KM_TO_MI(number);
    }

    /* format the number */
    buff = g_strdup_printf ("%.3f", number);
    g_object_set (renderer,
                  "text", buff,
                  NULL);
    g_free (buff);
}

/* 0 decimal digits */
static void
float_to_int_cell_data_function (GtkTreeViewColumn *col,
                                 GtkCellRenderer   *renderer,
                                 GtkTreeModel      *model,
                                 GtkTreeIter       *iter,
                                 gpointer           column)
{
    gdouble    number;
    gchar     *buff;
    guint      coli = GPOINTER_TO_UINT (column);

    gtk_tree_model_get (model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf ("%.0f", number);
    g_object_set (renderer,
                  "text", buff,
                  NULL);
    g_free (buff);
}

/* 2 decimal digits */
static void
two_dec_cell_data_function (GtkTreeViewColumn *col,
                            GtkCellRenderer   *renderer,
                            GtkTreeModel      *model,
                            GtkTreeIter       *iter,
                            gpointer           column)
{
    gdouble    number;
    gchar     *buff;
    guint      coli = GPOINTER_TO_UINT (column);

    gtk_tree_model_get (model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf ("%.2f", number);
    g_object_set (renderer,
                  "text", buff,
                  NULL);
    g_free (buff);
}


/* AOS/LOS; convert julian date to string */
static void
event_cell_data_function (GtkTreeViewColumn *col,
                          GtkCellRenderer   *renderer,
                          GtkTreeModel      *model,
                          GtkTreeIter       *iter,
                          gpointer           column)
{
    gdouble    number;
    gchar      buff[TIME_FORMAT_MAX_LENGTH];
    gchar     *fmtstr;
    guint      coli = GPOINTER_TO_UINT (column);
    time_t     t;
    guint size;


    gtk_tree_model_get (model, iter, coli, &number, -1);
    
    if (number == 0.0) {
        g_object_set (renderer,
                      "text", "--- N/A ---",
                      NULL);
    }
    else {

        /* convert julian date to struct tm */
        t = (number - 2440587.5)*86400.;

        /* format the number */
        fmtstr = sat_cfg_get_str (SAT_CFG_STR_TIME_FORMAT);

        /* format either local time or UTC depending on check box */
        if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
            size = strftime (buff, TIME_FORMAT_MAX_LENGTH, fmtstr, localtime (&t));
        else
            size = strftime (buff, TIME_FORMAT_MAX_LENGTH, fmtstr, gmtime (&t));
        
        if (size == 0)
            /* size > TIME_FORMAT_MAX_LENGTH */
            buff[TIME_FORMAT_MAX_LENGTH-1] = '\0';

        g_object_set (renderer,
                      "text", buff,
                      NULL);

        g_free (fmtstr);
    }

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
void
gtk_sat_list_reconf          (GtkWidget *widget, GKeyFile *cfgdat)
{
    sat_log_log (SAT_LOG_LEVEL_WARN, _("%s: FIXME I am not implemented"));
}



/** \brief Manage "popup-menu" events.
 *  \param treeview The tree view in the GtkSatList widget
 *  \param list Pointer to the GtkSatList widget.
 *
 * This function is called when the "popup-menu" signal is emitted. This
 * usually happens if the user presses SHJIFT-F10? It is used as a wrapper
 * for the function that actually creates the popup menu.
 */
static gboolean
popup_menu_cb (GtkWidget *treeview, gpointer list)
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
static gboolean
button_press_cb (GtkWidget *treeview, GdkEventButton *event, gpointer list)
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


static void
view_popup_menu (GtkWidget *treeview, GdkEventButton *event, gpointer list)
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
                            SAT_LIST_COL_CATNUM, catnum,
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


/*** FIXME: formalise with other copies, only need az,el and jul_utc */
static void
Calculate_RADec (sat_t *sat, qth_t *qth, obs_astro_t *obs_set)
{
    /* Reference:  Methods of Orbit Determination by  */
    /*                Pedro Ramon Escobal, pp. 401-402 */

    double phi,theta,sin_theta,cos_theta,sin_phi,cos_phi,
        az,el,Lxh,Lyh,Lzh,Sx,Ex,Zx,Sy,Ey,Zy,Sz,Ez,Zz,
        Lx,Ly,Lz,cos_delta,sin_alpha,cos_alpha;
    geodetic_t geodetic;


    geodetic.lon = qth->lon * de2ra;
    geodetic.lat = qth->lat * de2ra;
    geodetic.alt = qth->alt / 1000.0;
    geodetic.theta = 0;



    az = sat->az * de2ra;
    el = sat->el * de2ra;
    phi   = geodetic.lat;
    theta = FMod2p(ThetaG_JD(sat->jul_utc) + geodetic.lon);
    sin_theta = sin(theta);
    cos_theta = cos(theta);
    sin_phi = sin(phi);
    cos_phi = cos(phi);
    Lxh = -cos(az) * cos(el);
    Lyh =  sin(az) * cos(el);
    Lzh =  sin(el);
    Sx = sin_phi * cos_theta;
    Ex = -sin_theta;
    Zx = cos_theta * cos_phi;
    Sy = sin_phi * sin_theta;
    Ey = cos_theta;
    Zy = sin_theta*cos_phi;
    Sz = -cos_phi;
    Ez = 0;
    Zz = sin_phi;
    Lx = Sx*Lxh + Ex * Lyh + Zx*Lzh;
    Ly = Sy*Lxh + Ey * Lyh + Zy*Lzh;
    Lz = Sz*Lxh + Ez * Lyh + Zz*Lzh;
    obs_set->dec = ArcSin(Lz);  /* Declination (radians)*/
    cos_delta = sqrt(1 - Sqr(Lz));
    sin_alpha = Ly / cos_delta;
    cos_alpha = Lx / cos_delta;
    obs_set->ra = AcTan(sin_alpha,cos_alpha); /* Right Ascension (radians)*/
    obs_set->ra = FMod2p(obs_set->ra);

}




/** \brief Reload reference to satellites (e.g. after TLE update). */
void
gtk_sat_list_reload_sats (GtkWidget *satlist, GHashTable *sats)
{
    GTK_SAT_LIST (satlist)->satellites = sats;
}
