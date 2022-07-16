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
/** Dialogs to show satellite passes */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gpredict-utils.h"
#include "gtk-azel-plot.h"
#include "gtk-polar-plot.h"
#include "gtk-sat-data.h"
#include "locator.h"
#include "pass-popup-menu.h"
#include "predict-tools.h"
#include "print-pass.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-vis.h"
#include "sat-pass-dialogs.h"
#include "save-pass.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

#define RESPONSE_PRINT 10
#define RESPONSE_SAVE  11

/** Column titles for multi-pass lists */
const gchar    *MULTI_PASS_COL_TITLE[MULTI_PASS_COL_NUMBER] = {
    N_("AOS"),
    N_("TCA"),
    N_("LOS"),
    N_("Duration"),
    N_("Max El"),
    N_("AOS Az"),
    N_("Max El Az"),
    N_("LOS Az"),
    N_("Orbit"),
    N_("Vis")
};

/** Descriptive text for multi-pass list columns. */
const gchar    *MULTI_PASS_COL_HINT[MULTI_PASS_COL_NUMBER] = {
    N_("Acquisition of signal (AOS)"),
    N_("Time of Closest Approach (TCA)"),
    N_("Loss of signal (LOS)"),
    N_("Duration of pass"),
    N_("Maximum elevation"),
    N_("Azimuth at AOS"),
    N_("Az at max. elevation"),
    N_("Azimuth at LOS"),
    N_("Orbit number"),
    N_("Visibility during pass")
};

const gdouble   MULTI_PASS_COL_XALIGN[MULTI_PASS_COL_NUMBER] = {
    0.5,                        // AOS
    0.5,                        // TCA
    0.5,                        // LOS
    0.5,                        // duration
    1.0,                        // max el
    1.0,                        // aos az
    1.0,                        // max el az
    1.0,                        // los az
    1.0,                        // orbit
    0.5,                        // visibility
};

/** Column titles indexed with column symb. refs. */
const gchar    *SINGLE_PASS_COL_TITLE[SINGLE_PASS_COL_NUMBER] = {
    N_("Time"),
    N_("Az"),
    N_("El"),
    N_("Ra"),
    N_("Dec"),
    N_("Range"),
    N_("Rate"),
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
    N_("Vis")
};

/** Column title hints indexed with column symb. refs. */
const gchar    *SINGLE_PASS_COL_HINT[SINGLE_PASS_COL_NUMBER] = {
    N_("Time"),
    N_("Azimuth"),
    N_("Elevation"),
    N_("Right Ascension"),
    N_("Declination"),
    N_("Slant Range"),
    N_("Range Rate"),
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
    N_("Visibility")
};

const gdouble   SINGLE_PASS_COL_XALIGN[SINGLE_PASS_COL_NUMBER] = {
    0.5,                        // time
    1.0,                        // az
    1.0,                        // el
    1.0,                        // RA
    1.0,                        // dec
    1.0,                        // range
    1.0,                        // range rate
    1.0,                        // lat
    1.0,                        // lon
    0.0,                        // SSP
    1.0,                        // footprint
    1.0,                        // alt
    0.0,                        // vel
    1.0,                        // doppler
    0.0,                        // loss
    0.0,                        // delay
    1.0,                        // MA
    1.0,                        // phase
    0.5,                        // visibility
};

static void     check_and_set_single_cell_renderer(GtkTreeViewColumn * column,
                                                   GtkCellRenderer * renderer,
                                                   gint i);

static void     check_and_set_multi_cell_renderer(GtkTreeViewColumn * column,
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

static void     time_cell_data_function(GtkTreeViewColumn * col,
                                        GtkCellRenderer * renderer,
                                        GtkTreeModel * model,
                                        GtkTreeIter * iter, gpointer column);

static void     duration_cell_data_function(GtkTreeViewColumn * col,
                                            GtkCellRenderer * renderer,
                                            GtkTreeModel * model,
                                            GtkTreeIter * iter,
                                            gpointer column);

static gint     single_pass_dialog_delete(GtkWidget *, GdkEvent *, gpointer);
static void     single_pass_dialog_destroy(GtkWidget *, gpointer);
static gint     multi_pass_dialog_delete(GtkWidget *, GdkEvent *, gpointer);
static void     multi_pass_dialog_destroy(GtkWidget *, gpointer);

static void     row_activated_cb(GtkTreeView * view, GtkTreePath * path,
                                 GtkTreeViewColumn * column, gpointer data);

static gboolean popup_menu_cb(GtkWidget * treeview, gpointer data);

static gboolean button_press_cb(GtkWidget * treeview,
                                GdkEventButton * event, gpointer data);

static void     view_popup_menu(GtkWidget * treeview,
                                GdkEventButton * event, gpointer data);

static void     Calc_RADec(gdouble jul_utc, gdouble saz, gdouble sel,
                           qth_t * qth, obs_astro_t * obs_set);

static void     single_pass_response(GtkWidget * dialog, gint response,
                                     gpointer data);
static void     multi_pass_response(GtkWidget * dialog, gint response,
                                    gpointer data);


/**
 * Show details about a satellite pass.
 *
 * @param parent The parent widget.
 * @param satname The name of the satellite.
 * @param qth Pointer to the QTH data.
 * @param pass The pass info.
 * @param toplevel The toplevel window or NULL.
 *
 * This function creates a dialog window containing a notebook with three pages:
 *   1. A list showing the details of a pass
 *   2. Polar plot of the pass
 *   3. Az/El plot of the pass
 *
 * Reference to the parent widget is needed to acquire the correct top-level
 * window, otherwise simply using the main window would bring that to front
 * covering any possible module windows. This would be unfortunate in the case
 * of fullscreen modules.
 */
void show_pass(const gchar * satname, qth_t * qth, pass_t * pass,
               GtkWidget * toplevel)
{
    GtkWidget      *dialog;     /* the dialog window */
    GtkWidget      *notebook;   /* the notebook widet */
    GtkWidget      *list;
    GtkListStore   *liststore;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter     item;
    GtkWidget      *swin;       /* scrolled window containing the list view */
    GtkWidget      *polar;      /* polar plot */
    GtkWidget      *azel;       /* Az/El plot */
    GtkWidget      *hbox;       /* hbox used in tab headers */
    GtkWidget      *image;      /* icon used in tab header */
    gchar          *title;
    guint           flags;
    guint           i, num;
    pass_detail_t  *detail;
    gchar          *buff;
    gint            retcode;
    gdouble         doppler;
    gdouble         delay;
    gdouble         loss;
    obs_astro_t     astro;
    gdouble         ra, dec;

    /* get columns flags */
    flags = sat_cfg_get_int(SAT_CFG_INT_PRED_SINGLE_COL);

    /* create list */
    list = gtk_tree_view_new();

    for (i = 0; i < SINGLE_PASS_COL_NUMBER; i++)
    {
        renderer = gtk_cell_renderer_text_new();
        g_object_set(G_OBJECT(renderer), "xalign", SINGLE_PASS_COL_XALIGN[i],
                     NULL);
        column =
            gtk_tree_view_column_new_with_attributes(_
                                                     (SINGLE_PASS_COL_TITLE
                                                      [i]), renderer, "text",
                                                     i, NULL);
        gtk_tree_view_insert_column(GTK_TREE_VIEW(list), column, -1);

        /* only aligns the headers */
        gtk_tree_view_column_set_alignment(column, 0.5);

        /* set cell data function; allows to format data before rendering */
        check_and_set_single_cell_renderer(column, renderer, i);

        /* hide columns that have not been specified */
        if (!(flags & (1 << i)))
        {
            gtk_tree_view_column_set_visible(column, FALSE);
        }
    }

    /* create and fill model */
    liststore = gtk_list_store_new(SINGLE_PASS_COL_NUMBER, G_TYPE_DOUBLE,       // time
                                   G_TYPE_DOUBLE,       // az
                                   G_TYPE_DOUBLE,       // el
                                   G_TYPE_DOUBLE,       // ra
                                   G_TYPE_DOUBLE,       // dec
                                   G_TYPE_DOUBLE,       // range
                                   G_TYPE_DOUBLE,       // range rate
                                   G_TYPE_DOUBLE,       // lat
                                   G_TYPE_DOUBLE,       // lon
                                   G_TYPE_STRING,       // SSP
                                   G_TYPE_DOUBLE,       // footprint
                                   G_TYPE_DOUBLE,       // alt
                                   G_TYPE_DOUBLE,       // vel
                                   G_TYPE_DOUBLE,       // doppler
                                   G_TYPE_DOUBLE,       // loss
                                   G_TYPE_DOUBLE,       // delay
                                   G_TYPE_DOUBLE,       // ma
                                   G_TYPE_DOUBLE,       // phase
                                   G_TYPE_STRING);      // visibility

    /* add rows to list store */
    num = g_slist_length(pass->details);

    for (i = 0; i < num; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(pass->details, i));

        gtk_list_store_append(liststore, &item);
        gtk_list_store_set(liststore, &item,
                           SINGLE_PASS_COL_TIME, detail->time,
                           SINGLE_PASS_COL_AZ, detail->az,
                           SINGLE_PASS_COL_EL, detail->el,
                           SINGLE_PASS_COL_RANGE, detail->range,
                           SINGLE_PASS_COL_RANGE_RATE, detail->range_rate,
                           SINGLE_PASS_COL_LAT, detail->lat,
                           SINGLE_PASS_COL_LON, detail->lon,
                           SINGLE_PASS_COL_FOOTPRINT, detail->footprint,
                           SINGLE_PASS_COL_ALT, detail->alt,
                           SINGLE_PASS_COL_VEL, detail->velo,
                           SINGLE_PASS_COL_MA, detail->ma,
                           SINGLE_PASS_COL_PHASE, detail->phase, -1);

        /*     SINGLE_PASS_COL_RA */
        /*     SINGLE_PASS_COL_DEC */
        if (flags & (SINGLE_PASS_FLAG_RA | SINGLE_PASS_FLAG_DEC))
        {
            Calc_RADec(detail->time, detail->az, detail->el, qth, &astro);

            ra = Degrees(astro.ra);
            dec = Degrees(astro.dec);

            gtk_list_store_set(liststore, &item,
                               SINGLE_PASS_COL_RA, ra, SINGLE_PASS_COL_DEC,
                               dec, -1);
        }

        /*     SINGLE_PASS_COL_SSP */
        if (flags & SINGLE_PASS_FLAG_SSP)
        {
            buff = g_try_malloc(7);
            retcode = longlat2locator(detail->lon, detail->lat, buff, 3);
            if (retcode == RIG_OK)
            {
                buff[6] = '\0';
                gtk_list_store_set(liststore, &item, SINGLE_PASS_COL_SSP, buff,
                                   -1);
            }
            g_free(buff);
        }

        /*      SINGLE_PASS_COL_DOPPLER */
        if (flags & SINGLE_PASS_FLAG_DOPPLER)
        {
            doppler = -100.0e06 * (detail->range_rate / 299792.4580);   // Hz
            gtk_list_store_set(liststore, &item, SINGLE_PASS_COL_DOPPLER,
                               doppler, -1);
        }

        /*     SINGLE_PASS_COL_LOSS */
        if (flags & SINGLE_PASS_FLAG_LOSS)
        {
            loss = 72.4 + 20.0 * log10(detail->range);  // dB
            gtk_list_store_set(liststore, &item, SINGLE_PASS_COL_LOSS, loss,
                               -1);
        }

        /*     SINGLE_PASS_COL_DELAY */
        if (flags & SINGLE_PASS_FLAG_DELAY)
        {
            delay = detail->range / 299.7924580;        // msec 
            gtk_list_store_set(liststore, &item, SINGLE_PASS_COL_DELAY, delay,
                               -1);
        }

        /*     SINGLE_PASS_COL_VIS */
        if (flags & SINGLE_PASS_FLAG_VIS)
        {
            buff = g_strdup_printf("%c", vis_to_chr(detail->vis));
            gtk_list_store_set(liststore, &item, SINGLE_PASS_COL_VIS, buff,
                               -1);
            g_free(buff);
        }
    }

    /* connect model to tree view */
    gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(liststore));
    g_object_unref(liststore);

    /* scrolled window */
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(swin), list);


    /* create notebook and add pages */
    notebook = gtk_notebook_new();
#ifdef G_OS_WIN32
    image = gtk_image_new_from_icon_name("format-justify-fill-symbolic",
                                         GTK_ICON_SIZE_BUTTON);
#else
    image = gtk_image_new_from_icon_name("format-justify-fill",
                                         GTK_ICON_SIZE_BUTTON);
#endif
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Data")), FALSE, TRUE,
                       5);
    gtk_widget_show_all(hbox);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), swin, hbox);

    /* polar plot */
    polar = gtk_polar_plot_new(qth, pass);
    buff = icon_file_name("gpredict-polar-small.png");
    image = gtk_image_new_from_file(buff);
    g_free(buff);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Polar")), FALSE, TRUE,
                       5);
    gtk_widget_show_all(hbox);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), polar, hbox);

    /* Az/El plot */
    azel = gtk_azel_plot_new(qth, pass);
    buff = icon_file_name("gpredict-azel-small.png");
    image = gtk_image_new_from_file(buff);
    g_free(buff);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Az/El")), FALSE, TRUE,
                       5);
    gtk_widget_show_all(hbox);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), azel, hbox);

    /* create dialog */
    title = g_strdup_printf(_("Pass details for %s (orbit %d)"),
                            satname, pass->orbit);

    /* use NULL as parent to avoid conflict when using undocked windows
       as parents.
     */
    dialog = gtk_dialog_new_with_buttons(title,
                                         GTK_WINDOW(toplevel),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Print", RESPONSE_PRINT,
                                         "_Save", RESPONSE_SAVE,
                                         "_Close", GTK_RESPONSE_CLOSE,
                                         NULL);
    g_free(title);

    /* Make Close button default */
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);

    /* window icon */
    buff = icon_file_name("gpredict-sat-list.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(dialog), buff, NULL);
    g_free(buff);

    /* allow interaction with other windows */
    gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);

    g_object_set_data(G_OBJECT(dialog), "sat", (gpointer) satname);
    g_object_set_data(G_OBJECT(dialog), "qth", qth);
    g_object_set_data(G_OBJECT(dialog), "pass", pass);

    g_signal_connect(dialog, "response", G_CALLBACK(single_pass_response),
                     NULL);
    g_signal_connect(dialog, "destroy", G_CALLBACK(single_pass_dialog_destroy),
                     NULL);
    g_signal_connect(dialog, "delete_event",
                     G_CALLBACK(single_pass_dialog_delete), NULL);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       notebook, TRUE, TRUE, 0);

    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 300);
    gtk_widget_show_all(dialog);
}

/**
 * Manage button responses for single-pass dialogues.
 *
 * @param dialog The dialog widget.
 * @param response The ID of the response signal, i.e. the pressed button.
 * @param data User data (currently NULL).
 *
 * Use sat, qth, and passes labels to obtain the relevant data
 */
static void single_pass_response(GtkWidget * dialog, gint response,
                                 gpointer data)
{
    (void)data;
    pass_t         *pass;
    qth_t          *qth;

    switch (response)
    {
    case RESPONSE_PRINT:
        pass = (pass_t *) g_object_get_data(G_OBJECT(dialog), "pass");
        qth = (qth_t *) g_object_get_data(G_OBJECT(dialog), "qth");
        print_pass(pass, qth, GTK_WINDOW(dialog));
        break;
    case RESPONSE_SAVE:
        save_pass(dialog);
        break;
        /* Close button or delete events */
    default:
        gtk_widget_destroy(dialog);
        break;
    }
}

/** Set cell renderer function. */
static void check_and_set_single_cell_renderer(GtkTreeViewColumn * column,
                                               GtkCellRenderer * renderer,
                                               gint i)
{

    switch (i)
    {
        /* general float with 2 dec. precision
           no extra format besides a degree char
         */
    case SINGLE_PASS_COL_AZ:
    case SINGLE_PASS_COL_EL:
    case SINGLE_PASS_COL_RA:
    case SINGLE_PASS_COL_DEC:
    case SINGLE_PASS_COL_MA:
    case SINGLE_PASS_COL_PHASE:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                degree_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

        /* LAT/LON format */
    case SINGLE_PASS_COL_LAT:
    case SINGLE_PASS_COL_LON:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                latlon_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

        /* distances and velocities */
    case SINGLE_PASS_COL_RANGE:
    case SINGLE_PASS_COL_ALT:
    case SINGLE_PASS_COL_FOOTPRINT:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                distance_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SINGLE_PASS_COL_VEL:
    case SINGLE_PASS_COL_RANGE_RATE:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                range_rate_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SINGLE_PASS_COL_DOPPLER:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                float_to_int_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SINGLE_PASS_COL_DELAY:
    case SINGLE_PASS_COL_LOSS:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                two_dec_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case SINGLE_PASS_COL_TIME:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                time_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    default:
        break;
    }
}

/* render column containing lat/lon
   by using this instead of the default data function, we can
   control the number of decimals and display the coordinates in a
   fancy way, including degree sign and NWSE suffixes.

   Please note that this function only affects how the numbers are
   displayed (rendered), the tree_store will still contain the
   original floating point numbers. Very cool!
*/
static void latlon_cell_data_function(GtkTreeViewColumn * col,
                                      GtkCellRenderer * renderer,
                                      GtkTreeModel * model,
                                      GtkTreeIter * iter, gpointer column)
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

        if (coli == SINGLE_PASS_COL_LAT)
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
        else if (coli == SINGLE_PASS_COL_LON)
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
    g_object_set(renderer, "text", buff, "xalign", 1.0, NULL);
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

    (void)col;

    gtk_tree_model_get(model, iter, coli, &number, -1);

    /* format the number */
    buff = g_strdup_printf("%.2f\302\260", number);
    g_object_set(renderer, "text", buff, "xalign", 1.0, NULL);
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

    (void)col;

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
static void time_cell_data_function(GtkTreeViewColumn * col,
                                    GtkCellRenderer * renderer,
                                    GtkTreeModel * model,
                                    GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar           buff[TIME_FORMAT_MAX_LENGTH];
    gchar          *fmtstr;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;

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

static gint single_pass_dialog_delete(GtkWidget * dialog, GdkEvent * event,
                                      gpointer pass)
{
    (void)dialog;
    (void)event;
    (void)pass;

    /* dialog will be destroyed */
    return FALSE;
}

static void single_pass_dialog_destroy(GtkWidget * dialog, gpointer data)
{
    pass_t         *pass = PASS(g_object_get_data(G_OBJECT(dialog), "pass"));

    (void)data;

    free_pass(PASS(pass));

    gtk_widget_destroy(dialog);
}

/*** FIXME: other copies */
static void Calc_RADec(gdouble jul_utc, gdouble saz, gdouble sel,
                       qth_t * qth, obs_astro_t * obs_set)
{

    double          phi, theta, sin_theta, cos_theta, sin_phi, cos_phi,
        az, el, Lxh, Lyh, Lzh, Sx, Ex, Zx, Sy, Ey, Zy, Sz, Ez, Zz,
        Lx, Ly, Lz, cos_delta, sin_alpha, cos_alpha;
    geodetic_t      geodetic;

    geodetic.lon = qth->lon * de2ra;
    geodetic.lat = qth->lat * de2ra;
    geodetic.alt = qth->alt / 1000.0;
    geodetic.theta = 0;

    az = saz * de2ra;
    el = sel * de2ra;
    phi = geodetic.lat;
    theta = FMod2p(ThetaG_JD(jul_utc) + geodetic.lon);
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

/***   MULTI PASS  ***/

/**
 * Show details about a satellite pass.
 *
 * @param satname The name of the satellite.
 * @param qth Pointer to the QTH data.
 * @param passes List of passes to show.
 * @param toplevel The toplevel window or NULL.
 *
 * This function creates a dialog window with a list showing the
 * details of a pass.
 *
 */
void show_passes(const gchar * satname, qth_t * qth, GSList * passes,
                 GtkWidget * toplevel)
{
    GtkWidget      *dialog;
    GtkWidget      *list;
    GtkListStore   *liststore;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter     item;
    GtkWidget      *swin;
    gchar          *title;
    guint           flags;
    guint           i, num;
    pass_t         *pass = NULL;
    gchar          *buff;

    /* get columns flags */
    flags = sat_cfg_get_int(SAT_CFG_INT_PRED_MULTI_COL);

    /* create list */
    list = gtk_tree_view_new();

    for (i = 0; i < MULTI_PASS_COL_NUMBER; i++)
    {
        renderer = gtk_cell_renderer_text_new();
        g_object_set(G_OBJECT(renderer), "xalign", MULTI_PASS_COL_XALIGN[i],
                     NULL);
        column =
            gtk_tree_view_column_new_with_attributes(_
                                                     (MULTI_PASS_COL_TITLE[i]),
                                                     renderer, "text", i,
                                                     NULL);
        gtk_tree_view_insert_column(GTK_TREE_VIEW(list), column, -1);

        /* only aligns the headers */
        gtk_tree_view_column_set_alignment(column, 0.5);

        /* set cell data function; allows to format data before rendering */
        check_and_set_multi_cell_renderer(column, renderer, i);

        /* hide columns that have not been specified */
        if (!(flags & (1 << i)))
        {
            gtk_tree_view_column_set_visible(column, FALSE);
        }
    }

    /* create and fill model */
    liststore = gtk_list_store_new(MULTI_PASS_COL_NUMBER + 1, G_TYPE_DOUBLE,    // aos time
                                   G_TYPE_DOUBLE,       // tca time
                                   G_TYPE_DOUBLE,       // los time
                                   G_TYPE_DOUBLE,       // duration
                                   G_TYPE_DOUBLE,       // aos az
                                   G_TYPE_DOUBLE,       // max el
                                   G_TYPE_DOUBLE,       // az @ max el
                                   G_TYPE_DOUBLE,       // los az
                                   G_TYPE_INT,  // orbit
                                   G_TYPE_STRING,       // visibility
                                   G_TYPE_INT); // row number

    /* add rows to list store */
    num = g_slist_length(passes);

    for (i = 0; i < num; i++)
    {
        pass = PASS(g_slist_nth_data(passes, i));
        gtk_list_store_append(liststore, &item);
        gtk_list_store_set(liststore, &item,
                           MULTI_PASS_COL_AOS_TIME, pass->aos,
                           MULTI_PASS_COL_TCA, pass->tca,
                           MULTI_PASS_COL_LOS_TIME, pass->los,
                           MULTI_PASS_COL_DURATION, (pass->los - pass->aos),
                           MULTI_PASS_COL_AOS_AZ, pass->aos_az,
                           MULTI_PASS_COL_MAX_EL, pass->max_el,
                           MULTI_PASS_COL_MAX_EL_AZ, pass->maxel_az,
                           MULTI_PASS_COL_LOS_AZ, pass->los_az,
                           MULTI_PASS_COL_ORBIT, pass->orbit,
                           MULTI_PASS_COL_VIS, pass->vis,
                           MULTI_PASS_COL_NUMBER, i, -1);
    }

    /* connect model to tree view */
    gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(liststore));
    g_object_unref(liststore);

    /* store reference to passes and QTH */
    g_object_set_data(G_OBJECT(list), "passes", passes);
    g_object_set_data(G_OBJECT(list), "qth", qth);

    /* mouse events => popup menu */
    g_signal_connect(list, "button-press-event", G_CALLBACK(button_press_cb),
                     NULL);
    g_signal_connect(list, "popup-menu", G_CALLBACK(popup_menu_cb), NULL);

    /* "row-activated" signal is used to catch double click events, which means
       a pass has been double clicked => show details */
    g_signal_connect(list, "row-activated", G_CALLBACK(row_activated_cb),
                     toplevel);

    /* scrolled window */
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(swin), list);

    /* create dialog */
    title = g_strdup_printf(_("Upcoming passes for %s"), satname);

    /* use NULL as parent to avoid conflict when using undocked windows
       as parents.    */
    dialog = gtk_dialog_new_with_buttons(title,
                                         GTK_WINDOW(toplevel),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Print", RESPONSE_PRINT,
                                         "_Save", RESPONSE_SAVE,
                                         "_Close", GTK_RESPONSE_CLOSE,
                                         NULL);
    g_free(title);

    /* Make Close button default */
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);

    /* window icon */
    buff = icon_file_name("gpredict-sat-list.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(dialog), buff, NULL);
    g_free(buff);

    /* allow interaction with other windows */
    gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);

    g_object_set_data(G_OBJECT(dialog), "sat", (gpointer) satname);
    g_object_set_data(G_OBJECT(dialog), "qth", qth);
    g_object_set_data(G_OBJECT(dialog), "passes", passes);

    g_signal_connect(dialog, "response", G_CALLBACK(multi_pass_response),
                     NULL);
    g_signal_connect(dialog, "destroy", G_CALLBACK(multi_pass_dialog_destroy),
                     NULL);
    g_signal_connect(dialog, "delete_event",
                     G_CALLBACK(multi_pass_dialog_delete), NULL);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       swin, TRUE, TRUE, 0);

    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 300);
    gtk_widget_show_all(dialog);
}

/**
 * Manage button responses for multi-pass dialogues.
 *
 * @param dialog The dialog widget.
 * @param response The ID of the response signal, i.e. the pressed button.
 * @param data User data (currently NULL).
 *
 * Use sat, qth, and passes labels to obtain the relevant data
 *
 */
static void multi_pass_response(GtkWidget * dialog, gint response,
                                gpointer data)
{
    (void)data;

    switch (response)
    {
    case RESPONSE_PRINT:
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: PRINT not implemented"),
                    __func__);
        break;
    case RESPONSE_SAVE:
        save_passes(dialog);
        break;
        /* Close button or delete events */
    default:
        gtk_widget_destroy(dialog);
        break;
    }
}

static gint multi_pass_dialog_delete(GtkWidget * dialog, GdkEvent * event,
                                     gpointer pass)
{
    (void)dialog;
    (void)event;
    (void)pass;

    /* dialog will be destroyed */
    return FALSE;
}

static void multi_pass_dialog_destroy(GtkWidget * dialog, gpointer data)
{
    GSList         *passes =
        (GSList *) g_object_get_data(G_OBJECT(dialog), "passes");

    (void)data;

    free_passes(passes);
    gtk_widget_destroy(dialog);
}

/** Set cell renderer function. */
static void check_and_set_multi_cell_renderer(GtkTreeViewColumn * column,
                                              GtkCellRenderer * renderer,
                                              gint i)
{
    switch (i)
    {
        /* general float with 2 dec. precision
           no extra format besides a degree char
         */
    case MULTI_PASS_COL_AOS_AZ:
    case MULTI_PASS_COL_LOS_AZ:
    case MULTI_PASS_COL_MAX_EL:
    case MULTI_PASS_COL_MAX_EL_AZ:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                degree_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case MULTI_PASS_COL_AOS_TIME:
    case MULTI_PASS_COL_TCA:
    case MULTI_PASS_COL_LOS_TIME:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                time_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    case MULTI_PASS_COL_DURATION:
        gtk_tree_view_column_set_cell_data_func(column,
                                                renderer,
                                                duration_cell_data_function,
                                                GUINT_TO_POINTER(i), NULL);
        break;

    default:
        break;
    }
}

/* duration; convert delta t in days to HH:MM:SS */
static void duration_cell_data_function(GtkTreeViewColumn * col,
                                        GtkCellRenderer * renderer,
                                        GtkTreeModel * model,
                                        GtkTreeIter * iter, gpointer column)
{
    gdouble         number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);
    guint           h, m, s;

    (void)col;

    gtk_tree_model_get(model, iter, coli, &number, -1);

    if (number == 0.0)
    {
        g_object_set(renderer, "text", "- N/A -", NULL);
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

        buff = g_strdup_printf("%02d:%02d:%02d", h, m, s);

        g_object_set(renderer, "text", buff, NULL);

        g_free(buff);
    }
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
static gboolean popup_menu_cb(GtkWidget * treeview, gpointer data)
{
    /* if there is no selection, select the first row */
    view_popup_menu(treeview, NULL, data);

    return TRUE;                /* we handled this */
}

/**
 * Manage button press events.
 *
 * @param treeview The tree view in the GtkSatList widget.
 * @param event The event received.
 * @param list Pointer to the GtkSatList widget.
 */
static gboolean button_press_cb(GtkWidget * treeview, GdkEventButton * event,
                                gpointer data)
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

        view_popup_menu(treeview, event, data);

        return TRUE;            /* we handled this */
    }

    return FALSE;               /* we did not handle this */
}

static void view_popup_menu(GtkWidget * treeview, GdkEventButton * event,
                            gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    guint           rownum = 0;
    GSList         *passes = NULL;
    pass_t         *pass = NULL;
    qth_t          *qth;

    (void)data;

    /* get selected satellite */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        /* get data */
        passes = (GSList *) g_object_get_data(G_OBJECT(treeview), "passes");
        qth = (qth_t *) g_object_get_data(G_OBJECT(treeview), "qth");

        gtk_tree_model_get(model, &iter, MULTI_PASS_COL_NUMBER, &rownum, -1);

        /* get selected pass */
        pass = copy_pass(PASS(g_slist_nth_data(passes, rownum)));

        pass_popup_menu_exec(qth, pass, event,
                             gtk_widget_get_toplevel(treeview));
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: There is no selection; skip popup."), __FILE__,
                    __LINE__);
    }
}

/**
 * Signal handler for managing double clicks in multi-pass dialog.
 *
 * @param view Pointer to the GtkTreeView object.
 * @param path The path of the row that was activated.
 * @param column The column where the activation occurred.
 * @param data Pointer to the toplevel window.
 *
 * This function is called when the user double clicks on a pass in the
 * multi-pass dialog. This will cause the pass details to be shown.
 */
static void row_activated_cb(GtkTreeView * treeview, GtkTreePath * path,
                             GtkTreeViewColumn * column, gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    GtkWidget      *toplevel = GTK_WIDGET(data);
    guint           rownum = 0;
    GSList         *passes = NULL;
    pass_t         *pass = NULL;
    qth_t          *qth;

    (void)path;
    (void)column;

    /* get selected satellite */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        /* get data */
        passes = (GSList *) g_object_get_data(G_OBJECT(treeview), "passes");
        qth = (qth_t *) g_object_get_data(G_OBJECT(treeview), "qth");

        gtk_tree_model_get(model, &iter, MULTI_PASS_COL_NUMBER, &rownum, -1);

        /* get selected pass */
        pass = copy_pass(PASS(g_slist_nth_data(passes, rownum)));

        show_pass(pass->satname, qth, pass, toplevel);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: There is no selection; skip popup."), __FILE__,
                    __LINE__);
    }
}
