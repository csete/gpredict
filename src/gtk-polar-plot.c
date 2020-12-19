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
 * Polar Plot Widget.
 * @ingroup widgets
 *
 * GtkPolarPlot is a graphical widget that can display a satellite pass
 * in an Az/El polar plot. The widget was originally created to display
 * a single satellite pass in the detailed pass prediction dialog.
 * 
 * Later, a few utility functions were added in order to make the GtkPolarPlot
 * more dynamic and useful in other contexts too. In addition to a satellite
 * pass, GtkPolarPlot can show a target object (small square), a target
 * position marker (thread), and a current position marker (small circle).
 * These three objects are very useful in the rotator control window.
 */
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <goocanvas.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-polar-plot.h"
#include "gtk-sat-data.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

#define POLV_DEFAULT_SIZE 200
#define POLV_DEFAULT_MARGIN 20

/* extra size for line outside 0 deg circle (inside margin) */
#define POLV_LINE_EXTRA 5

#define MARKER_SIZE_HALF 2


static GtkVBoxClass *parent_class = NULL;

static void gtk_polar_plot_init(GtkPolarPlot * polview,
				gpointer g_class)
{
    (void)g_class;

    polview->qth = NULL;
    polview->pass = NULL;
    polview->size = 0;
    polview->r = 0;
    polview->cx = 0;
    polview->cy = 0;
    polview->swap = 0;
    polview->qthinfo = FALSE;
    polview->cursinfo = FALSE;
    polview->extratick = FALSE;
    polview->target = NULL;
}

static void gtk_polar_plot_destroy(GtkWidget * widget)
{
    if (GTK_POLAR_PLOT(widget)->pass != NULL)
    {
        free_pass(GTK_POLAR_PLOT(widget)->pass);
        GTK_POLAR_PLOT(widget)->pass = NULL;
    }

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_polar_plot_class_init(GtkPolarPlotClass * class,
				      gpointer class_data)
{
    GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

    (void)class_data;

    parent_class = g_type_class_peek_parent(class);
    widget_class->destroy = gtk_polar_plot_destroy;
}

GType gtk_polar_plot_get_type()
{
    static GType    gtk_polar_plot_type = 0;

    if (!gtk_polar_plot_type)
    {
        static const GTypeInfo gtk_polar_plot_info = {
            sizeof(GtkPolarPlotClass),
            NULL,               /* base init */
            NULL,               /* base finalise */
            (GClassInitFunc) gtk_polar_plot_class_init,
            NULL,               /* class finalise */
            NULL,               /* class data */
            sizeof(GtkPolarPlot),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_polar_plot_init,
            NULL
        };

        gtk_polar_plot_type = g_type_register_static(GTK_TYPE_BOX,
                                                     "GtkPolarPlot",
                                                     &gtk_polar_plot_info, 0);
    }

    return gtk_polar_plot_type;
}

static GooCanvasItemModel *create_time_tick(GtkPolarPlot * pv, gdouble time,
                                            gfloat x, gfloat y)
{
    GooCanvasItemModel *item;
    GooCanvasAnchorType     anchor;
    GooCanvasItemModel *root;
    guint32         col;
    gchar           buff[6];

    root = goo_canvas_get_root_item_model(GOO_CANVAS(pv->canvas));

    col = sat_cfg_get_int(SAT_CFG_INT_POLAR_TRACK_COL);

    daynum_to_str(buff, 6, "%H:%M", time);

    if (x > pv->cx)
    {
        anchor = GOO_CANVAS_ANCHOR_EAST;
        x -= 5;
    }
    else
    {
        anchor = GOO_CANVAS_ANCHOR_WEST;
        x += 5;
    }

    item = goo_canvas_text_model_new(root, buff,
                                     (gdouble) x, (gdouble) y,
                                     -1, anchor,
                                     "font", "Sans 7",
                                     "fill-color-rgba", col, NULL);

    return item;
}

/** Convert Az/El to canvas based XY coordinates. */
static void azel_to_xy(GtkPolarPlot * p, gdouble az, gdouble el,
                       gfloat * x, gfloat * y)
{
    gdouble         rel;

    if (el < 0.0)
    {
        /* FIXME: generate bug report */
        *x = 0.0;
        *y = 0.0;

        return;
    }

    /* convert angles to radians */
    az = de2ra * az;
    el = de2ra * el;

    /* radius @ el */
    rel = p->r - (2 * p->r * el) / M_PI;

    switch (p->swap)
    {
    case POLAR_PLOT_NWSE:
        az = 2 * M_PI - az;
        break;

    case POLAR_PLOT_SENW:
        az = M_PI - az;
        break;

    case POLAR_PLOT_SWNE:
        az = M_PI + az;
        break;

    default:
        break;
    }

    *x = (gfloat) (p->cx + rel * sin(az));
    *y = (gfloat) (p->cy - rel * cos(az));
}

/** Convert canvas based coordinates to Az/El. */
static void xy_to_azel(GtkPolarPlot * p, gfloat x, gfloat y,
                       gfloat * az, gfloat * el)
{
    gfloat          rel;

    /* distance from center to cursor */
    rel = p->r - sqrt((x - p->cx) * (x - p->cx) + (y - p->cy) * (y - p->cy));

    /* scale according to p->r = 90 deg */
    *el = 90.0 * rel / p->r;

    if (x >= p->cx)
    {
        /* 1. and 2. quadrant */
        *az = atan2(x - p->cx, p->cy - y) / de2ra;
    }
    else
    {
        /* 3 and 4. quadrant */
        *az = 360 + atan2(x - p->cx, p->cy - y) / de2ra;
    }

    /* correct for orientation */
    switch (p->swap)
    {
    case POLAR_PLOT_NWSE:
        *az = 360.0 - *az;
        break;

    case POLAR_PLOT_SENW:
        if (*az <= 180)
            *az = 180.0 - *az;
        else
            *az = 540.0 - *az;
        break;

    case POLAR_PLOT_SWNE:
        if (*az >= 180.0)
            *az = *az - 180.0;
        else
            *az = 180.0 + *az;
        break;

    default:
        break;
    }
}

static void create_track(GtkPolarPlot * pv)
{
    guint           i;
    GooCanvasItemModel *root;
    pass_detail_t  *detail;
    guint           num;
    GooCanvasPoints *points;
    gfloat          x, y;
    guint32         col;
    guint           tres, ttidx;

    root = goo_canvas_get_root_item_model(GOO_CANVAS(pv->canvas));

    /* create points */
    num = g_slist_length(pv->pass->details);

    /* time resolution for time ticks; we need
       3 additional points to AOS and LOS ticks.
     */
    tres = (num - 2) / (TRACK_TICK_NUM - 1);

    points = goo_canvas_points_new(num);

    /* first point should be (aos_az,0.0) */
    azel_to_xy(pv, pv->pass->aos_az, 0.0, &x, &y);
    points->coords[0] = (double)x;
    points->coords[1] = (double)y;
    pv->trtick[0] = create_time_tick(pv, pv->pass->aos, x, y);

    ttidx = 1;

    for (i = 1; i < num - 1; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(pv->pass->details, i));
        if (detail->el >= 0.0)
            azel_to_xy(pv, detail->az, detail->el, &x, &y);
        points->coords[2 * i] = (double)x;
        points->coords[2 * i + 1] = (double)y;

        if (!(i % tres))
        {
            if (ttidx < TRACK_TICK_NUM)
                /* create a time tick */
                pv->trtick[ttidx] = create_time_tick(pv, detail->time, x, y);
            ttidx++;
        }
    }

    /* last point should be (los_az, 0.0)  */
    azel_to_xy(pv, pv->pass->los_az, 0.0, &x, &y);
    points->coords[2 * (num - 1)] = (double)x;
    points->coords[2 * (num - 1) + 1] = (double)y;

    /* create poly-line */
    col = sat_cfg_get_int(SAT_CFG_INT_POLAR_TRACK_COL);

    pv->track = goo_canvas_polyline_model_new(root, FALSE, 0,
                                              "points", points,
                                              "line-width", 1.0,
                                              "stroke-color-rgba", col,
                                              "line-cap",
                                              CAIRO_LINE_CAP_SQUARE,
                                              "line-join",
                                              CAIRO_LINE_JOIN_MITER, NULL);
    goo_canvas_points_unref(points);
}

/**
 * Transform pole coordinates.
 *
 * This function transforms the pols coordinates (x,y) taking into account
 * the orientation of the polar plot.
 */
static void
correct_pole_coor(GtkPolarPlot * polv,
                  polar_plot_pole_t pole,
                  gfloat * x, gfloat * y, GooCanvasAnchorType * anch)
{
    switch (pole)
    {
    case POLAR_PLOT_POLE_N:
        if ((polv->swap == POLAR_PLOT_SENW) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* North and South are swapped */
            *y = *y + POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_NORTH;
        }
        else
        {
            *y = *y - POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_SOUTH;
        }
        break;

    case POLAR_PLOT_POLE_E:
        if ((polv->swap == POLAR_PLOT_NWSE) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* East and West are swapped */
            *x = *x - POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_EAST;
        }
        else
        {
            *x = *x + POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_WEST;
        }
        break;

    case POLAR_PLOT_POLE_S:
        if ((polv->swap == POLAR_PLOT_SENW) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* North and South are swapped */
            *y = *y - POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_SOUTH;
        }
        else
        {
            *y = *y + POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_NORTH;
        }
        break;

    case POLAR_PLOT_POLE_W:
        if ((polv->swap == POLAR_PLOT_NWSE) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* East and West are swapped */
            *x = *x + POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_WEST;
        }
        else
        {
            *x = *x - POLV_LINE_EXTRA;
            *anch = GOO_CANVAS_ANCHOR_EAST;
        }
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Incorrect polar plot orientation."),
                    __FILE__, __LINE__);
        break;
    }
}

static GooCanvasItemModel *create_canvas_model(GtkPolarPlot * polv)
{
    GooCanvasItemModel *root;
    gfloat          x, y;
    guint32         col;
    GooCanvasAnchorType     anch = GOO_CANVAS_ANCHOR_CENTER;

    root = goo_canvas_group_model_new(NULL, NULL);

    polv->bgd = goo_canvas_rect_model_new(root, 0.0, 0.0,
                                          POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE,
                                          "fill-color-rgba", 0xFFFFFFFF,
                                          "stroke-color-rgba", 0xFFFFFFFF,
                                          NULL);

    /* graph dimensions */
    polv->size = POLV_DEFAULT_SIZE;
    polv->r = (polv->size / 2) - POLV_DEFAULT_MARGIN;
    polv->cx = POLV_DEFAULT_SIZE / 2;
    polv->cy = POLV_DEFAULT_SIZE / 2;

    col = sat_cfg_get_int(SAT_CFG_INT_POLAR_AXIS_COL);

    /* Add elevation circles at 0, 30 and 60 deg */
    polv->C00 = goo_canvas_ellipse_model_new(root,
                                             polv->cx, polv->cy,
                                             polv->r, polv->r,
                                             "line-width", 1.0,
                                             "stroke-color-rgba", col, NULL);

    polv->C30 = goo_canvas_ellipse_model_new(root,
                                             polv->cx, polv->cy,
                                             0.6667 * polv->r,
                                             0.6667 * polv->r, "line-width",
                                             1.0, "stroke-color-rgba", col,
                                             NULL);

    polv->C60 = goo_canvas_ellipse_model_new(root,
                                             polv->cx, polv->cy,
                                             0.333 * polv->r, 0.3333 * polv->r,
                                             "line-width", 1.0,
                                             "stroke-color-rgba", col, NULL);

    /* add horixontal and vertical guidance lines */
    polv->hl = goo_canvas_polyline_model_new_line(root,
                                                  polv->cx - polv->r -
                                                  POLV_LINE_EXTRA, polv->cy,
                                                  polv->cx + polv->r +
                                                  POLV_LINE_EXTRA, polv->cy,
                                                  "stroke-color-rgba", col,
                                                  "line-width", 1.0, NULL);
    polv->vl =
        goo_canvas_polyline_model_new_line(root, polv->cx,
                                           polv->cy - polv->r -
                                           POLV_LINE_EXTRA, polv->cx,
                                           polv->cy + polv->r +
                                           POLV_LINE_EXTRA,
                                           "stroke-color-rgba", col,
                                           "line-width", 1.0, NULL);

    /* N, S, E and W labels.  */
    col = sat_cfg_get_int(SAT_CFG_INT_POLAR_TICK_COL);
    azel_to_xy(polv, 0.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_N, &x, &y, &anch);
    polv->N = goo_canvas_text_model_new(root, _("N"),
                                        x,
                                        y,
                                        -1,
                                        anch,
                                        "font", "Sans 8",
                                        "fill-color-rgba", col, NULL);

    azel_to_xy(polv, 180.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_S, &x, &y, &anch);
    polv->S = goo_canvas_text_model_new(root, _("S"),
                                        x,
                                        y,
                                        -1,
                                        anch,
                                        "font", "Sans 8",
                                        "fill-color-rgba", col, NULL);

    azel_to_xy(polv, 90.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_E, &x, &y, &anch);
    polv->E = goo_canvas_text_model_new(root, _("E"),
                                        x,
                                        y,
                                        -1,
                                        anch,
                                        "font", "Sans 8",
                                        "fill-color-rgba", col, NULL);

    azel_to_xy(polv, 270.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_W, &x, &y, &anch);
    polv->W = goo_canvas_text_model_new(root, _("W"),
                                        x,
                                        y,
                                        -1,
                                        anch,
                                        "font", "Sans 8",
                                        "fill-color-rgba", col, NULL);

    /* cursor text */
    col = sat_cfg_get_int(SAT_CFG_INT_POLAR_INFO_COL);
    polv->curs = goo_canvas_text_model_new(root, "",
                                           polv->cx - polv->r -
                                           2 * POLV_LINE_EXTRA,
                                           polv->cy + polv->r +
                                           POLV_LINE_EXTRA, -1, GOO_CANVAS_ANCHOR_W,
                                           "font", "Sans 8", "fill-color-rgba",
                                           col, NULL);

    /* location info */
    polv->locnam = goo_canvas_text_model_new(root, polv->qth->name,
                                             polv->cx - polv->r -
                                             2 * POLV_LINE_EXTRA,
                                             polv->cy - polv->r -
                                             POLV_LINE_EXTRA, -1,
                                             GOO_CANVAS_ANCHOR_SW, "font", "Sans 8",
                                             "fill-color-rgba", col, NULL);

    return root;
}

/** Update sky track drawing after size allocate. */
static void update_track(GtkPolarPlot * pv)
{
    guint           num, i;
    GooCanvasPoints *points;
    gfloat          x, y;
    pass_detail_t  *detail;
    guint           tres, ttidx;

    /* create points */
    num = g_slist_length(pv->pass->details);

    points = goo_canvas_points_new(num);

    /* first point should be (aos_az,0.0) */
    azel_to_xy(pv, pv->pass->aos_az, 0.0, &x, &y);
    points->coords[0] = (double)x;
    points->coords[1] = (double)y;

    /* time tick 0 */
    g_object_set(pv->trtick[0], "x", (gdouble) x, "y", (gdouble) y, NULL);

    /* time resolution for time ticks; we need
       3 additional points to AOS and LOS ticks.
     */
    tres = (num - 2) / (TRACK_TICK_NUM - 1);
    ttidx = 1;

    for (i = 1; i < num - 1; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(pv->pass->details, i));
        if (detail->el >= 0.0)
            azel_to_xy(pv, detail->az, detail->el, &x, &y);
        points->coords[2 * i] = (double)x;
        points->coords[2 * i + 1] = (double)y;

        if (!(i % tres))
        {
            /* make room between text and track */
            if (x > pv->cx)
                x -= 5;
            else
                x += 5;

            /* update time tick */
            if (ttidx < TRACK_TICK_NUM)
            {
                g_object_set(pv->trtick[ttidx],
                             "x", (gdouble) x, "y", (gdouble) y, NULL);
            }
            ttidx++;
        }
    }

    /* last point should be (los_az, 0.0)  */
    azel_to_xy(pv, pv->pass->los_az, 0.0, &x, &y);
    points->coords[2 * (num - 1)] = (double)x;
    points->coords[2 * (num - 1) + 1] = (double)y;

    g_object_set(pv->track, "points", points, NULL);

    goo_canvas_points_unref(points);
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
    GtkPolarPlot   *polv;
    GooCanvasPoints *prec;
    gfloat          x, y;
    GooCanvasAnchorType     anch = GOO_CANVAS_ANCHOR_CENTER;

    if (gtk_widget_get_realized(widget))
    {
        /* get graph dimensions */
        polv = GTK_POLAR_PLOT(data);

        polv->size = MIN(allocation->width, allocation->height);
        polv->r = (polv->size / 2) - POLV_DEFAULT_MARGIN;
        polv->cx = allocation->width / 2;
        polv->cy = allocation->height / 2;

        goo_canvas_set_bounds(GOO_CANVAS(GTK_POLAR_PLOT(polv)->canvas), 0, 0,
                              allocation->width, allocation->height);

        /* background item */
        g_object_set(polv->bgd, "width", (gdouble) allocation->width,
                     "height", (gdouble) allocation->height, NULL);

        /* update coordinate system */
        g_object_set(polv->C00,
                     "center-x", (gdouble) polv->cx,
                     "center-y", (gdouble) polv->cy,
                     "radius-x", (gdouble) polv->r,
                     "radius-y", (gdouble) polv->r, NULL);
        g_object_set(polv->C30,
                     "center-x", (gdouble) polv->cx,
                     "center-y", (gdouble) polv->cy,
                     "radius-x", (gdouble) 0.6667 * polv->r,
                     "radius-y", (gdouble) 0.6667 * polv->r, NULL);
        g_object_set(polv->C60,
                     "center-x", (gdouble) polv->cx,
                     "center-y", (gdouble) polv->cy,
                     "radius-x", (gdouble) 0.333 * polv->r,
                     "radius-y", (gdouble) 0.333 * polv->r, NULL);

        /* horizontal line */
        prec = goo_canvas_points_new(2);
        prec->coords[0] = polv->cx - polv->r - POLV_LINE_EXTRA;
        prec->coords[1] = polv->cy;
        prec->coords[2] = polv->cx + polv->r + POLV_LINE_EXTRA;
        prec->coords[3] = polv->cy;
        g_object_set(polv->hl, "points", prec, NULL);

        /* vertical line */
        prec->coords[0] = polv->cx;
        prec->coords[1] = polv->cy - polv->r - POLV_LINE_EXTRA;
        prec->coords[2] = polv->cx;
        prec->coords[3] = polv->cy + polv->r + POLV_LINE_EXTRA;
        g_object_set(polv->vl, "points", prec, NULL);

        /* free memory */
        goo_canvas_points_unref(prec);

        /* N/E/S/W */
        azel_to_xy(polv, 0.0, 0.0, &x, &y);
        correct_pole_coor(polv, POLAR_PLOT_POLE_N, &x, &y, &anch);
        g_object_set(polv->N, "x", x, "y", y, NULL);

        azel_to_xy(polv, 90.0, 0.0, &x, &y);
        correct_pole_coor(polv, POLAR_PLOT_POLE_E, &x, &y, &anch);
        g_object_set(polv->E, "x", x, "y", y, NULL);

        azel_to_xy(polv, 180.0, 0.0, &x, &y);
        correct_pole_coor(polv, POLAR_PLOT_POLE_S, &x, &y, &anch);
        g_object_set(polv->S, "x", x, "y", y, NULL);

        azel_to_xy(polv, 270.0, 0.0, &x, &y);
        correct_pole_coor(polv, POLAR_PLOT_POLE_W, &x, &y, &anch);
        g_object_set(polv->W, "x", x, "y", y, NULL);

        /* cursor track */
        g_object_set(polv->curs,
                     "x", (gfloat) (polv->cx - polv->r - 2 * POLV_LINE_EXTRA),
                     "y", (gfloat) (polv->cy + polv->r + POLV_LINE_EXTRA),
                     NULL);

        /* location name */
        g_object_set(polv->locnam,
                     "x", (gfloat) (polv->cx - polv->r - 2 * POLV_LINE_EXTRA),
                     "y", (gfloat) (polv->cy - polv->r - POLV_LINE_EXTRA),
                     NULL);

        /* sky track */
        if (polv->pass != NULL)
            update_track(polv);
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

/** Manage mouse motion events. */
static gboolean on_motion_notify(GooCanvasItem * item, GooCanvasItem * target,
                                 GdkEventMotion * event, gpointer data)
{
    GtkPolarPlot   *polv = GTK_POLAR_PLOT(data);
    gfloat          az, el;
    gchar          *text;

    (void)item;
    (void)target;

    if (polv->cursinfo)
    {
        xy_to_azel(polv, event->x, event->y, &az, &el);

        if (el > 0.0)
        {
            /* cursor track */
            text = g_strdup_printf("AZ %.0f\302\260\nEL %.0f\302\260", az, el);
            g_object_set(polv->curs, "text", text, NULL);
            g_free(text);
        }
        else
        {
            g_object_set(polv->curs, "text", "", NULL);
        }
    }

    return TRUE;
}

/**
 * Finish canvas item setup.
 *
 * This function is called when a canvas item is created. Its purpose is to connect
 * the corresponding signals to the created items.
 */
static void on_item_created(GooCanvas * canvas, GooCanvasItem * item,
                            GooCanvasItemModel * model, gpointer data)
{
    (void)canvas;
    if (!goo_canvas_item_model_get_parent(model))
    {
        /* root item / canvas */
        g_signal_connect(item, "motion_notify_event",
                         (GCallback) on_motion_notify, data);
    }
}

/**
 * Create a new GtkPolarPlot widget.
 *
 * @param qth  Pointer to the QTH.
 * @param pass Pointer to the satellite pass to display. If NULL no
 *             pass will be displayed.
 *
 */
GtkWidget *gtk_polar_plot_new(qth_t * qth, pass_t * pass)
{
    GtkPolarPlot   *polv;
    GooCanvasItemModel *root;

    polv = GTK_POLAR_PLOT(g_object_new(GTK_TYPE_POLAR_PLOT, NULL));

    polv->qth = qth;

    if (pass != NULL)
        polv->pass = copy_pass(pass);

    /* get settings */
    polv->swap = sat_cfg_get_int(SAT_CFG_INT_POLAR_ORIENTATION);
    polv->qthinfo = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_QTH_INFO);
    polv->extratick = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);
    polv->cursinfo = TRUE;

    /* create the canvas */
    polv->canvas = goo_canvas_new();
    gtk_widget_set_size_request(polv->canvas,
                                POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);
    goo_canvas_set_bounds(GOO_CANVAS(polv->canvas), 0, 0,
                          POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);

    /* connect size-request signal */
    g_signal_connect(polv->canvas, "size-allocate",
                     G_CALLBACK(size_allocate_cb), polv);
    g_signal_connect(polv->canvas, "item_created",
                     (GCallback) on_item_created, polv);
    g_signal_connect_after(polv->canvas, "realize",
                           (GCallback) on_canvas_realized, polv);

    gtk_widget_show(polv->canvas);

    /* Create the canvas model */
    root = create_canvas_model(polv);
    goo_canvas_set_root_item_model(GOO_CANVAS(polv->canvas), root);
    g_object_unref(root);

    if (polv->pass != NULL)
        create_track(polv);

    gtk_box_pack_start(GTK_BOX(polv), polv->canvas, TRUE, TRUE, 0);

    return GTK_WIDGET(polv);
}

/**
 * Set new pass
 *
 * @param plot Pointer to the GtkPolarPlot widget.
 * @param pass Pointer to the new pass data. Use NULL to disable
 *             display of pass.
 */
void gtk_polar_plot_set_pass(GtkPolarPlot * plot, pass_t * pass)
{
    GooCanvasItemModel *root;
    gint            idx, i;

    /* remove sky track, time ticks and the pass itself */
    if (plot->pass != NULL)
    {
        /* remove sat from canvas */
        root = goo_canvas_get_root_item_model(GOO_CANVAS(plot->canvas));
        idx = goo_canvas_item_model_find_child(root, plot->track);

        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);

        for (i = 0; i < TRACK_TICK_NUM; i++)
        {
            idx = goo_canvas_item_model_find_child(root, plot->trtick[i]);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);
        }

        free_pass(plot->pass);
        plot->pass = NULL;
    }

    if (pass != NULL)
    {
        plot->pass = copy_pass(pass);
        create_track(plot);
    }
}

/**
 * Set target object position
 *
 * @param plot Pointer to the GtkPolarPlot widget
 * @param az Azimuth of the target object
 * @param el Elevation of the target object
 * 
 * If either az or el are negative the target object will be hidden
 */
void gtk_polar_plot_set_target_pos(GtkPolarPlot * plot, gdouble az, gdouble el)
{
    GooCanvasItemModel *root;
    gint            idx;
    gfloat          x, y;
    guint32         col;

    if (plot == NULL)
        return;

    root = goo_canvas_get_root_item_model(GOO_CANVAS(plot->canvas));

    if ((az < 0.0) || (el < 0.0))
    {
        if (plot->target != NULL)
        {
            /* the target object is visible; delete it */
            idx = goo_canvas_item_model_find_child(root, plot->target);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);

            plot->target = NULL;
        }
        /* else the target object is not visible; nothing to do */
    }
    else
    {
        /* we need to either update or create the object */
        azel_to_xy(plot, az, el, &x, &y);

        if (plot->target != NULL)
        {
            /* the target object already exists; move it */
            g_object_set(plot->target,
                         "x", x - MARKER_SIZE_HALF,
                         "y", y - MARKER_SIZE_HALF, NULL);
        }
        else
        {
            /* the target object does not exist; create it */
            col = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_COL);
            plot->target = goo_canvas_rect_model_new(root,
                                                     x - MARKER_SIZE_HALF,
                                                     y - MARKER_SIZE_HALF,
                                                     2 * MARKER_SIZE_HALF,
                                                     2 * MARKER_SIZE_HALF,
                                                     "fill-color-rgba", col,
                                                     "stroke-color-rgba", col,
                                                     NULL);
        }
    }
}

/**
 * Set controller object position
 *
 * @param plot Pointer to the GtkPolarPlot widget
 * @param az Azimuth of the controller object
 * @param el Elevation of the controller object
 * 
 * If either az or el are negative the controller object will be hidden
 */
void gtk_polar_plot_set_ctrl_pos(GtkPolarPlot * plot, gdouble az, gdouble el)
{
    GooCanvasItemModel *root;
    gint            idx;
    gfloat          x, y;
    guint32         col;

    if (plot == NULL)
        return;

    root = goo_canvas_get_root_item_model(GOO_CANVAS(plot->canvas));

    if ((az < 0.0) || (el < 0.0))
    {
        if (plot->ctrl != NULL)
        {
            /* the target object is visible; delete it */
            idx = goo_canvas_item_model_find_child(root, plot->ctrl);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);

            plot->ctrl = NULL;
        }
        /* else the target object is not visible; nothing to do */
    }
    else
    {
        /* we need to either update or create the object */
        azel_to_xy(plot, az, el, &x, &y);

        if (plot->ctrl != NULL)
        {
            /* the target object already exists; move it */
            g_object_set(plot->ctrl, "center_x", x, "center_y", y, NULL);
        }
        else
        {
            /* the target object does not exist; create it */
            col = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_COL);
            plot->ctrl = goo_canvas_ellipse_model_new(root,
                                                      x, y, 7, 7,
                                                      "fill-color-rgba",
                                                      0xFF00000F,
                                                      "stroke-color-rgba", col,
                                                      "line-width", 0.8, NULL);
        }
    }
}

/**
 * Set rotator object position
 *
 * @param plot Pointer to the GtkPolarPlot widget
 * @param az Azimuth of the rotator object
 * @param el Elevation of the rotator object
 * 
 * If either az or el are negative the controller object will be hidden
 */
void gtk_polar_plot_set_rotor_pos(GtkPolarPlot * plot, gdouble az, gdouble el)
{
    GooCanvasItemModel *root;
    GooCanvasPoints *prec;
    gint            idx;
    gfloat          x, y;
    guint32         col;

    if (plot == NULL)
        return;

    root = goo_canvas_get_root_item_model(GOO_CANVAS(plot->canvas));

    if ((az < 0.0) || (el < 0.0))
    {
        if (plot->rot1 != NULL)
        {
            /* the target object is visible; delete it */
            idx = goo_canvas_item_model_find_child(root, plot->rot1);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);

            plot->rot1 = NULL;
        }
        if (plot->rot2 != NULL)
        {
            /* the target object is visible; delete it */
            idx = goo_canvas_item_model_find_child(root, plot->rot2);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);

            plot->rot2 = NULL;
        }
        if (plot->rot3 != NULL)
        {
            /* the target object is visible; delete it */
            idx = goo_canvas_item_model_find_child(root, plot->rot3);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);

            plot->rot3 = NULL;
        }
        if (plot->rot4 != NULL)
        {
            /* the target object is visible; delete it */
            idx = goo_canvas_item_model_find_child(root, plot->rot4);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);

            plot->rot4 = NULL;
        }
    }
    else
    {
        /* we need to either update or create the object */
        azel_to_xy(plot, az, el, &x, &y);
        col = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_COL);

        if (plot->rot1 != NULL)
        {
            /* the target object already exists; move it */
            prec = goo_canvas_points_new(2);
            prec->coords[0] = x;
            prec->coords[1] = y - 4;
            prec->coords[2] = x;
            prec->coords[3] = y - 14;
            g_object_set(plot->rot1, "points", prec, NULL);
            goo_canvas_points_unref(prec);
        }
        else
        {
            /* the target object does not exist; create it */
            plot->rot1 = goo_canvas_polyline_model_new_line(root,
                                                            x, y - 4, x,
                                                            y - 14,
                                                            "fill-color-rgba",
                                                            col,
                                                            "stroke-color-rgba",
                                                            col, "line-width",
                                                            1.0, NULL);
        }
        if (plot->rot2 != NULL)
        {
            /* the target object already exists; move it */
            prec = goo_canvas_points_new(2);
            prec->coords[0] = x + 4;
            prec->coords[1] = y;
            prec->coords[2] = x + 14;
            prec->coords[3] = y;
            g_object_set(plot->rot2, "points", prec, NULL);
            goo_canvas_points_unref(prec);
        }
        else
        {
            /* the target object does not exist; create it */
            plot->rot2 = goo_canvas_polyline_model_new_line(root,
                                                            x + 4, y, x + 14,
                                                            y,
                                                            "fill-color-rgba",
                                                            col,
                                                            "stroke-color-rgba",
                                                            col, "line-width",
                                                            1.0, NULL);
        }
        if (plot->rot3 != NULL)
        {
            /* the target object already exists; move it */
            prec = goo_canvas_points_new(2);
            prec->coords[0] = x;
            prec->coords[1] = y + 4;
            prec->coords[2] = x;
            prec->coords[3] = y + 14;
            g_object_set(plot->rot3, "points", prec, NULL);
            goo_canvas_points_unref(prec);
        }
        else
        {
            /* the target object does not exist; create it */
            plot->rot3 = goo_canvas_polyline_model_new_line(root,
                                                            x, y + 4, x,
                                                            y + 14,
                                                            "fill-color-rgba",
                                                            col,
                                                            "stroke-color-rgba",
                                                            col, "line-width",
                                                            1.0, NULL);
        }
        if (plot->rot4 != NULL)
        {
            /* the target object already exists; move it */
            prec = goo_canvas_points_new(2);
            prec->coords[0] = x - 4;
            prec->coords[1] = y;
            prec->coords[2] = x - 14;
            prec->coords[3] = y;
            g_object_set(plot->rot4, "points", prec, NULL);
            goo_canvas_points_unref(prec);
        }
        else
        {
            /* the target object does not exist; create it */
            plot->rot4 = goo_canvas_polyline_model_new_line(root,
                                                            x - 4, y, x - 14,
                                                            y,
                                                            "fill-color-rgba",
                                                            col,
                                                            "stroke-color-rgba",
                                                            col, "line-width",
                                                            1.0, NULL);
        }
    }
}

/**
 * Show/hide time tick
 *
 * @param plot Pointer to the GtkPolarPlot widget
 * @param show TRUE => show tick. FALSE => don't show
 * 
 */
void gtk_polar_plot_show_time_ticks(GtkPolarPlot * plot, gboolean show)
{
    (void)plot;
    (void)show;
    g_print("NOT IMPLEMENTED %s\n", __func__);
}
