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
#include <gtk/gtk.h>
#include <math.h>

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


static GtkBoxClass *parent_class = NULL;

/** Convert rgba color to cairo-friendly format */
static void rgba_to_cairo(guint32 rgba, gdouble *r, gdouble *g, gdouble *b, gdouble *a)
{
    *r = ((rgba >> 24) & 0xFF) / 255.0;
    *g = ((rgba >> 16) & 0xFF) / 255.0;
    *b = ((rgba >> 8) & 0xFF) / 255.0;
    *a = (rgba & 0xFF) / 255.0;
}

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
    polview->track_points = NULL;
    polview->track_count = 0;
    polview->target_az = -1.0;
    polview->target_el = -1.0;
    polview->ctrl_az = -1.0;
    polview->ctrl_el = -1.0;
    polview->rotor_az = -1.0;
    polview->rotor_el = -1.0;
    polview->curs_text = NULL;
    polview->font = NULL;
}

static void gtk_polar_plot_destroy(GtkWidget * widget)
{
    GtkPolarPlot *polv = GTK_POLAR_PLOT(widget);

    if (polv->pass != NULL)
    {
        free_pass(polv->pass);
        polv->pass = NULL;
    }

    g_free(polv->track_points);
    polv->track_points = NULL;

    g_free(polv->curs_text);
    polv->curs_text = NULL;

    g_free(polv->font);
    polv->font = NULL;

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
    pass_detail_t  *detail;
    guint           num;
    gfloat          x, y;
    guint           tres, ttidx;

    /* create points */
    num = g_slist_length(pv->pass->details);

    g_free(pv->track_points);
    pv->track_points = g_new(gdouble, num * 2);
    pv->track_count = num;

    /* time resolution for time ticks */
    tres = (num - 2) / (TRACK_TICK_NUM - 1);

    /* first point should be (aos_az,0.0) */
    azel_to_xy(pv, pv->pass->aos_az, 0.0, &x, &y);
    pv->track_points[0] = (gdouble)x;
    pv->track_points[1] = (gdouble)y;

    /* first time tick */
    pv->trtick[0].x = x;
    pv->trtick[0].y = y;
    daynum_to_str(pv->trtick[0].text, 6, "%H:%M", pv->pass->aos);

    ttidx = 1;

    for (i = 1; i < num - 1; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(pv->pass->details, i));
        if (detail->el >= 0.0)
            azel_to_xy(pv, detail->az, detail->el, &x, &y);
        pv->track_points[2 * i] = (gdouble)x;
        pv->track_points[2 * i + 1] = (gdouble)y;

        if (!(i % tres))
        {
            if (ttidx < TRACK_TICK_NUM)
            {
                /* create a time tick */
                pv->trtick[ttidx].x = x;
                pv->trtick[ttidx].y = y;
                daynum_to_str(pv->trtick[ttidx].text, 6, "%H:%M", detail->time);
            }
            ttidx++;
        }
    }

    /* last point should be (los_az, 0.0)  */
    azel_to_xy(pv, pv->pass->los_az, 0.0, &x, &y);
    pv->track_points[2 * (num - 1)] = (gdouble)x;
    pv->track_points[2 * (num - 1) + 1] = (gdouble)y;
}

/**
 * Transform pole coordinates.
 *
 * This function transforms the pole coordinates (x,y) taking into account
 * the orientation of the polar plot.
 */
static void
correct_pole_coor(GtkPolarPlot * polv, polar_plot_pole_t pole,
                  gfloat * x, gfloat * y, gboolean *anchor_west)
{
    *anchor_west = TRUE;

    switch (pole)
    {
    case POLAR_PLOT_POLE_N:
        if ((polv->swap == POLAR_PLOT_SENW) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* North and South are swapped */
            *y = *y + POLV_LINE_EXTRA;
        }
        else
        {
            *y = *y - POLV_LINE_EXTRA;
        }
        break;

    case POLAR_PLOT_POLE_E:
        if ((polv->swap == POLAR_PLOT_NWSE) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* East and West are swapped */
            *x = *x - POLV_LINE_EXTRA;
            *anchor_west = FALSE;
        }
        else
        {
            *x = *x + POLV_LINE_EXTRA;
        }
        break;

    case POLAR_PLOT_POLE_S:
        if ((polv->swap == POLAR_PLOT_SENW) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* North and South are swapped */
            *y = *y - POLV_LINE_EXTRA;
        }
        else
        {
            *y = *y + POLV_LINE_EXTRA;
        }
        break;

    case POLAR_PLOT_POLE_W:
        if ((polv->swap == POLAR_PLOT_NWSE) || (polv->swap == POLAR_PLOT_SWNE))
        {
            /* East and West are swapped */
            *x = *x + POLV_LINE_EXTRA;
        }
        else
        {
            *x = *x - POLV_LINE_EXTRA;
            *anchor_west = FALSE;
        }
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Incorrect polar plot orientation."),
                    __FILE__, __LINE__);
        break;
    }
}

/** Update sky track drawing after size allocate. */
static void update_track(GtkPolarPlot * pv)
{
    guint           num, i;
    gfloat          x, y;
    pass_detail_t  *detail;
    guint           tres, ttidx;

    if (pv->pass == NULL)
        return;

    /* create points */
    num = g_slist_length(pv->pass->details);

    g_free(pv->track_points);
    pv->track_points = g_new(gdouble, num * 2);
    pv->track_count = num;

    /* first point should be (aos_az,0.0) */
    azel_to_xy(pv, pv->pass->aos_az, 0.0, &x, &y);
    pv->track_points[0] = (gdouble)x;
    pv->track_points[1] = (gdouble)y;

    /* time tick 0 */
    pv->trtick[0].x = x;
    pv->trtick[0].y = y;

    /* time resolution for time ticks */
    tres = (num - 2) / (TRACK_TICK_NUM - 1);
    ttidx = 1;

    for (i = 1; i < num - 1; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(pv->pass->details, i));
        if (detail->el >= 0.0)
            azel_to_xy(pv, detail->az, detail->el, &x, &y);
        pv->track_points[2 * i] = (gdouble)x;
        pv->track_points[2 * i + 1] = (gdouble)y;

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
                pv->trtick[ttidx].x = x;
                pv->trtick[ttidx].y = y;
            }
            ttidx++;
        }
    }

    /* last point should be (los_az, 0.0)  */
    azel_to_xy(pv, pv->pass->los_az, 0.0, &x, &y);
    pv->track_points[2 * (num - 1)] = (gdouble)x;
    pv->track_points[2 * (num - 1) + 1] = (gdouble)y;
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GtkPolarPlot   *polv = GTK_POLAR_PLOT(data);
    gdouble         r, g, b, a;
    gfloat          x, y;
    gboolean        anchor_west;
    PangoLayout    *layout;
    PangoFontDescription *font_desc;
    gint            tw, th;
    guint           i;

    (void)widget;

    /* White background */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    /* Set up font */
    layout = pango_cairo_create_layout(cr);
    font_desc = pango_font_description_from_string(polv->font ? polv->font : "Sans 9");
    pango_layout_set_font_description(layout, font_desc);

    /* Axis color for circles and lines */
    rgba_to_cairo(polv->col_axis, &r, &g, &b, &a);
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_set_line_width(cr, 1.0);

    /* 0 degree circle */
    cairo_arc(cr, polv->cx, polv->cy, polv->r, 0, 2 * M_PI);
    cairo_stroke(cr);

    /* 30 degree circle */
    cairo_arc(cr, polv->cx, polv->cy, 0.6667 * polv->r, 0, 2 * M_PI);
    cairo_stroke(cr);

    /* 60 degree circle */
    cairo_arc(cr, polv->cx, polv->cy, 0.333 * polv->r, 0, 2 * M_PI);
    cairo_stroke(cr);

    /* Horizontal line */
    cairo_move_to(cr, polv->cx - polv->r - POLV_LINE_EXTRA, polv->cy);
    cairo_line_to(cr, polv->cx + polv->r + POLV_LINE_EXTRA, polv->cy);
    cairo_stroke(cr);

    /* Vertical line */
    cairo_move_to(cr, polv->cx, polv->cy - polv->r - POLV_LINE_EXTRA);
    cairo_line_to(cr, polv->cx, polv->cy + polv->r + POLV_LINE_EXTRA);
    cairo_stroke(cr);

    /* N/S/E/W labels */
    rgba_to_cairo(polv->col_tick, &r, &g, &b, &a);
    cairo_set_source_rgba(cr, r, g, b, a);

    /* N label */
    azel_to_xy(polv, 0.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_N, &x, &y, &anchor_west);
    pango_layout_set_text(layout, _("N"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, x - tw / 2, y - th);
    pango_cairo_show_layout(cr, layout);

    /* E label */
    azel_to_xy(polv, 90.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_E, &x, &y, &anchor_west);
    pango_layout_set_text(layout, _("E"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    if (anchor_west)
        cairo_move_to(cr, x, y - th / 2);
    else
        cairo_move_to(cr, x - tw, y - th / 2);
    pango_cairo_show_layout(cr, layout);

    /* S label */
    azel_to_xy(polv, 180.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_S, &x, &y, &anchor_west);
    pango_layout_set_text(layout, _("S"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, x - tw / 2, y);
    pango_cairo_show_layout(cr, layout);

    /* W label */
    azel_to_xy(polv, 270.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_PLOT_POLE_W, &x, &y, &anchor_west);
    pango_layout_set_text(layout, _("W"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    if (anchor_west)
        cairo_move_to(cr, x, y - th / 2);
    else
        cairo_move_to(cr, x - tw, y - th / 2);
    pango_cairo_show_layout(cr, layout);

    /* Location name (if enabled) */
    if (polv->qthinfo && polv->qth)
    {
        rgba_to_cairo(polv->col_info, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        pango_layout_set_text(layout, polv->qth->name, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, polv->cx - polv->r - 2 * POLV_LINE_EXTRA,
                      polv->cy - polv->r - POLV_LINE_EXTRA - th);
        pango_cairo_show_layout(cr, layout);
    }

    /* Cursor tracking text */
    if (polv->cursinfo && polv->curs_text)
    {
        rgba_to_cairo(polv->col_info, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        pango_layout_set_text(layout, polv->curs_text, -1);
        cairo_move_to(cr, polv->cx - polv->r - 2 * POLV_LINE_EXTRA,
                      polv->cy + polv->r + POLV_LINE_EXTRA);
        pango_cairo_show_layout(cr, layout);
    }

    /* Draw satellite track */
    if (polv->track_points && polv->track_count > 1)
    {
        rgba_to_cairo(polv->col_track, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, polv->track_points[0], polv->track_points[1]);
        for (i = 1; i < (guint)polv->track_count; i++)
        {
            cairo_line_to(cr, polv->track_points[2 * i], polv->track_points[2 * i + 1]);
        }
        cairo_stroke(cr);

        /* Draw time ticks along track */
        for (i = 0; i < TRACK_TICK_NUM; i++)
        {
            x = polv->trtick[i].x;
            y = polv->trtick[i].y;

            pango_layout_set_text(layout, polv->trtick[i].text, -1);
            pango_layout_get_pixel_size(layout, &tw, &th);

            if (x > polv->cx)
                cairo_move_to(cr, x - tw - 5, y - th / 2);
            else
                cairo_move_to(cr, x + 5, y - th / 2);

            pango_cairo_show_layout(cr, layout);
        }
    }

    /* Draw target (small filled square) */
    if (polv->target_az >= 0.0 && polv->target_el >= 0.0)
    {
        azel_to_xy(polv, polv->target_az, polv->target_el, &x, &y);
        rgba_to_cairo(polv->col_sat, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_rectangle(cr, x - MARKER_SIZE_HALF, y - MARKER_SIZE_HALF,
                        2 * MARKER_SIZE_HALF, 2 * MARKER_SIZE_HALF);
        cairo_fill(cr);
    }

    /* Draw controller (small circle) */
    if (polv->ctrl_az >= 0.0 && polv->ctrl_el >= 0.0)
    {
        azel_to_xy(polv, polv->ctrl_az, polv->ctrl_el, &x, &y);
        rgba_to_cairo(polv->col_sat, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_arc(cr, x, y, 7, 0, 2 * M_PI);
        cairo_stroke(cr);
    }

    /* Draw rotor position (cross hair) */
    if (polv->rotor_az >= 0.0 && polv->rotor_el >= 0.0)
    {
        azel_to_xy(polv, polv->rotor_az, polv->rotor_el, &x, &y);
        rgba_to_cairo(polv->col_sat, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_set_line_width(cr, 1.0);

        /* Up */
        cairo_move_to(cr, x, y - 4);
        cairo_line_to(cr, x, y - 14);
        /* Right */
        cairo_move_to(cr, x + 4, y);
        cairo_line_to(cr, x + 14, y);
        /* Down */
        cairo_move_to(cr, x, y + 4);
        cairo_line_to(cr, x, y + 14);
        /* Left */
        cairo_move_to(cr, x - 4, y);
        cairo_line_to(cr, x - 14, y);

        cairo_stroke(cr);
    }

    pango_font_description_free(font_desc);
    g_object_unref(layout);

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    GtkPolarPlot   *polv = GTK_POLAR_PLOT(data);
    gfloat          az, el;

    (void)widget;

    if (polv->cursinfo)
    {
        xy_to_azel(polv, event->x, event->y, &az, &el);

        if (el > 0.0)
        {
            g_free(polv->curs_text);
            polv->curs_text = g_strdup_printf("AZ %.0f\302\260\nEL %.0f\302\260", az, el);
        }
        else
        {
            g_free(polv->curs_text);
            polv->curs_text = NULL;
        }

        gtk_widget_queue_draw(polv->canvas);
    }

    return TRUE;
}

/**
 * Manage new size allocation.
 */
static void size_allocate_cb(GtkWidget * widget, GtkAllocation * allocation,
                             gpointer data)
{
    GtkPolarPlot   *polv;

    if (gtk_widget_get_realized(widget))
    {
        polv = GTK_POLAR_PLOT(data);

        polv->size = MIN(allocation->width, allocation->height);
        polv->r = (polv->size / 2) - POLV_DEFAULT_MARGIN;
        polv->cx = allocation->width / 2;
        polv->cy = allocation->height / 2;

        /* sky track */
        if (polv->pass != NULL)
            update_track(polv);

        gtk_widget_queue_draw(widget);
    }
}

/**
 * Manage canvas realise signals.
 */
static void on_canvas_realized(GtkWidget * canvas, gpointer data)
{
    GtkAllocation   aloc;

    gtk_widget_get_allocation(canvas, &aloc);
    size_allocate_cb(canvas, &aloc, data);
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
    GValue          font_value = G_VALUE_INIT;

    polv = GTK_POLAR_PLOT(g_object_new(GTK_TYPE_POLAR_PLOT, NULL));

    polv->qth = qth;

    if (pass != NULL)
        polv->pass = copy_pass(pass);

    /* get settings */
    polv->swap = sat_cfg_get_int(SAT_CFG_INT_POLAR_ORIENTATION);
    polv->qthinfo = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_QTH_INFO);
    polv->extratick = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);
    polv->cursinfo = TRUE;

    /* get colors */
    polv->col_bgd = 0xFFFFFFFF;
    polv->col_axis = sat_cfg_get_int(SAT_CFG_INT_POLAR_AXIS_COL);
    polv->col_tick = sat_cfg_get_int(SAT_CFG_INT_POLAR_TICK_COL);
    polv->col_info = sat_cfg_get_int(SAT_CFG_INT_POLAR_INFO_COL);
    polv->col_sat = sat_cfg_get_int(SAT_CFG_INT_POLAR_SAT_COL);
    polv->col_track = sat_cfg_get_int(SAT_CFG_INT_POLAR_TRACK_COL);

    /* get default font */
    g_value_init(&font_value, G_TYPE_STRING);
    g_object_get_property(G_OBJECT(gtk_settings_get_default()), "gtk-font-name", &font_value);
    polv->font = g_value_dup_string(&font_value);
    g_value_unset(&font_value);

    /* graph dimensions */
    polv->size = POLV_DEFAULT_SIZE;
    polv->r = (polv->size / 2) - POLV_DEFAULT_MARGIN;
    polv->cx = POLV_DEFAULT_SIZE / 2;
    polv->cy = POLV_DEFAULT_SIZE / 2;

    /* create the canvas (drawing area) */
    polv->canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(polv->canvas, POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);
    gtk_widget_add_events(polv->canvas, GDK_POINTER_MOTION_MASK);

    /* connect signals */
    g_signal_connect(polv->canvas, "draw", G_CALLBACK(on_draw), polv);
    g_signal_connect(polv->canvas, "motion-notify-event", G_CALLBACK(on_motion_notify), polv);
    g_signal_connect(polv->canvas, "size-allocate", G_CALLBACK(size_allocate_cb), polv);
    g_signal_connect_after(polv->canvas, "realize", G_CALLBACK(on_canvas_realized), polv);

    gtk_widget_show(polv->canvas);

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
    /* remove sky track and the pass itself */
    if (plot->pass != NULL)
    {
        free_pass(plot->pass);
        plot->pass = NULL;
    }

    g_free(plot->track_points);
    plot->track_points = NULL;
    plot->track_count = 0;

    if (pass != NULL)
    {
        plot->pass = copy_pass(pass);
        create_track(plot);
    }

    gtk_widget_queue_draw(plot->canvas);
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
    if (plot == NULL)
        return;

    plot->target_az = az;
    plot->target_el = el;

    gtk_widget_queue_draw(plot->canvas);
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
    if (plot == NULL)
        return;

    plot->ctrl_az = az;
    plot->ctrl_el = el;

    gtk_widget_queue_draw(plot->canvas);
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
    if (plot == NULL)
        return;

    plot->rotor_az = az;
    plot->rotor_el = el;

    gtk_widget_queue_draw(plot->canvas);
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
