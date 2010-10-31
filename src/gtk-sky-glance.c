/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2010  Alexandru Csete, OZ9AEC.

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
/** \brief Sky at a glance Widget.
 * 
 * The sky at a glance widget provides a convenient overview of the upcoming
 * satellite passes in a timeline format. The widget is tied to a specific
 * module and uses the QTH and satellite data from the module.
 *
 * Note about the sizing policy:
 * Initially we require 10 pixels per sat + 5 pix margin between the sats.
 *
 * When we get additional space due to resizing, the space will be allocated
 * to make the rectangles taller.
 *
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "mod-cfg-get-param.h"
#include "time-tools.h"
#include "gtk-sat-data.h"
#include "gpredict-utils.h"
#include "predict-tools.h"
#include "sat-pass-dialogs.h"
#include "time-tools.h"
//#include "gtk-sky-glance-popup.h"
#include "gtk-sky-glance.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include <goocanvas.h>



#define SKG_DEFAULT_WIDTH 600
#define SKG_DEFAULT_HEIGHT 300
#define SKG_PIX_PER_SAT 10
#define SKG_MARGIN 15
#define SKG_FOOTER 50


static void gtk_sky_glance_class_init  (GtkSkyGlanceClass *class);
static void gtk_sky_glance_init        (GtkSkyGlance *skg);
static void gtk_sky_glance_destroy     (GtkObject *object);
static void size_allocate_cb           (GtkWidget *widget,
                                        GtkAllocation *allocation,
                                        gpointer data);
static gboolean on_motion_notify       (GooCanvasItem *item,
                                        GooCanvasItem *target,
                                        GdkEventMotion *event,
                                        gpointer data);
static void on_item_created            (GooCanvas *canvas,
                                        GooCanvasItem *item,
                                        GooCanvasItemModel *model,
                                        gpointer data);
static void on_canvas_realized         (GtkWidget *canvas, gpointer data);
static gboolean on_button_press        (GooCanvasItem *item,
                                        GooCanvasItem *target,
                                        GdkEventButton *event,
                                        gpointer data);
static gboolean on_button_release      (GooCanvasItem *item,
                                        GooCanvasItem *target,
                                        GdkEventButton *event,
                                        gpointer data);

static gboolean on_mouse_enter         (GooCanvasItem    *item,
                                        GooCanvasItem    *target_item,
                                        GdkEventCrossing *event,
                                        gpointer          data);
static gboolean on_mouse_leave         (GooCanvasItem    *item,
                                        GooCanvasItem    *target_item,
                                        GdkEventCrossing *event,
                                        gpointer          data);


static GooCanvasItemModel* create_canvas_model (GtkSkyGlance *skg);


static void create_sat (gpointer key, gpointer value, gpointer data);

static gdouble t2x (GtkSkyGlance *skg, gdouble t);
static gdouble x2t (GtkSkyGlance *skg, gdouble x);
static gchar *time_to_str (gdouble julutc);


static GtkVBoxClass *parent_class = NULL;


GtkType
gtk_sky_glance_get_type ()
{
    static GType gtk_sky_glance_type = 0;

    if (!gtk_sky_glance_type) {
        static const GTypeInfo gtk_sky_glance_info = {
            sizeof (GtkSkyGlanceClass),
            NULL, /* base init */
            NULL, /* base finalise */
            (GClassInitFunc) gtk_sky_glance_class_init,
            NULL, /* class finalise */
            NULL, /* class data */
            sizeof (GtkSkyGlance),
            1,  /* n_preallocs */
            (GInstanceInitFunc) gtk_sky_glance_init,
        };

        gtk_sky_glance_type = g_type_register_static (GTK_TYPE_VBOX,
                                                        "GtkSkyGlance",
                                                        &gtk_sky_glance_info,
                                                        0);
    }

    return gtk_sky_glance_type;
}


static void
gtk_sky_glance_class_init (GtkSkyGlanceClass *class)
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

    object_class->destroy = gtk_sky_glance_destroy;
    //widget_class->size_allocate = gtk_sky_glance_size_allocate;
}


static void
gtk_sky_glance_init (GtkSkyGlance *skg)
{
    skg->sats      = NULL;
    skg->qth       = NULL;
    skg->passes    = NULL;
    skg->satlab    = NULL;
    skg->x0        = 0;
    skg->y0        = 0;
    skg->w         = 0;
    skg->h         = 0;
    skg->pps       = 0;
    skg->numsat    = 0;
    skg->satcnt    = 0;
    skg->ts        = 0.0;
    skg->te        = 0.0;
}


/** \brief Destroy the GtkSkyGlance widget
 *  \param object Pointer to the GtkSkyGlance widget
 *
 * This function is called when the GtkSkyGlance widget is destroyed. It frees 
 * the memory that has been allocated when the widget was created.
 * 
 * \bug For some reason, this function is called twice when parent is destroyed.
 */
static void
gtk_sky_glance_destroy (GtkObject *object)
{
    sky_pass_t *skypass;
    guint   i, n;


    /* free passes */
    /* FIXME: TBC whether this is enough */
    if (GTK_SKY_GLANCE (object)->passes != NULL) {

        n = g_slist_length (GTK_SKY_GLANCE (object)->passes);
        for (i = 0; i < n; i++) {
            skypass = (sky_pass_t *) g_slist_nth_data (GTK_SKY_GLANCE (object)->passes, i);
            free_pass (skypass->pass);
            g_free (skypass);
        }

        g_slist_free (GTK_SKY_GLANCE (object)->passes);
        GTK_SKY_GLANCE (object)->passes = NULL;
    }

    /* for the rest we only need to free the GSList because the
        canvas items will be freed when removed from canvas.
    */
    if (GTK_SKY_GLANCE (object)->satlab != NULL) {     
        g_slist_free (GTK_SKY_GLANCE (object)->satlab);
        GTK_SKY_GLANCE (object)->satlab = NULL;
    }
    if (GTK_SKY_GLANCE (object)->majors != NULL) {
        g_slist_free (GTK_SKY_GLANCE (object)->majors);
        GTK_SKY_GLANCE (object)->majors = NULL;
    }
    if (GTK_SKY_GLANCE (object)->minors != NULL) {
        g_slist_free (GTK_SKY_GLANCE (object)->minors);
        GTK_SKY_GLANCE (object)->minors = NULL;
    }
    if (GTK_SKY_GLANCE (object)->labels != NULL) {
        g_slist_free (GTK_SKY_GLANCE (object)->labels);
        GTK_SKY_GLANCE (object)->labels = NULL;
    }

    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);

}



/** \brief Create a new GtkSkyGlance widget.
 *  \param sats Pointer to the hash table containing the asociated satellites.
 *  \param qth Pointer to the ground station data.
 *  \param ts The t0 for the timeline or 0 to use the current date and time.
 */
GtkWidget*
gtk_sky_glance_new (GHashTable *sats, qth_t *qth, gdouble ts)
{
    GtkWidget *skg;
    GooCanvasItemModel *root;
    GdkColor bg_color = {0, 0xFFFF, 0xFFFF, 0xFFFF};
    guint number;


    /* check that we have at least one satellite */
    number = g_hash_table_size (sats);
    if (number == 0) {
        /* no satellites */
        skg = gtk_label_new (_("This module has no satellites!"));

        return skg;
    }

    skg = g_object_new (GTK_TYPE_SKY_GLANCE, NULL);

    /* FIXME? */
    GTK_SKY_GLANCE (skg)->sats = sats;
    GTK_SKY_GLANCE (skg)->qth  = qth;


    /* get settings */
    GTK_SKY_GLANCE (skg)->numsat = g_hash_table_size (sats);

    /* if ts = 0 use current time */
    if (ts > 0.0) {
        GTK_SKY_GLANCE (skg)->ts = ts;
    }
    else {
        GTK_SKY_GLANCE (skg)->ts = get_current_daynum ();
    }

    GTK_SKY_GLANCE (skg)->te = GTK_SKY_GLANCE (skg)->ts +
        sat_cfg_get_int (SAT_CFG_INT_SKYATGL_TIME)*(1.0/24.0);

    /* calculate preferred sizes */
    GTK_SKY_GLANCE (skg)->w = SKG_DEFAULT_WIDTH;
    GTK_SKY_GLANCE (skg)->h = GTK_SKY_GLANCE (skg)->numsat * SKG_PIX_PER_SAT +
        (GTK_SKY_GLANCE (skg)->numsat + 1) * SKG_MARGIN;
    GTK_SKY_GLANCE (skg)->pps = SKG_PIX_PER_SAT;

    /* create the canvas */
    GTK_SKY_GLANCE (skg)->canvas = goo_canvas_new ();
    g_object_set (G_OBJECT (GTK_SKY_GLANCE(skg)->canvas), "has-tooltip", TRUE, NULL);
    gtk_widget_modify_base (GTK_SKY_GLANCE (skg)->canvas, GTK_STATE_NORMAL, &bg_color);
    gtk_widget_set_size_request (GTK_SKY_GLANCE (skg)->canvas,
                                 GTK_SKY_GLANCE (skg)->w,
                                 GTK_SKY_GLANCE (skg)->h + SKG_FOOTER);
    goo_canvas_set_bounds (GOO_CANVAS (GTK_SKY_GLANCE (skg)->canvas), 0, 0,
                           GTK_SKY_GLANCE (skg)->w,
                           GTK_SKY_GLANCE (skg)->h + SKG_FOOTER);

    /* connect size-request signal */
    g_signal_connect (GTK_SKY_GLANCE (skg)->canvas, "size-allocate",
                      G_CALLBACK (size_allocate_cb), skg);
    g_signal_connect (GTK_SKY_GLANCE (skg)->canvas, "item_created",
                      (GtkSignalFunc) on_item_created, skg);
    g_signal_connect_after (GTK_SKY_GLANCE (skg)->canvas, "realize",
                            (GtkSignalFunc) on_canvas_realized, skg);

    gtk_widget_show (GTK_SKY_GLANCE (skg)->canvas);


    /* Create the canvas model */
    root = create_canvas_model (GTK_SKY_GLANCE (skg));
    goo_canvas_set_root_item_model (GOO_CANVAS (GTK_SKY_GLANCE (skg)->canvas), root);

    g_object_unref (root);

    /* add satellite passes */
    g_hash_table_foreach (GTK_SKY_GLANCE (skg)->sats, create_sat, skg);

    gtk_container_add (GTK_CONTAINER (skg), GTK_SKY_GLANCE (skg)->canvas);

    return skg;
}


/** \brief Create the model for the GtkSkyGlance canvas
 *  \param skg Pointer to the GtkSkyGlance widget
 */
static GooCanvasItemModel *
create_canvas_model (GtkSkyGlance *skg)
{
    GooCanvasItemModel *root;
    GooCanvasItemModel *hrt,*hrl,*hrm;
    guint              i,n;
    gdouble            th,tm;
    time_t             tt;
    gdouble            xh,xm;
    gchar              buff[3];


    root = goo_canvas_group_model_new (NULL, NULL);

    /* cursor tracking line */
    skg->cursor = goo_canvas_polyline_model_new_line (root,
                                                        skg->x0, skg->y0, skg->x0, skg->h,
                                                        "stroke-color-rgba", 0x000000AF,
                                                        "line-width", 0.5,
                                                        NULL);

    /* time label */
    skg->timel = goo_canvas_text_model_new (root, "--:--", skg->x0 + 5, skg->y0, -1, GTK_ANCHOR_NW,
                                            "font", "Sans 8",
                                            "fill-color-rgba", 0x000000AF,
                                            NULL);

    /* footer */
    skg->footer = goo_canvas_rect_model_new (root,
                                                skg->x0, skg->h,
                                                skg->w, SKG_FOOTER,
                                                "fill-color-rgba", 0x00003FFF,
                                                "stroke-color-rgba", 0xFFFFFFFF,
                                                NULL);

    /* time ticks and labels */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
        skg->axisl = goo_canvas_text_model_new (root, _("TIME"),
                                                skg->w / 2, skg->h + SKG_FOOTER - 5,
                                                -1, GTK_ANCHOR_S,
                                                "font", "Sans 9",
                                                "fill-color-rgba", 0xFFFFFFFF,
                                                NULL);
    else
        skg->axisl = goo_canvas_text_model_new (root, _("UTC"),
                                                skg->w / 2, skg->h + SKG_FOOTER - 5,
                                                -1, GTK_ANCHOR_S,
                                                "font", "Sans 9",
                                                "fill-color-rgba", 0xFFFFFFFF,
                                                NULL);

    /* get the first hour and first 30 min slot */
    th = ceil (skg->ts * 24.0) / 24.0;

    /* workaround for bug 1839140 (first hour incorrexct) */
    th += 0.00069;

    /* the first 30 min tick can be either before
        or after the first hour tick
    */
    if ((th - skg->ts) > 0.0208333) {
        tm = th - 0.0208333;
    }
    else {
        tm = th + 0.0208333;
    }

    /* the number of steps equals the number of hours */
    n = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_TIME);
    for (i = 0; i < n; i++) {

        /* hour tick */
        xh = t2x (skg, th);
        hrt = goo_canvas_polyline_model_new_line (root, xh, skg->h, xh, skg->h + 10,
                                                    "stroke-color-rgba", 0xFFFFFFFF,
                                                    NULL);

        /* hour tick label */
        tt = (th - 2440587.5)*86400.0;
        if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
            strftime (buff, 3, "%H", localtime (&tt));
        else
            strftime (buff, 3, "%H", gmtime (&tt));
        
        buff[2] = '\0';
        hrl = goo_canvas_text_model_new (root, buff, xh, skg->h + 12,
                                            -1, GTK_ANCHOR_N,
                                            "font", "Sans 8",
                                            "fill-color-rgba", 0xFFFFFFFF,
                                            NULL);

        /* 30 min tick */
        xm = t2x (skg, tm);
        hrm = goo_canvas_polyline_model_new_line (root, xm, skg->h, xm, skg->h + 5,
                                                    "stroke-color-rgba", 0xFFFFFFFF,
                                                    NULL);

        /* store canvas items */
        skg->majors = g_slist_append (skg->majors, hrt);
        skg->labels = g_slist_append (skg->labels, hrl);
        skg->minors = g_slist_append (skg->minors, hrm);

        th += 0.0416667;
        tm += 0.0416667;
    }

    return root;
}




/** \brief Manage new size allocation.
 *
 * This function is called when the canvas receives a new size allocation,
 * e.g. when the container is re-sized. The function re-calculates the graph
 * dimensions based on the new canvas size.
 */
static void
size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
    GtkSkyGlance     *skg;
    GooCanvasPoints  *pts;
    GooCanvasItem    *obj;
    gint              i,j,n;  
    guint             curcat; 
    gdouble           th,tm;
    gdouble           xh,xm;
    sky_pass_t       *skp;
    gdouble           x,y,w,h;


    if (GTK_WIDGET_REALIZED (widget)) {

        /* get graph dimensions */
        skg = GTK_SKY_GLANCE (data);

        skg->w = allocation->width;
        skg->h = allocation->height - SKG_FOOTER;
        skg->x0 = 0;
        skg->y0 = 0;

        skg->pps = (skg->h - SKG_MARGIN) / skg->numsat - SKG_MARGIN;

        goo_canvas_set_bounds (GOO_CANVAS (GTK_SKY_GLANCE (skg)->canvas), 0, 0,
                                allocation->width, allocation->height);


        /* update cursor tracking line */
        pts = goo_canvas_points_new (2);
        pts->coords[0] = skg->x0;
        pts->coords[1] = skg->y0;
        pts->coords[2] = skg->x0;
        pts->coords[3] = skg->h;
        g_object_set (skg->cursor, "points", pts, NULL);
        goo_canvas_points_unref (pts);

        /* time label */
        g_object_set (skg->timel,
                        "x", (gdouble) skg->x0 + 5,
                        NULL);

        
        /* update footer */
        g_object_set (skg->footer,
                        "x", (gdouble) skg->x0,
                        "y", (gdouble) skg->h,
                        "width", (gdouble) skg->w,
                        "height", (gdouble) SKG_FOOTER,
                        NULL);

        g_object_set (skg->axisl,
                        "x", (gdouble) (skg->w / 2),
                        "y", (gdouble) (skg->h + SKG_FOOTER - 5),
                        NULL);

        /* get the first hour and first 30 min slot */
        th = ceil (skg->ts * 24.0) / 24.0;

        /* workaround for bug 1839140 (first hour incorrexct) */
        th += 0.00069;
        
        if ((th - skg->ts) > 0.0208333) {
            tm = th - 0.0208333;
        }
        else {
            tm = th + 0.0208333;
        }

        /* the number of steps equals the number of hours */
        n = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_TIME);
        for (i = 0; i < n; i++) {
            xh = t2x (skg, th);

            pts = goo_canvas_points_new (2);
            pts->coords[0] = xh;
            pts->coords[1] = skg->h;
            pts->coords[2] = xh;
            pts->coords[3] = skg->h + 10;

            obj = g_slist_nth_data (skg->majors, i);
            g_object_set (obj, "points", pts, NULL);

            goo_canvas_points_unref (pts);

            obj = g_slist_nth_data (skg->labels, i);
            g_object_set (obj,
                            "x", (gdouble) xh,
                            "y", (gdouble) (skg->h + 12),
                            NULL);

            /* 30 min tick */
            xm = t2x (skg, tm);

            pts = goo_canvas_points_new (2);
            pts->coords[0] = xm;
            pts->coords[1] = skg->h;
            pts->coords[2] = xm;
            pts->coords[3] = skg->h + 5;

            obj = g_slist_nth_data (skg->minors, i);
            g_object_set (obj, "points", pts, NULL);

            goo_canvas_points_unref (pts);

            th += 0.04167;
            tm += 0.04167;
        }

        /* update pass items */
        n = g_slist_length (skg->passes);
        j = -1;
        curcat = 0;
        y = 10.0;
        h = 10.0;
        for (i = 0; i < n; i++) {

            /* get pass */
            skp = (sky_pass_t *) g_slist_nth_data (skg->passes, i);

            x = t2x (skg, skp->pass->aos);
            w = t2x (skg, skp->pass->los) - x;

            /* new satellite? */
            if (skp->catnum != curcat) {
                j++;
                curcat = skp->catnum;
                y = j * (skg->pps + SKG_MARGIN) + SKG_MARGIN;
                h = skg->pps;

                /* update label */
                obj = g_slist_nth_data (skg->satlab, j);
                if (x > (skg->x0 + 100))
                    g_object_set (obj, "x", x-5, "y", y+h/2.0,
                                    "anchor", GTK_ANCHOR_E, NULL);
                else
                    g_object_set (obj, "x", x+w+5, "y", y+h/2.0,
                                    "anchor", GTK_ANCHOR_W, NULL);
            }

            g_object_set (skp->box,
                          "x", x,
                          "y", y,
                          "width", w,
                          "height", h,
                          NULL);
        }

    }
}


/** \brief Manage canvas realise signals.
 *
 * This function is used to re-initialise the graph dimensions when
 * the graph is realized, i.e. displayed for the first time. This is
 * necessary in order to compensate for missing "re-allocate" signals for
 * graphs that have not yet been realised, e.g. when opening several module
 */
static void
on_canvas_realized (GtkWidget *canvas, gpointer data)
{
    GtkAllocation aloc;

    aloc.width = canvas->allocation.width;
    aloc.height = canvas->allocation.height;
    size_allocate_cb (canvas, &aloc, data);

}


/** \brief Manage mouse motion events. */
static gboolean
on_motion_notify (GooCanvasItem *item,
                  GooCanvasItem *target,
                  GdkEventMotion *event,
                  gpointer data)
{
    GtkSkyGlance    *skg = GTK_SKY_GLANCE (data);
    GooCanvasPoints *pts;
    gdouble          t;
    time_t           tt;
    gchar            buff[6];

    /* update cursor tracking line and time label */
    pts = goo_canvas_points_new (2);
    pts->coords[0] = event->x;
    pts->coords[1] = skg->y0;
    pts->coords[2] = event->x;
    pts->coords[3] = skg->h;
    g_object_set (skg->cursor, "points", pts, NULL);
    goo_canvas_points_unref (pts);

    /* get time corresponding to x */
    t = x2t (skg, event->x);

    /* convert julian date to struct tm */
    tt = (t - 2440587.5)*86400.;

    /* format either local time or UTC depending on check box */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
        strftime (buff, 6, "%H:%M", localtime (&tt));
    else
        strftime (buff, 6, "%H:%M", gmtime (&tt));

    buff[5] = '\0';

    /* in order to avoid label clipping close to the edges of
        the chart, the label is placed left/right of the cursor
        tracking line depending on which half we are in.
        => Currently disabled, time display stays in upper left corner
    */
/*
    if (event->x > (skg->w / 2)) {
        g_object_set (skg->timel,
                        "text", buff,
                        "x", (gdouble) event->x - 5,
                        "anchor", GTK_ANCHOR_NE,
                        NULL);
    }
    else {
        g_object_set (skg->timel,
                        "text", buff,
                        "x", (gdouble) event->x + 5,
                        "anchor", GTK_ANCHOR_NW,
                        NULL);
    }
*/
    g_object_set (skg->timel,
                  "text", buff,
                  NULL);



    return TRUE;
}


/** \brief Finish canvas item setup.
 *  \param canvas Pointer to the GooCanvas object
 *  \param item Pointer to the GooCanvasItem that received the signals
 *  \param model Pointer to the model associated with the GooCanvasItem object
 *  \param data Pointer to the GtkSkyGlance object.
 *
 * This function is called when a canvas item is created. Its purpose is to connect
 * the corresponding signals to the created items.
 */
static void
on_item_created (GooCanvas *canvas,
                GooCanvasItem *item,
                GooCanvasItemModel *model,
                gpointer data)
{
    if (!goo_canvas_item_model_get_parent (model))     {
        /* root item / canvas */
        g_signal_connect (item, "motion_notify_event", (GtkSignalFunc) on_motion_notify, data);
    }

    else if (!g_object_get_data (G_OBJECT (item), "skip-signal-connection")) {
        //g_signal_connect (item, "button_press_event", (GtkSignalFunc) on_button_press, data);
        g_signal_connect (item, "button_release_event", (GtkSignalFunc) on_button_release, data);
        g_signal_connect (item, "enter_notify_event", (GtkSignalFunc) on_mouse_enter, data);
        g_signal_connect (item, "leave_notify_event", (GtkSignalFunc) on_mouse_leave, data);
    }
}


/** \brief Manage button press events
 *  \param item The GooCanvasItem object that received the button press event.
 *  \param target The target of the event (what?).
 *  \param event Event data, such as X and Y coordinates.
 *  \param data User data; points to the GtkSkyAtGlance object.
 *  \return Always TRUE to prevent further propagation of the event.
 *
 * This function is called when a mouse button is pressed on a satellite pass object.
 * If the pressed button is 1 (left) pass details will be show.
 */
static gboolean
on_button_press (GooCanvasItem *item,
                 GooCanvasItem *target,
                 GdkEventButton *event,
                 gpointer data)
{
/*  GooCanvasItemModel *model = goo_canvas_item_get_model (item); */
/*  GtkSkyGlance  *skg = GTK_SKY_GLANCE (data); */


    switch (event->button) {

    default:
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s::%s: Button %d has no function..."),
                     __FILE__, __FUNCTION__, event->button);

        break;
    }       

    return TRUE;
}


/** \brief Manage button release events.
 *  \param item The GooCanvasItem object that received the button press event.
 *  \param target The target of the event (what?).
 *  \param event Event data, such as X and Y coordinates.
 *  \param data User data; points to the GtkSkyAtGlance object.
 *  \return Always TRUE to prevent further propagation of the event.
 *
 * This function is called when the mouse button is released above
 * a satellite pass object.
 *
 * We do not currently use this for anything.
 */
static gboolean
on_button_release (GooCanvasItem *item,
                   GooCanvasItem *target,
                   GdkEventButton *event,
                   gpointer data)
{

    GooCanvasItemModel *item_model = goo_canvas_item_get_model (item);
    GtkSkyGlance  *skg = GTK_SKY_GLANCE (data);

    /* get pointer to pass_t structure */
    pass_t *pass = (pass_t *) g_object_get_data(G_OBJECT(item_model), "pass");
    pass_t *new_pass;


    if G_UNLIKELY(pass == NULL) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s::%s: Could not retrieve pass_t object"),
                     __FILE__, __FUNCTION__);
        return TRUE;
    }


    switch (event->button) {

        /* LEFT button released */
    case 1:
        new_pass = copy_pass (pass);
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s::%s: Showing pass details for %s - we may have a memory leak here"),
                     __FILE__, __FUNCTION__, pass->satname);

        /* show the pass details */
        show_pass (pass->satname, skg->qth, new_pass, NULL);

        break;

    default:
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s::%s: Button %d has no function..."),
                     __FILE__, __FUNCTION__, event->button);
        break;
    }

    return TRUE;
}


/** \brief Manage mouse-enter events on canvas items (satellite pass boxes)
 * \param item The GooCanvasItem that received the signal
 * \param target_item The target if the even (have no idea what this is...)
 * \param event Info about the event
 * \param data Pointer to the GtkSkyAtGlance object
 * \return Always TRUE to prevent further propagation of the event.
 *
 * This function is used to be notified when the mouse enters over a satellite
 * pass box. We could use it to highlihght the pass under the mouse.
 */
static gboolean on_mouse_enter         (GooCanvasItem    *item,
                                        GooCanvasItem    *target_item,
                                        GdkEventCrossing *event,
                                        gpointer          data)
{
    GooCanvasItemModel *item_model = goo_canvas_item_get_model (item);
    GtkSkyGlance  *skg = GTK_SKY_GLANCE (data);

    /* get pointer to pass_t structure */
    pass_t *pass = (pass_t *) g_object_get_data(G_OBJECT(item_model), "pass");
    if G_UNLIKELY(pass == NULL) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s::%s: Could not retrieve pass_t object"),
                     __FILE__, __FUNCTION__);
        return TRUE;
    }

    //g_print("Mouse enter: %s AOS:\n");

    return TRUE;
}


/** \brief Manage mouse-leave events on canvas items (satellite pass boxes)
 * \param item The GooCanvasItem that received the signal
 * \param target_item The target if the even (have no idea what this is...)
 * \param event Info about the event
 * \param data Pointer to the GtkSkyAtGlance object
 * \return Always TRUE to prevent further propagation of the event.
 *
 * This function is used to be notified when the mouse leaves a satellite
 * pass box.
 */
static gboolean on_mouse_leave         (GooCanvasItem    *item,
                                        GooCanvasItem    *target_item,
                                        GdkEventCrossing *event,
                                        gpointer          data)
{
    //g_print("Mouse leave\n");

    return TRUE;
}





/** \brief Convert time value to x position.
 *  \param skg The GtkSkyGlance widget.
 *  \param t Julian dateuser is presented with brief info about the
 * satellite pass and a suggestion to click on the box for more info.
 *  \return X coordinate.
 *
 * No error checking is made to ensure that we are within visible range.
 *
 */
static gdouble t2x (GtkSkyGlance *skg, gdouble t)
{
    gdouble frac;

    frac = (t - skg->ts) / (skg->te - skg->ts);

    return (skg->x0 + frac * skg->w);

}


/** \brief Convert x coordinate to Julian date.
 *  \param skg The GtkSkyGlance widget.
 *  \param x The X coordinate.
 *  \return The Julian date corresponding to X.
 *
 * No error checking is made to ensure that we are within visible range.
 *
 */
static gdouble x2t (GtkSkyGlance *skg, gdouble x)
{
    gdouble frac;

    frac = (x - skg->x0) / skg->w;

    return (skg->ts + frac * (skg->te - skg->ts));
}


/** \brief Fetch the basic colour and add alpha channel */
static void get_colours (guint i, guint *bcol, guint *fcol)
{
    guint tmp;

    /* ensure that we are within 1..10 */
    i = i % 10;

    switch (i) {

    case 0:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_01);
        break;

    case 1:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_02);
        break;

    case 2:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_03);
        break;

    case 3:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_04);
        break;

    case 4:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_05);
        break;

    case 5:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_06);
        break;

    case 6:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_07);
        break;

    case 7:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_08);
        break;

    case 8:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_09);
        break;

    case 9:
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_10);
        break;

    default:
        sat_log_log (SAT_LOG_LEVEL_BUG,
                        _("%s:%d: Colour index out of valid range (%d)"),
                        __FILE__, __LINE__, i);
        tmp = sat_cfg_get_int (SAT_CFG_INT_SKYATGL_COL_01);
        break;
    }

    /* border colour is solid with no tranparency */
    *bcol = (tmp * 0x100) | 0xFF;

    /* fill colour is slightly transparent */
    *fcol = (tmp * 0x100) | 0xA0;
}


/** \brief Create canvas items for a satellite
 *  \param key Pointer to the hash key (catnum of sat)
 *  \param value Pointer to the current satellite.
 *  \param data Pointer to the GtkSkyGlance object.
 *
 * This function is called by g_hash_table_foreach with each satellite in the
 * satellite hash table. It gets the passes for the current satellite and creates
 * the corresponding canvas items.
 */
static void
create_sat (gpointer key, gpointer value, gpointer data)
{
    sat_t              *sat = SAT(value);
    GtkSkyGlance       *skg = GTK_SKY_GLANCE(data);
    GSList             *passes = NULL;
    gdouble             maxdt;
    guint               i,n;
    pass_t             *tmppass = NULL;
    sky_pass_t         *skypass;
    guint               bcol,fcol;          /* colours */
    GooCanvasItemModel *root;
    GooCanvasItemModel *lab;

    /* tooltips vars */
    gchar *tooltip; /* the complete tooltips string */
    gchar *aosstr;  /* AOS time string */
    gchar *losstr;  /* LOS time string */
    gchar *tcastr;  /* TCA time string */


    /* FIXME: 
        Include current pass if sat is up now
    */

    /* check that we didn't exceed 10 sats */
    /*      if (++skg->satcnt > 10) { */
    /*           return; */
    /*      } */

    /* get canvas root */
    root = goo_canvas_get_root_item_model (GOO_CANVAS (skg->canvas));

    get_colours (skg->satcnt++, &bcol, &fcol);

    maxdt = skg->te - skg->ts;

    /* get passes for satellite */
    passes = get_passes (sat, skg->qth, skg->ts, maxdt, 10); 
    n = g_slist_length (passes);
    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                    _("%s:%d: %s has %d passes within %.4f days\n"),
                    __FILE__, __LINE__, sat->nickname, n, maxdt);

    /* add sky_pass_t items to skg->passes */
    if (passes != NULL) {

        /* add pass items */
        for (i = 0; i < n; i++) {
            
            skypass = g_try_new (sky_pass_t, 1);

            if (skypass != NULL) {

                /* create pass structure items */
                skypass->catnum = sat->tle.catnr;
                tmppass = (pass_t *) g_slist_nth_data (passes, i);
                skypass->pass = copy_pass (tmppass);

                aosstr = time_to_str (skypass->pass->aos);
                losstr = time_to_str (skypass->pass->los);
                tcastr = time_to_str (skypass->pass->tca);

                /* box tooltip will contain pass summary */
                tooltip = g_strdup_printf("<big><b>%s</b>\n</big>\n"\
                                          "<tt>AOS: %s  Az:%.0f\302\260\n" \
                                          "TCA: %s  Az:%.0f\302\260 / El:%.1f\302\260\n" \
                                          "LOS: %s  Az:%.0f\302\260</tt>\n" \
                                          "\n<i>Click for details</i>",
                                          skypass->pass->satname,
                                          aosstr, skypass->pass->aos_az,
                                          tcastr, skypass->pass->maxel_az, skypass->pass->max_el,
                                          losstr, skypass->pass->los_az);

                g_free (aosstr);
                g_free (losstr);
                g_free (tcastr);

                skypass->box = goo_canvas_rect_model_new (root, 10, 10, 20, 20, /* dummy coordinates */
                                                          "stroke-color-rgba", bcol,
                                                          "fill-color-rgba", fcol,
                                                          "line-width", 1.0,
                                                          "antialias", CAIRO_ANTIALIAS_NONE,
                                                          "tooltip", tooltip,
                                                          NULL);
                g_free (tooltip);

                /* store this pass in list */
                skg->passes = g_slist_append (skg->passes, skypass);

                /* store a pointer to the pass data in the GooCanvasItem so that we
                   can access it later during various events, e.g mouse click */
                g_object_set_data (G_OBJECT (skypass->box), "pass", skypass->pass);

            }
            else {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                                _("%s:%d: Could not allocate memory for pass object"),
                                __FILE__, __LINE__);
            }

        }

        free_passes (passes);

        /* add satellite label */
        lab = goo_canvas_text_model_new (root, sat->nickname,
                                            5, 0, -1, GTK_ANCHOR_W,
                                            "font", "Sans 8",
                                            "fill-color-rgba", bcol,
                                            NULL);
        skg->satlab = g_slist_append (skg->satlab, lab);
    }
}



/** \brief Convert "jul_utc" time to formatted string
  * \param julutc The time to convert
  * \return A newly allocated string containing the formatted time (should be freed by caller)
  *
  * \bug This code is duplicated many places.
  */
static gchar *time_to_str (gdouble julutc)
{
    gchar   buff[TIME_FORMAT_MAX_LENGTH];
    gchar  *fmtstr;
    gchar  *timestr;
    time_t  t;
    guint   size;


    /* convert julian date to struct time_t */
    t = (julutc - 2440587.5)*86400.;

    /* format the number */
    fmtstr = sat_cfg_get_str (SAT_CFG_STR_TIME_FORMAT);

    /* format either local time or UTC depending on check box */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME)) {
        size = strftime (buff, TIME_FORMAT_MAX_LENGTH, fmtstr, localtime (&t));
    }
    else {
        size = strftime (buff, TIME_FORMAT_MAX_LENGTH, fmtstr, gmtime (&t));
    }

    g_free (fmtstr);


    if (size == 0)
        /* size > MAX_LENGTH */
        buff[TIME_FORMAT_MAX_LENGTH-1] = '\0';

    timestr = g_strdup (buff);


    return timestr;
}
