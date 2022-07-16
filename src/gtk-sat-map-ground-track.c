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
/**
 * Implementation of the satellite ground tracks.
 *
 * @note The ground track functions should only be called from gtk-sat-map.c
 *       and gtk-sat-map-popup.c.
 *
 */
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "config-keys.h"
#include "gtk-sat-map.h"
#include "gtk-sat-map-ground-track.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"


static void     create_polylines(GtkSatMap * satmap, sat_t * sat, qth_t * qth,
                                 sat_map_obj_t * obj);
static gboolean ssp_wrap_detected(GtkSatMap * satmap, gdouble x1, gdouble x2);
static void     free_ssp(gpointer ssp, gpointer data);


/**
 * Create and show ground track for a satellite.
 *
 * @param satmap The satellite map widget.
 * @param sat Pointer to the satellite object.
 * @param qth Pointer to the QTH data.
 * @param obj the satellite object.
 *  
 * Gpredict allows the user to require the ground track for any number of orbits
 * ahead. Therefore, the resulting ground track may cross the map boundaries many
 * times, and using one single polyline for the whole ground track would look very
 * silly. To avoid this, the points will be split into several polylines.
 */
void ground_track_create(GtkSatMap * satmap, sat_t * sat, qth_t * qth,
                         sat_map_obj_t * obj)
{
    long            this_orbit; /* current orbit number */
    long            max_orbit;  /* target orbit number, ie. this + num - 1 */
    double          t0;         /* time when this_orbit starts */
    double          t;
    ssp_t          *this_ssp;

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Creating ground track for %s"),
                __func__, sat->nickname);

    /* just to be safe... if empty GSList is not NULL => segfault */
    obj->track_data.latlon = NULL;

    /* get configuration parameters */
    this_orbit = sat->orbit;
    max_orbit = sat->orbit - 1 + mod_cfg_get_int(satmap->cfgdata,
                                                 MOD_CFG_MAP_SECTION,
                                                 MOD_CFG_MAP_TRACK_NUM,
                                                 SAT_CFG_INT_MAP_TRACK_NUM);

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Start orbit: %d"), __func__, this_orbit);
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: End orbit %d"), __func__, max_orbit);

    /* find the time when the current orbit started */

    /* Iterate backwards in time until we reach sat->orbit < this_orbit.
       Use predict_calc from predict-tools.c as SGP/SDP driver.
       As a built-in safety, we stop iteration if the orbit crossing is
       more than 24 hours back in time.
     */
    t0 = satmap->tstamp;        //get_current_daynum ();
    /* use == instead of >= as it is more robust */
    for (t = t0; (sat->orbit == this_orbit) && ((t + 1.0) > t0); t -= 0.0007)
        predict_calc(sat, qth, t);

    /* set it so that we are in the same orbit as this_orbit
       and not a different one */
    t += 2 * 0.0007;
    t0 = t;
    predict_calc(sat, qth, t0);

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: T0: %f (%d)"), __func__, t0, sat->orbit);

    /* calculate (lat,lon) for the required orbits */
    while ((sat->orbit <= max_orbit) &&
           (sat->orbit >= this_orbit) && (!decayed(sat)))
    {
        /* We use 30 sec time steps. If resolution is too fine, the
           line drawing routine will filter out unnecessary points
         */
        t += 0.00035;
        predict_calc(sat, qth, t);

        /* store this SSP */

        /* Note: g_slist_append() has to traverse the entire list to find the end, which
           is inefficient when adding multiple elements. Therefore, we use g_slist_prepend()
           and reverse the entire list when we are done.
         */
        this_ssp = g_try_new(ssp_t, 1);

        if (this_ssp == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: MAYDAY: Insufficient memory for ground track!"),
                        __func__);
            return;
        }

        this_ssp->lat = sat->ssplat;
        this_ssp->lon = sat->ssplon;
        obj->track_data.latlon =
            g_slist_prepend(obj->track_data.latlon, this_ssp);

    }
    /* log if there is a problem with the orbit calculation */
    if (sat->orbit != (max_orbit + 1))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Problem computing ground track for %s"),
                    __func__, sat->nickname);
        return;
    }

    /* Reset satellite structure to eliminate glitches in single sat 
       view and other places when new ground track is laid out */
    predict_calc(sat, qth, satmap->tstamp);

    /* reverse GSList */
    obj->track_data.latlon = g_slist_reverse(obj->track_data.latlon);

    /* split points into polylines */
    create_polylines(satmap, sat, qth, obj);

    /* misc book-keeping */
    obj->track_orbit = this_orbit;
}

/**
 * Update the ground track for a satellite.
 *
 * @param satmap The satellite map widget.
 * @param sat Pointer to the satellite object.
 * @param qth Pointer to the QTH data.
 * @param obj the satellite object.
 * @param recalc Flag indicating whether ground track should be recalculated.
 *
 *    If (recalc=TRUE)
 *       call ground_track_delete (clear_ssp=TRUE)
 *       call ground_track_create
 *    Else
 *       call ground_track_delete (clear_ssp=FALSE)
 *       call create_polylines
 *
 *
 * The purpose with the recalc flag is to allow updates of ground track look without having
 * to recalculate the whole ground track (recalc=FALSE). 
 */
void ground_track_update(GtkSatMap * satmap, sat_t * sat, qth_t * qth,
                         sat_map_obj_t * obj, gboolean recalc)
{
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Updating ground track for %s"),
                __func__, sat->nickname);

    if (decayed(sat))
    {
        ground_track_delete(satmap, sat, qth, obj, TRUE);
        return;
    }

    if (recalc == TRUE)
    {
        ground_track_delete(satmap, sat, qth, obj, TRUE);
        ground_track_create(satmap, sat, qth, obj);
    }
    else
    {
        ground_track_delete(satmap, sat, qth, obj, FALSE);
        create_polylines(satmap, sat, qth, obj);
    }
}

/**
 * Delete the ground track for a satellite.
 *
 * @param satmap The satellite map widget.
 * @param sat Pointer to the satellite object.
 * @param qth Pointer to the QTH data.
 * @param obj the satellite object.
 * @param clear_ssp Flag indicating whether SSP data should be cleared as well (TRUE=yes);
 */
void ground_track_delete(GtkSatMap * satmap, sat_t * sat, qth_t * qth,
                         sat_map_obj_t * obj, gboolean clear_ssp)
{
    guint           i, n;
    gint            j;
    GooCanvasItemModel *line;
    GooCanvasItemModel *root;

    (void)qth;

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Deleting ground track for %s"),
                __func__, sat->nickname);

    root = goo_canvas_get_root_item_model(GOO_CANVAS(satmap->canvas));

    /* remove plylines */
    if (obj->track_data.lines != NULL)
    {
        n = g_slist_length(obj->track_data.lines);

        for (i = 0; i < n; i++)
        {
            /* get line */
            line =
                GOO_CANVAS_ITEM_MODEL(g_slist_nth_data
                                      (obj->track_data.lines, i));

            /* find its ID and remove it */
            j = goo_canvas_item_model_find_child(root, line);
            if (j == -1)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: Could not find part %d of ground track"),
                            __func__, j);
            }
            else
            {
                goo_canvas_item_model_remove_child(root, j);
            }
        }

        g_slist_free(obj->track_data.lines);
        obj->track_data.lines = NULL;
    }

    /* clear SSP too? */
    if (clear_ssp == TRUE)
    {
        if (obj->track_data.latlon != NULL)
        {
            /* free allocated ssp_t */
            g_slist_foreach(obj->track_data.latlon, free_ssp, NULL);

            /* free the SList itself */
            g_slist_free(obj->track_data.latlon);
            obj->track_data.latlon = NULL;

        }

        obj->track_orbit = 0;
    }
}

/**
 * Free an ssp_t structure.
 *
 * The ssp_t items in the obj->track_data.latlon GSList are dunamically allocated
 * hence they need to be freed when the cround track is deleted. This function
 * is intended to be called from a g_slist_foreach() iterator.
 */
static void free_ssp(gpointer ssp, gpointer data)
{
    (void)data;
    g_free(ssp);
}

/** Create polylines. */
static void create_polylines(GtkSatMap * satmap, sat_t * sat, qth_t * qth,
                             sat_map_obj_t * obj)
{
    ssp_t          *ssp, *buff; /* map coordinates */
    double          lastx, lasty;
    GSList         *points = NULL;
    GooCanvasItemModel *root;
    GooCanvasItemModel *line;
    GooCanvasPoints *gpoints;
    guint           start;
    guint           i, j, n, num_points;
    guint32         col;

    (void)sat;
    (void)qth;

    /* initialise parameters */
    lastx = -50.0;
    lasty = -50.0;
    start = 0;
    num_points = 0;
    n = g_slist_length(obj->track_data.latlon);
    col = mod_cfg_get_int(satmap->cfgdata,
                          MOD_CFG_MAP_SECTION,
                          MOD_CFG_MAP_TRACK_COL, SAT_CFG_INT_MAP_TRACK_COL);

    /* loop over each SSP */
    for (i = 0; i < n; i++)
    {
        buff = (ssp_t *) g_slist_nth_data(obj->track_data.latlon, i);
        ssp = g_try_new(ssp_t, 1);
        gtk_sat_map_lonlat_to_xy(satmap, buff->lon, buff->lat, &ssp->lon,
                                 &ssp->lat);

        /* if this is the first point, just add it to the list */
        if (i == start)
        {
            points = g_slist_prepend(points, ssp);
            lastx = ssp->lon;
            lasty = ssp->lat;
        }
        else
        {
            /* if SSP is on the other side of the map */
            if (ssp_wrap_detected(satmap, lastx, ssp->lon))
            {
                points = g_slist_reverse(points);
                num_points = g_slist_length(points);

                /* we need at least 2 points to draw a line */
                if (num_points > 1)
                {
                    /* convert SSPs to GooCanvasPoints */
                    gpoints = goo_canvas_points_new(num_points);
                    for (j = 0; j < num_points; j++)
                    {
                        buff = (ssp_t *) g_slist_nth_data(points, j);
                        gpoints->coords[2 * j] = buff->lon;
                        gpoints->coords[2 * j + 1] = buff->lat;
                    }

                    /* create a new polyline using the current set of points */
                    root =
                        goo_canvas_get_root_item_model(GOO_CANVAS
                                                       (satmap->canvas));

                    line = goo_canvas_polyline_model_new(root, FALSE, 0,
                                                         "points", gpoints,
                                                         "line-width", 1.0,
                                                         "stroke-color-rgba",
                                                         col, "line-cap",
                                                         CAIRO_LINE_CAP_SQUARE,
                                                         "line-join",
                                                         CAIRO_LINE_JOIN_MITER,
                                                         NULL);
                    goo_canvas_points_unref(gpoints);
                    goo_canvas_item_model_lower(line, obj->marker);

                    /* store line in sat object */
                    obj->track_data.lines =
                        g_slist_append(obj->track_data.lines, line);

                }

                /* reset parameters and continue with a new set */
                g_slist_foreach(points, free_ssp, NULL);
                g_slist_free(points);
                points = NULL;
                start = i;
                lastx = ssp->lon;
                lasty = ssp->lat;
                num_points = 0;

                /* Add current SSP to the new list */
                points = g_slist_prepend(points, ssp);
                lastx = ssp->lon;
                lasty = ssp->lat;
            }

            /* else if this SSP is separable from the previous */
            else if ((fabs(lastx - ssp->lon) > 1.0) ||
                     (fabs(lasty - ssp->lat) > 1.0))
            {

                /* add SSP to list */
                points = g_slist_prepend(points, ssp);
                lastx = ssp->lon;
                lasty = ssp->lon;
            }
            /* else do nothing */
            else g_free(ssp);
        }
    }

    /* create (last) line if we have at least two points */
    points = g_slist_reverse(points);
    num_points = g_slist_length(points);

    if (num_points > 1)
    {
        /* convert SSPs to GooCanvasPoints */
        gpoints = goo_canvas_points_new(num_points);
        for (j = 0; j < num_points; j++)
        {
            buff = (ssp_t *) g_slist_nth_data(points, j);
            gpoints->coords[2 * j] = buff->lon;
            gpoints->coords[2 * j + 1] = buff->lat;
        }

        /* create a new polyline using the current set of points */
        root = goo_canvas_get_root_item_model(GOO_CANVAS(satmap->canvas));

        line = goo_canvas_polyline_model_new(root, FALSE, 0,
                                             "points", gpoints,
                                             "line-width", 1.0,
                                             "stroke-color-rgba", col,
                                             "line-cap", CAIRO_LINE_CAP_SQUARE,
                                             "line-join",
                                             CAIRO_LINE_JOIN_MITER, NULL);
        goo_canvas_points_unref(gpoints);
        goo_canvas_item_model_lower(line, obj->marker);

        /* store line in sat object */
        obj->track_data.lines = g_slist_append(obj->track_data.lines, line);

        /* reset parameters and continue with a new set */
        g_slist_foreach(points, free_ssp, NULL);
        g_slist_free(points);
    }
}

/** Check whether ground track wraps around map borders */
static gboolean ssp_wrap_detected(GtkSatMap * satmap, gdouble x1, gdouble x2)
{
    gboolean        retval = FALSE;

    if (fabs(x1 - x2) > satmap->width / 2.0)
        retval = TRUE;

    return retval;
}
