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
#include <gtk/gtk.h>
#include <math.h>

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
    GtkAzelPlot *azel = GTK_AZEL_PLOT(widget);
    guint i;

    g_free(azel->curs_text);
    azel->curs_text = NULL;

    g_free(azel->az_points);
    azel->az_points = NULL;

    g_free(azel->el_points);
    azel->el_points = NULL;

    for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++)
    {
        g_free(azel->xlabels[i]);
        g_free(azel->azlabels[i]);
        g_free(azel->ellabels[i]);
        azel->xlabels[i] = NULL;
        azel->azlabels[i] = NULL;
        azel->ellabels[i] = NULL;
    }

    g_free(azel->font);
    azel->font = NULL;

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
    azel->pass = NULL;
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
    azel->curs_text = NULL;
    azel->az_points = NULL;
    azel->el_points = NULL;
    azel->num_points = 0;
    azel->font = NULL;
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

    /* El */
    dpp = (gdouble) (90.0 / (p->y0 - p->ymax));
    *y = (gdouble) (p->y0 - el / dpp);
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

static void calculate_graph_points(GtkAzelPlot *azel)
{
    guint i, n;
    pass_detail_t *detail;
    gdouble dx, dy;

    n = g_slist_length(azel->pass->details);
    azel->num_points = n;

    g_free(azel->az_points);
    g_free(azel->el_points);

    azel->az_points = g_new(gdouble, n * 2);
    azel->el_points = g_new(gdouble, n * 2);

    for (i = 0; i < n; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(azel->pass->details, i));
        az_to_xy(azel, detail->time, detail->az, &dx, &dy);
        azel->az_points[2 * i] = dx;
        azel->az_points[2 * i + 1] = dy;

        el_to_xy(azel, detail->time, detail->el, &dx, &dy);
        azel->el_points[2 * i] = dx;
        azel->el_points[2 * i + 1] = dy;
    }

    /* Adjust endpoints */
    if (n > 0)
    {
        azel->az_points[0] = azel->x0;
        azel->az_points[2 * n - 2] = azel->xmax;
        azel->el_points[1] = azel->y0;
        azel->el_points[2 * n - 1] = azel->y0;
    }
}

static void calculate_tick_labels(GtkAzelPlot *azel)
{
    guint i;
    gdouble xstep;
    gdouble t, az, el;
    gchar buff[7];

    xstep = (azel->xmax - azel->x0) / (AZEL_PLOT_NUM_TICKS + 1);

    for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++)
    {
        /* x tick labels - time */
        xy_to_graph(azel, azel->x0 + (i + 1) * xstep, 0.0, &t, &az, &el);
        daynum_to_str(buff, 7, "%H:%M", t);

        g_free(azel->xlabels[i]);
        azel->xlabels[i] = g_strdup(buff);

        /* Az tick labels */
        g_free(azel->azlabels[i]);
        azel->azlabels[i] = g_strdup_printf("%.0f\302\260",
                              azel->maxaz / (AZEL_PLOT_NUM_TICKS + 1) * (i + 1));

        /* El tick labels */
        g_free(azel->ellabels[i]);
        azel->ellabels[i] = g_strdup_printf("%.0f\302\260",
                            90.0 / (AZEL_PLOT_NUM_TICKS + 1) * (i + 1));
    }
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GtkAzelPlot *azel = GTK_AZEL_PLOT(data);
    gdouble xstep, ystep;
    guint i;
    PangoLayout *layout;
    PangoFontDescription *font_desc;

    (void)widget;

    /* Background */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    /* Frame */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, azel->x0, azel->ymax, azel->xmax - azel->x0, azel->y0 - azel->ymax);
    cairo_stroke(cr);

    xstep = (azel->xmax - azel->x0) / (AZEL_PLOT_NUM_TICKS + 1);
    ystep = (azel->y0 - azel->ymax) / (AZEL_PLOT_NUM_TICKS + 1);

    /* Tick marks */
    for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++)
    {
        /* bottom x tick marks */
        cairo_move_to(cr, azel->x0 + (i + 1) * xstep, azel->y0);
        cairo_line_to(cr, azel->x0 + (i + 1) * xstep, azel->y0 - MARKER_SIZE);

        /* top x tick marks */
        cairo_move_to(cr, azel->x0 + (i + 1) * xstep, azel->ymax);
        cairo_line_to(cr, azel->x0 + (i + 1) * xstep, azel->ymax + MARKER_SIZE);

        /* left y tick marks */
        cairo_move_to(cr, azel->x0, azel->y0 - (i + 1) * ystep);
        cairo_line_to(cr, azel->x0 + MARKER_SIZE, azel->y0 - (i + 1) * ystep);

        /* right y tick marks */
        cairo_move_to(cr, azel->xmax, azel->y0 - (i + 1) * ystep);
        cairo_line_to(cr, azel->xmax - MARKER_SIZE, azel->y0 - (i + 1) * ystep);
    }
    cairo_stroke(cr);

    /* Set up font */
    layout = pango_cairo_create_layout(cr);
    font_desc = pango_font_description_from_string(azel->font ? azel->font : "Sans 9");
    pango_layout_set_font_description(layout, font_desc);

    /* Draw tick labels */
    for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++)
    {
        int tw, th;

        /* x tick labels */
        if (azel->xlabels[i])
        {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            pango_layout_set_text(layout, azel->xlabels[i], -1);
            pango_layout_get_pixel_size(layout, &tw, &th);
            cairo_move_to(cr, azel->x0 + (i + 1) * xstep - tw / 2, azel->y0 + 5);
            pango_cairo_show_layout(cr, layout);
        }

        /* Az tick labels (left, blue) */
        if (azel->azlabels[i])
        {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.75, 1.0);
            pango_layout_set_text(layout, azel->azlabels[i], -1);
            pango_layout_get_pixel_size(layout, &tw, &th);
            cairo_move_to(cr, azel->x0 - 5 - tw, azel->y0 - (i + 1) * ystep - th / 2);
            pango_cairo_show_layout(cr, layout);
        }

        /* El tick labels (right, red) */
        if (azel->ellabels[i])
        {
            cairo_set_source_rgba(cr, 0.75, 0.0, 0.0, 1.0);
            pango_layout_set_text(layout, azel->ellabels[i], -1);
            pango_layout_get_pixel_size(layout, &tw, &th);
            cairo_move_to(cr, azel->xmax + 5, azel->y0 - (i + 1) * ystep - th / 2);
            pango_cairo_show_layout(cr, layout);
        }
    }

    /* Az legend */
    {
        int tw, th;
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.75, 1.0);
        pango_layout_set_text(layout, _("Az"), -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, azel->x0 - 7 - tw, azel->ymax);
        pango_cairo_show_layout(cr, layout);
    }

    /* El legend */
    {
        int tw, th;
        cairo_set_source_rgba(cr, 0.75, 0.0, 0.0, 1.0);
        pango_layout_set_text(layout, _("El"), -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, azel->xmax + 7, azel->ymax);
        pango_cairo_show_layout(cr, layout);
    }

    /* x legend */
    {
        int tw, th;
        const gchar *xleg = sat_cfg_get_bool(SAT_CFG_BOOL_USE_LOCAL_TIME) ? _("Local Time") : _("UTC");
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        pango_layout_set_text(layout, xleg, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, azel->x0 + (azel->xmax - azel->x0) / 2 - tw / 2, azel->height - 5 - th);
        pango_cairo_show_layout(cr, layout);
    }

    /* Cursor text */
    if (azel->curs_text && azel->cursinfo)
    {
        int tw, th;
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        pango_layout_set_text(layout, azel->curs_text, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, azel->x0 + (azel->xmax - azel->x0) / 2 - tw / 2, 5);
        pango_cairo_show_layout(cr, layout);
    }

    pango_font_description_free(font_desc);
    g_object_unref(layout);

    /* Draw Az graph (blue) */
    if (azel->az_points && azel->num_points > 1)
    {
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.75, 1.0);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, azel->az_points[0], azel->az_points[1]);
        for (i = 1; i < (guint)azel->num_points; i++)
        {
            cairo_line_to(cr, azel->az_points[2 * i], azel->az_points[2 * i + 1]);
        }
        cairo_stroke(cr);
    }

    /* Draw El graph (red) */
    if (azel->el_points && azel->num_points > 1)
    {
        cairo_set_source_rgba(cr, 0.75, 0.0, 0.0, 1.0);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, azel->el_points[0], azel->el_points[1]);
        for (i = 1; i < (guint)azel->num_points; i++)
        {
            cairo_line_to(cr, azel->el_points[2 * i], azel->el_points[2 * i + 1]);
        }
        cairo_stroke(cr);
    }

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    GtkAzelPlot    *azel = GTK_AZEL_PLOT(data);
    gdouble         t, az, el;
    gfloat          x, y;
    gchar           buff[10];

    (void)widget;

    if (azel->cursinfo)
    {
        x = event->x;
        y = event->y;

        if ((x > azel->x0) && (x < azel->xmax) &&
            (y > azel->ymax) && (y < azel->y0))
        {
            xy_to_graph(azel, x, y, &t, &az, &el);
            daynum_to_str(buff, 10, "%H:%M:%S", t);

            g_free(azel->curs_text);
            azel->curs_text = g_strdup_printf("T: %s, AZ: %.0f\302\260, EL: %.0f\302\260",
                                buff, az, el);
        }
        else
        {
            g_free(azel->curs_text);
            azel->curs_text = NULL;
        }
        gtk_widget_queue_draw(azel->canvas);
    }

    return TRUE;
}

static void size_allocate_cb(GtkWidget * widget, GtkAllocation * allocation,
                             gpointer data)
{
    GtkAzelPlot *azel = GTK_AZEL_PLOT(data);

    (void)widget;

    azel->width = allocation->width;
    azel->height = allocation->height;

    /* update coordinate system */
    azel->x0 = AZEL_X_MARGIN;
    azel->xmax = azel->width - AZEL_X_MARGIN;
    azel->y0 = azel->height - AZEL_Y_MARGIN;
    azel->ymax = AZEL_Y_MARGIN;

    /* Recalculate graph points and tick labels */
    if (azel->pass)
    {
        calculate_graph_points(azel);
        calculate_tick_labels(azel);
    }
}

/**
 * Create a new GtkAzelPlot widget.
 * @param qth Pointer to the ground station data.
 * @param pass Pointer to the satellite pass to display.
 */
GtkWidget *gtk_azel_plot_new(qth_t * qth, pass_t * pass)
{
    GtkAzelPlot    *azel;
    guint           i, n;
    pass_detail_t  *detail;
    GValue          font_value = G_VALUE_INIT;

    azel = GTK_AZEL_PLOT(g_object_new(GTK_TYPE_AZEL_PLOT, NULL));
    azel->qth = qth;
    azel->pass = pass;

    /* get settings */
    azel->qthinfo = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_QTH_INFO);
    azel->extratick = sat_cfg_get_bool(SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);
    azel->cursinfo = TRUE;

    /* get colors */
    azel->col_axis = sat_cfg_get_int(SAT_CFG_INT_POLAR_AXIS_COL);
    azel->col_az = 0x0000BFFF;
    azel->col_el = 0xBF0000FF;

    /* get default font */
    g_value_init(&font_value, G_TYPE_STRING);
    g_object_get_property(G_OBJECT(gtk_settings_get_default()), "gtk-font-name", &font_value);
    azel->font = g_value_dup_string(&font_value);
    g_value_unset(&font_value);

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

    /* initial dimensions */
    azel->width = AZEL_DEFAULT_SIZE;
    azel->height = AZEL_DEFAULT_SIZE;
    azel->x0 = AZEL_X_MARGIN;
    azel->xmax = azel->width - AZEL_X_MARGIN;
    azel->y0 = azel->height - AZEL_Y_MARGIN;
    azel->ymax = AZEL_Y_MARGIN;

    /* calculate initial graph points */
    calculate_graph_points(azel);
    calculate_tick_labels(azel);

    /* create the drawing area */
    azel->canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(azel->canvas, AZEL_DEFAULT_SIZE, AZEL_DEFAULT_SIZE);
    gtk_widget_add_events(azel->canvas, GDK_POINTER_MOTION_MASK);

    g_signal_connect(azel->canvas, "draw", G_CALLBACK(on_draw), azel);
    g_signal_connect(azel->canvas, "motion-notify-event", G_CALLBACK(on_motion_notify), azel);
    g_signal_connect(azel->canvas, "size-allocate", G_CALLBACK(size_allocate_cb), azel);

    gtk_widget_show(azel->canvas);
    gtk_box_pack_start(GTK_BOX(azel), azel->canvas, TRUE, TRUE, 0);

    return GTK_WIDGET(azel);
}
