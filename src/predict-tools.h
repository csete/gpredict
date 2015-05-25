/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>
    Charles Suprin <hamaa1vs@gmail.com>

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
#ifndef PREDICT_TOOLS_H
#define PREDICT_TOOLS_H 1

#include <glib.h>
#include "gtk-sat-data.h"
#include "sat-vis.h"
#include "sgpsdp/sgp4sdp4.h"


/** \brief Brief satellite pass info. */
typedef struct {
    gchar      *satname;  /*!< satellite name */
    gdouble     aos;      /*!< AOS time in "jul_utc" */
    gdouble     tca;      /*!< TCA time in "jul_utc" */
    gdouble     los;      /*!< LOS time in "jul_utc" */
    gdouble     max_el;   /*!< Maximum elevation during pass */
    gdouble     aos_az;   /*!< Azimuth at AOS */
    gdouble     los_az;   /*!< Azimuth at LOS */
    gint        orbit;    /*!< Orbit number */
    gdouble     maxel_az; /*!< Azimuth at maximum elevation */
    gchar       vis[4];   /*!< Visibility string, e.g. VSE, -S-, V-- */
    GSList     *details;  /*!< List of pass_detail_t entries */
    qth_small_t qth_comp; /*!< Short version of qth at time computed */
} pass_t;

/**
 * \brief Pass detail entry.
 *
 * In order to ensure maximum flexibility at a minimal effort, only the
 * raw position and velocity is calculated. Calculations of the
 * "human readable" parameters are the responsibility of the consumer.
 * This way we can use the same prediction engine for various consumers
 * without having too much overhead and complexity in the low level code.
 */
typedef struct {
    gdouble   time;   /*!< time in "jul_utc" */
    vector_t  pos;    /*!< Raw unprocessed position at time */
    vector_t  vel;    /*!< Raw unprocessed velocity at time */
    gdouble   velo;
    gdouble   az;
    gdouble   el;
    gdouble   range;
    gdouble   range_rate;
    gdouble   lat;
    gdouble   lon;
    gdouble   alt;
    gdouble   ma;
    gdouble   phase;
    gdouble   footprint;
    sat_vis_t vis;
    gint      orbit;
} pass_detail_t;

/* type casting macros */
#define PASS(x) ((pass_t *) x)
#define PASS_DETAIL(x) ((pass_detail_t *) x)

/* SGP4/SDP4 driver */
void predict_calc (sat_t *sat, qth_t *qth, gdouble t);

/* AOS/LOS time calculators */
gdouble find_aos           (sat_t *sat, qth_t *qth, gdouble start, gdouble maxdt);
gdouble find_los           (sat_t *sat, qth_t *qth, gdouble start, gdouble maxdt);
gdouble find_prev_aos      (sat_t *sat, qth_t *qth, gdouble start);

/* next events */
pass_t *get_next_pass      (sat_t *sat, qth_t *qth, gdouble maxdt);
GSList *get_next_passes    (sat_t *sat, qth_t *qth, gdouble maxdt, guint num);

/* future events */
pass_t *get_pass           (sat_t *sat, qth_t *qth, gdouble start, gdouble maxdt);
GSList *get_passes         (sat_t *sat, qth_t *qth, gdouble start, gdouble maxdt, guint num);
pass_t *get_current_pass   (sat_t *sat, qth_t *qth, gdouble start);
pass_t *get_pass_no_min_el (sat_t *sat, qth_t *qth, gdouble start, gdouble maxdt);

/* copying */
pass_t        *copy_pass         (pass_t *pass);
GSList        *copy_pass_details (GSList *details);
pass_detail_t *copy_pass_detail  (pass_detail_t *detail);

/* memory cleaning */
void free_pass         (pass_t *pass);
void free_passes       (GSList *passes);
void free_pass_detail  (pass_detail_t *detail);
void free_pass_details (GSList *details);

#endif
