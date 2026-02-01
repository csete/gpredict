/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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

/**
 * Sky at a glance Widget.
 * 
 * The sky at a glance widget provides a convenient overview of the upcoming
 * satellite passes in a timeline format. The widget is tied to a specific
 * module and uses the QTH and satellite data from the module.
 */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-data.h"
#include "gtk-sky-glance.h"
#include "mod-cfg-get-param.h"
#include "predict-tools.h"
#include "sat-pass-dialogs.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"


#define SKG_DEFAULT_WIDTH       600
#define SKG_DEFAULT_HEIGHT      300
#define SKG_PIX_PER_SAT         10
#define SKG_MARGIN              15
#define SKG_FOOTER              50
#define SKG_CURSOR_WIDTH        0.5

static GtkBoxClass *parent_class = NULL;

/** Convert rgba color to cairo-friendly format */
static void rgba_to_cairo(guint32 rgba, gdouble *r, gdouble *g, gdouble *b, gdouble *a)
{
    *r = ((rgba >> 24) & 0xFF) / 255.0;
    *g = ((rgba >> 16) & 0xFF) / 255.0;
    *b = ((rgba >> 8) & 0xFF) / 255.0;
    *a = (rgba & 0xFF) / 255.0;
}

static void gtk_sky_glance_init(GtkSkyGlance * skg,
	gpointer g_class)
{
    (void)g_class;

    skg->sats = NULL;
    skg->qth = NULL;
    skg->passes = NULL;
    skg->satlab = NULL;
    skg->x0 = 0;
    skg->y0 = 0;
    skg->w = 0;
    skg->h = 0;
    skg->pps = 0;
    skg->numsat = 0;
    skg->satcnt = 0;
    skg->ts = 0.0;
    skg->te = 0.0;
    skg->num_ticks = 0;
    skg->major_x = NULL;
    skg->minor_x = NULL;
    skg->tick_labels = NULL;
    skg->cursor_x = 0.0;
    skg->time_label = NULL;
    skg->font = NULL;
}

static void free_sat_label(gpointer data)
{
    sat_label_t *label = (sat_label_t *)data;
    if (label)
    {
        g_free(label->name);
        g_free(label);
    }
}

static void gtk_sky_glance_destroy(GtkWidget * widget)
{
    GtkSkyGlance *skg = GTK_SKY_GLANCE(widget);
    sky_pass_t   *skypass;
    guint         i, n;

    /* free passes */
    /* FIXME: TBC whether this is enough */
    if (skg->passes != NULL)
    {
        n = g_slist_length(skg->passes);
        for (i = 0; i < n; i++)
        {
            skypass = (sky_pass_t *)g_slist_nth_data(skg->passes, i);
            free_pass(skypass->pass);
            g_free(skypass);
        }
        g_slist_free(skg->passes);
        skg->passes = NULL;
    }

    /* free satellite labels */
    if (skg->satlab != NULL)
    {
        g_slist_free_full(skg->satlab, free_sat_label);
        skg->satlab = NULL;
    }

    /* free tick data */
    g_free(skg->major_x);
    skg->major_x = NULL;

    g_free(skg->minor_x);
    skg->minor_x = NULL;

    if (skg->tick_labels)
    {
        for (i = 0; i < (guint)skg->num_ticks; i++)
        {
            g_free(skg->tick_labels[i]);
        }
        g_free(skg->tick_labels);
        skg->tick_labels = NULL;
    }

    g_free(skg->time_label);
    skg->time_label = NULL;

    g_free(skg->font);
    skg->font = NULL;

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}


static void gtk_sky_glance_class_init(GtkSkyGlanceClass * class,
				      gpointer class_data)
{
    GtkWidgetClass *widget_class;

    (void)class_data;

    widget_class = (GtkWidgetClass *) class;
    widget_class->destroy = gtk_sky_glance_destroy;
    parent_class = g_type_class_peek_parent(class);
}

GType gtk_sky_glance_get_type()
{
    static GType    gtk_sky_glance_type = 0;

    if (!gtk_sky_glance_type)
    {
        static const GTypeInfo gtk_sky_glance_info = {
            sizeof(GtkSkyGlanceClass),
            NULL,               /* base init */
            NULL,               /* base finalise */
            (GClassInitFunc) gtk_sky_glance_class_init,
            NULL,               /* class finalise */
            NULL,               /* class data */
            sizeof(GtkSkyGlance),
            1,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_sky_glance_init,
            NULL
        };

        gtk_sky_glance_type = g_type_register_static(GTK_TYPE_BOX,
                                                     "GtkSkyGlance",
                                                     &gtk_sky_glance_info, 0);
    }

    return gtk_sky_glance_type;
}

/**
 * Convert time value to x position.
 */
static gdouble t2x(GtkSkyGlance * skg, gdouble t)
{
    gdouble         frac;

    frac = (t - skg->ts) / (skg->te - skg->ts);

    return (skg->x0 + frac * skg->w);
}

/**
 * Convert x coordinate to Julian date.
 */
static gdouble x2t(GtkSkyGlance * skg, gdouble x)
{
    gdouble         frac;

    frac = (x - skg->x0) / skg->w;

    return (skg->ts + frac * (skg->te - skg->ts));
}

static sky_pass_t *find_pass_at_pos(GtkSkyGlance *skg, gdouble mx, gdouble my)
{
    GSList     *node;
    sky_pass_t *skypass;

    node = skg->passes;
    while (node)
    {
        skypass = (sky_pass_t *)node->data;
        if (mx >= skypass->x && mx <= skypass->x + skypass->w &&
            my >= skypass->y && my <= skypass->y + skypass->h)
        {
            return skypass;
        }
        node = node->next;
    }
    return NULL;
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GtkSkyGlance   *skg = GTK_SKY_GLANCE(data);
    gdouble         r, g, b, a;
    PangoLayout    *layout;
    PangoFontDescription *font_desc;
    gint            tw, th;
    guint           i;
    sky_pass_t     *skypass;
    GSList         *node;
    sat_label_t    *label;

    (void)widget;

    /* Set up font */
    layout = pango_cairo_create_layout(cr);
    font_desc = pango_font_description_from_string(skg->font ? skg->font : "Sans 9");
    pango_layout_set_font_description(layout, font_desc);

    /* Background */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, skg->x0, skg->y0, skg->w, skg->h);
    cairo_fill(cr);

    /* Draw satellite pass boxes */
    node = skg->passes;
    while (node)
    {
        skypass = (sky_pass_t *)node->data;

        /* Fill */
        rgba_to_cairo(skypass->fcol, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_rectangle(cr, skypass->x, skypass->y, skypass->w, skypass->h);
        cairo_fill(cr);

        /* Border */
        rgba_to_cairo(skypass->bcol, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, skypass->x, skypass->y, skypass->w, skypass->h);
        cairo_stroke(cr);

        node = node->next;
    }

    /* Draw satellite labels */
    node = skg->satlab;
    while (node)
    {
        label = (sat_label_t *)node->data;

        rgba_to_cairo(label->color, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        pango_layout_set_text(layout, label->name, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);

        if (label->anchor == 1)  /* East anchor */
            cairo_move_to(cr, label->x - tw, label->y - th / 2);
        else  /* West anchor */
            cairo_move_to(cr, label->x, label->y - th / 2);

        pango_cairo_show_layout(cr, layout);

        node = node->next;
    }

    /* Cursor tracking line */
    if (skg->cursor_x > skg->x0 && skg->cursor_x < skg->x0 + skg->w)
    {
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.7);
        cairo_set_line_width(cr, SKG_CURSOR_WIDTH);
        cairo_move_to(cr, skg->cursor_x, skg->y0);
        cairo_line_to(cr, skg->cursor_x, skg->y0 + skg->h);
        cairo_stroke(cr);

        /* Time label */
        if (skg->time_label)
        {
            pango_layout_set_text(layout, skg->time_label, -1);
            pango_layout_get_pixel_size(layout, &tw, &th);
            cairo_move_to(cr, skg->x0 + 5, skg->y0);
            pango_cairo_show_layout(cr, layout);
        }
    }

    /* Footer background */
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.25, 1.0);
    cairo_rectangle(cr, skg->x0, skg->h, skg->w, SKG_FOOTER);
    cairo_fill(cr);

    /* Time axis ticks and labels */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 1.0);

    for (i = 0; i < (guint)skg->num_ticks; i++)
    {
        /* Major tick */
        if (skg->major_x)
        {
            cairo_move_to(cr, skg->major_x[i], skg->h);
            cairo_line_to(cr, skg->major_x[i], skg->h + 10);
            cairo_stroke(cr);

            /* Tick label */
            if (skg->tick_labels && skg->tick_labels[i])
            {
                pango_layout_set_text(layout, skg->tick_labels[i], -1);
                pango_layout_get_pixel_size(layout, &tw, &th);
                cairo_move_to(cr, skg->major_x[i] - tw / 2, skg->h + 12);
                pango_cairo_show_layout(cr, layout);
            }
        }

        /* Minor tick */
        if (skg->minor_x)
        {
            cairo_move_to(cr, skg->minor_x[i], skg->h);
            cairo_line_to(cr, skg->minor_x[i], skg->h + 5);
            cairo_stroke(cr);
        }
    }

    /* Axis label */
    {
        const gchar *axis_text = sat_cfg_get_bool(SAT_CFG_BOOL_USE_LOCAL_TIME) ? _("TIME") : _("UTC");
        pango_layout_set_text(layout, axis_text, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, skg->w / 2 - tw / 2, skg->h + SKG_FOOTER - 5 - th);
        pango_cairo_show_layout(cr, layout);
    }

    pango_font_description_free(font_desc);
    g_object_unref(layout);

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    GtkSkyGlance   *skg = GTK_SKY_GLANCE(data);
    gdouble         t;
    gchar           buff[6];

    (void)widget;

    skg->cursor_x = event->x;

    /* get time corresponding to x */
    t = x2t(skg, event->x);
    daynum_to_str(buff, 6, "%H:%M", t);

    g_free(skg->time_label);
    skg->time_label = g_strdup(buff);

    gtk_widget_queue_draw(skg->canvas);

    return TRUE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkSkyGlance   *skg = GTK_SKY_GLANCE(data);
    sky_pass_t     *skypass;
    pass_t         *new_pass;

    (void)widget;

    if (event->button != 1)
        return FALSE;

    skypass = find_pass_at_pos(skg, event->x, event->y);
    if (skypass == NULL)
        return FALSE;

    if (skypass->pass == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s::%s: Could not retrieve pass_t object"),
                    __FILE__, __func__);
        return TRUE;
    }

    new_pass = copy_pass(skypass->pass);
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s::%s: Showing pass details for %s"),
                __FILE__, __func__, skypass->pass->satname);

    /* show the pass details */
    show_pass(skypass->pass->satname, skg->qth, new_pass, NULL);

    return TRUE;
}

static void size_allocate_cb(GtkWidget * widget, GtkAllocation * allocation,
                             gpointer data)
{
    GtkSkyGlance   *skg;
    gint            i, j, n;
    guint           curcat;
    gdouble         th, tm;
    sky_pass_t     *skp;
    gdouble         x, y, w, h;
    sat_label_t    *label;
    GSList         *node;

    if (gtk_widget_get_realized(widget))
    {
        skg = GTK_SKY_GLANCE(data);
        skg->w = allocation->width;
        skg->h = allocation->height - SKG_FOOTER;
        skg->x0 = 0;
        skg->y0 = 0;
        skg->pps = (skg->h - SKG_MARGIN) / skg->numsat - SKG_MARGIN;

        /* Update tick positions */
        th = ceil(skg->ts * 24.0) / 24.0;
        th += 0.00069;  /* workaround for bug 1839140 */

        if ((th - skg->ts) > 0.0208333)
            tm = th - 0.0208333;
        else
            tm = th + 0.0208333;

        for (i = 0; i < skg->num_ticks; i++)
        {
            if (skg->major_x)
                skg->major_x[i] = t2x(skg, th);
            if (skg->minor_x)
                skg->minor_x[i] = t2x(skg, tm);

            th += 0.04167;
            tm += 0.04167;
        }

        /* Update pass box positions */
        n = g_slist_length(skg->passes);
        j = -1;
        curcat = 0;
        y = 10.0;
        h = 10.0;
        node = skg->satlab;
        for (i = 0; i < n; i++)
        {
            skp = (sky_pass_t *)g_slist_nth_data(skg->passes, i);

            x = t2x(skg, skp->pass->aos);
            w = t2x(skg, skp->pass->los) - x;

            /* new satellite? */
            if (skp->catnum != curcat)
            {
                j++;
                curcat = skp->catnum;
                y = j * (skg->pps + SKG_MARGIN) + SKG_MARGIN;
                h = skg->pps;

                /* update label position */
                if (node)
                {
                    label = (sat_label_t *)node->data;
                    label->y = y + h / 2.0;
                    if (x > (skg->x0 + 100))
                    {
                        label->x = x - 5;
                        label->anchor = 1;  /* East */
                    }
                    else
                    {
                        label->x = x + w + 5;
                        label->anchor = 0;  /* West */
                    }
                    node = node->next;
                }
            }

            skp->x = x;
            skp->y = y;
            skp->w = w;
            skp->h = h;
        }

        gtk_widget_queue_draw(skg->canvas);
    }
}

static void on_canvas_realized(GtkWidget * canvas, gpointer data)
{
    GtkAllocation   aloc;

    gtk_widget_get_allocation(canvas, &aloc);
    size_allocate_cb(canvas, &aloc, data);
}

/** Fetch the basic colour and add alpha channel */
static void get_colors(guint i, guint * bcol, guint * fcol)
{
    guint           tmp;

    /* ensure that we are within 1..10 */
    i = i % 10;

    switch (i)
    {
    case 0:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_01);
        break;

    case 1:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_02);
        break;

    case 2:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_03);
        break;

    case 3:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_04);
        break;

    case 4:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_05);
        break;

    case 5:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_06);
        break;

    case 6:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_07);
        break;

    case 7:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_08);
        break;

    case 8:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_09);
        break;

    case 9:
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_10);
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Colour index out of valid range (%d)"),
                    __FILE__, __LINE__, i);
        tmp = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_COL_01);
        break;
    }

    /* border colour is solid with no tranparency */
    *bcol = (tmp * 0x100) | 0xFF;

    /* fill colour is slightly transparent */
    *fcol = (tmp * 0x100) | 0xA0;
}

/**
 * Create canvas items for a satellite
 */
static void create_sat(gpointer key, gpointer value, gpointer data)
{
    sat_t          *sat = SAT(value);
    GtkSkyGlance   *skg = GTK_SKY_GLANCE(data);
    GSList         *passes = NULL;
    gdouble         maxdt;
    guint           i, n;
    pass_t         *tmppass = NULL;
    sky_pass_t     *skypass;
    guint           bcol, fcol;
    sat_label_t    *label;

    (void)key;

    get_colors(skg->satcnt++, &bcol, &fcol);
    maxdt = skg->te - skg->ts;

    /* get passes for satellite */
    passes = get_passes(sat, skg->qth, skg->ts, maxdt, 10);
    n = g_slist_length(passes);
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%d: %s has %d passes within %.4f days\n"),
                __FILE__, __LINE__, sat->nickname, n, maxdt);

    if (passes != NULL)
    {
        /* add pass items */
        for (i = 0; i < n; i++)
        {
            skypass = g_try_new0(sky_pass_t, 1);
            if (skypass == NULL)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s:%s: Could not allocate memory."),
                            __FILE__, __func__);
                continue;
            }

            skypass->catnum = sat->tle.catnr;
            tmppass = (pass_t *)g_slist_nth_data(passes, i);
            skypass->pass = copy_pass(tmppass);
            skypass->bcol = bcol;
            skypass->fcol = fcol;

            /* Initial position will be set in size_allocate_cb */
            skypass->x = 0;
            skypass->y = 0;
            skypass->w = 10;
            skypass->h = 10;

            skg->passes = g_slist_append(skg->passes, skypass);
        }

        free_passes(passes);

        /* add satellite label */
        label = g_try_new0(sat_label_t, 1);
        if (label)
        {
            label->name = g_strdup(sat->nickname);
            label->color = bcol;
            label->x = 5;
            label->y = 0;
            label->anchor = 0;
            skg->satlab = g_slist_append(skg->satlab, label);
        }
    }
}

/**
 * Create the time tick data
 */
static void create_time_ticks(GtkSkyGlance * skg)
{
    guint           i;
    gdouble         th, tm;
    gchar           buff[3];

    skg->num_ticks = sat_cfg_get_int(SAT_CFG_INT_SKYATGL_TIME);

    skg->major_x = g_new0(gdouble, skg->num_ticks);
    skg->minor_x = g_new0(gdouble, skg->num_ticks);
    skg->tick_labels = g_new0(gchar *, skg->num_ticks);

    /* get the first hour and first 30 min slot */
    th = ceil(skg->ts * 24.0) / 24.0;
    th += 0.00069;  /* workaround for bug 1839140 */

    if ((th - skg->ts) > 0.0208333)
        tm = th - 0.0208333;
    else
        tm = th + 0.0208333;

    for (i = 0; i < (guint)skg->num_ticks; i++)
    {
        skg->major_x[i] = t2x(skg, th);
        skg->minor_x[i] = t2x(skg, tm);

        daynum_to_str(buff, 3, "%H", th);
        skg->tick_labels[i] = g_strdup(buff);

        th += 0.0416667;
        tm += 0.0416667;
    }
}

/**
 * Create a new GtkSkyGlance widget.
 *
 * @param sats Pointer to the hash table containing the associated satellites.
 * @param qth Pointer to the ground station data.
 * @param ts The t0 for the timeline or 0 to use the current date and time.
 */
GtkWidget      *gtk_sky_glance_new(GHashTable * sats, qth_t * qth, gdouble ts)
{
    GtkSkyGlance   *skg;
    guint           number;
    GValue          font_value = G_VALUE_INIT;

    /* check that we have at least one satellite */
    number = g_hash_table_size(sats);
    if (number == 0)
        /* no satellites */
        return gtk_label_new(_("This module has no satellites!"));

    skg = GTK_SKY_GLANCE(g_object_new(GTK_TYPE_SKY_GLANCE, NULL));

    /* default font */
    g_value_init(&font_value, G_TYPE_STRING);
    g_object_get_property(G_OBJECT(gtk_settings_get_default()), "gtk-font-name", &font_value);
    skg->font = g_value_dup_string(&font_value);
    g_value_unset(&font_value);

    /* FIXME? */
    skg->sats = sats;
    skg->qth = qth;

    /* get settings */
    skg->numsat = g_hash_table_size(sats);

    /* if ts = 0 use current time */
    skg->ts = ts > 0.0 ? ts : get_current_daynum();
    skg->te = skg->ts + sat_cfg_get_int(SAT_CFG_INT_SKYATGL_TIME) * (1.0 / 24.0);

    /* calculate preferred sizes */
    skg->w = SKG_DEFAULT_WIDTH;
    skg->h = skg->numsat * SKG_PIX_PER_SAT + (skg->numsat + 1) * SKG_MARGIN;
    skg->pps = SKG_PIX_PER_SAT;

    /* create the canvas (drawing area) */
    skg->canvas = gtk_drawing_area_new();
    gtk_widget_set_has_tooltip(skg->canvas, TRUE);
    gtk_widget_set_size_request(skg->canvas, skg->w, skg->h + SKG_FOOTER);
    gtk_widget_add_events(skg->canvas, GDK_POINTER_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    /* connect signals */
    g_signal_connect(skg->canvas, "draw", G_CALLBACK(on_draw), skg);
    g_signal_connect(skg->canvas, "motion-notify-event", G_CALLBACK(on_motion_notify), skg);
    g_signal_connect(skg->canvas, "button-release-event", G_CALLBACK(on_button_release), skg);
    g_signal_connect(skg->canvas, "size-allocate", G_CALLBACK(size_allocate_cb), skg);
    g_signal_connect_after(skg->canvas, "realize", G_CALLBACK(on_canvas_realized), skg);

    gtk_widget_show(skg->canvas);

    /* Create the time tick data */
    create_time_ticks(skg);

    /* Create satellite pass data */
    g_hash_table_foreach(skg->sats, create_sat, skg);

    gtk_box_pack_start(GTK_BOX(skg), skg->canvas, TRUE, TRUE, 0);

    return GTK_WIDGET(skg);
}
