/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC
  Copyright (C)       2011  Charles Suprin, AA1 VS

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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-polar-view.h"
#include "gtk-polar-view-popup.h"
#include "gtk-sat-data.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "sat-cfg.h"
#include "sat-info.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

#define POLV_DEFAULT_SIZE 100
#define POLV_DEFAULT_MARGIN 25
#define MARKER_SIZE_HALF 2

/* extra size for line outside 0 deg circle (inside margin) */
#define POLV_LINE_EXTRA 5

static void     update_sat(gpointer key, gpointer value, gpointer data);

static GtkBoxClass *parent_class = NULL;

/** Convert rgba color to cairo-friendly format */
static void rgba_to_cairo(guint32 rgba, gdouble *r, gdouble *g, gdouble *b, gdouble *a)
{
    *r = ((rgba >> 24) & 0xFF) / 255.0;
    *g = ((rgba >> 16) & 0xFF) / 255.0;
    *b = ((rgba >> 8) & 0xFF) / 255.0;
    *a = (rgba & 0xFF) / 255.0;
}

static void gtk_polar_view_store_showtracks(GtkPolarView * pv)
{
    mod_cfg_set_integer_list_boolean(pv->cfgdata,
                                     pv->showtracks_on,
                                     MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_SHOWTRACKS);
    mod_cfg_set_integer_list_boolean(pv->cfgdata,
                                     pv->showtracks_off,
                                     MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_HIDETRACKS);
}

static void gtk_polar_view_load_showtracks(GtkPolarView * pv)
{
    mod_cfg_get_integer_list_boolean(pv->cfgdata,
                                     MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_HIDETRACKS,
                                     pv->showtracks_off);

    mod_cfg_get_integer_list_boolean(pv->cfgdata,
                                     MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_SHOWTRACKS,
                                     pv->showtracks_on);
}

static void free_sat_obj(gpointer data)
{
    sat_obj_t *obj = SAT_OBJ(data);
    if (obj)
    {
        g_free(obj->nickname);
        g_free(obj->tooltip);
        g_slist_free_full(obj->track_points, g_free);
        if (obj->pass)
            free_pass(obj->pass);
        g_free(obj);
    }
}

static void gtk_polar_view_destroy(GtkWidget * widget)
{
    GtkPolarView *polv = GTK_POLAR_VIEW(widget);

    gtk_polar_view_store_showtracks(polv);

    g_free(polv->curs_text);
    polv->curs_text = NULL;

    g_free(polv->next_text);
    polv->next_text = NULL;

    g_free(polv->sel_text);
    polv->sel_text = NULL;

    g_free(polv->font);
    polv->font = NULL;

    if (polv->obj)
    {
        g_hash_table_destroy(polv->obj);
        polv->obj = NULL;
    }

    if (polv->showtracks_on)
    {
        g_hash_table_destroy(polv->showtracks_on);
        polv->showtracks_on = NULL;
    }

    if (polv->showtracks_off)
    {
        g_hash_table_destroy(polv->showtracks_off);
        polv->showtracks_off = NULL;
    }

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_polar_view_class_init(GtkPolarViewClass * class,
				      gpointer class_data)
{
    GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

    (void)class_data;

    widget_class->destroy = gtk_polar_view_destroy;

    parent_class = g_type_class_peek_parent(class);
}

static void gtk_polar_view_init(GtkPolarView * polview,
				gpointer g_class)
{
    (void)g_class;

    polview->sats = NULL;
    polview->qth = NULL;
    polview->obj = NULL;
    polview->naos = 0.0;
    polview->ncat = 0;
    polview->size = 0;
    polview->r = 0;
    polview->cx = 0;
    polview->cy = 0;
    polview->refresh = 0;
    polview->counter = 0;
    polview->swap = 0;
    polview->satname = FALSE;
    polview->satmarker = FALSE;
    polview->qthinfo = FALSE;
    polview->eventinfo = FALSE;
    polview->cursinfo = FALSE;
    polview->extratick = FALSE;
    polview->resize = FALSE;
    polview->curs_text = NULL;
    polview->next_text = NULL;
    polview->sel_text = NULL;
    polview->font = NULL;
}

GType gtk_polar_view_get_type()
{
    static GType    gtk_polar_view_type = 0;

    if (!gtk_polar_view_type)
    {
        static const GTypeInfo gtk_polar_view_info = {
            sizeof(GtkPolarViewClass),
            NULL,               /* base init */
            NULL,               /* base finalise */
            (GClassInitFunc) gtk_polar_view_class_init,
            NULL,               /* class finalise */
            NULL,               /* class data */
            sizeof(GtkPolarView),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_polar_view_init,
            NULL
        };

        gtk_polar_view_type = g_type_register_static(GTK_TYPE_BOX,
                                                     "GtkPolarView",
                                                     &gtk_polar_view_info, 0);
    }

    return gtk_polar_view_type;
}

/** Convert Az/El to canvas based XY coordinates. */
static void azel_to_xy(GtkPolarView * p, gdouble az, gdouble el, gfloat * x,
                       gfloat * y)
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

    case POLAR_VIEW_NWSE:
        az = 2 * M_PI - az;
        break;

    case POLAR_VIEW_SENW:
        az = M_PI - az;
        break;

    case POLAR_VIEW_SWNE:
        az = M_PI + az;
        break;

    default:
        break;
    }

    *x = (gfloat) (p->cx + rel * sin(az));
    *y = (gfloat) (p->cy - rel * cos(az));
}

/** Convert canvas based coordinates to Az/El. */
static void xy_to_azel(GtkPolarView * p, gfloat x, gfloat y, gfloat * az,
                       gfloat * el)
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

    case POLAR_VIEW_NWSE:
        *az = 360.0 - *az;
        break;

    case POLAR_VIEW_SENW:
        if (*az <= 180)
            *az = 180.0 - *az;
        else
            *az = 540.0 - *az;
        break;

    case POLAR_VIEW_SWNE:
        if (*az >= 180.0)
            *az = *az - 180.0;
        else
            *az = 180.0 + *az;
        break;

    default:
        break;
    }
}

/**
 * Transform pole coordinates.
 */
static void
correct_pole_coor(GtkPolarView * polv, polar_view_pole_t pole,
                  gfloat * x, gfloat * y, gboolean *anchor_south, gboolean *anchor_east)
{
    *anchor_south = FALSE;
    *anchor_east = FALSE;

    switch (pole)
    {
    case POLAR_VIEW_POLE_N:
        if ((polv->swap == POLAR_VIEW_SENW) || (polv->swap == POLAR_VIEW_SWNE))
        {
            *y = *y + POLV_LINE_EXTRA;
        }
        else
        {
            *y = *y - POLV_LINE_EXTRA;
            *anchor_south = TRUE;
        }
        break;

    case POLAR_VIEW_POLE_E:
        if ((polv->swap == POLAR_VIEW_NWSE) || (polv->swap == POLAR_VIEW_SWNE))
        {
            *x = *x - POLV_LINE_EXTRA;
            *anchor_east = TRUE;
        }
        else
        {
            *x = *x + POLV_LINE_EXTRA;
        }
        break;

    case POLAR_VIEW_POLE_S:
        if ((polv->swap == POLAR_VIEW_SENW) || (polv->swap == POLAR_VIEW_SWNE))
        {
            *y = *y - POLV_LINE_EXTRA;
            *anchor_south = TRUE;
        }
        else
        {
            *y = *y + POLV_LINE_EXTRA;
        }
        break;

    case POLAR_VIEW_POLE_W:
        if ((polv->swap == POLAR_VIEW_NWSE) || (polv->swap == POLAR_VIEW_SWNE))
        {
            *x = *x + POLV_LINE_EXTRA;
        }
        else
        {
            *x = *x - POLV_LINE_EXTRA;
            *anchor_east = TRUE;
        }
        break;

    default:
        break;
    }
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GtkPolarView   *polv = GTK_POLAR_VIEW(data);
    gdouble         r, g, b, a;
    gfloat          x, y;
    gboolean        anchor_south, anchor_east;
    PangoLayout    *layout;
    PangoFontDescription *font_desc;
    gint            tw, th;
    GHashTableIter  iter;
    gpointer        key, value;
    sat_obj_t      *obj;
    GSList         *node;
    gdouble        *point;
    guint           i;

    (void)widget;

    /* Background */
    rgba_to_cairo(polv->col_bgd, &r, &g, &b, &a);
    cairo_set_source_rgba(cr, r, g, b, a);
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
    correct_pole_coor(polv, POLAR_VIEW_POLE_N, &x, &y, &anchor_south, &anchor_east);
    pango_layout_set_text(layout, _("N"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, x - tw / 2, anchor_south ? y : y - th);
    pango_cairo_show_layout(cr, layout);

    /* E label */
    azel_to_xy(polv, 90.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_VIEW_POLE_E, &x, &y, &anchor_south, &anchor_east);
    pango_layout_set_text(layout, _("E"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, anchor_east ? x - tw : x, y - th / 2);
    pango_cairo_show_layout(cr, layout);

    /* S label */
    azel_to_xy(polv, 180.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_VIEW_POLE_S, &x, &y, &anchor_south, &anchor_east);
    pango_layout_set_text(layout, _("S"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, x - tw / 2, anchor_south ? y : y - th);
    pango_cairo_show_layout(cr, layout);

    /* W label */
    azel_to_xy(polv, 270.0, 0.0, &x, &y);
    correct_pole_coor(polv, POLAR_VIEW_POLE_W, &x, &y, &anchor_south, &anchor_east);
    pango_layout_set_text(layout, _("W"), -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, anchor_east ? x - tw : x, y - th / 2);
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

    /* Next event text */
    if (polv->eventinfo && polv->next_text)
    {
        rgba_to_cairo(polv->col_info, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
        pango_layout_set_text(layout, polv->next_text, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, polv->cx + polv->r + 2 * POLV_LINE_EXTRA - tw,
                      polv->cy - polv->r - POLV_LINE_EXTRA - th);
        pango_cairo_show_layout(cr, layout);
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
    }

    /* Selected satellite text */
    if (polv->sel_text)
    {
        rgba_to_cairo(polv->col_info, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
        pango_layout_set_text(layout, polv->sel_text, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, polv->cx + polv->r + 2 * POLV_LINE_EXTRA - tw,
                      polv->cy + polv->r + POLV_LINE_EXTRA);
        pango_cairo_show_layout(cr, layout);
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
    }

    /* Draw satellite objects */
    if (polv->obj)
    {
        g_hash_table_iter_init(&iter, polv->obj);
        while (g_hash_table_iter_next(&iter, &key, &value))
        {
            obj = SAT_OBJ(value);

            /* Draw track if enabled */
            if (obj->showtrack && obj->track_points)
            {
                rgba_to_cairo(polv->col_track, &r, &g, &b, &a);
                cairo_set_source_rgba(cr, r, g, b, a);
                cairo_set_line_width(cr, 1.0);

                node = obj->track_points;
                if (node)
                {
                    point = (gdouble *)node->data;
                    cairo_move_to(cr, point[0], point[1]);
                    node = node->next;

                    while (node)
                    {
                        point = (gdouble *)node->data;
                        cairo_line_to(cr, point[0], point[1]);
                        node = node->next;
                    }
                    cairo_stroke(cr);
                }

                /* Draw time ticks */
                for (i = 0; i < TRACK_TICK_NUM; i++)
                {
                    if (obj->trtick[i].text[0] != '\0')
                    {
                        x = obj->trtick[i].x;
                        y = obj->trtick[i].y;
                        pango_layout_set_text(layout, obj->trtick[i].text, -1);
                        pango_layout_get_pixel_size(layout, &tw, &th);

                        if (x > polv->cx)
                            cairo_move_to(cr, x - tw - 5, y - th / 2);
                        else
                            cairo_move_to(cr, x + 5, y - th / 2);

                        pango_cairo_show_layout(cr, layout);
                    }
                }
            }

            /* Draw satellite marker */
            if (polv->satmarker)
            {
                if (obj->selected)
                    rgba_to_cairo(polv->col_sat_sel, &r, &g, &b, &a);
                else
                    rgba_to_cairo(polv->col_sat, &r, &g, &b, &a);

                cairo_set_source_rgba(cr, r, g, b, a);
                cairo_rectangle(cr, obj->x - MARKER_SIZE_HALF, obj->y - MARKER_SIZE_HALF,
                                2 * MARKER_SIZE_HALF, 2 * MARKER_SIZE_HALF);
                cairo_fill(cr);
            }

            /* Draw satellite name */
            if (polv->satname && obj->nickname)
            {
                if (obj->selected)
                    rgba_to_cairo(polv->col_sat_sel, &r, &g, &b, &a);
                else
                    rgba_to_cairo(polv->col_sat, &r, &g, &b, &a);

                cairo_set_source_rgba(cr, r, g, b, a);
                pango_layout_set_text(layout, obj->nickname, -1);
                pango_layout_get_pixel_size(layout, &tw, &th);
                cairo_move_to(cr, obj->x - tw / 2, obj->y + 2);
                pango_cairo_show_layout(cr, layout);
            }
        }
    }

    pango_font_description_free(font_desc);
    g_object_unref(layout);

    return FALSE;
}

static sat_obj_t *find_sat_at_pos(GtkPolarView *polv, gfloat mx, gfloat my)
{
    GHashTableIter  iter;
    gpointer        key, value;
    sat_obj_t      *obj;
    gfloat          dx, dy;
    const gfloat    hit_radius = 10.0;

    if (polv->obj == NULL)
        return NULL;

    g_hash_table_iter_init(&iter, polv->obj);
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
        obj = SAT_OBJ(value);
        dx = mx - obj->x;
        dy = my - obj->y;
        if (dx * dx + dy * dy < hit_radius * hit_radius)
            return obj;
    }
    return NULL;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkPolarView   *polv = GTK_POLAR_VIEW(data);
    sat_obj_t      *obj;
    sat_t          *sat = NULL;
    gint           *catpoint = NULL;

    (void)widget;

    obj = find_sat_at_pos(polv, event->x, event->y);

    if (obj == NULL)
        return FALSE;

    switch (event->button)
    {
    case 1:
        if (event->type == GDK_2BUTTON_PRESS)
        {
            /* Double-click: show satellite info */
            catpoint = g_try_new0(gint, 1);
            *catpoint = obj->catnum;
            sat = SAT(g_hash_table_lookup(polv->sats, catpoint));
            if (sat != NULL)
            {
                show_sat_info(sat, gtk_widget_get_toplevel(GTK_WIDGET(polv)));
            }
            g_free(catpoint);
        }
        break;

    case 3:
        /* Right-click: popup menu */
        catpoint = g_try_new0(gint, 1);
        *catpoint = obj->catnum;
        sat = SAT(g_hash_table_lookup(polv->sats, catpoint));
        if (sat != NULL)
        {
            gtk_polar_view_popup_exec(sat, polv->qth, polv, event,
                                      gtk_widget_get_toplevel(GTK_WIDGET(polv)));
        }
        g_free(catpoint);
        break;

    default:
        break;
    }

    return TRUE;
}

static void clear_selection(gpointer key, gpointer val, gpointer data)
{
    gint           *old = key;
    gint           *new = data;
    sat_obj_t      *obj = SAT_OBJ(val);

    if ((*old != *new) && (obj->selected))
    {
        obj->selected = FALSE;
    }
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkPolarView   *polv = GTK_POLAR_VIEW(data);
    sat_obj_t      *obj;
    gint           *catpoint;

    (void)widget;

    if (event->button != 1)
        return FALSE;

    obj = find_sat_at_pos(polv, event->x, event->y);

    if (obj == NULL)
        return FALSE;

    obj->selected = !obj->selected;

    catpoint = g_try_new0(gint, 1);
    *catpoint = obj->catnum;

    if (!obj->selected)
    {
        g_free(polv->sel_text);
        polv->sel_text = NULL;
        *catpoint = 0;
    }

    /* clear other selections */
    g_hash_table_foreach(polv->obj, clear_selection, catpoint);

    g_free(catpoint);

    gtk_widget_queue_draw(polv->canvas);

    return TRUE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    GtkPolarView   *polv = GTK_POLAR_VIEW(data);
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

static void size_allocate_cb(GtkWidget * widget, GtkAllocation * allocation,
                             gpointer data)
{
    (void)widget;
    (void)allocation;
    GTK_POLAR_VIEW(data)->resize = TRUE;
}

static void on_canvas_realized(GtkWidget * canvas, gpointer data)
{
    GtkAllocation   aloc;

    gtk_widget_get_allocation(canvas, &aloc);
    size_allocate_cb(canvas, &aloc, data);
}

GtkWidget *gtk_polar_view_new(GKeyFile * cfgdata, GHashTable * sats, qth_t * qth)
{
    GtkPolarView   *polv;
    GValue          font_value = G_VALUE_INIT;

    polv = GTK_POLAR_VIEW(g_object_new(GTK_TYPE_POLAR_VIEW, NULL));

    polv->cfgdata = cfgdata;
    polv->sats = sats;
    polv->qth = qth;

    polv->obj = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_sat_obj);
    polv->showtracks_on = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL);
    polv->showtracks_off = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL);

    /* get settings */
    polv->refresh = mod_cfg_get_int(cfgdata, MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_REFRESH,
                                    SAT_CFG_INT_POLAR_REFRESH);

    polv->showtrack = mod_cfg_get_bool(cfgdata, MOD_CFG_POLAR_SECTION,
                                       MOD_CFG_POLAR_SHOW_TRACK_AUTO,
                                       SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO);

    polv->counter = 1;

    polv->swap = mod_cfg_get_int(cfgdata, MOD_CFG_POLAR_SECTION,
                                 MOD_CFG_POLAR_ORIENTATION,
                                 SAT_CFG_INT_POLAR_ORIENTATION);

    polv->satname = mod_cfg_get_bool(cfgdata, MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_SHOW_SAT_NAME,
                                     SAT_CFG_BOOL_POL_SHOW_SAT_NAME);
    polv->satmarker = mod_cfg_get_bool(cfgdata, MOD_CFG_POLAR_SECTION,
                                       MOD_CFG_POLAR_SHOW_SAT_MARKER,
                                       SAT_CFG_BOOL_POL_SHOW_SAT_MARKER);

    polv->qthinfo = mod_cfg_get_bool(cfgdata, MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_SHOW_QTH_INFO,
                                     SAT_CFG_BOOL_POL_SHOW_QTH_INFO);

    polv->eventinfo = mod_cfg_get_bool(cfgdata, MOD_CFG_POLAR_SECTION,
                                       MOD_CFG_POLAR_SHOW_NEXT_EVENT,
                                       SAT_CFG_BOOL_POL_SHOW_NEXT_EV);

    polv->cursinfo = mod_cfg_get_bool(cfgdata, MOD_CFG_POLAR_SECTION,
                                      MOD_CFG_POLAR_SHOW_CURS_TRACK,
                                      SAT_CFG_BOOL_POL_SHOW_CURS_TRACK);

    polv->extratick = mod_cfg_get_bool(cfgdata, MOD_CFG_POLAR_SECTION,
                                       MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS,
                                       SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);

    gtk_polar_view_load_showtracks(polv);

    /* get colors */
    polv->col_bgd = mod_cfg_get_int(polv->cfgdata, MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_BGD_COL, SAT_CFG_INT_POLAR_BGD_COL);
    polv->col_axis = mod_cfg_get_int(polv->cfgdata, MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_AXIS_COL, SAT_CFG_INT_POLAR_AXIS_COL);
    polv->col_tick = mod_cfg_get_int(polv->cfgdata, MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_TICK_COL, SAT_CFG_INT_POLAR_TICK_COL);
    polv->col_info = mod_cfg_get_int(polv->cfgdata, MOD_CFG_POLAR_SECTION,
                                     MOD_CFG_POLAR_INFO_COL, SAT_CFG_INT_POLAR_INFO_COL);
    polv->col_sat = mod_cfg_get_int(polv->cfgdata, MOD_CFG_POLAR_SECTION,
                                    MOD_CFG_POLAR_SAT_COL, SAT_CFG_INT_POLAR_SAT_COL);
    polv->col_sat_sel = mod_cfg_get_int(polv->cfgdata, MOD_CFG_POLAR_SECTION,
                                        MOD_CFG_POLAR_SAT_SEL_COL, SAT_CFG_INT_POLAR_SAT_SEL_COL);
    polv->col_track = mod_cfg_get_int(polv->cfgdata, MOD_CFG_POLAR_SECTION,
                                      MOD_CFG_POLAR_TRACK_COL, SAT_CFG_INT_POLAR_TRACK_COL);

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
    gtk_widget_set_has_tooltip(polv->canvas, TRUE);
    gtk_widget_set_size_request(polv->canvas, POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);
    gtk_widget_add_events(polv->canvas, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK);

    /* connect signals */
    g_signal_connect(polv->canvas, "draw", G_CALLBACK(on_draw), polv);
    g_signal_connect(polv->canvas, "motion-notify-event", G_CALLBACK(on_motion_notify), polv);
    g_signal_connect(polv->canvas, "button-press-event", G_CALLBACK(on_button_press), polv);
    g_signal_connect(polv->canvas, "button-release-event", G_CALLBACK(on_button_release), polv);
    g_signal_connect(polv->canvas, "size-allocate", G_CALLBACK(size_allocate_cb), polv);
    g_signal_connect_after(polv->canvas, "realize", G_CALLBACK(on_canvas_realized), polv);

    gtk_widget_show(polv->canvas);
    gtk_box_pack_start(GTK_BOX(polv), polv->canvas, TRUE, TRUE, 0);

    return GTK_WIDGET(polv);
}

static void update_polv_size(GtkPolarView * polv)
{
    GtkAllocation   allocation;

    if (gtk_widget_get_realized(GTK_WIDGET(polv)))
    {
        gtk_widget_get_allocation(GTK_WIDGET(polv), &allocation);

        polv->size = MIN(allocation.width, allocation.height);
        polv->r = (polv->size / 2) - POLV_DEFAULT_MARGIN;
        polv->cx = allocation.width / 2;
        polv->cy = allocation.height / 2;

        /* Update satellite positions */
        g_hash_table_foreach(polv->sats, update_sat, polv);
    }
}

/** Convert LOS timestamp to human readable countdown string */
static gchar *los_time_to_str(GtkPolarView * polv, sat_t * sat)
{
    guint           h, m, s;
    gdouble         number, now;
    gchar          *text = NULL;

    now = polv->tstamp;
    number = sat->los - now;

    /* convert julian date to seconds */
    s = (guint) (number * 86400);

    /* extract hours */
    h = (guint) floor(s / 3600);
    s -= 3600 * h;

    /* extract minutes */
    m = (guint) floor(s / 60);
    s -= 60 * m;

    if (h > 0)
        text = g_strdup_printf(_("LOS in %02d:%02d:%02d"), h, m, s);
    else
        text = g_strdup_printf(_("LOS in %02d:%02d"), m, s);

    return text;
}

void gtk_polar_view_update(GtkWidget * widget)
{
    GtkPolarView   *polv = GTK_POLAR_VIEW(widget);
    gdouble         number, now;
    gchar          *buff;
    guint           h, m, s;
    sat_t          *sat = NULL;
    gint           *catnr;

    if (polv->resize)
    {
        update_polv_size(polv);
        polv->resize = FALSE;
    }

    /* check refresh rate and refresh sats if time */
    /* FIXME need to add location based update */
    if (polv->counter < polv->refresh)
    {
        polv->counter++;
    }
    else
    {
        /* reset data */
        polv->counter = 1;
        polv->naos = 0.0;
        polv->ncat = 0;

        /* update sats */
        g_hash_table_foreach(polv->sats, update_sat, polv);

        /* update countdown to NEXT AOS label */
        if (polv->eventinfo)
        {
            if (polv->ncat > 0)
            {
                catnr = g_try_new0(gint, 1);
                *catnr = polv->ncat;
                sat = SAT(g_hash_table_lookup(polv->sats, catnr));
                g_free(catnr);

                if (sat != NULL)
                {
                    now = polv->tstamp;
                    number = polv->naos - now;

                    s = (guint) (number * 86400);
                    h = (guint) floor(s / 3600);
                    s -= 3600 * h;
                    m = (guint) floor(s / 60);
                    s -= 60 * m;

                    if (h > 0)
                        buff = g_strdup_printf(_("Next: %s\nin %02d:%02d:%02d"),
                                               sat->nickname, h, m, s);
                    else
                        buff = g_strdup_printf(_("Next: %s\nin %02d:%02d"),
                                               sat->nickname, m, s);

                    g_free(polv->next_text);
                    polv->next_text = buff;
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Can not find NEXT satellite."), __func__);
                    g_free(polv->next_text);
                    polv->next_text = g_strdup(_("Next: ERR"));
                }
            }
            else
            {
                g_free(polv->next_text);
                polv->next_text = g_strdup(_("Next: N/A"));
            }
        }
        else
        {
            g_free(polv->next_text);
            polv->next_text = NULL;
        }

        gtk_widget_queue_draw(polv->canvas);
    }
}

static void update_sat(gpointer key, gpointer value, gpointer data)
{
    gint           *catnum;
    sat_t          *sat = SAT(value);
    GtkPolarView   *polv = GTK_POLAR_VIEW(data);
    sat_obj_t      *obj = NULL;
    gfloat          x, y;
    gdouble         now;
    gchar          *losstr;
    gchar          *text;
    guint           num, i;
    pass_detail_t  *detail;
    gdouble        *point;
    guint           tres, ttidx;

    (void)key;

    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;

    now = polv->tstamp;

    /* update next AOS */
    if (sat->aos > now)
    {
        if ((sat->aos < polv->naos) || (polv->naos == 0.0))
        {
            polv->naos = sat->aos;
            polv->ncat = sat->tle.catnr;
        }
    }

    /* if sat is out of range */
    if ((sat->el < 0.00) || decayed(sat))
    {
        obj = SAT_OBJ(g_hash_table_lookup(polv->obj, catnum));

        if (obj != NULL)
        {
            /* if this was the selected satellite we need to
               clear the info text
             */
            if (obj->selected)
            {
                g_free(polv->sel_text);
                polv->sel_text = NULL;
            }

            /* remove sat object from hash table (this will free it) */
            g_hash_table_remove(polv->obj, catnum);
        }

        g_free(catnum);
    }
    else
    {
        /* sat is within range */
        obj = SAT_OBJ(g_hash_table_lookup(polv->obj, catnum));
        azel_to_xy(polv, sat->az, sat->el, &x, &y);

        if (obj != NULL)
        {
            /* update existing satellite */
            obj->x = x;
            obj->y = y;

            /* update nickname */
            g_free(obj->nickname);
            obj->nickname = g_strdup(sat->nickname);

            /* update LOS count down */
            if (sat->los > 0.0)
                losstr = los_time_to_str(polv, sat);
            else
                losstr = g_strdup_printf(_("%s\nAlways in range"), sat->nickname);

            /* update tooltip */
            g_free(obj->tooltip);
            obj->tooltip = g_markup_printf_escaped("<b>%s</b>\nAz: %5.1f\302\260\nEl: %5.1f\302\260\n%s",
                                                   sat->nickname, sat->az, sat->el, losstr);

            /* update selection info */
            if (obj->selected)
            {
                g_free(polv->sel_text);
                polv->sel_text = g_strdup_printf("%s\n%s", sat->nickname, losstr);
            }

            /* Check if pass needs update */
            if (obj->pass)
            {
                /** FIXME: threshold */
                gboolean qth_upd = qth_small_dist(polv->qth, (obj->pass->qth_comp)) > 1.0;
                gboolean time_upd = !((obj->pass->aos <= now) && (obj->pass->los >= now));

                if (qth_upd || time_upd)
                {
                    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                _("%s:%s: Updating satellite pass SAT:%d Q:%d T:%d\n"),
                                __FILE__, __func__, *catnum, qth_upd, time_upd);

                    /* Free old track and pass */
                    g_slist_free_full(obj->track_points, g_free);
                    obj->track_points = NULL;
                    free_pass(obj->pass);
                    obj->pass = NULL;

                    /* Compute new pass */
                    obj->pass = get_current_pass(sat, polv->qth, now);

                    /* Recreate track if needed */
                    if (obj->showtrack && obj->pass)
                        gtk_polar_view_create_track(polv, obj, sat);
                }
            }

            g_free(losstr);
            g_free(catnum);     // FIXME: why free here, what about else?
        }
        else
        {
            /* add new satellite */
            obj = g_try_new0(sat_obj_t, 1);

            if (obj != NULL)
            {
                obj->selected = FALSE;
                obj->x = x;
                obj->y = y;
                obj->catnum = sat->tle.catnr;
                obj->nickname = g_strdup(sat->nickname);
                obj->track_points = NULL;

                if (g_hash_table_lookup_extended(polv->showtracks_on, catnum, NULL, NULL))
                    obj->showtrack = TRUE;
                else if (g_hash_table_lookup_extended(polv->showtracks_off, catnum, NULL, NULL))
                    obj->showtrack = FALSE;
                else
                    obj->showtrack = polv->showtrack;

                obj->istarget = FALSE;

                /* create tooltip */
                obj->tooltip = g_markup_printf_escaped("<b>%s</b>\nAz: %5.1f\302\260\nEl: %5.1f\302\260\n",
                                                       sat->nickname, sat->az, sat->el);

                /* get info about the current pass */
                obj->pass = get_current_pass(sat, polv->qth, now);

                /* add sat to hash table */
                g_hash_table_insert(polv->obj, catnum, obj);

                /* create the sky track if necessary */
                if (obj->showtrack)
                    gtk_polar_view_create_track(polv, obj, sat);
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: Cannot allocate memory for satellite %d."),
                            __func__, sat->tle.catnr);
                g_free(catnum);
                return;
            }
        }
    }
}

void gtk_polar_view_create_track(GtkPolarView * pv, sat_obj_t * obj, sat_t * sat)
{
    guint           num, i;
    pass_detail_t  *detail;
    gfloat          x, y;
    gdouble        *point;
    guint           tres, ttidx;

    (void)sat;

    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s:%d: Failed to get satellite object."), __FILE__, __LINE__);
        return;
    }

    if (obj->pass == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s:%d: Failed to get satellite pass."), __FILE__, __LINE__);
        return;
    }

    /* Clear existing track points */
    g_slist_free_full(obj->track_points, g_free);
    obj->track_points = NULL;

    /* Create points */
    num = g_slist_length(obj->pass->details);
    if (num == 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s:%d: Pass had no points in it."), __FILE__, __LINE__);
        return;
    }

    /* time resolution for time ticks */
    tres = (num > 2) ? (num - 2) / (TRACK_TICK_NUM - 1) : 1;

    /* first point should be (aos_az,0.0) */
    azel_to_xy(pv, obj->pass->aos_az, 0.0, &x, &y);
    point = g_new(gdouble, 2);
    point[0] = x;
    point[1] = y;
    obj->track_points = g_slist_append(obj->track_points, point);

    /* first time tick */
    obj->trtick[0].x = x;
    obj->trtick[0].y = y;
    daynum_to_str(obj->trtick[0].text, 6, "%H:%M", obj->pass->aos);

    ttidx = 1;

    for (i = 1; i < num - 1; i++)
    {
        detail = PASS_DETAIL(g_slist_nth_data(obj->pass->details, i));
        if (detail->el >= 0.0)
            azel_to_xy(pv, detail->az, detail->el, &x, &y);

        point = g_new(gdouble, 2);
        point[0] = x;
        point[1] = y;
        obj->track_points = g_slist_append(obj->track_points, point);

        if (tres != 0 && !(i % tres))
        {
            if (ttidx < TRACK_TICK_NUM)
            {
                gfloat tx = x;
                if (tx > pv->cx)
                    tx -= 5;
                else
                    tx += 5;

                obj->trtick[ttidx].x = tx;
                obj->trtick[ttidx].y = y;
                daynum_to_str(obj->trtick[ttidx].text, 6, "%H:%M", detail->time);
            }
            ttidx++;
        }
    }

    /* last point should be (los_az, 0.0) */
    azel_to_xy(pv, obj->pass->los_az, 0.0, &x, &y);
    point = g_new(gdouble, 2);
    point[0] = x;
    point[1] = y;
    obj->track_points = g_slist_append(obj->track_points, point);
}

void gtk_polar_view_delete_track(GtkPolarView * pv, sat_obj_t * obj, sat_t * sat)
{
    (void)pv;
    (void)sat;

    if (obj)
    {
        g_slist_free_full(obj->track_points, g_free);
        obj->track_points = NULL;

        /* Clear time ticks */
        memset(obj->trtick, 0, sizeof(obj->trtick));
    }
}

void gtk_polar_view_reload_sats(GtkWidget * polv, GHashTable * sats)
{
    GTK_POLAR_VIEW(polv)->sats = sats;
    GTK_POLAR_VIEW(polv)->naos = 0.0;
    GTK_POLAR_VIEW(polv)->ncat = 0;
}

void gtk_polar_view_select_sat(GtkWidget * widget, gint catnum)
{
    GtkPolarView   *polv = GTK_POLAR_VIEW(widget);
    gint           *catpoint = NULL;
    sat_obj_t      *obj = NULL;

    catpoint = g_try_new0(gint, 1);
    *catpoint = catnum;

    obj = SAT_OBJ(g_hash_table_lookup(polv->obj, catpoint));
    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s Requested satellite (%d) is not within range"),
                    __func__, catnum);
    }
    else
    {
        obj->selected = TRUE;
    }

    /* clear previous selection, if any */
    g_hash_table_foreach(polv->obj, clear_selection, catpoint);

    g_free(catpoint);

    gtk_widget_queue_draw(polv->canvas);
}
