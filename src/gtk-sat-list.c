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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-data.h"
#include "gtk-sat-list.h"
#include "gtk-sat-list-popup.h"
#include "locator.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "sat-cfg.h"
#include "sat-info.h"
#include "sat-log.h"
#include "sat-vis.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

/** Column titles indexed with column symb. refs. */
const gchar    *SAT_LIST_COL_TITLE[SAT_LIST_COL_NUMBER] = {
    N_("Satellite"),
    N_("Catnum"),
    N_("Az"),
    N_("El"),
    N_("Dir"),
    N_("Ra"),
    N_("Dec"),
    N_("Range"),
    N_("Rate"),
    N_("Next Event"),
    N_("Next AOS"),
    N_("Next LOS"),
    N_("Lat"),
    N_("Lon"),
    N_("SSP"),
    N_("Footp"),
    N_("Alt"),
    N_("Vel"),
    N_("Dop"),
    N_("Loss"),
    N_("Del"),
    N_("MA"),
    N_("Phase"),
    N_("Orbit"),
    N_("Vis"),
    N_("Decayed"),
    N_("Status"),
    N_("BOLD")                  /* should never be seen */
};


/** Column title hints indexed with column symb. refs. */
const gchar    *SAT_LIST_COL_HINT[SAT_LIST_COL_NUMBER] = {
    N_("Satellite Name"),
    N_("Catalogue Number"),
    N_("Azimuth"),
    N_("Elevation"),
    N_("Direction"),
    N_("Right Ascension"),
    N_("Declination"),
    N_("Slant Range"),
    N_("Range Rate"),
    N_("Next Event"),
    N_("Next AOS"),
    N_("Next LOS"),
    N_("Latitude"),
    N_("Longitude"),
    N_("Sub-Satellite Point"),
    N_("Footprint"),
    N_("Altitude"),
    N_("Velocity"),
    N_("Doppler Shift @ 100MHz"),
    N_("Signal Loss @ 100MHz"),
    N_("Signal Delay"),
    N_("Mean Anomaly"),
    N_("Orbit Phase"),
    N_("Orbit Number"),
    N_("Visibility"),
    N_("Decayed"),
    N_("Operational Status"),
    N_("---")
};

const gfloat    SAT_LIST_COL_XALIGN[SAT_LIST_COL_NUMBER] = {
    0.0,                        // name
    0.5,                        // catnum
    1.0,                        // az
    1.0,                        // el
    0.5,                        // direction
    0.0,                        // RA
    0.0,                        // dec
    1.0,                        // range
    1.0,                        // range rate
    0.5,                        // next event
    0.5,                        // AOS
    0.5,                        // LOS
    1.0,                        // lat
    1.0,                        // lon
    0.5,                        // SSP
    0.5,                        // footprint
    1.0,                        // alt
    0.0,                        // vel
    0.0,                        // doppler
    0.0,                        // loss
    0.0,                        // delay
    0.0,                        // MA
    0.0,                        // phase
    1.0,                        // orbit
    0.5,                        // visibility
    0.0,                        // Operational Status
};

static void     gtk_sat_list_class_init(GtkSatListClass * class,
					gpointer class_data);
static void     gtk_sat_list_init(GtkSatList * list,
				  gpointer g_class);
static void     gtk_sat_list_destroy(GtkWidget * widget);
static GtkTreeModel *create_and_fill_model(GHashTable * sats);
static void     sat_list_add_satellites(gpointer key, gpointer value,
                                        gpointer user_data);
static gboolean sat_list_update_sats(GtkTreeModel * model, GtkTreePath * path,
                                     GtkTreeIter * iter, gpointer data);

/* cell rendering related functions */
static void     check_and_set_cell_renderer(GtkTreeViewColumn * column,
                                            GtkCellRenderer * renderer,
                                            gint i);

static void     latlon_cell_data_function(GtkTreeViewColumn * col,
                                          GtkCellRenderer * renderer,
                                          GtkTreeModel * model,
                                          GtkTreeIter * iter, gpointer column);

static void     degree_cell_data_function(GtkTreeViewColumn * col,
                                          GtkCellRenderer * renderer,
                                          GtkTreeModel * model,
                                          GtkTreeIter * iter, gpointer column);

static void     distance_cell_data_function(GtkTreeViewColumn * col,
                                            GtkCellRenderer * renderer,
                                            GtkTreeModel * model,
                                            GtkTreeIter * iter,
                                            gpointer column);

static void     range_rate_cell_data_function(GtkTreeViewColumn * col,
                                              GtkCellRenderer * renderer,
                                              GtkTreeModel * model,
                                              GtkTreeIter * iter,
                                              gpointer column);

static void     float_to_int_cell_data_function(GtkTreeViewColumn * col,
                                                GtkCellRenderer * renderer,
                                                GtkTreeModel * model,
                                                GtkTreeIter * iter,
                                                gpointer column);

static void     two_dec_cell_data_function(GtkTreeViewColumn * col,
                                           GtkCellRenderer * renderer,
                                           GtkTreeModel * model,
                                           GtkTreeIter * iter,
                                           gpointer column);


static void     operational_status_cell_data_function(GtkTreeViewColumn * col,
                                           GtkCellRenderer * renderer,
                                           GtkTreeModel * model,
                                           GtkTreeIter * iter,
                                           gpointer column);


static void     event_cell_data_function(GtkTreeViewColumn * col,
                                         GtkCellRenderer * renderer,
                                         GtkTreeModel * model,
                                         GtkTreeIter * iter, gpointer column);


static gint     event_cell_compare_function(GtkTreeModel * model,
                                            GtkTreeIter * a, GtkTreeIter * b,
                                            gpointer user_data);

static gboolean popup_menu_cb(GtkWidget * treeview, gpointer list);
static gboolean button_press_cb(GtkWidget * treeview, GdkEventButton * event,
                                gpointer list);
static void     row_activated_cb(GtkTreeView * tree_view, GtkTreePath * path,
                                 GtkTreeViewColumn * column, gpointer list);

static void     view_popup_menu(GtkWidget * treeview, GdkEventButton * event,
                                gpointer list);
static void     Calculate_RADec(sat_t * sat, qth_t * qth,
                                obs_astro_t * obs_set);

static GtkVBoxClass *parent_class = NULL;


GType gtk_sat_list_get_type()
{
    static GType    gtk_sat_list_type = 0;

    if (!gtk_sat_list_type)
    {
        static const GTypeInfo gtk_sat_list_info = {
            sizeof(GtkSatListClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_sat_list_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkSatList),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_sat_list_init,
            NULL
        };

        gtk_sat_list_type = g_type_register_static(GTK_TYPE_BOX,
                                                   "GtkSatList",
                                                   &gtk_sat_list_info, 0);
    }

    return gtk_sat_list_type;
}

static void gtk_sat_list_class_init(GtkSatListClass * class,
				    gpointer class_data)
{
    GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

    (void)class_data;

    widget_class->destroy = gtk_sat_list_destroy;

    parent_class = g_type_class_peek_parent(class);
}

static void gtk_sat_list_init(GtkSatList * list,
			      gpointer g_class)
{
    (void)list;
    (void)g_class;
}

static void gtk_sat_list_destroy(GtkWidget * widget)
{
    GtkSatList     *list = GTK_SAT_LIST(widget);

    g_key_file_set_integer(list->cfgdata, MOD_CFG_LIST_SECTION,
                           MOD_CFG_LIST_SORT_COLUMN, list->sort_column);

    g_key_file_set_integer(list->cfgdata, MOD_CFG_LIST_SECTION,
                           MOD_CFG_LIST_SORT_ORDER, list->sort_order);

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

GtkWidget      *gtk_sat_list_new(GKeyFile * cfgdata, GHashTable * sats,
                                 qth_t * qth, guint32 columns)
{
//    GtkWidget      *widget;
    GtkSatList     *satlist;
    GtkTreeModel   *model, *filter, *sortable;
    guint           i;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;


//    widget = g_object_new(GTK_TYPE_SAT_LIST, NULL);
    satlist = GTK_SAT_LIST(g_object_new(GTK_TYPE_SAT_LIST, NULL));

    satlist->update = gtk_sat_list_update;

    /* Read configuration data. */
    /* ... */
    satlist->cfgdata = cfgdata;
    /* read initial sorting criteria */
    satlist->sort_column = SAT_LIST_COL_NAME;
    satlist->sort_order = GTK_SORT_ASCENDING;
    if (g_key_file_has_key(satlist->cfgdata, MOD_CFG_LIST_SECTION,
                           MOD_CFG_LIST_SORT_COLUMN, NULL))
    {
        satlist->sort_column =
            g_key_file_get_integer(satlist->cfgdata,
                                   MOD_CFG_LIST_SECTION,
                                   MOD_CFG_LIST_SORT_COLUMN, NULL);
        if ((satlist->sort_column > SAT_LIST_COL_NUMBER) ||
            (satlist->sort_column < 0))
        {
            satlist->sort_column = SAT_LIST_COL_NAME;
        }
    }
    if (g_key_file_has_key(satlist->cfgdata,
                           MOD_CFG_LIST_SECTION,
                           MOD_CFG_LIST_SORT_ORDER, NULL))
    {
        satlist->sort_order =
            g_key_file_get_integer(satlist->cfgdata,
                                   MOD_CFG_LIST_SECTION,
                                   MOD_CFG_LIST_SORT_ORDER, NULL);
        if ((satlist->sort_order > 1) || (satlist->sort_order < 0))
            satlist->sort_order = GTK_SORT_ASCENDING;

    }

    satlist->satellites = sats;
    satlist->qth = qth;

    /* initialise column flags */
    if (columns > 0)
        satlist->flags = columns;
    else
        satlist->flags = mod_cfg_get_int(cfgdata, MOD_CFG_LIST_SECTION,
                                         MOD_CFG_LIST_COLUMNS, SAT_CFG_INT_LIST_COLUMNS);

    /* get refresh rate and cycle counter */
    satlist->refresh = mod_cfg_get_int(cfgdata, MOD_CFG_LIST_SECTION,
                                       MOD_CFG_LIST_REFRESH, SAT_CFG_INT_LIST_REFRESH);
    satlist->counter = 1;

    /* create the tree view and add columns */
    satlist->treeview = gtk_tree_view_new();

    /* create treeview columns */
    for (i = 0; i < SAT_LIST_COL_NUMBER; i++)
    {
        renderer = gtk_cell_renderer_text_new();
        g_object_set(G_OBJECT(renderer), "xalign", SAT_LIST_COL_XALIGN[i],
                     NULL);

        /* in win32 use special font for direction column because default font
           does not have arrow symbols.
         */
#ifdef G_OS_WIN32
        if (i == SAT_LIST_COL_DIR)
            g_object_set(G_OBJECT(renderer), "font", "courier 12", NULL);
#endif

        column =
            gtk_tree_view_column_new_with_attributes(_(SAT_LIST_COL_TITLE[i]),
                                                     renderer,
                                                     "text", i,
                                                     "weight", SAT_LIST_COL_BOLD,
                                                     NULL);
        gtk_tree_view_insert_column(GTK_TREE_VIEW(satlist->treeview), column, -1);

        /* only aligns the headers */
        gtk_tree_view_column_set_alignment(column, 0.5);

        /* set sort id */
        gtk_tree_view_column_set_sort_column_id(column, i);

        /* set cell data function; allows to format data before rendering */
        check_and_set_cell_renderer(column, renderer, i);

        /* hide columns that have not been specified */
        if (!(satlist->flags & (1 << i)))
            gtk_tree_view_column_set_visible(column, FALSE);
    }

    /* create model and finalise treeview */
    model = create_and_fill_model(satlist->satellites);
    filter = gtk_tree_model_filter_new(model, NULL);
    sortable = gtk_tree_model_sort_new_with_model(filter);
    satlist->sortable = sortable;
    gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter),
                                             SAT_LIST_COL_DECAY);
    gtk_tree_view_set_model(GTK_TREE_VIEW(satlist->treeview), sortable);

    /* We need a special sort function for AOS/LOS events that works
       with all date and time formats (see bug #1861323)
     */
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), SAT_LIST_COL_AOS,
                                    event_cell_compare_function,
                                    GTK_WIDGET(satlist), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), SAT_LIST_COL_LOS,
                                    event_cell_compare_function,
                                    GTK_WIDGET(satlist), NULL);

    /* satellite name should be initial sorting criteria */
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable),
                                         satlist->sort_column,
                                         satlist->sort_order);

    g_object_unref(model);
    g_object_unref(filter);
    g_object_unref(sortable);

    g_signal_connect(satlist->treeview, "button-press-event",
                     G_CALLBACK(button_press_cb), satlist);
    g_signal_connect(satlist->treeview, "popup-menu",
                     G_CALLBACK(popup_menu_cb), satlist);
    g_signal_connect(satlist->treeview, "row-activated",
                     G_CALLBACK(row_activated_cb), satlist);

    satlist->swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(satlist->swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(satlist->swin), satlist->treeview);
    gtk_box_pack_start(GTK_BOX(satlist), satlist->swin, TRUE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(satlist));

    return GTK_WIDGET(satlist);
}

static GtkTreeModel *create_and_fill_model(GHashTable * sats)
{
    GtkListStore   *liststore;

    liststore = gtk_list_store_new(SAT_LIST_COL_NUMBER, G_TYPE_STRING,  // name
                                   G_TYPE_INT,  // catnum
                                   G_TYPE_DOUBLE,       // az
                                   G_TYPE_DOUBLE,       // el
                                   G_TYPE_STRING,       // direction
                                   G_TYPE_DOUBLE,       // RA
                                   G_TYPE_DOUBLE,       // Dec
                                   G_TYPE_DOUBLE,       // range
                                   G_TYPE_DOUBLE,       // range rate
                                   G_TYPE_STRING,       // next event
                                   G_TYPE_DOUBLE,       // next AOS
                                   G_TYPE_DOUBLE,       // next LOS
                                   G_TYPE_DOUBLE,       // ssp lat
                                   G_TYPE_DOUBLE,       // ssp lon
                                   G_TYPE_STRING,       // ssp qra
                                   G_TYPE_DOUBLE,       // footprint
                                   G_TYPE_DOUBLE,       // alt
                                   G_TYPE_DOUBLE,       // vel
                                   G_TYPE_DOUBLE,       // doppler
                                   G_TYPE_DOUBLE,       // path loss
                                   G_TYPE_DOUBLE,       // delay
                                   G_TYPE_DOUBLE,       // mean anomaly
                                   G_TYPE_DOUBLE,       // phase
                                   G_TYPE_LONG, // orbit
                                   G_TYPE_STRING,       // visibility
                                   G_TYPE_BOOLEAN,      // decay
                                   G_TYPE_INT,       // Operational Status
                                   G_TYPE_INT   // weight/bold
        );


    g_hash_table_foreach(sats, sat_list_add_satellites, liststore);

    return GTK_TREE_MODEL(liststore);
}


static void sat_list_add_satellites(gpointer key, gpointer value,
                                    gpointer user_data)
{
    GtkListStore   *store = GTK_LIST_STORE(user_data);
    GtkTreeIter     item;
    sat_t          *sat = SAT(value);

    (void)key;

    gtk_list_store_append(store, &item);
    gtk_list_store_set(store, &item,
                       SAT_LIST_COL_NAME, sat->nickname,
                       SAT_LIST_COL_CATNUM, sat->tle.catnr,
                       SAT_LIST_COL_AZ, sat->az,
                       SAT_LIST_COL_EL, sat->el,
                       SAT_LIST_COL_VISIBILITY, "-",
                       SAT_LIST_COL_RA, sat->ra,
                       SAT_LIST_COL_DEC, sat->dec,
                       SAT_LIST_COL_RANGE, sat->range,
                       SAT_LIST_COL_RANGE_RATE, sat->range_rate,
                       SAT_LIST_COL_DIR, "-",
                       SAT_LIST_COL_NEXT_EVENT, "--- N/A ---",
                       SAT_LIST_COL_AOS, sat->aos,
                       SAT_LIST_COL_LOS, sat->los,
                       SAT_LIST_COL_LAT, sat->ssplat,
                       SAT_LIST_COL_LON, sat->ssplon,
                       SAT_LIST_COL_SSP, "",
                       SAT_LIST_COL_FOOTPRINT, sat->footprint,
                       SAT_LIST_COL_ALT, sat->alt,
                       SAT_LIST_COL_VEL, sat->velo,
                       SAT_LIST_COL_DOPPLER, 0.0,
                       SAT_LIST_COL_LOSS, 0.0,
                       SAT_LIST_COL_DELAY, 0.0,
                       SAT_LIST_COL_MA, sat->ma,
                       SAT_LIST_COL_PHASE, sat->phase,
                       SAT_LIST_COL_ORBIT, sat->orbit, 
                       SAT_LIST_COL_STAT_OPERATIONAL, (char *) sat->tle.status,
                       SAT_LIST_COL_DECAY, !decayed(sat), -1);
}

/** Update satellites */
void gtk_sat_list_update(GtkWidget * widget)
{
    GtkTreeModel   *model;
    GtkSatList     *satlist = GTK_SAT_LIST(widget);

    /* first, do some sanity checks */
    if ((satlist == NULL) || !IS_GTK_SAT_LIST(satlist))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Invalid GtkSatList!"),
                    __func__);
        return;
    }

    /* check refresh rate */
    if (satlist->counter < satlist->refresh)
    {
        satlist->counter++;
    }
    else
    {
        satlist->counter = 1;

        /* get and tranverse the model */
        model =
            gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER
                                            (gtk_tree_model_sort_get_model
                                             (GTK_TREE_MODEL_SORT
                                              (gtk_tree_view_get_model
                                               (GTK_TREE_VIEW
                                                (satlist->treeview))))));

        /*save the sort information */
        gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE
                                             (satlist->sortable),
                                             &(satlist->sort_column),
                                             &(satlist->sort_order));

        /* optimisation: detach model from view while updating */
        /* No, we do not do it, because it makes selections and scrolling
           impossible
         */
        /*         g_object_ref (model); */
        /*         gtk_tree_view_set_model (GTK_TREE_VIEW (satlist->treeview), NULL); */

        /* update */
        gtk_tree_model_foreach(model, sat_list_update_sats, satlist);

        /* re-attach model to view */
        /*         gtk_tree_view_set_model (GTK_TREE_VIEW (satlist->treeview), model); */

        /*         g_object_unref (model); */
    }
}

/** Update data in each column in a given row */
static gboolean sat_list_update_sats(GtkTreeModel * model, GtkTreePath * path,
                                     GtkTreeIter * iter, gpointer data)
{
    GtkSatList     *satlist = GTK_SAT_LIST(data);
    guint          *catnum;
    sat_t          *sat;
    gchar          *buff;
    gdouble         doppler;
    gdouble         delay;
    gdouble         loss;
    gdouble         oldrate;
    gint            retcode;

    (void)path;

    /* get the catalogue number for this row
       then look it up in the hash table
     */
    catnum = g_new0(guint, 1);
    gtk_tree_model_get(model, iter, SAT_LIST_COL_CATNUM, catnum, -1);
    sat = SAT(g_hash_table_lookup(satlist->satellites, catnum));

    if (sat == NULL)
    {
        /* satellite not tracked anymore => remove */
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Failed to get data for #%d."), __func__, *catnum);

        gtk_list_store_remove(GTK_LIST_STORE(model), iter);

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Satellite #%d removed from list."), __func__,
                    *catnum);
    }
    else
    {
        /* store new data */
        gtk_list_store_set(GTK_LIST_STORE(model), iter,
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
                           SAT_LIST_COL_DECAY, !decayed(sat),
                           SAT_LIST_COL_BOLD,
                           (sat->el >
                            0.0) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
                           -1);

        /* doppler shift @ 100 MHz */
        if (satlist->flags & SAT_LIST_FLAG_DOPPLER)
        {
            doppler = -100.0e06 * (sat->range_rate / 299792.4580);      // Hz
            gtk_list_store_set(GTK_LIST_STORE(model), iter,
                               SAT_LIST_COL_DOPPLER, doppler, -1);
        }

        /* delay */
        if (satlist->flags & SAT_LIST_FLAG_DELAY)
        {
            delay = sat->range / 299.7924580;   // msec 
            gtk_list_store_set(GTK_LIST_STORE(model), iter, SAT_LIST_COL_DELAY,
                               delay, -1);
        }

        /* path loss */
        if (satlist->flags & SAT_LIST_FLAG_LOSS)
        {
            loss = 72.4 + 20.0 * log10(sat->range);     // dB
            gtk_list_store_set(GTK_LIST_STORE(model), iter, SAT_LIST_COL_LOSS,
                               loss, -1);
        }

        /* calculate direction */
        if (satlist->flags & SAT_LIST_FLAG_DIR)
        {
            if (sat->otype == ORBIT_TYPE_GEO)
            {
                buff = g_strdup("G");
            }
            else if (decayed(sat))
            {
                buff = g_strdup("D");
            }
            else if (sat->range_rate > 0.001)
            {
                /* going down */
                buff = g_strdup("\342\206\223");
            }
            else if ((sat->range_rate <= 0.001) && (sat->range_rate >= -0.001))
            {
                gtk_tree_model_get(model, iter, SAT_LIST_COL_RANGE_RATE,
                                   &oldrate, -1);
                /* turning around; don't know which way ? */
                if (sat->range_rate < oldrate)
                {
                    /* starting to approach */
                    buff = g_strdup("\342\206\272");
                }
                else
                {
                    /* to receed */
                    buff = g_strdup("\342\206\267");
                }
            }
            else if (sat->range_rate < -0.001)
            {
                /* coming up */
                buff = g_strdup("\342\206\221");
            }
            else
            {
                buff = g_strdup("-");
            }

            gtk_list_store_set(GTK_LIST_STORE(model), iter, SAT_LIST_COL_DIR,
                               buff, -1);

            /* free memory */
            g_free(buff);

        }

        /* SSP locator */
        if (satlist->flags & SAT_LIST_FLAG_SSP)
        {

            buff = g_try_malloc(7);

            retcode = longlat2locator(sat->ssplon, sat->ssplat, buff, 3);
            if (retcode == RIG_OK)
            {
                buff[6] = '\0';
                gtk_list_store_set(GTK_LIST_STORE(model), iter,
                                   SAT_LIST_COL_SSP, buff, -1);
            }
            g_free(buff);
        }

        /* Ra and Dec */
        if (satlist->flags & (SAT_LIST_FLAG_RA | SAT_LIST_FLAG_DEC))
        {
            obs_astro_t     astro;

            Calculate_RADec(sat, satlist->qth, &astro);

            sat->ra = Degrees(astro.ra);
            sat->dec = Degrees(astro.dec);

            gtk_list_store_set(GTK_LIST_STORE(model), iter,
                               SAT_LIST_COL_RA, sat->ra, SAT_LIST_COL_DEC,
                               sat->dec, -1);
        }

        /* upcoming events */
        /*** FIXME: not necessary to update every time */
        if (satlist->flags & SAT_LIST_FLAG_AOS)
        {
            gtk_list_store_set(GTK_LIST_STORE(model), iter, SAT_LIST_COL_AOS,
                               sat->aos, -1);
        }
        if (satlist->flags & SAT_LIST_FLAG_LOS)
        {
            gtk_list_store_set(GTK_LIST_STORE(model), iter, SAT_LIST_COL_LOS,
                               sat->los, -1);

        }
        if (satlist->flags & SAT_LIST_FLAG_NEXT_EVENT)
        {
            gdouble         number;
            gchar           buff[TIME_FORMAT_MAX_LENGTH];
            gchar          *tfstr;
            gchar          *fmtstr;
            gchar          *alstr;


            if (sat->aos > sat->los)
            {
                /* next event is LOS */
                number = sat->los;
                alstr = g_strdup(" (LOS)");
            }
            else
            {
                /* next event is AOS */
                number = sat->aos;
                alstr = g_strdup(" (AOS)");
            }

            if (number == 0.0)
            {
                gtk_list_store_set(GTK_LIST_STORE(model), iter,
                                   SAT_LIST_COL_NEXT_EVENT, "--- N/A ---", -1);
            }
            else
            {

                /* format the number */
                tfstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
                fmtstr = g_strconcat(tfstr, alstr, NULL);
                g_free(tfstr);

                daynum_to_str(buff, TIME_FORMAT_MAX_LENGTH, fmtstr, number);

                gtk_list_store_set(GTK_LIST_STORE(model), iter,
                                   SAT_LIST_COL_NEXT_EVENT, buff, -1);


                g_free(fmtstr);
            }

            g_free(alstr);
        }

        if (satlist->flags & SAT_LIST_FLAG_VISIBILITY)
        {
            sat_vis_t       vis;

            vis = get_sat_vis(sat, satlist->qth, sat->jul_utc);
            buff = g_strdup_printf("%c", vis_to_chr(vis));
            gtk_list_store_set(GTK_LIST_STORE(model), iter,
                               SAT_LIST_COL_VISIBILITY, buff, -1);
            g_free(buff);
        }
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
        /* general float with 2 dec. precision
           no extra format besides a degree char
         */
    case SAT_LIST_COL_AZ:
    case SAT_LIST_COL_EL:
    case SAT_LIST_COL_RA:
    case SAT_LIST_COL_DEC:
    case SAT_LIST_COL_MA:
    case SAT_LIST_COL_PHASE:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                degree_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

        /* LAT/LON format */
    case SAT_LIST_COL_LAT:
    case SAT_LIST_COL_LON:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                latlon_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

        /* distances and velocities */
    case SAT_LIST_COL_RANGE:
    case SAT_LIST_COL_ALT:
    case SAT_LIST_COL_FOOTPRINT:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                distance_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SAT_LIST_COL_VEL:
    case SAT_LIST_COL_RANGE_RATE:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                range_rate_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SAT_LIST_COL_DOPPLER:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                float_to_int_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SAT_LIST_COL_DELAY:
    case SAT_LIST_COL_LOSS:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                two_dec_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SAT_LIST_COL_AOS:
    case SAT_LIST_COL_LOS:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                event_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;
    
    case SAT_LIST_COL_STAT_OPERATIONAL:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                operational_status_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;
    

    default:
        break;
    }
}

/* Render column containing the operational status */
static void     operational_status_cell_data_function(GtkTreeViewColumn * col,
                                           GtkCellRenderer * renderer,
                                           GtkTreeModel * model,
                                           GtkTreeIter * iter,
                                           gpointer column)
{
    gint            number;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;                  /* avoid unusued parameter compiler warning */

    gtk_tree_model_get(model, iter, coli, &number, -1);

    switch (number)
    {

    case OP_STAT_OPERATIONAL:
        g_object_set(renderer, "text", "Operational", NULL);
        break;

    case OP_STAT_NONOP:
        g_object_set(renderer, "text", "Non-operational", NULL);
        break;

    case OP_STAT_PARTIAL:
        g_object_set(renderer, "text", "Partially operational", NULL);
        break;

    case OP_STAT_STDBY:
        g_object_set(renderer, "text", "Backup/Standby", NULL);
        break;

    case OP_STAT_SPARE:
        g_object_set(renderer, "text", "Spare", NULL);
        break;

    case OP_STAT_EXTENDED:
        g_object_set(renderer, "text", "Extended Mission", NULL);
        break;

    default:
        g_object_set(renderer, "text", "Unknown", NULL);
        break;

    }

}

/* Render column containing lat/lon
   by using this instead of the default data function, we can
   control the number of decimals and display the coordinates in a
   fancy way, including degree sign and NWSE suffixes.

   Please note that this function only affects how the numbers are
   displayed (rendered), the tree_store will still contain the
   original floating point numbers. Very cool!
*/
static void latlon_cell_data_function(GtkTreeViewColumn * col,
                                      GtkCellRenderer * renderer,
                                      GtkTreeModel * model, GtkTreeIter * iter,
                                      gpointer column)
{
    gdouble         number = 0.0;
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

        if (coli == SAT_LIST_COL_LAT)
        {
            if (number < 0.00)
            {
                number = -number;
                hmf = 'S';
            }
            else
            {
                hmf = 'N';
            }
        }
        else if (coli == SAT_LIST_COL_LON)
        {
            if (number < 0.00)
            {
                number = -number;
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
                        _("%s:%d: Invalid column: %d"), __FILE__, __LINE__,
                        coli);
            hmf = '?';
        }
    }

    /* format the number */
    buff = g_strdup_printf("%.2f\302\260%c", number, hmf);
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* general floats with 2 digits + degree char */
static void degree_cell_data_function(GtkTreeViewColumn * col,
                                      GtkCellRenderer * renderer,
                                      GtkTreeModel * model,
                                      GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;                  /* avoid unusued parameter compiler warning */

    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf("%.2f\302\260", number);
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* distance and velocity, 0 decimal digits */
static void distance_cell_data_function(GtkTreeViewColumn * col,
                                        GtkCellRenderer * renderer,
                                        GtkTreeModel * model,
                                        GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;

    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* convert distance to miles? */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
    {
        number = KM_TO_MI(number);
    }

    /* format the number */
    buff = g_strdup_printf("%.0f", number);
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* range rate is special, because we may need to convert to miles
   and want 2-3 decimal digits.
*/
static void range_rate_cell_data_function(GtkTreeViewColumn * col,
                                          GtkCellRenderer * renderer,
                                          GtkTreeModel * model,
                                          GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;

    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* convert distance to miles? */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
    {
        number = KM_TO_MI(number);
    }

    /* format the number */
    buff = g_strdup_printf("%.3f", number);
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* 0 decimal digits */
static void float_to_int_cell_data_function(GtkTreeViewColumn * col,
                                            GtkCellRenderer * renderer,
                                            GtkTreeModel * model,
                                            GtkTreeIter * iter,
                                            gpointer column)
{
    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;                  /* avoid unusued parameter compiler warning */

    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf("%.0f", number);
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* 2 decimal digits */
static void two_dec_cell_data_function(GtkTreeViewColumn * col,
                                       GtkCellRenderer * renderer,
                                       GtkTreeModel * model,
                                       GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;

    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf("%.2f", number);
    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/* AOS/LOS; convert julian date to string */
static void event_cell_data_function(GtkTreeViewColumn * col,
                                     GtkCellRenderer * renderer,
                                     GtkTreeModel * model,
                                     GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar           buff[TIME_FORMAT_MAX_LENGTH];
    gchar          *fmtstr;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;                  /* avoid unusued parameter compiler warning */

    gtk_tree_model_get(model, iter, coli, &number, -1);

    if (number == 0.0)
    {
        g_object_set(renderer, "text", "--- N/A ---", NULL);
    }
    else
    {
        /* format the number */
        fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);

        daynum_to_str(buff, TIME_FORMAT_MAX_LENGTH, fmtstr, number);

        g_object_set(renderer, "text", buff, NULL);

        g_free(fmtstr);
    }

}

/**
 * Function to compare to Event cells.
 *
 * @param model Pointer to the GtkTreeModel.
 * @param a Pointer to the first element.
 * @param b Pointer to the second element.
 * @param user_data Always NULL (TBC).
 * 2return See detailed description.
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
    gint            sort_col;
    GtkSortType     sort_type;
    GtkSatList     *satlist = GTK_SAT_LIST(user_data);

    /* Since this function is used for both AOS and LOS columns,
       we need to get the sort column */
    gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(satlist->sortable),
                                         &sort_col, &sort_type);

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
void gtk_sat_list_reconf(GtkWidget * widget, GKeyFile * cfgdat)
{
    (void)widget;
    (void)cfgdat;
    sat_log_log(SAT_LOG_LEVEL_WARN, _("%s: FIXME I am not implemented"));
}

/**
 * Manage "popup-menu" events.
 *
 * @param treeview The tree view in the GtkSatList widget
 * @param list Pointer to the GtkSatList widget.
 *
 * This function is called when the "popup-menu" signal is emitted. This
 * usually happens if the user presses SHJIFT-F10? It is used as a wrapper
 * for the function that actually creates the popup menu.
 */
static gboolean popup_menu_cb(GtkWidget * treeview, gpointer list)
{
    /* if there is no selection, select the first row */
    view_popup_menu(treeview, NULL, list);

    return TRUE;                /* we handled this */
}

/**
 * Manage button press events.
 *
 * @param treeview The tree view in the GtkSatList widget.
 * @param event The event received.
 * @param list Pointer to the GtkSatList widget.
 *
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
                                                  (gint) event->y, &path, NULL,
                                                  NULL, NULL))
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

/** Manage row activated (double click) events. */
static void row_activated_cb(GtkTreeView * tree_view,
                             GtkTreePath * path,
                             GtkTreeViewColumn * column, gpointer list)
{
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    guint          *catnum;
    sat_t          *sat;

    (void)column;

    catnum = g_new0(guint, 1);

    model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, SAT_LIST_COL_CATNUM, catnum, -1);

    sat = SAT(g_hash_table_lookup(GTK_SAT_LIST(list)->satellites, catnum));

    if (sat == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%d Failed to get data for %d."), __FILE__, __LINE__,
                    *catnum);
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
        gtk_tree_model_get(model, &iter, SAT_LIST_COL_CATNUM, catnum, -1);

        sat = SAT(g_hash_table_lookup(GTK_SAT_LIST(list)->satellites, catnum));

        if (sat == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%d Failed to get data for %d."), __FILE__,
                        __LINE__, *catnum);

        }
        else
        {
            gtk_sat_list_popup_exec(sat, GTK_SAT_LIST(list)->qth, event,
                                    GTK_SAT_LIST(list));
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: There is no selection; skip popup."), __FILE__,
                    __LINE__);
    }

    g_free(catnum);
}

/*** FIXME: formalise with other copies, only need az,el and jul_utc */
static void Calculate_RADec(sat_t * sat, qth_t * qth, obs_astro_t * obs_set)
{
    /* Reference:  Methods of Orbit Determination by  */
    /*                Pedro Ramon Escobal, pp. 401-402 */

    double          phi, theta, sin_theta, cos_theta, sin_phi, cos_phi,
        az, el, Lxh, Lyh, Lzh, Sx, Ex, Zx, Sy, Ey, Zy, Sz, Ez, Zz,
        Lx, Ly, Lz, cos_delta, sin_alpha, cos_alpha;
    geodetic_t      geodetic;

    geodetic.lon = qth->lon * de2ra;
    geodetic.lat = qth->lat * de2ra;
    geodetic.alt = qth->alt / 1000.0;
    geodetic.theta = 0;

    az = sat->az * de2ra;
    el = sat->el * de2ra;
    phi = geodetic.lat;
    theta = FMod2p(ThetaG_JD(sat->jul_utc) + geodetic.lon);
    sin_theta = sin(theta);
    cos_theta = cos(theta);
    sin_phi = sin(phi);
    cos_phi = cos(phi);
    Lxh = -cos(az) * cos(el);
    Lyh = sin(az) * cos(el);
    Lzh = sin(el);
    Sx = sin_phi * cos_theta;
    Ex = -sin_theta;
    Zx = cos_theta * cos_phi;
    Sy = sin_phi * sin_theta;
    Ey = cos_theta;
    Zy = sin_theta * cos_phi;
    Sz = -cos_phi;
    Ez = 0;
    Zz = sin_phi;
    Lx = Sx * Lxh + Ex * Lyh + Zx * Lzh;
    Ly = Sy * Lxh + Ey * Lyh + Zy * Lzh;
    Lz = Sz * Lxh + Ez * Lyh + Zz * Lzh;
    obs_set->dec = ArcSin(Lz);  /* Declination (radians) */
    cos_delta = sqrt(1 - Sqr(Lz));
    sin_alpha = Ly / cos_delta;
    cos_alpha = Lx / cos_delta;
    obs_set->ra = AcTan(sin_alpha, cos_alpha);  /* Right Ascension (radians) */
    obs_set->ra = FMod2p(obs_set->ra);
}

/** Reload reference to satellites (e.g. after TLE update). */
void gtk_sat_list_reload_sats(GtkWidget * satlist, GHashTable * sats)
{
    GTK_SAT_LIST(satlist)->satellites = sats;
}

/** Select a satellite */
void gtk_sat_list_select_sat(GtkWidget * satlist, gint catnum)
{
    GtkSatList     *slist;
    GtkTreeModel   *model;
    GtkTreeSelection *selection;
    GtkTreeIter     iter;
    gint            i, n;
    gint            sat;


    slist = GTK_SAT_LIST(satlist);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(slist->treeview));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(slist->treeview));

    /* iterate over the satellite list until a amtch is found */
    n = g_hash_table_size(slist->satellites);
    for (i = 0; i < n; i++)
    {

        if (gtk_tree_model_iter_nth_child(model, &iter, NULL, i))
        {
            gtk_tree_model_get(model, &iter, SAT_LIST_COL_CATNUM, &sat, -1);
            if (sat == catnum)
            {
                gtk_tree_selection_select_iter(selection, &iter);
                i = n;
            }
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: GtkSatList has not child with index %d"),
                        __func__, i);
        }
    }
}
