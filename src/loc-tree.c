/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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

/** Tree widget containing locations info */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <math.h>
#include "compat.h"
#include "loc-tree.h"
#include "sat-cfg.h"
#include "sat-log.h"


/* long story... */
#define LTMN   123456.7
#define LTMNI  123456
#define LTEPS  1.0


static GtkTreeModel *loc_tree_create_and_fill_model(const gchar * fname);

static void     loc_tree_float_cell_data_function(GtkTreeViewColumn * col,
                                                  GtkCellRenderer * renderer,
                                                  GtkTreeModel * model,
                                                  GtkTreeIter * iter,
                                                  gpointer column);

static void     loc_tree_int_cell_data_function(GtkTreeViewColumn * col,
                                                GtkCellRenderer * renderer,
                                                GtkTreeModel * model,
                                                GtkTreeIter * iter,
                                                gpointer column);

static gboolean loc_tree_check_selection_cb(GtkTreeSelection * selection,
                                            GtkTreeModel * model,
                                            GtkTreePath * path,
                                            gboolean selpath, gpointer dialog);

static void     loc_tree_get_selection(GtkWidget * view,
                                       gchar ** loc,
                                       gfloat * lat,
                                       gfloat * lon, guint * alt, gchar ** wx);

/**
 * Create and initialise location selector.
 *
 * @param fname The name of the file, which contains locations data. Can be NULL.
 * @param flags Bitise or of flags indicating which columns to display.
 * @param location Newly allocated string containing location (city, country)
 * @param lat Pointer to where the latitude should be stored.
 * @param lon Pointer to where the longitude should be stored.
 * @param alt Pointer to where the altitude should be stored.
 * @param wx Newly allocated string containing the four letter weather station name.
 * @return TRUE if a location has been selected and the returned data is valid,
 *         FALSE otherwise, fx. if the user has clicked on the Cancel button.
 *
 * @note All data fields will be populated (and both strings allocated) no matter which
 *       flags have been passed by the user. The flags influence only how the tree view
 *       is displayed.
 */
gboolean loc_tree_create(const gchar * fname,
                         guint flags,
                         gchar ** loc,
                         gfloat * lat, gfloat * lon, guint * alt, gchar ** wx)
{
    GtkCellRenderer *renderer;  /* tree view cell renderer */
    GtkTreeViewColumn *column;  /* tree view column used to add columns */
    GtkTreeModel   *model;      /* tree model */
    GtkWidget      *view;       /* tree view widget */
    GtkTreeSelection *selection;        /* used to set selection checking func */
    GtkWidget      *swin;       /* scrolled window widget */
    GtkWidget      *dialog;     /* the dialog widget */
    gint            response;   /* response ID returned by gtk_dialog_run */
    gchar          *ffname;
    gboolean        retval;

    if (!fname)
        ffname = data_file_name("locations.dat");
    else
        ffname = g_strdup(fname);

    view = gtk_tree_view_new();

    /* Create columns.
       Note that there are several ways to create and add the individual
       columns, especially there are tree_view_insert_col functions, which
       do not require explicit creation of columns. I have chosen to
       explicitly create the columns in order to be able to hide them
       according to the flags parameter.
     */

    /* --- Column #1 --- */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Location"),
                                                      renderer,
                                                      "text", TREE_COL_NAM,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    if (!(flags & TREE_COL_FLAG_NAME))
    {
        gtk_tree_view_column_set_visible(column, FALSE);
    }

    /* --- Column #2 --- */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Lat"),
                                                      renderer,
                                                      "text", TREE_COL_LAT,
                                                      NULL);
    gtk_tree_view_column_set_alignment(column, 0.5);
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            loc_tree_float_cell_data_function,
                                            GUINT_TO_POINTER(TREE_COL_LAT),
                                            NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    if (!(flags & TREE_COL_FLAG_LAT))
    {
        gtk_tree_view_column_set_visible(column, FALSE);
    }

    /* --- Column #3 --- */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Lon"),
                                                      renderer,
                                                      "text", TREE_COL_LON,
                                                      NULL);
    gtk_tree_view_column_set_alignment(column, 0.5);
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            loc_tree_float_cell_data_function,
                                            GUINT_TO_POINTER(TREE_COL_LON),
                                            NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    if (!(flags & TREE_COL_FLAG_LON))
    {
        gtk_tree_view_column_set_visible(column, FALSE);
    }

    /* --- Column #4 --- */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Alt"),
                                                      renderer,
                                                      "text", TREE_COL_ALT,
                                                      NULL);
    gtk_tree_view_column_set_alignment(column, 0.5);
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            loc_tree_int_cell_data_function,
                                            GUINT_TO_POINTER(TREE_COL_ALT),
                                            NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    if (!(flags & TREE_COL_FLAG_ALT))
    {
        gtk_tree_view_column_set_visible(column, FALSE);
    }

    /* --- Column #5 --- */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("WX"),
                                                      renderer,
                                                      "text", TREE_COL_WX,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    if (!(flags & TREE_COL_FLAG_WX))
    {
        gtk_tree_view_column_set_visible(column, FALSE);
    }

    /* Invisible column holding 0 or 1 indicating whether a row can be selected
       or not. We use this to prevent the user from selecting regions or
       countries, since they are not valid locations.
     */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("X"),
                                                      renderer,
                                                      "text", TREE_COL_SELECT,
                                                      NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, -1);
    gtk_tree_view_column_set_visible(column, FALSE);

    /* create model and finalise treeview */
    model = loc_tree_create_and_fill_model(ffname);

    /* we are done with it */
    g_free(ffname);

    gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

    /* The tree view has acquired its own reference to the
     *  model, so we can drop ours. That way the model will
     *  be freed automatically when the tree view is destroyed */
    g_object_unref(model);

    /* make sure rows are checked when they are selected */
    /* ... but first create the dialog window .... */

    /* scrolled window */
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swin), view);

    gtk_widget_show_all(swin);

    /* dialog window */
    dialog = gtk_dialog_new_with_buttons(_("Select Location"),
                                         NULL,
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         "_OK", GTK_RESPONSE_ACCEPT,
                                         NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 400);

    /* OK button disabled by default until a valid selection is made */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                      GTK_RESPONSE_ACCEPT, FALSE);
    gtk_box_pack_start(GTK_BOX
                       (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), swin,
                       TRUE, TRUE, 0);

    /* connect selection checker for the tree-view;
       we have waited so far, because we want to pass the dialog as
       parameter
     */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_select_function(selection,
                                           loc_tree_check_selection_cb,
                                           dialog, NULL);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT)
    {
        loc_tree_get_selection(view, loc, lat, lon, alt, wx);
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Selected %s"), __func__, *loc);
        retval = TRUE;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: No location selected"),
                    __func__);
        retval = FALSE;
    }

    gtk_widget_destroy(dialog);

    return retval;
}

static GtkTreeModel *loc_tree_create_and_fill_model(const gchar * fname)
{
    GtkTreeStore   *treestore;  /* tree store, which is loaded and returned */
    GtkTreeIter     toplevel;   /* highest level rows, continent or region */
    GtkTreeIter     midlevel;   /* mid level rows, country or state in the US */
    GtkTreeIter     child;      /* lowest level rows, cities */
    GIOChannel     *locfile;    /* file we read locations from */
    gchar          *line;       /* line read from file */
    gchar         **buff;       /* temporary buffer to store line pieces */
    gsize           length;     /* line length */
    guint           i = 0;      /* number of lines read */
    gchar          *continent = g_strdup("DUMMY");      /* current continent */
    gchar          *country = g_strdup("DUMMY");        /* current country */
    GError         *error = NULL;       /* error data when reading file */

    treestore = gtk_tree_store_new(TREE_COL_NUM,
                                   G_TYPE_STRING,
                                   G_TYPE_FLOAT,
                                   G_TYPE_FLOAT,
                                   G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT);

    /* if the supplied file does not exist
       simply return the empty model
       FIXME: should we fall back to PACKAGE_DATA_DIR/locations.dat ?
     */
    if (!g_file_test(fname, G_FILE_TEST_EXISTS))
    {

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: %s does not exist!"), __func__, fname);

        g_free(continent);
        g_free(country);
        return GTK_TREE_MODEL(treestore);
    }

    /* open file and read it line by line */
    locfile = g_io_channel_new_file(fname, "r", &error);

    if (locfile)
    {

        while (g_io_channel_read_line(locfile,
                                      &line,
                                      &length, NULL, NULL) != G_IO_STATUS_EOF)
        {

            /* trim line and split it */
            line = g_strdelimit(line, "\n", '\0');

            buff = g_strsplit(line, ";", 7);

            /* buff[0] = continent / region
               buff[1] = country or state in US
               buff[2] = city
               buff[3] = weather station
               buff[4] = latitude (dec. deg. north)
               buff[5] = longitude (dec. deg. east)
               buff[6] = altitude
             */

            /* new region? */
            if (g_ascii_strcasecmp(buff[0], continent))
            {

                g_free(continent);

                continent = g_strdup(buff[0]);

                gtk_tree_store_append(treestore, &toplevel, NULL);
                gtk_tree_store_set(treestore, &toplevel,
                                   TREE_COL_NAM, continent,
                                   TREE_COL_LAT, LTMN,
                                   TREE_COL_LON, LTMN,
                                   TREE_COL_ALT, LTMNI,
                                   TREE_COL_SELECT, 0, -1);

            }

            /* new country? */
            if (g_ascii_strcasecmp(buff[1], country))
            {

                g_free(country);

                country = g_strdup(buff[1]);

                gtk_tree_store_append(treestore, &midlevel, &toplevel);
                gtk_tree_store_set(treestore, &midlevel,
                                   TREE_COL_NAM, country,
                                   TREE_COL_LAT, LTMN,
                                   TREE_COL_LON, LTMN,
                                   TREE_COL_ALT, LTMNI,
                                   TREE_COL_SELECT, 0, -1);
            }

            /* add city */
            gtk_tree_store_append(treestore, &child, &midlevel);
            gtk_tree_store_set(treestore, &child,
                               TREE_COL_NAM, buff[2],
                               TREE_COL_WX, buff[3],
                               TREE_COL_LAT, g_ascii_strtod(buff[4], NULL),
                               TREE_COL_LON, g_ascii_strtod(buff[5], NULL),
                               /* Crashes here if type is not correctly cast */
                               TREE_COL_ALT, (guint) g_ascii_strtod(buff[6],
                                                                    NULL),
                               TREE_COL_SELECT, 1, -1);


            /* finish and clean up */
            i++;

            /* free allocated memory */
            g_free(line);
            g_strfreev(buff);
        }

        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Read %d cities."), __func__, i);

        if (continent)
            g_free(continent);

        if (country)
            g_free(country);

        /* Close IO channel; don't care about status.
           Shutdown will flush the stream and close the channel
           as soon as the reference count is dropped. Order matters!
         */
        g_io_channel_shutdown(locfile, TRUE, NULL);
        g_io_channel_unref(locfile);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to open locfile (%s)"),
                    __func__, error->message);
        g_clear_error(&error);
    }

    return GTK_TREE_MODEL(treestore);
}

/* render column containing float
   by using this instead of the default data function, we can
   disable lat,lon and alt for the continent and country rows.

   Please note that this function only affects how the numbers are
   displayed (rendered), the tree_store will still contain the
   original floating point numbers. Very cool!
*/
static void loc_tree_float_cell_data_function(GtkTreeViewColumn * col,
                                              GtkCellRenderer * renderer,
                                              GtkTreeModel * model,
                                              GtkTreeIter * iter,
                                              gpointer column)
{
    gfloat          number;
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
        if (coli == TREE_COL_LAT)
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
        else if (coli == TREE_COL_LON)
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
                        _("%s: Invalid column: %d"), __func__, coli);
            hmf = '?';
        }
    }

    if (fabs(LTMN - number) > LTEPS)
        buff = g_strdup_printf("%.4f\302\260%c", number, hmf);
    else
        buff = g_strdup("");

    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/** Render column containing integer */
static void loc_tree_int_cell_data_function(GtkTreeViewColumn * col,
                                            GtkCellRenderer * renderer,
                                            GtkTreeModel * model,
                                            GtkTreeIter * iter,
                                            gpointer column)
{
    gint            number;
    gchar          *buff;
    guint           coli = GPOINTER_TO_UINT(column);

    (void)col;

    gtk_tree_model_get(model, iter, GPOINTER_TO_UINT(column), &number, -1);

    if (coli == TREE_COL_ALT)
    {
        if (number != LTMNI)
            buff = g_strdup_printf("%d", number);
        else
            buff = g_strdup("");
    }
    else
    {
        buff = g_strdup_printf("%d", number);
    }

    g_object_set(renderer, "text", buff, NULL);
    g_free(buff);
}

/**
 * Check current selection.
 *
 * This function is used to check the currently selected row. This is to avoid
 * selection of region and countries. The function is called as a callback function
 * every time a row is selected.
 * The decision is based on the integer value stored in the invisible column
 * TREE_COL_SELECT. A value of 0 means row may not be selected, while a value of 1
 * means that the row can be selected.
 */
static gboolean loc_tree_check_selection_cb(GtkTreeSelection * selection,
                                            GtkTreeModel * model,
                                            GtkTreePath * path,
                                            gboolean sel_path, gpointer dialog)
{

    GtkTreeIter     iter;

    (void)selection;
    (void)sel_path;

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        guint           value;

        gtk_tree_model_get(model, &iter, TREE_COL_SELECT, &value, -1);

        if (value)
        {
            gtk_dialog_set_response_sensitive(GTK_DIALOG(GTK_WIDGET(dialog)),
                                              GTK_RESPONSE_ACCEPT, TRUE);
            return TRUE;
        }
    }

    return FALSE;
}

/** get data fom selected row. */
static void loc_tree_get_selection(GtkWidget * view,
                                   gchar ** loc,
                                   gfloat * lat, gfloat * lon, guint * alt,
                                   gchar ** wx)
{
    GtkTreeSelection *selection;
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    GtkTreeIter     parent;
    gchar          *city;
    gchar          *country;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        /* get values */
        gtk_tree_model_get(model, &iter,
                           TREE_COL_NAM, &city,
                           TREE_COL_LAT, lat,
                           TREE_COL_LON, lon,
                           TREE_COL_ALT, alt, TREE_COL_WX, wx, -1);

        /* Location string shall be composed of "City, Country".
           Currently we have City in _loc1 and so we need to obtain
           the parent. */
        if (gtk_tree_model_iter_parent(model, &parent, &iter))
        {
            gtk_tree_model_get(model, &parent, TREE_COL_NAM, &country, -1);

            *loc = g_strconcat(city, ", ", country, NULL);
            g_free(city);
            g_free(country);
        }
        else
        {
            /* well no luck; send a warning message and return city
               only (actually, this is a bug, if it happens).
             */
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Failed to get parent for %s."), __func__, city);

            *loc = g_strdup(city);
            g_free(city);
        }
    }
    else
    {
        /* nothing selected; this function should not have been called
           => BUG!
         */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: No selection found!"), __func__);
    }
}
