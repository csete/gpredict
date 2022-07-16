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

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <goocanvas.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-azel-plot.h"
#include "gtk-sat-data.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

#define AZEL_DEFAULT_SIZE 300
#define AZEL_X_MARGIN 40
#define AZEL_Y_MARGIN 40

/* extra size for line outside 0 deg circle (inside margin) */
#define AZEL_LINE_EXTRA 5

#define MARKER_SIZE 5


static GtkBoxClass *parent_class = NULL;

static void gtk_azel_plot_destroy(GtkWidget * widget)
{
    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_azel_plot_class_init(GtkAzelPlotClass * class,
				     gpointer class_data)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    (void)class_data;

    widget_class->destroy = gtk_azel_plot_destroy;
    parent_class = g_type_class_peek_parent(class);

}

static void gtk_azel_plot_init(GtkAzelPlot * azel,
			       gpointer g_class)
{
    (void)g_class;

    azel->qth = NULL;
    azel->width = 0;
    azel->height = 0;
    azel->x0 = 0;
    azel->y0 = 0;
    azel->xmax = 0;
    azel->ymax = 0;
    azel->maxaz = 0.0;
    azel->qthinfo = FALSE;
    azel->cursinfo = FALSE;
    azel->extratick = FALSE;
}

GType gtk_azel_plot_get_type()
{
    static GType    gtk_azel_plot_type = 0;

    if (!gtk_azel_plot_type)
    {
        static const GTypeInfo gtk_azel_plot_info = {
            sizeof(GtkAzelPlotClass),
            NULL,               /* base init */
            NULL,               /* base finalise */
            (GClassInitFunc) gtk_azel_plot_class_init,
            NULL,               /* class finalise */
            NULL,               /* class data */
            sizeof(GtkAzelPlot),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_azel_plot_init,
            NULL
        };

        gtk_azel_plot_type = g_type_register_static(GTK_TYPE_BOX,
                                                    "GtkAzelPlot",
                                                    &gtk_azel_plot_info, 0);
    }

    return gtk_azel_plot_type;
}

static void az_to_xy(GtkAzelPlot * p, gdouble t, gdouble az, gdouble * x,
                     gdouble * y)
{
    gdouble         tpp;        /* time per pixel */
    gdouble         dpp;        /* degrees per pixel */

    /* time */
    tpp = (p->pass->los - p->pass->aos) / (p->xmax - p->x0);
    *x = p->x0 + (t - p->pass->aos) / tpp;

    /* Az */
    dpp = (gdouble) (p->maxaz / (p->y0 - p->ymax));
    *y = (gdouble) (p->y0 - az / dpp);
}

static void el_to_xy(GtkAzelPlot * p, gdouble t, gdouble el, gdouble * x,
                     gdouble * y)
{
    gdouble         tpp;        /* time per pixel */
    gdouble         dpp;        /* degrees per pixel */

    /* time */
    tpp = (p->pass->los - p->pass->aos) / (p->xmax - p->x0);
    *x = p->x0 + (t - p->pass->aos) / tpp;

    /* Az */
    dpp = (gdouble) (90.0 / (p->y0 - p->ymax));
    *y = (gdouble) (p->y0 - el / dpp);
}

/**
 * Manage new size allocation.
 *
 * This function is called when the canvas receives a new size allocation,
 * e.g. when the container is re-sized. The function re-calculates the graph
 * dimensions based on the new canvas size.
 */
static void size_allocate_cb(GtkWidget * widget, GtkAllocation * allocation,
                             gpointer data)
{
    GtkAzelPlot    *azel;
    GooCanvasPoints *pts;
    gdouble         dx, dy;
    gdouble         xstep, ystep;
    guint           i, n;
    pass_detail_t  *detail;

    if (gtk_widget_get_realized(widget))
    {
        /* get graph dimensions */
        azel = GTK_AZEL_PLOT(data);

        azel->width = allocation->width;
        azel->height = allocation->height;

        goo_canvas_set_bounds(GOO_CANVAS(GTK_AZEL_PLOT(azel)->canvas), 0, 0,
                              azel->width, azel->height);

        /* update coordinate system */
        azel->x0 = AZEL_X_MARGIN;
        azel->xmax = azel->width - AZEL_X_MARGIN;
        azel->y0 = azel->height - AZEL_Y_MARGIN;
        azel->ymax = AZEL_Y_MARGIN;

        /* background item */
        g_object_set(azel->bgd, "width", (gdouble) azel->width,
                     "height", (gdouble) azel->height, NULL);

        /* frame */
        g_object_set(azel->frame,
                     "x", (gdouble) azel->x0,
                     "y", (gdouble) azel->ymax,
                     "width", (gdouble) (azel->xmax - azel->x0),
                     "height", (gdouble) (azel->y0 - AZEL_Y_MARGIN), NULL);

        xstep = (azel->xmax - azel->x0) / (AZEL_PLOT_NUM_TICKS + 1);
        ystep = (azel->y0 - azel->ymax) / (AZEL_PLOT_NUM_TICKS + 1);

        /* tick marks */
        for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++)
        {
            pts = goo_canvas_points_new(2);

            /* bottom x tick marks */
            pts->coords[0] = (gdouble) (azel->x0 + (i + 1) * xstep);
            pts->coords[1] = (gdouble) azel->y0;
            pts->coords[2] = (gdouble) (azel->x0 + (i + 1) * xstep);
            pts->coords[3] = (gdouble) (azel->y0 - MARKER_SIZE);
            g_object_set(azel->xticksb[i], "points", pts, NULL);

            /* top x tick marks */
            pts->coords[0] = (gdouble) (azel->x0 + (i + 1) * xstep);
            pts->coords[1] = (gdouble) azel->ymax;
            pts->coords[2] = (gdouble) (azel->x0 + (i + 1) * xstep);
            pts->coords[3] = (gdouble) (azel->ymax + MARKER_SIZE);
            g_object_set(azel->xtickst[i], "points", pts, NULL);

            /* x tick labels */
            g_object_set(azel->xlab[i],
                         "x", (gfloat) (azel->x0 + (i + 1) * xstep),
                         "y", (gfloat) (azel->y0 + 5.0), NULL);

            /* left y tick marks */
            pts->coords[0] = (gdouble) (gdouble) azel->x0;
            pts->coords[1] = (gdouble) (gdouble) (azel->y0 - (i + 1) * ystep);
            pts->coords[2] = (gdouble) (gdouble) (azel->x0 + MARKER_SIZE);
            pts->coords[3] = (gdouble) (gdouble) (azel->y0 - (i + 1) * ystep);
            g_object_set(azel->yticksl[i], "points", pts, NULL);

            /* right y tick marks */
            pts->coords[0] = (gdouble) azel->xmax;
            pts->coords[1] = (gdouble) (azel->y0 - (i + 1) * ystep);
            pts->coords[2] = (gdouble) (azel->xmax - MARKER_SIZE);
            pts->coords[3] = (gdouble) (azel->y0 - (i + 1) * ystep);
            g_object_set(azel->yticksr[i], "points", pts, NULL);

            goo_canvas_points_unref(pts);

            /* Az tick labels */
            g_object_set(azel->azlab[i],
                         "x", (gfloat) (azel->x0 - 5),
                         "y", (gfloat) (azel->y0 - (i + 1) * ystep), NULL);

            /* El tick labels */
            g_object_set(azel->ellab[i],
                         "x", (gfloat) (azel->xmax + 5),
                         "y", (gfloat) (azel->y0 - (i + 1) * ystep), NULL);
        }

        /* Az legend */
        g_object_set(azel->azleg,
                     "x", (gfloat) (azel->x0 - 7),
                     "y", (gfloat) (azel->ymax), NULL);

        /* El legend */
        g_object_set(azel->elleg,
                     "x", (gfloat) (azel->xmax + 7),
                     "y", (gfloat) (azel->ymax), NULL);

        /* x legend */
        g_object_set(azel->xleg,
                     "x", (gfloat) (azel->x0 + (azel->xmax - azel->x0) / 2),
                     "y", (gfloat) (azel->height - 5), NULL);

        /* Az graph */
        n = g_slist_length(azel->pass->details);
        pts = goo_canvas_points_new(n);

        for (i = 0; i < n; i++)
        {
            detail = PASS_DETAIL(g_slist_nth_data(azel->pass->details, i));
            az_to_xy(azel, detail->time, detail->az, &dx, &dy);
            pts->coords[2 * i] = dx;
            pts->coords[2 * i + 1] = dy;
        }
        pts->coords[0] = azel->x0;
        pts->coords[2 * n - 2] = azel->xmax;
        g_object_set(azel->azg, "points", pts, NULL);
        goo_canvas_points_unref(pts);

        /* El graph */
        n = g_slist_length(azel->pass->details);
        pts = goo_canvas_points_new(n);

        for (i = 0; i < n; i++)
        {
            detail = PASS_DETAIL(g_slist_nth_data(azel->pass->details, i));
            el_to_xy(azel, detail->time, detail->el, &dx, &dy);
            pts->coords[2 * i] = dx;
            pts->coords[2 * i + 1] = dy;
        }
        pts->coords[1] = azel->y0;
        pts->coords[2 * n - 1] = azel->y0;
        g_object_set(azel->elg, "points", pts, NULL);
        goo_canvas_points_unref(pts);

        /* cursor track */
        g_object_set(azel->curs,
                     "x", (gfloat) (azel->x0 + (azel->xmax - azel->x0) / 2),
                     "y", (gfloat) 5.0, NULL);
    }
}


/** Convert canvas based coordinates to Az/El. */
static void xy_to_graph(GtkAzelPlot * p, gfloat x, gfloat y, gdouble * t,
                        gdouble * az, gdouble * el)
{
    gdouble         tpp;        /* time per pixel */
    gdouble         dpp;        /* degrees per pixel */

    /* time */
    tpp = (p->pass->los - p->pass->aos) / (p->xmax - p->x0);
    *t = p->pass->aos + tpp * (x - p->x0);

    /* az */
    dpp = p->maxaz / (p->y0 - p->ymax);
    *az = dpp * (p->y0 - y);

    /* el */
    dpp = 90.0 / (p->y0 - p->ymax);
    *el = dpp * (p->y0 - y);

}

/** Manage mouse motion events. */
static gboolean on_motion_notify(GooCanvasItem * item,
                                 GooCanvasItem * target,
                                 GdkEventMotion * event, gpointer data)
{
    GtkAzelPlot    *azel = GTK_AZEL_PLOT(data);
    gdouble         t, az, el;
    gfloat          x, y;
    gchar          *text;
    gchar           buff[10];

    (void)item;                 /* avoid unused warning compiler warning. */
    (void)target;               /* avoid unused warning compiler warning. */

    if (azel->cursinfo)
    {
        /* get (x,y) */
        x = event->x;
        y = event->y;

        /* get (t,az,el) */

        /* show vertical line at that time */

        /* print (t,az,el) */
        if ((x > azel->x0) && (x < azel->xmax) &&
            (y > azel->ymax) && (y < azel->y0))
        {

            xy_to_graph(azel, x, y, &t, &az, &el);

            daynum_to_str(buff, 10, "%H:%M:%S", t);

            /* cursor track */
            text =
                g_strdup_printf("T: %s, AZ: %.0f\302\260, EL: %.0f\302\260",
                                buff, az, el);
            g_object_set(azel->curs, "text", text, NULL);
            g_free(text);
        }
        else
        {
            g_object_set(azel->curs, "text", "", NULL);
        }
    }

    return TRUE;
}

/**
 * Finish canvas item setup.
 * @param canvas 
 * @param item 
 * @param model 
 * @param data Pointer to the GtkAzelPlot object.
 *
 * This function is called when a canvas item is created. Its purpose is to connect
 * the corresponding signals to the created items.
 */
static void on_item_created(GooCanvas * canvas,
                            GooCanvasItem * item,
                            GooCanvasItemModel * model, gpointer data)
{
    (void)canvas;

    if (!goo_canvas_item_model_get_parent(model))
    {
        /* root item / canvas */
        g_signal_connect(item, "motion_notify_event",
                         G_CALLBACK(on_motion_notify), data);
    }
}


/**
 * Manage canvas realise signals.
 *
 * This function is used to re-initialise the graph dimensions when
 * the graph is realized, i.e. displayed for the first time. This is
 * necessary in order to compensate for missing "re-allocate" signals for
 * graphs that have not yet been realised, e.g. when opening several module
 */
static void on_canvas_realized(GtkWidget * canvas, gpointer data)
{
    GtkAllocation   aloc;

    gtk_widget_get_allocation(canvas, &aloc);
    size_allocate_cb(canvas, &aloc, data);

}

static GooCanvasItemModel *create_canvas_model(GtkAzelPlot * azel)
{
    GooCanvasItemModel *root;
    guint32         col;
    guint           i;
    gdouble         xstep, ystep;
    gdouble         t, az, el;
    gchar           buff[7];
    gchar          *txt;

    root = goo_canvas_group_model_new(NULL, NULL);

    /* graph dimensions */
    azel->width = AZEL_DEFAULT_SIZE;
    azel->height = AZEL_DEFAULT_SIZE;

    /* update coordinate system */
    azel->x0 = AZEL_X_MARGIN;
    azel->xmax = azel->width - AZEL_X_MARGIN;
    azel->y0 = azel->height - AZEL_Y_MARGIN;
    azel->ymax = AZEL_Y_MARGIN;

    /* background item */
    azel->bgd = goo_canvas_rect_model_new(root, 0.0, 0.0,
                                          azel->width, azel->height,
                                          "fill-color-rgba", 0xFFFFFFFF,
                                          "stroke-color-rgba", 0xFFFFFFFF,
                                          NULL);

    col = sat_cfg_get_int(SAT_CFG_INT_POLAR_AXIS_COL);

    /* frame */
    azel->frame = goo_canvas_rect_model_new(root,
                                            (gdouble) azel->x0,
                                            (gdouble) azel->ymax,
                                            (gdouble) (azel->xmax - azel->x0),
                                            (gdouble) (azel->y0 -
                                                       AZEL_Y_MARGIN),
                                            "stroke-color-rgba", 0x000000FF,
                                            "line-cap", CAIRO_LINE_CAP_SQUARE,
                                            "line-join", CAIRO_LINE_JOIN_MITER,
                                            "line-width", 1.0, NULL);

    xstep = (azel->xmax - azel->x0) / (AZEL_PLOT_NUM_TICKS + 1);
    ystep = (azel->y0 - azel->ymax) / (AZEL_PLOT_NUM_TICKS + 1);

// We use these defines to shorten the function names (indent)
#define MKLINE goo_canvas_polyline_model_new_line
#define MKTEXT goo_canvas_text_model_new

    /* tick marks */
    for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++)
    {
        /* bottom x tick marks */
        azel->xticksb[i] = MKLINE(root,
                                  (gdouble) (azel->x0 + (i + 1) * xstep),
                                  (gdouble) azel->y0,
                                  (gdouble) (azel->x0 + (i + 1) * xstep),
                                  (gdouble) (azel->y0 - MARKER_SIZE),
                                  "fill-color-rgba", col,
                                  "line-cap", CAIRO_LINE_CAP_SQUARE,
                                  "line-join", CAIRO_LINE_JOIN_MITER,
                                  "line-width", 1.0, NULL);

        /* top x tick marks */
        azel->xtickst[i] = MKLINE(root,
                                  (gdouble) (azel->x0 + (i + 1) * xstep),
                                  (gdouble) azel->ymax,
                                  (gdouble) (azel->x0 + (i + 1) * xstep),
                                  (gdouble) (azel->ymax + MARKER_SIZE),
                                  "fill-color-rgba", col,
                                  "line-cap", CAIRO_LINE_CAP_SQUARE,
                                  "line-join", CAIRO_LINE_JOIN_MITER,
                                  "line-width", 1.0, NULL);

        /* x tick labels */
        /* get time */
        xy_to_graph(azel, azel->x0 + (i + 1) * xstep, 0.0, &t, &az, &el);

        daynum_to_str(buff, 7, "%H:%M", t);

        azel->xlab[i] = MKTEXT(root, buff,
                               (gfloat) (azel->x0 + (i + 1) * xstep),
                               (gfloat) (azel->y0 + 5),
                               -1,
                               GOO_CANVAS_ANCHOR_N,
                               "font", "Sans 8",
                               "fill-color-rgba", 0x000000FF, NULL);

        /* left y tick marks */
        azel->yticksl[i] = MKLINE(root,
                                  (gdouble) azel->x0,
                                  (gdouble) (azel->y0 - (i + 1) * ystep),
                                  (gdouble) (azel->x0 + MARKER_SIZE),
                                  (gdouble) (azel->y0 - (i + 1) * ystep),
                                  "fill-color-rgba", col,
                                  "line-cap", CAIRO_LINE_CAP_SQUARE,
                                  "line-join", CAIRO_LINE_JOIN_MITER,
                                  "line-width", 1.0, NULL);

        /* right y tick marks */
        azel->yticksr[i] = MKLINE(root,
                                  (gdouble) azel->xmax,
                                  (gdouble) (azel->y0 - (i + 1) * ystep),
                                  (gdouble) (azel->xmax - MARKER_SIZE),
                                  (gdouble) (azel->y0 - (i + 1) * ystep),
                                  "fill-color-rgba", col,
                                  "line-cap", CAIRO_LINE_CAP_SQUARE,
                                  "line-join", CAIRO_LINE_JOIN_MITER,
                                  "line-width", 1.0, NULL);

        /* Az tick labels */
        txt = g_strdup_printf("%.0f\302\260",
                              azel->maxaz / (AZEL_PLOT_NUM_TICKS + 1) * (i +
                                                                         1));
        azel->azlab[i] =
            MKTEXT(root, txt, (gfloat) (azel->x0 - 5),
                   (gfloat) (azel->y0 - (i + 1) * ystep), -1,
                   GOO_CANVAS_ANCHOR_E, "font", "Sans 8",
                   "fill-color-rgba", 0x0000BFFF, NULL);
        g_free(txt);

        /* El tick labels */
        txt =
            g_strdup_printf("%.0f\302\260",
                            90.0 / (AZEL_PLOT_NUM_TICKS + 1) * (i + 1));
        azel->ellab[i] =
            MKTEXT(root, txt, (gfloat) (azel->xmax + 5),
                   (gfloat) (azel->y0 - (i + 1) * ystep), -1,
                   GOO_CANVAS_ANCHOR_W,
                   "font", "Sans 8", "fill-color-rgba", 0xBF0000FF, NULL);
        g_free(txt);
    }

    /* x legend */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_LOCAL_TIME))
    {
        azel->xleg = MKTEXT(root, _("Local Time"),
                            (gfloat) (azel->x0 + (azel->xmax - azel->x0) / 2),
                            (gfloat) (azel->height - 5),
                            -1,
                            GOO_CANVAS_ANCHOR_S,
                            "font", "Sans 9",
                            "fill-color-rgba", 0x000000FF, NULL);
    }
    else
    {
        azel->xleg = MKTEXT(root, _("UTC"),
                            (gfloat) (azel->x0 + (azel->xmax - azel->x0) / 2),
                            (gfloat) (azel->height - 5),
                            -1,
                            GOO_CANVAS_ANCHOR_S,
                            "font", "Sans 9",
                            "fill-color-rgba", 0x000000FF, NULL);
    }

    /* cursor text in upper right corner */
    azel->curs = MKTEXT(root, "",
                        (gfloat) (azel->x0 + (azel->xmax - azel->x0) / 2),
                        5.0,
                        -1,
                        GOO_CANVAS_ANCHOR_N,
                        "font", "Sans 8", "fill-color-rgba", 0x000000FF, NULL);

    /* Az legend */
    azel->azleg = MKTEXT(root, _("Az"),
                         (gfloat) (azel->x0 - 7),
                         (gfloat) azel->ymax,
                         -1,
                         GOO_CANVAS_ANCHOR_NE,
                         "font", "Sans 9",
                         "fill-color-rgba", 0x0000BFFF, NULL);

    /* El legend */
    azel->elleg = MKTEXT(root, _("El"),
                         (gfloat) (azel->xmax + 7),
                         (gfloat) azel->ymax,
                         -1,
                         GOO_CANVAS_ANCHOR_NW,
                         "font", "Sans 9",
                         "fill-color-rgba", 0xBF0000FF, NULL);

    /* Az graph */
    azel->azg = MKLINE(root, 0, 0, 10, 10,
                       "stroke-color-rgba", 0x0000BFFF,
                       "line-cap", CAIRO_LINE_CAP_SQUARE,
                       "line-join", CAIRO_LINE_JOIN_MITER,
                       "line-width", 1.0, NULL);

    /* El graph */
    azel->elg = MKLINE(root, 30, 30, 40, 40,
                       "stroke-color-rgba", 0xBF0000FF,
                       "fill-color-rgba", 0xBF00001A,
                       "line-cap", CAIRO_LINE_CAP_SQUARE,
                       "line-join", CAIRO_LINE_JOIN_MITER,
                       "line-width", 1.0, NULL);

    return root;
}

/**
 * Create a new GtkAzelPlot widget.
 * @param cfgdata The configuration data of the parent module.
 * @param sats Pointer to the hash table containing the associated satellites.
 * @param qth Pointer to the ground station data.
 */
GtkWidget      *gtk_azel_plot_new(qth_t * qth, pass_t * pass)
{
    GtkAzelPlot    *azel;
    GooCanvasItemModel *root;
    guint           i, n;
    pass_detail_t  *detail;

    azel = GTK_AZEL_PLOT(g_object_new(GTK_TYPE_AZEL_PLOT, NULL));
    azel->qth = qth;
    azel->pass = pass;

    /* get settings */
    azel->qthinfo = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_QTH_INFO);
    azel->extratick = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);
    azel->cursinfo = TRUE;

    /* check maximum Az */
    n = g_slist_length(pass->details);
    for (i = 0; i < n; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(pass->details, i));

        if (detail->az > azel->maxaz)
        {
            azel->maxaz = detail->az;
        }
    }

    if (azel->maxaz > 180.0)
        azel->maxaz = 360.0;
    else
        azel->maxaz = 180.0;

    /* create the canvas */
    azel->canvas = goo_canvas_new();
    gtk_widget_set_size_request(azel->canvas, AZEL_DEFAULT_SIZE,
                                AZEL_DEFAULT_SIZE);
    goo_canvas_set_bounds(GOO_CANVAS(azel->canvas), 0, 0,
                          AZEL_DEFAULT_SIZE, AZEL_DEFAULT_SIZE);

    /* connect size-request signal */
    g_signal_connect(azel->canvas, "size-allocate",
                     G_CALLBACK(size_allocate_cb), azel);
    g_signal_connect(azel->canvas, "item_created",
                     G_CALLBACK(on_item_created), azel);
    g_signal_connect_after(azel->canvas, "realize",
                           G_CALLBACK(on_canvas_realized), azel);

    gtk_widget_show(azel->canvas);

    /* Create the canvas model */
    root = create_canvas_model(azel);
    goo_canvas_set_root_item_model(GOO_CANVAS(azel->canvas), root);
    g_object_unref(root);
    gtk_box_pack_start(GTK_BOX(azel), azel->canvas, TRUE, TRUE, 0);

    return GTK_WIDGET(azel);
}
