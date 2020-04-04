/*
 * Gpredict: Real-time satellite tracking and orbit prediction program
 *
 * Copyright (C)  2001-2019  Alexandru Csete, OZ9AEC
 *                           Charles Suprin, AA1VS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, visit http://www.fsf.org/
*/
/**
 * Main module container.
 *
 * The GtkSatModule widget is the top level container that contains the
 * individual views. These views are of type GtkSatList, GtkSatMap, GtkSingleSat,
 * GtkPolarView and GtkEventList (as of version 1.2).
 *
 * The views are organized in a grid (GtkTable) where each view can occupy one
 * or more squares, see the create_module_layout() function.
 * 
 * A module can have a GtkRigCtrl, a GtkRotCtrl and a GtkSkyGlance widget
 * associated to it. These associations exist because theu share QTH and
 * satellite data.
 */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <sys/time.h>

#include "compat.h"
#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-event-list.h"
#include "gtk-polar-view.h"
#include "gtk-rig-ctrl.h"
#include "gtk-rot-ctrl.h"
#include "gtk-sat-list.h"
#include "gtk-sat-map.h"
#include "gtk-sat-module.h"
#include "gtk-sat-module-popup.h"
#include "gtk-sat-module-tmg.h"
#include "gtk-single-sat.h"
#include "gtk-sky-glance.h"
#include "mod-cfg.h"
#include "mod-cfg-get-param.h"
#include "mod-mgr.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"


static GtkVBoxClass *parent_class = NULL;

static void gtk_sat_module_free_sat(gpointer sat)
{
    gtk_sat_data_free_sat(SAT(sat));
}

static void update_autotrack(GtkSatModule * module)
{
    GList          *satlist = NULL;
    GList          *iter;
    sat_t          *sat = NULL;
    guint           i, n;
    double          next_aos;
    gint            next_sat;

    if (module->target > 0)
        sat = g_hash_table_lookup(module->satellites, &module->target);

    /* do nothing if current target is still above horizon */
    if (sat != NULL && sat->el > 0.0)
        return;

    /* set target to satellite with next AOS */
    satlist = g_hash_table_get_values(module->satellites);
    iter = satlist;
    n = g_list_length(satlist);
    if (n == 0)
        return;

    next_aos = module->tmgCdnum + 10.f; /* hope there is AOS within 10 days */
    next_sat = module->target;

    i = 0;
    while (i++ < n)
    {
        sat = (sat_t *) iter->data;

        /* if sat is above horizon, select it and we are done */
        if (sat->el > 0.0)
        {
            next_sat = sat->tle.catnr;
            break;
        }

        /* we have a candidate if AOS is in the future */
        if (sat->aos > module->tmgCdnum && sat->aos < next_aos)
        {
            next_aos = sat->aos;
            next_sat = sat->tle.catnr;
        }

        iter = iter->next;
    }

    if (next_sat != module->target)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("Autotrack: Changing target satellite %d -> %d"),
                    module->target, next_sat);
        gtk_sat_module_select_sat(module, next_sat);
    }

    g_list_free(satlist);
}

static void gtk_sat_module_destroy(GtkWidget * widget)
{
    GtkSatModule   *module = GTK_SAT_MODULE(widget);

    /*save the configuration */
    mod_cfg_save(module->name, module->cfgdata);

    /* stop timeout */
    if (module->timerid > 0)
    {
        g_source_remove(module->timerid);
        module->timerid = 0;
    }

    /* destroy time controller */
    if (module->tmgActive)
    {
        gtk_widget_destroy(module->tmgWin);
        module->tmgActive = FALSE;
    }

    /* destroy radio and rotator controllers */
    if (module->rigctrlwin)
    {
        gtk_widget_destroy(module->rigctrlwin);
    }
    if (module->rotctrlwin)
    {
        gtk_widget_destroy(module->rotctrlwin);
    }

    /* destroy sky at a glance window */
    if (module->skgwin)
    {
        gtk_widget_destroy(module->skgwin);
    }

    /* clean up QTH */
    if (module->qth)
    {
        qth_data_free(module->qth);
        module->qth = NULL;
    }

    /* clean up satellites */
    if (module->satellites)
    {
        g_hash_table_destroy(module->satellites);
        module->satellites = NULL;
    }

    if (module->grid)
    {
        g_free(module->grid);
        module->grid = NULL;
    }

    /* FIXME: free module->views? */

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_sat_module_class_init(GtkSatModuleClass * class)
{
    GtkWidgetClass    *widget_class;

    widget_class = (GtkWidgetClass *) class;
    widget_class->destroy = gtk_sat_module_destroy;
    parent_class = g_type_class_peek_parent(class);
}

/** Initialise GtkSatModule widget */
static void gtk_sat_module_init(GtkSatModule * module)
{
    /* initialise data structures */
    module->win = NULL;

    module->qth = g_try_new0(qth_t, 1);
    qth_init(module->qth);

    module->satellites = g_hash_table_new_full(g_int_hash, g_int_equal,
                                               g_free, gtk_sat_module_free_sat);

    module->rotctrlwin = NULL;
    module->rotctrl = NULL;
    module->rigctrlwin = NULL;
    module->rigctrl = NULL;
    module->skgwin = NULL;
    module->skg = NULL;
    module->lastSkgUpd = 0.0;

    module->state = GTK_SAT_MOD_STATE_DOCKED;

    g_mutex_init(&module->busy);

    /* open the gpsd device */
    module->gps_data = NULL;

    module->grid = NULL;
    module->views = NULL;
    module->nviews = 0;

    module->timerid = 0;

    module->throttle = 1;
    module->rtNow = 0.0;
    module->rtPrev = 0.0;
    module->tmgActive = FALSE;
    module->tmgPdnum = 0.0;
    module->tmgCdnum = 0.0;
    module->tmgReset = FALSE;

    module->target = -1;
    module->autotrack = FALSE;
}

GType gtk_sat_module_get_type()
{
    static GType    gtk_sat_module_type = 0;

    if (!gtk_sat_module_type)
    {
        static const GTypeInfo gtk_sat_module_info = {
            sizeof(GtkSatModuleClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_sat_module_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkSatModule),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_sat_module_init,
            NULL
        };

        gtk_sat_module_type = g_type_register_static(GTK_TYPE_BOX,
                                                     "GtkSatModule",
                                                     &gtk_sat_module_info, 0);
    }

    return gtk_sat_module_type;
}

/**
 * Create a new view.
 *
 * @param module Pointer to the parent GtkSatModule widget
 * @param num The number ID of the view to create, see gtk_sat_mod_view_t
 * @return Pointer to a new GtkWidget of type corresponding to num. If num
 *         is invalid, a GtkSatList is returned.
 */
static GtkWidget *create_view(GtkSatModule * module, guint num)
{
    GtkWidget      *view;

    switch (num)
    {
    case GTK_SAT_MOD_VIEW_LIST:
        view = gtk_sat_list_new(module->cfgdata,
                                module->satellites, module->qth, 0);
        break;

    case GTK_SAT_MOD_VIEW_MAP:
        view = gtk_sat_map_new(module->cfgdata,
                               module->satellites, module->qth);
        break;

    case GTK_SAT_MOD_VIEW_POLAR:
        view = gtk_polar_view_new(module->cfgdata,
                                  module->satellites, module->qth);
        break;

    case GTK_SAT_MOD_VIEW_SINGLE:
        view = gtk_single_sat_new(module->cfgdata,
                                  module->satellites, module->qth, 0);
        break;

    case GTK_SAT_MOD_VIEW_EVENT:
        view = gtk_event_list_new(module->cfgdata,
                                  module->satellites, module->qth, 0);
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Invalid child type (%d). Using GtkSatList."),
                    __FILE__, __LINE__, num);

        view = gtk_sat_list_new(module->cfgdata,
                                module->satellites, module->qth, 0);
        break;
    }

    return view;
}

/**
 * Calculate the layout grid size.
 *
 * @param module Pointer to the GtkSatModule widget.
 * @param rows Return value for number of rows
 * @param cols Return value for number of columns
 *
 * It is assumed that module->grid and module->nviews have chierent values.
 */
static void get_grid_size(GtkSatModule * module, guint * rows, guint * cols)
{
    guint           i;
    guint           xmax = 0;
    guint           ymax = 0;

    for (i = 0; i < module->nviews; i++)
    {
        xmax = MAX(xmax, module->grid[5 * i + 2]);
        ymax = MAX(ymax, module->grid[5 * i + 4]);
    }

    *cols = xmax;
    *rows = ymax;
}

/**
 * Create module layout and add views.
 *
 * It is assumed that module->grid and module->nviews have
 * coherent values.
 */
static void create_module_layout(GtkSatModule * module)
{
    GtkWidget      *table;
    GtkWidget      *view;
    guint           rows, cols;
    guint           i;

    /* calculate the number of rows and columns necessary */
    get_grid_size(module, &rows, &cols);
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Layout has %d columns and %d rows."),
                __func__, cols, rows);

    table = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(table), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(table), TRUE);

    for (i = 0; i < module->nviews; i++)
    {
        /* create the view */
        view = create_view(module, module->grid[5 * i]);

        /* store a pointer to the view */
        module->views = g_slist_append(module->views, view);

        /* add view to the grid */
        gtk_grid_attach(GTK_GRID(table), view,
                        module->grid[5 * i + 1],                            // left
                        module->grid[5 * i + 3],                            // top
                        module->grid[5 * i + 2] - module->grid[5 * i + 1],  // width
                        module->grid[5 * i + 4] - module->grid[5 * i + 3]); // height
    }

    gtk_box_pack_start(GTK_BOX(module), table, TRUE, TRUE, 0);
}


/**
 * Read satellites into memory.
 *
 * This function reads the list of satellites from the configfile and
 * and then adds each satellite to the hash table.
 */
static void gtk_sat_module_load_sats(GtkSatModule * module)
{
    gint           *sats = NULL;
    gsize           length;
    GError         *error = NULL;
    guint           i;
    sat_t          *sat;
    guint          *key = NULL;
    guint           succ = 0;

    /* get list of satellites from config file; abort in case of error */
    sats = g_key_file_get_integer_list(module->cfgdata,
                                       MOD_CFG_GLOBAL_SECTION,
                                       MOD_CFG_SATS_KEY, &length, &error);

    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to get list of satellites (%s)"),
                    __func__, error->message);

        g_clear_error(&error);

        /* GLib API says nothing about the contents in case of error */
        if (sats)
        {
            g_free(sats);
        }

        return;
    }

    /* read each satellite into hash table */
    for (i = 0; i < length; i++)
    {
        sat = g_new(sat_t, 1);

        if (gtk_sat_data_read_sat(sats[i], sat))
        {
            /* the satellite could not be read */
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error reading data for #%d"),
                        __func__, sats[i]);

            g_free(sat);
        }
        else
        {
            /* check whether satellite is already in list
               in order to avoid duplicates
             */
            key = g_new0(guint, 1);
            *key = sats[i];

            if (g_hash_table_lookup(module->satellites, key) == NULL)
            {
                gtk_sat_data_init_sat(sat, module->qth);
                g_hash_table_insert(module->satellites, key, sat);
                succ++;
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s: Read data for #%d"), __func__, sats[i]);
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_WARN,
                            _("%s: Sat #%d already in list"),
                            __func__, sats[i]);

                /* it is not needed in this case */
                g_free(sat);
            }

        }
    }

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Read %d out of %d satellites"), __func__, succ, length);

    g_free(sats);
}

/**
 * Update GtkSkyGlance view
 *
 * @param module Pointer to the GtkSatModule widget
 *
 * This function checks to see if the sky-at-a-glance display needs to be
 * updated due to time or qth moving. It checks how long ago the GtkSkyGlance
 * widget was updated and performs an update if necessary. The current timeout
 * is set to 60 sec. It also checks how far away the qth is from where the
 * GtkSkyGlance widget was last updated and triggers an update if necessary.
 * The current distance is set to 1km.
 *
 * This is a cheap/lazy implementation of automatic update. Instead of
 * performing a real update by "moving" the objects on the GtkSkyGlance canvas,
 * we simply replace the current GtkSkyGlance object with a new one.
 * Ugly but safe.
 *
 * To ensure smooth performance while running in simulated real time with high
 * throttle value or manual time mode, the caller is responsible for only calling
 * this function at an appropriate frequency (e.g. every 10 cycle).
 */
static void update_skg(GtkSatModule * module)
{
    /* update SKG if ~60 seconds have passed or we have moved 1 km */
    if (G_UNLIKELY(fabs(module->tmgCdnum - module->lastSkgUpd) > 7.0e-4) ||
        G_UNLIKELY(qth_small_dist(module->qth, module->lastSkgUpdqth) > 1.0))
    {

        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Updating GtkSkyGlance for %s"),
                    __func__, module->name);

        gtk_container_remove(GTK_CONTAINER(module->skgwin), module->skg);
        module->skg =
            gtk_sky_glance_new(module->satellites, module->qth,
                               module->tmgCdnum);
        gtk_container_add(GTK_CONTAINER(module->skgwin), module->skg);
        gtk_widget_show_all(module->skg);

        module->lastSkgUpd = module->tmgCdnum;
        qth_small_save(module->qth, &(module->lastSkgUpdqth));
    }
}

static void update_header(GtkSatModule * module)
{
    gchar          *fmtstr;
    gchar           buff[TIME_FORMAT_MAX_LENGTH + 1];
    gchar          *buff2;

    fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
    daynum_to_str(buff, TIME_FORMAT_MAX_LENGTH, fmtstr, module->tmgCdnum);

    if (module->qth->type == QTH_GPSD_TYPE)
    {
        buff2 =
            g_strdup_printf("%s GPS %0.3f seconds old", buff,
                            fabs(module->tmgCdnum -
                                 module->qth->gpsd_update) * (24 * 3600));
        gtk_label_set_text(GTK_LABEL(module->header), buff2);
        g_free(buff2);
    }
    else
        gtk_label_set_text(GTK_LABEL(module->header), buff);

    g_free(fmtstr);

    if (module->tmgActive)
        tmg_update_state(module);
}

/**
 * Update a child widget.
 *
 * @param child Pointer to the child widget (views)
 * @param tstamp The current timestamp
 *
 * This function is called by the main loop of the GtkSatModule widget for
 * each view in the layout grid.
 */
static void update_child(GtkWidget * child, gdouble tstamp)
{
    if (IS_GTK_SAT_LIST(child))
    {
        GTK_SAT_LIST(child)->tstamp = tstamp;
        gtk_sat_list_update(child);
    }

    else if (IS_GTK_SAT_MAP(child))
    {
        GTK_SAT_MAP(child)->tstamp = tstamp;
        gtk_sat_map_update(child);
    }

    else if (IS_GTK_POLAR_VIEW(child))
    {
        GTK_POLAR_VIEW(child)->tstamp = tstamp;
        gtk_polar_view_update(child);
    }

    else if (IS_GTK_SINGLE_SAT(child))
    {
        GTK_SINGLE_SAT(child)->tstamp = tstamp;
        gtk_single_sat_update(child);
    }

    else if (IS_GTK_EVENT_LIST(child))
    {
        GTK_EVENT_LIST(child)->tstamp = tstamp;
        gtk_event_list_update(child);
    }

    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Unknown child type"), __FILE__, __LINE__);
    }
}

/**
 * Update a given satellite.
 *
 * @param key The hash table key (catnum)
 * @param val The hash table value (sat_t structure)
 * @param data User data (the GtkSatModule widget).
 *
 * This function updates the tracking data for a given satelite. It is called by
 * the timeout handler for each element in the hash table.
 */
static void gtk_sat_module_update_sat(gpointer key, gpointer val,
                                      gpointer data)
{
    sat_t          *sat;
    GtkSatModule   *module;
    gdouble         daynum;
    gdouble         maxdt;

    (void)key;

    g_return_if_fail((val != NULL) && (data != NULL));

    sat = SAT(val);
    module = GTK_SAT_MODULE(data);
    maxdt = (gdouble) sat_cfg_get_int(SAT_CFG_INT_PRED_LOOK_AHEAD);

    /* get current time (real or simulated */
    daynum = module->tmgCdnum;

    /* update events if the event counter has been reset
       and the other requirements are fulfilled */
    if ((GTK_SAT_MODULE(module)->event_count == 0) &&
        has_aos(sat, module->qth))
    {
        /* Note that has_aos may return TRUE for geostationary sats
           whose orbit deviate from a true-geostat orbit, however,
           find_aos and find_los will not go beyond the time limit
           we specify (in those cases they return 0.0 for AOS/LOS times.
           We use SAT_CFG_INT_PRED_LOOK_AHEAD for upper time limit */
        sat->aos = find_aos(sat, module->qth, daynum, maxdt);
        sat->los = find_los(sat, module->qth, daynum, maxdt);
    }
    /*
       Update AOS and LOS for this satellite if it was known and is before
       the current time.

       daynum is the current time in the module.

       The conditional aos < daynum is merely saying that aos occured
       in the past. Therefore it cannot be the next event or aos/los
       for that satellite.

       The conditional aos > 0.0 is a short hand for saying that the
       aos was successfully computed before. find_aos returns 0.0 when it
       cannot find an AOS.

       This code should not execute find_aos(los) if the conditional before
       is triggered as the newly computed aos(los) should either be in
       the future (aos > daynum) or (aos == 0 ).

       Single sat/list/event/map views all use these values and they
       should be up to date.

       The above code is still required for dealing with circumstances
       where the qth moves from someplace where the qth can have an AOS and
       where qth does not and for satellites in parking orbits where the
       AOS may be further than maxdt out results in aos==0.0 until the
       next aos is closer than maxdt. It also prevents the aos from
       being computed every pass through the module for the parking orbits.

       To be completely correct, when time can move forward and backwards
       as it can with the time controller, the time the aos/los was
       computed should be stored and associated with aos/los. That way
       if daynum <time_computed, the aos can be recomputed as there is
       no assurance that the current stored aos is the next aos. As a
       practical matter the above code handles time reversing acceptably
       for most circumstances.
     */
    if (sat->aos > 0 && sat->aos < daynum)
        sat->aos = find_aos(sat, module->qth, daynum, maxdt);

    if (sat->los > 0 && sat->los < daynum)
        sat->los = find_los(sat, module->qth, daynum, maxdt);

    predict_calc(sat, module->qth, daynum);
}

/** Module timeout callback. */
static gboolean gtk_sat_module_timeout_cb(gpointer module)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(module);
    GtkWidget      *child;
    gboolean        needupdate = FALSE;
    GdkWindowState  state;
    gdouble         delta;
    guint           i;

    /*update the qth position */
    qth_data_update(mod->qth, mod->tmgCdnum);

    /* in docked state, update only if tab is visible */
    switch (mod->state)
    {
    case GTK_SAT_MOD_STATE_DOCKED:
        if (mod_mgr_mod_is_visible(GTK_WIDGET(module)))
            needupdate = TRUE;

        break;

    default:
        state = gdk_window_get_state(GDK_WINDOW
                                     (gtk_widget_get_window
                                      (GTK_WIDGET(module))));
        if (state & GDK_WINDOW_STATE_ICONIFIED)
            needupdate = FALSE;
        else
            needupdate = TRUE;

        break;
    }

    if (needupdate)
    {
        if (g_mutex_trylock(&mod->busy) == FALSE)
        {
            sat_log_log(SAT_LOG_LEVEL_WARN,
                        _("%s: Previous cycle missed it's deadline."),
                        __func__);

            return TRUE;
        }

        mod->rtNow = get_current_daynum();

        /* Update time if throttle != 0 */
        if (mod->throttle)
        {
            delta = mod->throttle * (mod->rtNow - mod->rtPrev);
            mod->tmgCdnum = mod->tmgPdnum + delta;
        }
        /* else nothing to do since tmg_time_set updates
           mod->tmgCdnum every time */

        /* time to update header? */
        mod->head_count++;
        if (mod->head_count >= mod->head_timeout)
        {
            /* reset counter */
            mod->head_count = 0;
            update_header(mod);
        }

        /* reset event update counter if is has expired or if we have moved
           significantly */
        if (mod->event_count == mod->event_timeout ||
            qth_small_dist(mod->qth, mod->qth_event) > 1.0)
        {
            mod->event_count = 0;       // will trigger find_aos() and find_los()
        }

        /* if the events are going to be recalculated store the position */
        if (mod->event_count == 0)
        {
            qth_small_save(mod->qth, &(mod->qth_event));
        }

        /* update satellite data */
        if (mod->satellites != NULL)
            g_hash_table_foreach(mod->satellites,
                                 gtk_sat_module_update_sat, module);

        /* update children */
        for (i = 0; i < mod->nviews; i++)
        {
            child = GTK_WIDGET(g_slist_nth_data(mod->views, i));
            update_child(child, mod->tmgCdnum);
        }

        /* update satellite data (it may have got out of sync during child updates) */
        if (mod->satellites != NULL)
            g_hash_table_foreach(mod->satellites,
                                 gtk_sat_module_update_sat, module);

        /* update target if autotracking is enabled */
        if (mod->autotrack)
            update_autotrack(mod);

        /* send notice to radio and rotator controller */
        if (mod->rigctrl)
            gtk_rig_ctrl_update(GTK_RIG_CTRL(mod->rigctrl), mod->tmgCdnum);
        if (mod->rotctrl)
            gtk_rot_ctrl_update(GTK_ROT_CTRL(mod->rotctrl), mod->tmgCdnum);

        /* check and update Sky at glance */
        /* FIXME: We should have some timeout counter to ensure that we don't
           update GtkSkyGlance too often when running with high throttle values;
           however, the update does not seem to add any significant load even
           when running at max throttle */
        if (mod->skg)
            update_skg(mod);

        mod->event_count++;

        /* store time keeping variables */
        mod->rtPrev = mod->rtNow;
        mod->tmgPdnum = mod->tmgCdnum;

        if (mod->tmgActive)
        {
            /* update time control spin buttons when we are
               in RT or SRT mode */
            if (mod->throttle)
                tmg_update_widgets(mod);
        }

        g_mutex_unlock(&mod->busy);
    }

    return TRUE;
}

/**
 * Module options.
 *
 * Invoke module-wide popup menu
 */
static void gtk_sat_module_popup_cb(GtkWidget * button, gpointer data)
{
    (void)button;
    gtk_sat_module_popup(GTK_SAT_MODULE(data));
}

/**
 * Read moule configuration data.
 *
 * @param module The GtkSatModule to which the configuration will be applied.
 * @param cfgfile The configuration file.
 */
static void gtk_sat_module_read_cfg_data(GtkSatModule * module,
                                         const gchar * cfgfile)
{
    gchar          *buffer = NULL;
    gchar          *qthfile;
    gchar          *confdir;
    gchar         **buffv;
    guint           length, i;
    GError         *error = NULL;

    module->cfgdata = g_key_file_new();
    g_key_file_set_list_separator(module->cfgdata, ';');

    /* Bail out with error message if data can not be read */
    if (!g_key_file_load_from_file(module->cfgdata, cfgfile,
                                   G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        g_key_file_free(module->cfgdata);
        module->cfgdata = NULL;
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not load config data from %s (%s)."),
                    __func__, cfgfile, error->message);

        g_clear_error(&error);

        return;
    }

    /* debug message */
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Reading configuration from %s"), __func__, cfgfile);

    /* set module name */
    buffer = g_path_get_basename(cfgfile);
    buffv = g_strsplit(buffer, ".mod", 0);
    module->name = g_strdup(buffv[0]);
    g_free(buffer);
    g_strfreev(buffv);

    /* get qth file */
    buffer = mod_cfg_get_str(module->cfgdata,
                             MOD_CFG_GLOBAL_SECTION,
                             MOD_CFG_QTH_FILE_KEY, SAT_CFG_STR_DEF_QTH);

    confdir = get_user_conf_dir();
    qthfile = g_strconcat(confdir, G_DIR_SEPARATOR_S, buffer, NULL);

    /* load QTH data */
    if (!qth_data_read(qthfile, module->qth))
    {
        /* QTH file was not found for some reason */
        g_free(buffer);
        g_free(qthfile);

        /* remove cfg key */
        g_key_file_remove_key(module->cfgdata,
                              MOD_CFG_GLOBAL_SECTION,
                              MOD_CFG_QTH_FILE_KEY, NULL);

        /* save modified cfg data to file */
        mod_cfg_save(module->name, module->cfgdata);

        /* try SAT_CFG_STR_DEF_QTH */
        buffer = sat_cfg_get_str(SAT_CFG_STR_DEF_QTH);
        qthfile = g_strconcat(confdir, G_DIR_SEPARATOR_S, buffer, NULL);

        if (!qth_data_read(qthfile, module->qth))
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _
                        ("%s: Can not load default QTH file %s; using built-in defaults"),
                        __func__, buffer);

            /* settings are really screwed up; we need some safe values here */
            qth_safe(module->qth);
        }
    }

    g_free(buffer);
    g_free(confdir);
    g_free(qthfile);

    /* get timeout value */
    module->timeout = mod_cfg_get_int(module->cfgdata,
                                      MOD_CFG_GLOBAL_SECTION,
                                      MOD_CFG_TIMEOUT_KEY,
                                      SAT_CFG_INT_MODULE_TIMEOUT);

    /* get grid layout configuration (introduced in 1.2) */
    buffer = mod_cfg_get_str(module->cfgdata,
                             MOD_CFG_GLOBAL_SECTION,
                             MOD_CFG_GRID, SAT_CFG_STR_MODULE_GRID);

    /* convert to an integer list */
    buffv = g_strsplit(buffer, ";", 0);
    length = g_strv_length(buffv);
    if ((length == 0) || (length % 5 != 0))
    {
        /* the grid configuration is bogus; override with global default */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module layout is invalid: %s. Using default."),
                    __func__, buffer);
        g_free(buffer);
        g_strfreev(buffv);

        buffer = sat_cfg_get_str_def(SAT_CFG_STR_MODULE_GRID);
        buffv = g_strsplit(buffer, ";", 0);
        length = g_strv_length(buffv);
    }

    /* make a debug log entry */
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: GRID(%d): %s"), __func__, length, buffer);
    g_free(buffer);

    /* number of views: we have five numbers per view (type,left,right,top,bottom) */
    module->nviews = length / 5;
    module->grid = g_try_new0(guint, length);

    /* if we cannot allocate memory for the grid zero the views out and log */
    if (module->grid != NULL)
    {
        /* convert chars to integers */
        for (i = 0; i < length; i++)
        {
            module->grid[i] = (gint) g_ascii_strtoll(buffv[i], NULL, 0);
            //g_print ("%d: %s => %d\n", i, buffv[i], module->grid[i]);
        }
    }
    else
    {
        module->nviews = 0;
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unable to allocate memory for grid."), __func__);
    }
    g_strfreev(buffv);
}

/**
 * Create a new GtkSatModule widget.
 *
 * @param cfgfile The name of the configuration file (.mod)
 *
 * @bug Program goes into infinite loop when there is something
 *      wrong with cfg file.
 */
GtkWidget *gtk_sat_module_new(const gchar * cfgfile)
{
    GtkSatModule   *module;
    GtkWidget      *butbox;

    /* Read configuration data.
     * If cfgfile is not existing or is NULL, start the wizard
     * in order to create a new configuration.
     */
    if ((cfgfile == NULL) || !g_file_test(cfgfile, G_FILE_TEST_EXISTS))
    {

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s is not valid."), __func__, cfgfile);

        return NULL;
    }

    module = GTK_SAT_MODULE(g_object_new(GTK_TYPE_SAT_MODULE, NULL));
    g_object_set(module, "orientation", GTK_ORIENTATION_VERTICAL, NULL);
    gtk_sat_module_read_cfg_data(module, cfgfile);

    if (module->cfgdata == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s has problems."), __func__, cfgfile);

        return NULL;
    }

    /*initialize the qth engine and get position */
    qth_data_update_init(module->qth);

    /* module state */
    if ((g_key_file_has_key(module->cfgdata,
                            MOD_CFG_GLOBAL_SECTION,
                            MOD_CFG_STATE, NULL)) &&
        sat_cfg_get_bool(SAT_CFG_BOOL_MOD_STATE))
    {
        module->state = g_key_file_get_integer(module->cfgdata,
                                               MOD_CFG_GLOBAL_SECTION,
                                               MOD_CFG_STATE, NULL);
    }
    else
    {
        module->state = GTK_SAT_MOD_STATE_DOCKED;
    }

    /* time keeping */
    module->rtNow = get_current_daynum();
    module->rtPrev = get_current_daynum();
    module->tmgPdnum = get_current_daynum();
    module->tmgCdnum = get_current_daynum();

    gtk_sat_module_load_sats(module);

    /* menu */
    GtkWidget * image = gtk_image_new_from_icon_name("open-menu-symbolic",
                                         GTK_ICON_SIZE_BUTTON);

    module->popup_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(module->popup_button), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(module->popup_button, _("Module menu"));
    gtk_container_add(GTK_CONTAINER(module->popup_button), image);
    g_signal_connect(module->popup_button, "clicked",
                     G_CALLBACK(gtk_sat_module_popup_cb), module);


    /* create header; header should not be updated more than once pr. second */
    module->header = gtk_label_new(NULL);
    module->head_count = 0;
    module->head_timeout = module->timeout > 1000 ? 1 :
        (guint) floor(1000 / module->timeout);

    /* Event timeout updates every minute */
    module->event_timeout = module->timeout > 60000 ? 1 :
         (guint) floor(60000 / module->timeout);
    /* force update the first time */
    module->event_count = module->event_timeout;

    butbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(butbox),
                       module->header, FALSE, FALSE, 10);
    gtk_box_pack_end(GTK_BOX(butbox),
                     module->popup_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(module), butbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(module),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    create_module_layout(module);
    gtk_widget_show_all(GTK_WIDGET(module));

    /* start timeout */
    module->timerid = g_timeout_add(module->timeout, gtk_sat_module_timeout_cb,
                                    module);

    return GTK_WIDGET(module);
}

/**
 * Close module.
 *
 * @param button The button widget that received the signal.
 * @param data Pointer the GtkSatModule widget, which should be destroyed.
 *
 * This function is called when the user clicks on the "close" minibutton.
 * The functions checks the state of the module. If the module is docked
 * it is removed from the mod-mgr notebook whereafter it is destroyed.
 * if the module is either in undocked or fullscreen state, the parent
 * window is destroyed, which will automatically destroy the module as
 * well.
 *
 * NOTE: Don't use button, since we don't know what kind of widget it is
 *       (it may be button or menu item).
 */
void gtk_sat_module_close_cb(GtkWidget * button, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    gchar          *name;
    gint            retcode;

    (void)button;

    name = g_strdup(module->name);

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Module %s received CLOSE signal."), __func__, name);

    /* save configuration to ensure that dynamic data like state is stored */
    mod_cfg_save(module->name, module->cfgdata);

    switch (module->state)
    {
    case GTK_SAT_MOD_STATE_DOCKED:
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Module %s is in DOCKED state."), __func__, name);
        retcode = mod_mgr_remove_module(GTK_WIDGET(module));

        if (retcode)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module %s was not found in mod-mgr (%d)\n"
                          "Internal state is corrupt?"),
                        __func__, name, retcode);
        }

        break;

    case GTK_SAT_MOD_STATE_WINDOW:
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Module %s is in WINDOW state."), __func__, name);

        retcode = mod_mgr_remove_module(GTK_WIDGET(module));

        if (retcode)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module %s was not found in mod-mgr (%d)\n"
                          "Internal state is corrupt?"),
                        __func__, name, retcode);
        }

        /* increase referene count */
        g_object_ref(module);

        /* remove module from window, destroy window */
        gtk_container_remove(GTK_CONTAINER(GTK_SAT_MODULE(module)->win),
                             GTK_WIDGET(module));
        gtk_widget_destroy(GTK_SAT_MODULE(module)->win);
        GTK_SAT_MODULE(module)->win = NULL;

        /* release module */
        g_object_unref(module);

        break;

    case GTK_SAT_MOD_STATE_FULLSCREEN:
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Module %s is in FULLSCREEN state."),
                    __func__, name);

        retcode = mod_mgr_remove_module(GTK_WIDGET(module));

        if (retcode)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module %s was not found in mod-mgr (%d)\n"
                          "Internal state is corrupt?"),
                        __func__, name, retcode);
        }


        /* increase referene count */
        g_object_ref(module);

        /* remove module from window, destroy window */
        gtk_container_remove(GTK_CONTAINER(GTK_SAT_MODULE(module)->win),
                             GTK_WIDGET(module));
        gtk_widget_destroy(GTK_SAT_MODULE(module)->win);
        GTK_SAT_MODULE(module)->win = NULL;

        /* release module */
        g_object_unref(module);

        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Module %s has unknown state: %d"),
                    __func__, name, module->state);
        break;
    }

    /* appearantly, module will be destroyed when removed from notebook */
    /* gtk_widget_destroy (GTK_WIDGET (module)); */
    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Module %s closed."), __func__, name);

    g_free(name);
}

/**
 * Configure module.
 *
 * @param button The button widget that received the signal.
 * @param data Pointer the GtkSatModule widget, which should be reconfigured
 *
 * This function is called when the user clicks on the "configure" minibutton.
 * The function incokes the mod_cfg_edit funcion, which has the same look and feel
 * as the dialog used to create a new module.
 *
 * NOTE: Don't use button, since we don't know what kind of widget it is
 *       (it may be button or menu item).
 */
void gtk_sat_module_config_cb(GtkWidget * button, gpointer data)
{
    GtkSatModule   *module = GTK_SAT_MODULE(data);
    GtkAllocation   alloc;
    GtkWidget      *toplevel;
    gchar          *name;
    gchar          *cfgfile;
    mod_cfg_status_t retcode;
    gtk_sat_mod_state_t laststate;
    gint            w, h;

    (void)button;

    if (module->win != NULL)
        toplevel = module->win;
    else
        toplevel = gtk_widget_get_toplevel(GTK_WIDGET(data));

    name = g_strdup(module->name);

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: Module %s received CONFIG signal."), __func__, name);

    /* stop timeout */
    if (!g_source_remove(module->timerid))
    {
        /* internal error, since the timerid appears
           to be invalid.
         */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not stop timeout callback\n"
                      "%s: Source ID %d seems invalid."),
                    __func__, __func__, module->timerid);
    }
    else
    {
        module->timerid = -1;
        retcode = mod_cfg_edit(name, module->cfgdata, toplevel);
        if (retcode == MOD_CFG_OK)
        {
            /* save changes */
            retcode = mod_cfg_save(name, module->cfgdata);
            if (retcode != MOD_CFG_OK)
            {
                /**** FIXME: dialog */
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _
                            ("%s: Module configuration failed for some reason."),
                            __func__);
                /* don't try to reload config since it may be
                   invalid; keep original */
            }
            else
            {
                /* store state and size */
                laststate = module->state;
                gtk_widget_get_allocation(GTK_WIDGET(module), &alloc);
                w = alloc.width;
                h = alloc.height;

                gtk_sat_module_close_cb(NULL, module);

                gchar          *confdir = get_modules_dir();

                cfgfile =
                    g_strconcat(confdir, G_DIR_SEPARATOR_S, name, ".mod",
                                NULL);
                g_free(confdir);

                module = GTK_SAT_MODULE(gtk_sat_module_new(cfgfile));
                module->state = laststate;

                switch (laststate)
                {

                case GTK_SAT_MOD_STATE_DOCKED:

                    /* re-open module by adding it to the mod-mgr */
                    mod_mgr_add_module(GTK_WIDGET(module), TRUE);

                    break;

                case GTK_SAT_MOD_STATE_WINDOW:

                    /* add to module manager */
                    mod_mgr_add_module(GTK_WIDGET(module), FALSE);

                    /* create window */
                    module->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                    gtk_window_set_title(GTK_WINDOW(module->win),
                                         module->name);
                    gtk_window_set_default_size(GTK_WINDOW(module->win), w, h);

                    /** FIXME: window icon and such */

                    /* add module to window */
                    gtk_container_add(GTK_CONTAINER(module->win),
                                      GTK_WIDGET(module));

                    /* show window */
                    gtk_widget_show_all(module->win);

                    break;

                case GTK_SAT_MOD_STATE_FULLSCREEN:

                    /* add to module manager */
                    mod_mgr_add_module(GTK_WIDGET(module), FALSE);

                    /* create window */
                    module->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                    gtk_window_set_title(GTK_WINDOW(module->win),
                                         module->name);
                    gtk_window_set_default_size(GTK_WINDOW(module->win), w, h);

                    /** FIXME: window icon and such */

                    /* add module to window */
                    gtk_container_add(GTK_CONTAINER(module->win),
                                      GTK_WIDGET(module));

                    /* show window */
                    gtk_widget_show_all(module->win);

                    gtk_window_fullscreen(GTK_WINDOW(module->win));

                    break;

                default:
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Module %s has unknown state: %d"),
                                __func__, name, module->state);
                    break;
                }

                g_free(cfgfile);
            }
        }
        else
        {
            /* user cancelled => just re-start timer */
            module->timerid = g_timeout_add(module->timeout,
                                            gtk_sat_module_timeout_cb, data);
        }
    }

    g_free(name);
}

static gboolean empty(gpointer key, gpointer val, gpointer data)
{
    (void)key;
    (void)val;
    (void)data;

    /* TRUE => sat removed from hash table */
    return TRUE;
}

/** Reload satellites in view */
static void reload_sats_in_child(GtkWidget * widget, GtkSatModule * module)
{
    if (IS_GTK_SINGLE_SAT(G_OBJECT(widget)))
    {
        gtk_single_sat_reload_sats(widget, module->satellites);
    }
    else if (IS_GTK_POLAR_VIEW(widget))
    {
        gtk_polar_view_reload_sats(widget, module->satellites);
    }
    else if (IS_GTK_SAT_MAP(widget))
    {
        gtk_sat_map_reload_sats(widget, module->satellites);
    }
    else if (IS_GTK_SAT_LIST(widget))
    {
        /* NOP */
    }
    else if (IS_GTK_EVENT_LIST(widget))
    {
        /* NOP */
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Unknown child type"), __FILE__, __LINE__);
    }
}

/**
 * Reload satellites.
 *
 * @param module Pointer to a GtkSatModule widget.
 *
 * This function is used to reload the satellites in a module. This is can be
 * useful when:
 *
 *   1. The TLE files have been updated.
 *   2. The module configuration has changed (i.e. which satellites to track).
 *
 * The function assumes that module->cfgdata has already been updated, and so
 * all it has to do is to free module->satellites and re-execute the satellite
 * loading sequence.
 */
void gtk_sat_module_reload_sats(GtkSatModule * module)
{
    GtkWidget      *child;
    guint           i;

    g_return_if_fail(IS_GTK_SAT_MODULE(module));

    /* lock module */
    g_mutex_lock(&module->busy);

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Reloading satellites for module %s"),
                __func__, module->name);

    /* remove each element from the hash table, but keep the hash table */
    g_hash_table_foreach_remove(module->satellites, empty, NULL);

    /* reset event counter so that next AOS/LOS gets re-calculated */
    module->event_count = 0;

    /* load satellites */
    gtk_sat_module_load_sats(module);

    /* update children */
    for (i = 0; i < module->nviews; i++)
    {
        child = GTK_WIDGET(g_slist_nth_data(module->views, i));
        reload_sats_in_child(child, module);
    }

    /* FIXME: radio and rotator controller */

    /* unlock module */
    g_mutex_unlock(&module->busy);
}

/** Select a new satellite */
void gtk_sat_module_select_sat(GtkSatModule * module, gint catnum)
{
    GtkWidget      *child;
    guint           i;

    module->target = catnum;

    /* select satellite in each view */
    for (i = 0; i < module->nviews; i++)
    {
        child = GTK_WIDGET(g_slist_nth_data(module->views, i));

        if (IS_GTK_SINGLE_SAT(G_OBJECT(child)))
        {
            gtk_single_sat_select_sat(child, catnum);
        }
        else if (IS_GTK_SAT_MAP(child))
        {
            gtk_sat_map_select_sat(child, catnum);
        }
        else if (IS_GTK_SAT_LIST(child))
        {
            gtk_sat_list_select_sat(child, catnum);
        }
        else if (IS_GTK_EVENT_LIST(child))
        {
            gtk_event_list_select_sat(child, catnum);
        }
        else if (IS_GTK_POLAR_VIEW(child))
        {
            gtk_polar_view_select_sat(child, catnum);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Unknown child type"),
                        __func__);
        }
    }

    /* select sat in radio and rotator controllers */
    if (module->rigctrl != NULL)
        gtk_rig_ctrl_select_sat(GTK_RIG_CTRL(module->rigctrl), catnum);

    if (module->rotctrl != NULL)
        gtk_rot_ctrl_select_sat(GTK_ROT_CTRL(module->rotctrl), catnum);
}

/**
 * Re-configure module.
 *
 * @param module The module.
 * @param local Flag indicating whether reconfiguration is requested from 
 *              local configuration dialog.
 *
 */
void gtk_sat_module_reconf(GtkSatModule * module, gboolean local)
{
    (void)module;
    (void)local;
}
