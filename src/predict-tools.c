/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.
    Parts are Copyright John A. Magliacane, KD2BD 1991-2003 (indicated below)

    Authors: Alexandru Csete <oz9aec@gmail.com>
             John A. Magliacane, KD2BD.
             Charles Suprin <hamaa1vs@gmail.com>
	     Daniel Estevez <daniel@destevez.net>

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

#include <glib.h>
#include <glib/gi18n.h>

#include "gtk-sat-data.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

static pass_t  *get_pass_engine(sat_t * sat_in, qth_t * qth, gdouble start,
                                gdouble maxdt, gdouble min_el);

/**
 * \brief SGP4SDP4 driver for doing AOS/LOS calculations.
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the QTH data.
 * \param t The time for calculation (Julian Date)
 */
void predict_calc(sat_t * sat, qth_t * qth, gdouble t)
{
    obs_set_t       obs_set;
    geodetic_t      sat_geodetic;
    geodetic_t      obs_geodetic;
    double          age;

    obs_geodetic.lon = qth->lon * de2ra;
    obs_geodetic.lat = qth->lat * de2ra;
    obs_geodetic.alt = qth->alt / 1000.0;
    obs_geodetic.theta = 0;

    sat->jul_utc = t;
    sat->tsince = (sat->jul_utc - sat->jul_epoch) * xmnpda;

    /* call the norad routines according to the deep-space flag */
    if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
        SDP4(sat, sat->tsince);
    else
        SGP4(sat, sat->tsince);

    Convert_Sat_State(&sat->pos, &sat->vel);

    /* get the velocity of the satellite */
    Magnitude(&sat->vel);
    sat->velo = sat->vel.w;
    Calculate_Obs(sat->jul_utc, &sat->pos, &sat->vel, &obs_geodetic, &obs_set);
    Calculate_LatLonAlt(sat->jul_utc, &sat->pos, &sat_geodetic);

    while (sat_geodetic.lon < -pi)
        sat_geodetic.lon += twopi;

    while (sat_geodetic.lon > (pi))
        sat_geodetic.lon -= twopi;

    sat->az = Degrees(obs_set.az);
    sat->el = Degrees(obs_set.el);
    sat->range = obs_set.range;
    sat->range_rate = obs_set.range_rate;
    sat->ssplat = Degrees(sat_geodetic.lat);
    sat->ssplon = Degrees(sat_geodetic.lon);
    sat->alt = sat_geodetic.alt;
    sat->ma = Degrees(sat->phase);
    sat->ma *= 256.0 / 360.0;
    sat->phase = Degrees(sat->phase);

    /* same formulas, but the one from predict is nicer */
    //sat->footprint = 2.0 * xkmper * acos (xkmper/sat->pos.w);
    sat->footprint = 12756.33 * acos(xkmper / (xkmper + sat->alt));
    age = sat->jul_utc - sat->jul_epoch;
    sat->orbit = (long)floor((sat->tle.xno * xmnpda / twopi +
                              age * sat->tle.bstar * ae) * age +
                             (sat->tle.xmo + sat->tle.omegao) / twopi) + sat->tle.revnum ;
}

/**
 * \brief Find the AOS time of the next pass.
 * \author Alexandru Csete, OZ9AEC
 * \author John A. Magliacane, KD2BD
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the QTH data.
 * \param start The time where calculation should start.
 * \param maxdt The upper time limit in days (0.0 = no limit)
 * \return The time of the next AOS or 0.0 if the satellite has no AOS.
 *
 * This function finds the time of AOS for the first coming pass taking place
 * no earlier that start.
 * If the satellite is currently within range, the function first calls
 * find_los to get the next LOS time. Then the calculations are done using
 * the new start time.
 */
gdouble find_aos(sat_t * sat, qth_t * qth, gdouble start, gdouble maxdt)
{
    gdouble         t = start;
    gdouble         aostime = 0.0;

    /* make sure current sat values are in sync with the time */
    predict_calc(sat, qth, start);

    /* check whether satellite has aos */
    if (!has_aos(sat, qth))
        return 0.0;

    if (sat->el > 0.0)
        t = find_los(sat, qth, start, maxdt) + 0.014;   // +20 min

    /* invalid time (potentially returned by find_los) */
    if (t < 0.1)
        return 0.0;

    /* update satellite data */
    predict_calc(sat, qth, t);

    /* use upper time limit */
    if (maxdt > 0.0)
    {

        /* coarse time steps */
        while ((sat->el < -1.0) && (t <= (start + maxdt)))
        {
            t -= 0.00035 * (sat->el * ((sat->alt / 8400.0) + 0.46) - 2.0);
            predict_calc(sat, qth, t);
        }

        /* fine steps */
        while ((aostime == 0.0) && (t <= (start + maxdt)))
        {

            if (fabs(sat->el) < 0.005)
            {
                aostime = t;
            }
            else
            {
                t -= sat->el * sqrt(sat->alt) / 530000.0;
                predict_calc(sat, qth, t);
            }

        }

    }
    /* don't use upper time limit */
    else
    {
        /* coarse time steps */
        while (sat->el < -1.0)
        {
            t -= 0.00035 * (sat->el * ((sat->alt / 8400.0) + 0.46) - 2.0);
            predict_calc(sat, qth, t);
        }

        /* fine steps */
        while (aostime == 0.0)
        {

            if (fabs(sat->el) < 0.005)
            {
                aostime = t;
            }
            else
            {
                t -= sat->el * sqrt(sat->alt) / 530000.0;
                predict_calc(sat, qth, t);
            }
        }
    }

    return aostime;
}

/**
 * \brief Find the LOS time of the next pass.
 * \author Alexandru Csete, OZ9AEC
 * \author John A. Magliacane, KD2BD
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the QTH data.
 * \param start The time where calculation should start.
 * \param maxdt The upper time limit in days (0.0 = no limit)
 * \return The time of the next LOS or 0.0 if the satellite has no LOS.
 *
 * This function finds the time of LOS for the first coming pass taking place
 * no earlier that start.
 * If the satellite is currently out of range, the function first calls
 * find_aos to get the next AOS time. Then the calculations are done using
 * the new start time.
 * The function has a built-in watchdog to ensure that we don't end up in
 * lengthy loops.
 */
gdouble find_los(sat_t * sat, qth_t * qth, gdouble start, gdouble maxdt)
{
    gdouble         t = start;
    gdouble         lostime = 0.0;
    gdouble         eltemp;

    predict_calc(sat, qth, start);

    /* check whether satellite has aos */
    if (!has_aos(sat, qth))
    {
        return 0.0;
    }

    if (sat->el < 0.0)
        t = find_aos(sat, qth, start, maxdt) + 0.001;   // +1.5 min

    /* invalid time (potentially returned by find_aos) */
    if (t < 0.01)
        return 0.0;

    /* update satellite data */
    predict_calc(sat, qth, t);

    /* use upper time limit */
    if (maxdt > 0.0)
    {
        /* coarse steps */
        while ((sat->el >= 1.0) && (t <= (start + maxdt)))
        {
            t += cos((sat->el - 1.0) * de2ra) * sqrt(sat->alt) / 25000.0;
            predict_calc(sat, qth, t);
        }

        /* fine steps */
        while ((lostime == 0.0) && (t <= (start + maxdt)))
        {
            t += sat->el * sqrt(sat->alt) / 502500.0;
            predict_calc(sat, qth, t);

            if (fabs(sat->el) < 0.005)
            {
                /* Two things are true at LOS time, the elevation is a zero and
                   sat is descending. This checks that those two are true. */
                eltemp = sat->el;

                /* check elevation 1 second earlier */
                predict_calc(sat, qth, t - 1.0 / 86400.0);

                if (sat->el > eltemp)
                    lostime = t;
            }
        }
    }
    /* don't use upper limit */
    else
    {
        /* coarse steps */
        while (sat->el >= 1.0)
        {
            t += cos((sat->el - 1.0) * de2ra) * sqrt(sat->alt) / 25000.0;
            predict_calc(sat, qth, t);
        }

        /* fine steps */
        while (lostime == 0.0)
        {
            t += sat->el * sqrt(sat->alt) / 502500.0;
            predict_calc(sat, qth, t);

            if (fabs(sat->el) < 0.005)
            {
                /* two things are true at LOS time
                   The elevation is a zero and descending.
                   This checks that those two are true.
                 */
                eltemp = sat->el;

                /*check elevation 1 second earlier */
                predict_calc(sat, qth, t - 1.0 / 86400.0);

                if (sat->el > eltemp)
                    lostime = t;
            }
        }
    }

    return lostime;
}

/**
 * \brief Find AOS time of current pass.
 * \param sat The satellite to find AOS for.
 * \param qth The ground station.
 * \param start Start time, prefereably now.
 * \return The time of the previous AOS or 0.0 if the satellite has no AOS.
 *
 * This function can be used to find the AOS time in the past of the
 * current pass.
 */
gdouble find_prev_aos(sat_t * sat, qth_t * qth, gdouble start)
{
    gdouble         aostime = start;

    /* make sure current sat values are in sync with the time */
    predict_calc(sat, qth, start);

    /* check whether satellite has aos */
    if (!has_aos(sat, qth))
    {
        return 0.0;
    }

    while (sat->el >= 0.0)
    {
        aostime -= 0.0005;      // 0.75 min
        predict_calc(sat, qth, aostime);
    }

    return aostime;
}

/**
 * \brief Predict the next pass.
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the observer data.
 * \param maxdt The maximum number of days to look ahead.
 * \return Pointer newly allocated pass_t structure that should be freed
 *         with free_pass when no longer needed, or NULL if no pass can be
 *         found.
 *
 * This function simply wraps the get_pass function using the current time
 * as parameter.
 *
 * \note the data in sat will be corrupt (future) and must be refreshed
 *       by the caller, if the caller will need it later on (eg. if the caller
 *       is GtkSatList).
 */
pass_t         *get_next_pass(sat_t * sat, qth_t * qth, gdouble maxdt)
{
    gdouble         now;

    /* get the current time and call the get_pass function */
    now = get_current_daynum();

    return get_pass(sat, qth, now, maxdt);
}

/**
 * \brief Predict upcoming passes starting now
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the observer data.
 * \param maxdt The maximum number of days to look ahead.
 * \param num The number of passes to predict.
 * \return A singly linked list of pass_t structures or NULL if
 *         there was an error.
 *
 * This function simply wraps the get_passes function using the
 * current time as parameter.
 *
 * \note the data in sat will be corrupt (future) and must be refreshed
 *       by the caller, if the caller will need it later on (eg. if the caller
 *       is GtkSatList).
 */
GSList         *get_next_passes(sat_t * sat, qth_t * qth, gdouble maxdt,
                                guint num)
{
    gdouble         now;

    /* get the current time and call the get_pass function */
    now = get_current_daynum();

    return get_passes(sat, qth, now, maxdt, num);
}

/**
 * \brief Predict first pass after a certain time.
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the location data.
 * \param start Starting time.
 * \param maxdt The maximum number of days to look ahead (0 for no limit).
 * \return Pointer to a newly allocated pass_t structure or NULL if
 *         there was an error.
 * 
 *   This function assumes that you want a pass that achieves the 
 *   minimum elevation of is configured for.
 */
pass_t *get_pass(sat_t * sat_in, qth_t * qth, gdouble start, gdouble maxdt)
{
    int      min_ele = sat_cfg_get_int(SAT_CFG_INT_PRED_MIN_EL);

    if (min_ele == 0)
        min_ele = 1;

    return get_pass_engine(sat_in, qth, start, maxdt, min_ele);
}

/**
 * \brief Predict first pass after a certain time ignoring the min elevation.
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the location data.
 * \param start Starting time.
 * \param maxdt The maximum number of days to look ahead (0 for no limit).
 * \return Pointer to a newly allocated pass_t structure or NULL if
 *         there was an error.
 * This function assumes that you want a pass that achieves the 
 * minimum elevation of is configured for.
 */
pass_t         *get_pass_no_min_el(sat_t * sat_in, qth_t * qth, gdouble start,
                                   gdouble maxdt)
{
    return get_pass_engine(sat_in, qth, start, maxdt, 0.0);
}

/**
 * \brief Predict first pass after a certain time.
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the location data.
 * \param start Starting time.
 * \param maxdt The maximum number of days to look ahead (0 for no limit).
 * \return Pointer to a newly allocated pass_t structure or NULL if
 *         there was an error.
 *
 * This function will find the first upcoming pass with AOS no earlier than
 * t = start and no later than t = (start+maxdt).
 *
 * \note For no time limit use maxdt = 0.0
 *
 * \note the data in sat will be corrupt (future) and must be refreshed
 *       by the caller, if the caller will need it later on (eg. if the caller
 *       is GtkSatList).
 *
 * \note Prepending to a singly linked list is much faster than appending.
 *       Therefore, the elements are prepended whereafter the GSList is
 *       reversed
 */
static pass_t  *get_pass_engine(sat_t * sat_in, qth_t * qth, gdouble start,
                                gdouble maxdt, gdouble min_el)
{
    gdouble         aos = 0.0;  /* time of AOS */
    gdouble         tca = 0.0;  /* time of TCA */
    gdouble         los = 0.0;  /* time of LOS */
    gdouble         dt = 0.0;   /* time diff */
    gdouble         step = 0.0; /* time step */
    gdouble         t0 = start;
    gdouble         t;          /* current time counter */
    gdouble         tres = 0.0; /* required time resolution */
    gdouble         max_el = 0.0;       /* maximum elevation */
    pass_t         *pass = NULL;
    pass_detail_t  *detail = NULL;
    gboolean        done = FALSE;
    guint           iter = 0;   /* number of iterations */
    sat_t          *sat, sat_working;

    /* FIXME: watchdog */

    /*copy sat_in to a working structure */
    sat = memcpy(&sat_working, sat_in, sizeof(sat_t));

    /* get time resolution; sat-cfg stores it in seconds */
    tres = sat_cfg_get_int(SAT_CFG_INT_PRED_RESOLUTION) / 86400.0;

    /* loop until we find a pass with elevation > SAT_CFG_INT_PRED_MIN_EL
       or we run out of time
       FIXME: we should have a safety break
     */
    while (!done)
    {
        /* Find los of next pass or of current pass */
        los = find_los(sat, qth, t0, maxdt);    // See if a pass is ongoing
        aos = find_aos(sat, qth, t0, start + maxdt - t0);

        if (aos > los)
            // los is from an currently happening pass, find previous aos
            aos = find_prev_aos(sat, qth, t0);

        /* aos = 0.0 means no aos */
        if (aos == 0.0)
            done = TRUE;

        /* check whether we are within time limits;
           maxdt = 0 mean no time limit.
         */
        else if ((maxdt > 0.0) && (aos > (start + maxdt)))
        {
            done = TRUE;
        }
        else
        {
            //los = find_los (sat, qth, aos + 0.001, maxdt); // +1.5 min later
            dt = los - aos;

            /* get time step, which will give us the max number of entries */
            step = dt / sat_cfg_get_int(SAT_CFG_INT_PRED_NUM_ENTRIES);

            /* but if this is smaller than the required resolution
               we go with the resolution
             */
            if (step < tres)
                step = tres;

            /* create a pass_t entry; FIXME: g_try_new in 2.8 */
            pass = g_new(pass_t, 1);

            pass->aos = aos;
            pass->los = los;
            pass->max_el = 0.0;
            pass->aos_az = 0.0;
            pass->los_az = 0.0;
            pass->maxel_az = 0.0;
            pass->vis[0] = '-';
            pass->vis[1] = '-';
            pass->vis[2] = '-';
            pass->vis[3] = 0;
            pass->satname = g_strdup(sat->nickname);
            pass->details = NULL;
            /*copy qth data into the pass for later comparisons */
            qth_small_save(qth, &(pass->qth_comp));

            /* iterate over each time step */
            for (t = pass->aos; t <= pass->los; t += step)
            {

                /* calculate satellite data */
                predict_calc(sat, qth, t);

                /* in the first iter we want to store
                   pass->aos_az
                 */
                if (t == pass->aos)
                {
                    pass->aos_az = sat->az;
                    pass->orbit = sat->orbit;
                }

                /* append details to sat->details */
                detail = g_new(pass_detail_t, 1);
                detail->time = t;
                detail->pos.x = sat->pos.x;
                detail->pos.y = sat->pos.y;
                detail->pos.z = sat->pos.z;
                detail->pos.w = sat->pos.w;
                detail->vel.x = sat->vel.x;
                detail->vel.y = sat->vel.y;
                detail->vel.z = sat->vel.z;
                detail->vel.w = sat->vel.w;
                detail->velo = sat->velo;
                detail->az = sat->az;
                detail->el = sat->el;
                detail->range = sat->range;
                detail->range_rate = sat->range_rate;
                detail->lat = sat->ssplat;
                detail->lon = sat->ssplon;
                detail->alt = sat->alt;
                detail->ma = sat->ma;
                detail->phase = sat->phase;
                detail->footprint = sat->footprint;
                detail->orbit = sat->orbit;
                detail->vis = get_sat_vis(sat, qth, t);

                /* also store visibility "bit" */
                switch (detail->vis)
                {
                case SAT_VIS_VISIBLE:
                    pass->vis[0] = 'V';
                    break;
                case SAT_VIS_DAYLIGHT:
                    pass->vis[1] = 'D';
                    break;
                case SAT_VIS_ECLIPSED:
                    pass->vis[2] = 'E';
                    break;
                default:
                    break;
                }

                pass->details = g_slist_prepend(pass->details, detail);

                /* store elevation if greater than the
                   previously stored one
                 */
                if (sat->el > max_el)
                {
                    max_el = sat->el;
                    tca = t;
                    pass->maxel_az = sat->az;
                }

                /*     g_print ("TIME: %f\tAZ: %f\tEL: %f (MAX: %f)\n", */
                /*           t, sat->az, sat->el, max_el); */
            }

            pass->details = g_slist_reverse(pass->details);

            /* calculate satellite data */
            predict_calc(sat, qth, pass->los);
            /* store los_az, max_el and tca */
            pass->los_az = sat->az;
            pass->max_el = max_el;
            pass->tca = tca;

            /* check whether this pass is good */
            if (max_el >= min_el)
            {
                done = TRUE;
            }
            else
            {
                done = FALSE;
                t0 = los + 0.014;       // +20 min
                free_pass(pass);
                pass = NULL;
            }

            iter++;
        }
    }

    return pass;
}

/**
 * Predict passes after a certain time.
 *
 * This function calculates num upcoming passes with AOS no earlier
 * than t = start and not later that t = (start+maxdt). The function will
 * repeatedly call get_pass until the number of predicted passes is equal to
 * num, the time has reached limit or the get_pass function returns NULL.
 *
 * \note For no time limit use maxdt = 0.0
 *
 * \note the data in sat will be corrupt (future) and must be refreshed
 *       by the caller, if the caller will need it later on (eg. if the caller
 *       is GtkSatList).
 *
 * \note Prepending to a singly linked list is much faster than appending.
 *       Therefore, the elements are prepended whereafter the GSList is
 *       reversed
 */
GSList         *get_passes(sat_t * sat, qth_t * qth, gdouble start,
                           gdouble maxdt, guint num)
{
    GSList         *passes = NULL;
    pass_t         *pass = NULL;
    guint           i;
    gdouble         t;

    /* if no number has been specified
       set it to something big */
    if (num == 0)
        num = 100;

    t = start;

    for (i = 0; i < num; i++)
    {
        pass = get_pass(sat, qth, t, maxdt);

        if (pass != NULL)
        {
            passes = g_slist_prepend(passes, pass);
            t = pass->los + 0.014;      // +20 min

            /* if maxdt > 0.0 check whether we have reached t = start+maxdt
               if yes finish predictions
             */
            if ((maxdt > 0.0) && (t >= (start + maxdt)))
            {
                i = num;
            }
        }
        else
        {
            /* we can't get any more passes */
            i = num;
        }

    }

    if (passes != NULL)
        passes = g_slist_reverse(passes);

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Found %d passes for %s in time window [%f;%f]"),
                __func__, g_slist_length(passes), sat->nickname, start,
                start + maxdt);

    return passes;
}

pass_t         *copy_pass(pass_t * pass)
{
    pass_t         *new;

    new = g_try_new(pass_t, 1);

    if (new != NULL)
    {
        new->aos = pass->aos;
        new->los = pass->los;
        new->tca = pass->tca;
        new->max_el = pass->max_el;
        new->aos_az = pass->aos_az;
        new->los_az = pass->los_az;
        new->orbit = pass->orbit;
        new->maxel_az = pass->maxel_az;
        new->vis[0] = pass->vis[0];
        new->vis[1] = pass->vis[1];
        new->vis[2] = pass->vis[2];
        new->vis[3] = pass->vis[3];
        new->details = copy_pass_details(pass->details);

        if (pass->satname != NULL)
            new->satname = g_strdup(pass->satname);
        else
            new->satname = NULL;
    }

    return new;
}

GSList         *copy_pass_details(GSList * details)
{
    GSList         *new = NULL;
    guint           i, n;

    n = g_slist_length(details);
    for (i = 0; i < n; i++)
    {
        new = g_slist_prepend(new,
                              copy_pass_detail(PASS_DETAIL
                                               (g_slist_nth_data
                                                (details, i))));
    }

    new = g_slist_reverse(new);

    return new;
}

pass_detail_t  *copy_pass_detail(pass_detail_t * detail)
{
    pass_detail_t  *new;

    /* create a pass_t entry; FIXME: g_try_new in 2.8 */
    new = g_new(pass_detail_t, 1);

    new->time = detail->time;
    new->pos.x = detail->pos.x;
    new->pos.y = detail->pos.y;
    new->pos.z = detail->pos.z;
    new->pos.w = detail->pos.w;
    new->vel.x = detail->vel.x;
    new->vel.y = detail->vel.y;
    new->vel.z = detail->vel.z;
    new->vel.w = detail->vel.w;
    new->velo = detail->velo;
    new->az = detail->az;
    new->el = detail->el;
    new->range = detail->range;
    new->range_rate = detail->range_rate;
    new->lat = detail->lat;
    new->lon = detail->lon;
    new->alt = detail->alt;
    new->ma = detail->ma;
    new->phase = detail->phase;
    new->footprint = detail->footprint;
    new->orbit = detail->orbit;
    new->vis = detail->vis;

    return new;
}

void free_pass(pass_t * pass)
{
    if (pass != NULL)
    {
        free_pass_details(pass->details);

        if (pass->satname != NULL)
        {
            g_free(pass->satname);
            pass->satname = NULL;
        }

        g_free(pass);
        pass = NULL;
    }
}

/** \brief Free a list of passes. */
void free_passes(GSList * passes)
{
    guint           n, i;
    gpointer        pass;

    n = g_slist_length(passes);

    for (i = 0; i < n; i++)
    {
        pass = g_slist_nth_data(passes, i);

        /* free element data */
        free_pass(PASS(pass));
    }

    /* now free the list elements */
    g_slist_free(passes);
    passes = NULL;
}

/**
 * \brief Free a pass detail structure.
 *
 * This function is not rarely useful except for the
 * free_pass function.
 */
void free_pass_detail(pass_detail_t * detail)
{
    g_free(detail);
    detail = NULL;
}

/** Free the whole list of details. */
void free_pass_details(GSList * details)
{
    guint           n, i;
    gpointer        detail;

    n = g_slist_length(details);

    for (i = 0; i < n; i++)
    {

        detail = g_slist_nth_data(details, i);

        /* free element data */
        free_pass_detail(PASS_DETAIL(detail));
    }

    /* free list elements */
    g_slist_free(details);
    details = NULL;
}

/**
 * \brief Get current pass.
 * \param sat Pointer to the satellite data.
 * \param qth Pointer to the QTH data.
 * \param start Time to start calculations; use 0.0 for now.
 * \return Pointer to a newly allocated pass_t structure or NULL if
 *         there was an error.
 *
 * Assuming that sat->el > 0.0 this function calculates the details of the
 * current pass from AOS time to LOS time.
 * First the function goes back in time to before the AOS, then it calls
 * the get_pass_no_min_el function to get the details of the current pass
 * disregarding any minimum elevation requirements.
 *
 * \note The start parameter has been introduced to allow correct use of this
 *       function in non-realtime cases.
 */
pass_t         *get_current_pass(sat_t * sat_in, qth_t * qth, gdouble start)
{
    gdouble         t, t0;
    gdouble         el0;
    sat_t          *sat, sat_working;
    pass_t         *pass;

    /*copy sat_in to a working structure */
    sat = memcpy(&sat_working, sat_in, sizeof(sat_t));

    if (start > 0.0)
        t = start;
    else
        t = get_current_daynum();
    predict_calc(sat, qth, t);

    /*save initial conditions for later comparison */
    t0 = t;
    el0 = sat->el;

    /* check whether satellite has aos */
    if (!has_aos(sat, qth))
        return NULL;

    /* find a time before AOS */
    while (sat->el > 0.0)
    {
        predict_calc(sat, qth, t);
        t -= 0.007;             // +10 min
    }

    pass = get_pass_no_min_el(sat, qth, t, 0.0);
    if (el0 > 0.0)
    {
        /* this function is only specified if the elevation 
           is greater than zero at the time it is called */
        if (pass)
        {
            if (pass->aos > t0)
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _
                            ("%s: Returning a pass for %s that starts after the seeded time."),
                            __func__, sat->nickname);

            if (pass->los < t0)
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _
                            ("%s: Returning a pass for %s that ends before the seeded time."),
                            __func__, sat->nickname);
        }
    }

    return pass;
}
