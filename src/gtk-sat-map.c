/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.
  Copyright (C)  2006-2007  William J Beksi, KC2EXL.
  Copyright (C)  2013       Charles Suprin,  AA1VS.
 
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include <string.h>

#include "compat.h"
#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-data.h"
#include "gtk-sat-map-popup.h"
#include "gtk-sat-map-ground-track.h"
#include "gtk-sat-map.h"
#include "locator.h"
#include "map-tools.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-info.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"

#define MARKER_SIZE_HALF    1

/* Update terminator every 30 seconds */
#define TERMINATOR_UPDATE_INTERVAL (15.0/86400.0)

static void     gtk_sat_map_class_init(GtkSatMapClass * class,
                                       gpointer class_data);
static void     gtk_sat_map_init(GtkSatMap * polview,
                                 gpointer g_class);
static void     gtk_sat_map_destroy(GtkWidget * widget);
static void     size_allocate_cb(GtkWidget * widget,
                                 GtkAllocation * allocation, gpointer data);
static void     update_map_size(GtkSatMap * satmap);
static void     update_sat(gpointer key, gpointer value, gpointer data);
static void     plot_sat(gpointer key, gpointer value, gpointer data);
static void     free_sat_obj(gpointer key, gpointer value, gpointer data);
static void     lonlat_to_xy(GtkSatMap * m, gdouble lon, gdouble lat,
                             gfloat * x, gfloat * y);
static void     xy_to_lonlat(GtkSatMap * m, gfloat x, gfloat y, gfloat * lon,
                             gfloat * lat);
static gboolean on_motion_notify(GtkWidget * widget, GdkEventMotion * event,
                                 gpointer data);
static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event,
                                gpointer data);
static gboolean on_button_release(GtkWidget * widget, GdkEventButton * event,
                                  gpointer data);
static gboolean on_draw(GtkWidget * widget, cairo_t * cr, gpointer data);
static void     clear_selection(gpointer key, gpointer val, gpointer data);
static void     load_map_file(GtkSatMap * satmap, float clon);
static gdouble  arccos(gdouble, gdouble);
static gboolean pole_is_covered(sat_t * sat);
static gboolean north_pole_is_covered(sat_t * sat);
static gboolean south_pole_is_covered(sat_t * sat);
static gboolean mirror_lon(sat_t * sat, gdouble rangelon, gdouble * mlon,
                           gdouble mapbreak);
static guint    calculate_footprint(GtkSatMap * satmap, sat_t * sat,
                                    sat_map_obj_t * obj);
static void     split_points(GtkSatMap * satmap, sat_t * sat, gdouble sspx,
                             gdouble * points1, gint * n1,
                             gdouble * points2, gint * n2);
static void     sort_points_x(GtkSatMap * satmap, sat_t * sat,
                              gdouble * points, gint num);
static void     sort_points_y(GtkSatMap * satmap, sat_t * sat,
                              gdouble * points, gint num);
static gint     compare_coordinates_x(gconstpointer a, gconstpointer b,
                                      gpointer data);
static gint     compare_coordinates_y(gconstpointer a, gconstpointer b,
                                      gpointer data);
static void     update_selected(GtkSatMap * satmap, sat_t * sat);
static void     redraw_terminator(GtkSatMap * satmap);
static gchar   *aoslos_time_to_str(GtkSatMap * satmap, sat_t * sat);
static void     gtk_sat_map_load_showtracks(GtkSatMap * map);
static void     gtk_sat_map_store_showtracks(GtkSatMap * satmap);
static void     gtk_sat_map_load_hide_coverages(GtkSatMap * map);
static void     gtk_sat_map_store_hidecovs(GtkSatMap * satmap);
static void     reset_ground_track(gpointer key, gpointer value,
                                   gpointer user_data);
static sat_map_obj_t *find_sat_at_pos(GtkSatMap * satmap, gfloat mx, gfloat my);

static GtkBoxClass *parent_class = NULL;

/* Temporary arrays for footprint calculation */
static gdouble *temp_points1 = NULL;
static gdouble *temp_points2 = NULL;

/** Convert rgba color to cairo-friendly format */
static void rgba_to_cairo(guint32 rgba, gdouble * r, gdouble * g,
                          gdouble * b, gdouble * a)
{
    *r = ((rgba >> 24) & 0xFF) / 255.0;
    *g = ((rgba >> 16) & 0xFF) / 255.0;
    *b = ((rgba >> 8) & 0xFF) / 255.0;
    *a = (rgba & 0xFF) / 255.0;
}

GType gtk_sat_map_get_type()
{
    static GType    gtk_sat_map_type = 0;

    if (!gtk_sat_map_type)
    {
        static const GTypeInfo gtk_sat_map_info = {
            sizeof(GtkSatMapClass),
            NULL,               /* base init */
            NULL,               /* base finalize */
            (GClassInitFunc) gtk_sat_map_class_init,
            NULL,               /* class finalize */
            NULL,               /* class data */
            sizeof(GtkSatMap),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_sat_map_init,
            NULL
        };

        gtk_sat_map_type = g_type_register_static(GTK_TYPE_BOX,
                                                  "GtkSatMap",
                                                  &gtk_sat_map_info, 0);
    }

    return gtk_sat_map_type;
}

static void gtk_sat_map_class_init(GtkSatMapClass * class,
                                   gpointer class_data)
{
    GtkWidgetClass *widget_class;

    (void)class_data;

    widget_class = (GtkWidgetClass *) class;
    widget_class->destroy = gtk_sat_map_destroy;
    parent_class = g_type_class_peek_parent(class);
}

static void gtk_sat_map_init(GtkSatMap * satmap,
                             gpointer g_class)
{
    (void)g_class;

    satmap->sats = NULL;
    satmap->qth = NULL;
    satmap->obj = NULL;
    satmap->showtracks = g_hash_table_new_full(g_int_hash, g_int_equal,
                                               NULL, NULL);
    satmap->hidecovs = g_hash_table_new_full(g_int_hash, g_int_equal,
                                             NULL, NULL);
    satmap->naos = 0.0;
    satmap->ncat = 0;
    satmap->tstamp = 2458849.5;
    satmap->x0 = 0;
    satmap->y0 = 0;
    satmap->width = 0;
    satmap->height = 0;
    satmap->refresh = 0;
    satmap->counter = 0;
    satmap->satname = FALSE;
    satmap->satfp = FALSE;
    satmap->satmarker = FALSE;
    satmap->show_terminator = FALSE;
    satmap->qthinfo = FALSE;
    satmap->eventinfo = FALSE;
    satmap->cursinfo = FALSE;
    satmap->showgrid = FALSE;
    satmap->keepratio = FALSE;
    satmap->resize = FALSE;
    satmap->locnam_text = NULL;
    satmap->curs_text = NULL;
    satmap->next_text = NULL;
    satmap->sel_text = NULL;
    satmap->terminator_points = NULL;
    satmap->terminator_count = 0;
    satmap->font = NULL;
    satmap->map = NULL;
    satmap->grid_lines_valid = FALSE;
}

static void gtk_sat_map_destroy(GtkWidget * widget)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(widget);

    /* check widget isn't already destroyed */
    if (satmap->obj)
    {
        /* save config */
        gtk_sat_map_store_showtracks(GTK_SAT_MAP(widget));
        gtk_sat_map_store_hidecovs(GTK_SAT_MAP(widget));

        /* sat objects need a reference to the widget to free all allocations */
        g_hash_table_foreach(satmap->obj, free_sat_obj, satmap);
        g_hash_table_destroy(satmap->obj);
        satmap->obj = NULL;

        /* free the original map pixbuf */
        if (satmap->origmap)
        {
            g_object_unref(satmap->origmap);
            satmap->origmap = NULL;
        }

        /* free the scaled map pixbuf */
        if (satmap->map)
        {
            g_object_unref(satmap->map);
            satmap->map = NULL;
        }

        g_hash_table_destroy(satmap->showtracks);
        satmap->showtracks = NULL;
        g_hash_table_destroy(satmap->hidecovs);
        satmap->hidecovs = NULL;

        /* free text strings */
        g_free(satmap->locnam_text);
        satmap->locnam_text = NULL;
        g_free(satmap->curs_text);
        satmap->curs_text = NULL;
        g_free(satmap->next_text);
        satmap->next_text = NULL;
        g_free(satmap->sel_text);
        satmap->sel_text = NULL;
        g_free(satmap->font);
        satmap->font = NULL;
        g_free(satmap->infobgd);
        satmap->infobgd = NULL;

        /* free terminator points */
        g_free(satmap->terminator_points);
        satmap->terminator_points = NULL;
        satmap->terminator_count = 0;

        /* free temporary point arrays */
        g_free(temp_points1);
        temp_points1 = NULL;
        g_free(temp_points2);
        temp_points2 = NULL;
    }
    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

GtkWidget      *gtk_sat_map_new(GKeyFile * cfgdata, GHashTable * sats,
                                qth_t * qth)
{
    GtkSatMap      *satmap;
    guint32         col;
    GValue          font_value = G_VALUE_INIT;

    satmap = g_object_new(GTK_TYPE_SAT_MAP, NULL);

    satmap->cfgdata = cfgdata;
    satmap->sats = sats;
    satmap->qth = qth;

    satmap->obj = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, g_free);

    satmap->refresh = mod_cfg_get_int(cfgdata,
                                      MOD_CFG_MAP_SECTION,
                                      MOD_CFG_MAP_REFRESH,
                                      SAT_CFG_INT_MAP_REFRESH);
    satmap->counter = 1;

    satmap->satname = mod_cfg_get_bool(cfgdata,
                                       MOD_CFG_MAP_SECTION,
                                       MOD_CFG_MAP_SHOW_SAT_NAME,
                                       SAT_CFG_BOOL_MAP_SHOW_SAT_NAME);
    satmap->satfp = mod_cfg_get_bool(cfgdata,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_SHOW_SAT_FP,
                                     SAT_CFG_BOOL_MAP_SHOW_SAT_FP);
    satmap->satmarker = mod_cfg_get_bool(cfgdata,
                                         MOD_CFG_MAP_SECTION,
                                         MOD_CFG_MAP_SHOW_SAT_MARKER,
                                         SAT_CFG_BOOL_MAP_SHOW_SAT_MARKER);

    satmap->qthinfo = mod_cfg_get_bool(cfgdata,
                                       MOD_CFG_MAP_SECTION,
                                       MOD_CFG_MAP_SHOW_QTH_INFO,
                                       SAT_CFG_BOOL_MAP_SHOW_QTH_INFO);

    satmap->eventinfo = mod_cfg_get_bool(cfgdata,
                                         MOD_CFG_MAP_SECTION,
                                         MOD_CFG_MAP_SHOW_NEXT_EVENT,
                                         SAT_CFG_BOOL_MAP_SHOW_NEXT_EV);

    satmap->cursinfo = mod_cfg_get_bool(cfgdata,
                                        MOD_CFG_MAP_SECTION,
                                        MOD_CFG_MAP_SHOW_CURS_TRACK,
                                        SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK);

    satmap->showgrid = mod_cfg_get_bool(cfgdata,
                                        MOD_CFG_MAP_SECTION,
                                        MOD_CFG_MAP_SHOW_GRID,
                                        SAT_CFG_BOOL_MAP_SHOW_GRID);

    satmap->show_terminator = mod_cfg_get_bool(cfgdata,
                                               MOD_CFG_MAP_SECTION,
                                               MOD_CFG_MAP_SHOW_TERMINATOR,
                                               SAT_CFG_BOOL_MAP_SHOW_TERMINATOR);

    satmap->keepratio = mod_cfg_get_bool(cfgdata,
                                         MOD_CFG_MAP_SECTION,
                                         MOD_CFG_MAP_KEEP_RATIO,
                                         SAT_CFG_BOOL_MAP_KEEP_RATIO);

    col = mod_cfg_get_int(cfgdata,
                          MOD_CFG_MAP_SECTION,
                          MOD_CFG_MAP_INFO_BGD_COL,
                          SAT_CFG_INT_MAP_INFO_BGD_COL);
    satmap->infobgd = rgba2html(col);

    /* Load colors */
    satmap->col_qth = mod_cfg_get_int(cfgdata,
                                      MOD_CFG_MAP_SECTION,
                                      MOD_CFG_MAP_QTH_COL,
                                      SAT_CFG_INT_MAP_QTH_COL);
    satmap->col_info = mod_cfg_get_int(cfgdata,
                                       MOD_CFG_MAP_SECTION,
                                       MOD_CFG_MAP_INFO_COL,
                                       SAT_CFG_INT_MAP_INFO_COL);
    satmap->col_grid = mod_cfg_get_int(cfgdata,
                                       MOD_CFG_MAP_SECTION,
                                       MOD_CFG_MAP_GRID_COL,
                                       SAT_CFG_INT_MAP_GRID_COL);
    satmap->col_tick = satmap->col_grid;
    satmap->col_sat = mod_cfg_get_int(cfgdata,
                                      MOD_CFG_MAP_SECTION,
                                      MOD_CFG_MAP_SAT_COL,
                                      SAT_CFG_INT_MAP_SAT_COL);
    satmap->col_sat_sel = mod_cfg_get_int(cfgdata,
                                          MOD_CFG_MAP_SECTION,
                                          MOD_CFG_MAP_SAT_SEL_COL,
                                          SAT_CFG_INT_MAP_SAT_SEL_COL);
    satmap->col_shadow = mod_cfg_get_int(cfgdata,
                                         MOD_CFG_MAP_SECTION,
                                         MOD_CFG_MAP_SHADOW_ALPHA,
                                         SAT_CFG_INT_MAP_SHADOW_ALPHA);
    satmap->col_track = mod_cfg_get_int(cfgdata,
                                        MOD_CFG_MAP_SECTION,
                                        MOD_CFG_MAP_TRACK_COL,
                                        SAT_CFG_INT_MAP_TRACK_COL);
    satmap->col_terminator = mod_cfg_get_int(cfgdata,
                                             MOD_CFG_MAP_SECTION,
                                             MOD_CFG_MAP_TERMINATOR_COL,
                                             SAT_CFG_INT_MAP_TERMINATOR_COL);

    /* Get default font */
    g_value_init(&font_value, G_TYPE_STRING);
    g_object_get_property(G_OBJECT(gtk_settings_get_default()), "gtk-font-name",
                          &font_value);
    satmap->font = g_value_dup_string(&font_value);
    g_value_unset(&font_value);

    /* Create drawing area canvas */
    satmap->canvas = gtk_drawing_area_new();
    gtk_widget_set_has_tooltip(satmap->canvas, TRUE);

    float           clon = (float)mod_cfg_get_int(cfgdata,
                                                  MOD_CFG_MAP_SECTION,
                                                  MOD_CFG_MAP_CENTER,
                                                  SAT_CFG_INT_MAP_CENTER);

    load_map_file(satmap, clon);

    /* Initial size */
    satmap->width = 200;
    satmap->height = 100;
    satmap->x0 = 0;
    satmap->y0 = 0;

    /* Allocate temporary point arrays for footprint calculation */
    temp_points1 = g_new0(gdouble, 720);
    temp_points2 = g_new0(gdouble, 720);

    /* Connect signals */
    gtk_widget_add_events(satmap->canvas, GDK_POINTER_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    g_signal_connect(satmap->canvas, "draw", G_CALLBACK(on_draw), satmap);
    g_signal_connect(satmap->canvas, "motion-notify-event",
                     G_CALLBACK(on_motion_notify), satmap);
    g_signal_connect(satmap->canvas, "button-press-event",
                     G_CALLBACK(on_button_press), satmap);
    g_signal_connect(satmap->canvas, "button-release-event",
                     G_CALLBACK(on_button_release), satmap);
    g_signal_connect(satmap->canvas, "size-allocate",
                     G_CALLBACK(size_allocate_cb), satmap);

    gtk_widget_show(satmap->canvas);

    /* Initialize QTH info text */
    if (satmap->qthinfo)
    {
        satmap->locnam_text = g_strdup_printf("<span background=\"#%s\"> %s \302\267 %s </span>",
                                              satmap->infobgd,
                                              satmap->qth->name,
                                              satmap->qth->loc);
    }

    /* Initialize next event text */
    if (satmap->eventinfo)
    {
        satmap->next_text = g_strdup_printf("<span background=\"#%s\"> ... </span>",
                                            satmap->infobgd);
    }

    gtk_sat_map_load_showtracks(satmap);
    gtk_sat_map_load_hide_coverages(satmap);
    g_hash_table_foreach(satmap->sats, plot_sat, satmap);

    gtk_box_pack_start(GTK_BOX(satmap), satmap->canvas, TRUE, TRUE, 0);

    return GTK_WIDGET(satmap);
}

/** Draw callback for the canvas */
static gboolean on_draw(GtkWidget * widget, cairo_t * cr, gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    gdouble         r, g, b, a;
    PangoLayout    *layout;
    PangoFontDescription *font_desc;
    gint            tw, th;
    GHashTableIter  iter;
    gpointer        key, value;
    sat_map_obj_t  *obj;
    gfloat          x, y;
    gdouble         xstep, ystep;
    guint           i;
    gfloat          lon, lat;
    gchar          *buf;
    gchar           hmf = ' ';
    guint32         globe_shadow_col;
    GSList         *line_node;
    gdouble        *line_points;
    guint           num_points;

    (void)widget;

    /* Draw background map */
    if (satmap->map)
    {
        gdk_cairo_set_source_pixbuf(cr, satmap->map, satmap->x0, satmap->y0);
        cairo_paint(cr);
    }

    /* Set up font */
    layout = pango_cairo_create_layout(cr);
    font_desc = pango_font_description_from_string(satmap->font ? satmap->font : "Sans 9");
    pango_layout_set_font_description(layout, font_desc);

    /* Draw grid lines if enabled */
    if (satmap->showgrid && satmap->width > 0 && satmap->height > 0)
    {
        rgba_to_cairo(satmap->col_grid, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_set_line_width(cr, 0.5);

        xstep = (gdouble)(30.0 * satmap->width / 360.0);
        ystep = (gdouble)(30.0 * satmap->height / 180.0);

        /* Horizontal grid lines */
        for (i = 0; i < 5; i++)
        {
            cairo_move_to(cr, (gdouble)satmap->x0,
                          (gdouble)(satmap->y0 + (i + 1) * ystep));
            cairo_line_to(cr, (gdouble)(satmap->x0 + satmap->width),
                          (gdouble)(satmap->y0 + (i + 1) * ystep));
            cairo_stroke(cr);

            /* Label */
            xy_to_lonlat(satmap, satmap->x0, satmap->y0 + (i + 1) * ystep,
                         &lon, &lat);
            hmf = ' ';
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW))
            {
                if (lat < 0.00)
                {
                    lat = -lat;
                    hmf = 'S';
                }
                else
                {
                    hmf = 'N';
                }
            }
            buf = g_strdup_printf("%.0f\302\260%c", lat, hmf);
            pango_layout_set_text(layout, buf, -1);
            pango_layout_get_pixel_size(layout, &tw, &th);
            cairo_move_to(cr, (gdouble)(satmap->x0 + 15),
                          (gdouble)(satmap->y0 + (i + 1) * ystep));
            pango_cairo_show_layout(cr, layout);
            g_free(buf);
        }

        /* Vertical grid lines */
        for (i = 0; i < 11; i++)
        {
            cairo_move_to(cr, (gdouble)(satmap->x0 + (i + 1) * xstep),
                          (gdouble)satmap->y0);
            cairo_line_to(cr, (gdouble)(satmap->x0 + (i + 1) * xstep),
                          (gdouble)(satmap->y0 + satmap->height));
            cairo_stroke(cr);

            /* Label */
            xy_to_lonlat(satmap, satmap->x0 + (i + 1) * xstep, satmap->y0,
                         &lon, &lat);
            hmf = ' ';
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW))
            {
                if (lon < 0.00)
                {
                    lon = -lon;
                    hmf = 'W';
                }
                else
                {
                    hmf = 'E';
                }
            }
            buf = g_strdup_printf("%.0f\302\260%c", lon, hmf);
            pango_layout_set_text(layout, buf, -1);
            pango_layout_get_pixel_size(layout, &tw, &th);
            cairo_move_to(cr, (gdouble)(satmap->x0 + (i + 1) * xstep),
                          (gdouble)(satmap->y0 + satmap->height - 5 - th));
            pango_cairo_show_layout(cr, layout);
            g_free(buf);
        }
    }

    /* Draw terminator if enabled */
    if (satmap->show_terminator && satmap->terminator_points &&
        satmap->terminator_count > 2)
    {
        globe_shadow_col = mod_cfg_get_int(satmap->cfgdata,
                                           MOD_CFG_MAP_SECTION,
                                           MOD_CFG_MAP_GLOBAL_SHADOW_COL,
                                           SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL);

        rgba_to_cairo(globe_shadow_col, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);

        cairo_move_to(cr, satmap->terminator_points[0],
                      satmap->terminator_points[1]);
        for (i = 1; i < (guint)satmap->terminator_count; i++)
        {
            cairo_line_to(cr, satmap->terminator_points[2 * i],
                          satmap->terminator_points[2 * i + 1]);
        }
        cairo_close_path(cr);
        cairo_fill_preserve(cr);

        rgba_to_cairo(satmap->col_terminator, &r, &g, &b, &a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    /* Draw QTH marker */
    lonlat_to_xy(satmap, satmap->qth->lon, satmap->qth->lat, &x, &y);
    rgba_to_cairo(satmap->col_qth, &r, &g, &b, &a);
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, x - MARKER_SIZE_HALF, y - MARKER_SIZE_HALF,
                    2 * MARKER_SIZE_HALF, 2 * MARKER_SIZE_HALF);
    cairo_fill(cr);

    /* Draw QTH label */
    pango_layout_set_text(layout, satmap->qth->name, -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, x - tw / 2, y + 2);
    pango_cairo_show_layout(cr, layout);

    /* Draw satellite objects */
    if (satmap->obj)
    {
        g_hash_table_iter_init(&iter, satmap->obj);
        while (g_hash_table_iter_next(&iter, &key, &value))
        {
            obj = SAT_MAP_OBJ(value);

            /* Draw ground track if enabled */
            if (obj->showtrack && obj->track_data.lines)
            {
                rgba_to_cairo(satmap->col_track, &r, &g, &b, &a);
                cairo_set_source_rgba(cr, r, g, b, a);
                cairo_set_line_width(cr, 1.0);

                line_node = obj->track_data.lines;
                while (line_node)
                {
                    line_points = (gdouble *)line_node->data;
                    if (line_points)
                    {
                        num_points = (guint)line_points[0];
                        if (num_points > 1)
                        {
                            cairo_move_to(cr, line_points[1], line_points[2]);
                            for (i = 1; i < num_points; i++)
                            {
                                cairo_line_to(cr, line_points[2 * i + 1],
                                              line_points[2 * i + 2]);
                            }
                            cairo_stroke(cr);
                        }
                    }
                    line_node = line_node->next;
                }
            }

            /* Check visibility conditions */
            gboolean show_fp = satmap->satfp || obj->selected;
            gboolean show_marker = satmap->satmarker || obj->selected;
            gboolean show_label = satmap->satname || obj->selected;

            /* Draw range circle(s) / footprint */
            if (show_fp && obj->showcov)
            {
                guint32 covcol = mod_cfg_get_int(satmap->cfgdata,
                                                 MOD_CFG_MAP_SECTION,
                                                 MOD_CFG_MAP_SAT_COV_COL,
                                                 SAT_CFG_INT_MAP_SAT_COV_COL);

                /* Draw first range circle */
                if (obj->range1_points && obj->range1_count > 2)
                {
                    rgba_to_cairo(covcol, &r, &g, &b, &a);
                    cairo_set_source_rgba(cr, r, g, b, a);

                    cairo_move_to(cr, obj->range1_points[0],
                                  obj->range1_points[1]);
                    for (i = 1; i < (guint)obj->range1_count; i++)
                    {
                        cairo_line_to(cr, obj->range1_points[2 * i],
                                      obj->range1_points[2 * i + 1]);
                    }
                    cairo_close_path(cr);
                    cairo_fill_preserve(cr);

                    if (obj->selected)
                        rgba_to_cairo(satmap->col_sat_sel, &r, &g, &b, &a);
                    else
                        rgba_to_cairo(satmap->col_sat, &r, &g, &b, &a);
                    cairo_set_source_rgba(cr, r, g, b, a);
                    cairo_set_line_width(cr, 1.0);
                    cairo_stroke(cr);
                }

                /* Draw second range circle if present */
                if (obj->range2_points && obj->range2_count > 2)
                {
                    rgba_to_cairo(covcol, &r, &g, &b, &a);
                    cairo_set_source_rgba(cr, r, g, b, a);

                    cairo_move_to(cr, obj->range2_points[0],
                                  obj->range2_points[1]);
                    for (i = 1; i < (guint)obj->range2_count; i++)
                    {
                        cairo_line_to(cr, obj->range2_points[2 * i],
                                      obj->range2_points[2 * i + 1]);
                    }
                    cairo_close_path(cr);
                    cairo_fill_preserve(cr);

                    if (obj->selected)
                        rgba_to_cairo(satmap->col_sat_sel, &r, &g, &b, &a);
                    else
                        rgba_to_cairo(satmap->col_sat, &r, &g, &b, &a);
                    cairo_set_source_rgba(cr, r, g, b, a);
                    cairo_set_line_width(cr, 1.0);
                    cairo_stroke(cr);
                }
            }

            /* Draw satellite marker shadow */
            if (show_marker)
            {
                rgba_to_cairo(satmap->col_shadow, &r, &g, &b, &a);
                cairo_set_source_rgba(cr, 0, 0, 0, a);
                cairo_rectangle(cr, obj->x - MARKER_SIZE_HALF + 1,
                                obj->y - MARKER_SIZE_HALF + 1,
                                2 * MARKER_SIZE_HALF, 2 * MARKER_SIZE_HALF);
                cairo_fill(cr);
            }

            /* Draw satellite marker */
            if (show_marker)
            {
                if (obj->selected)
                    rgba_to_cairo(satmap->col_sat_sel, &r, &g, &b, &a);
                else
                    rgba_to_cairo(satmap->col_sat, &r, &g, &b, &a);
                cairo_set_source_rgba(cr, r, g, b, a);
                cairo_rectangle(cr, obj->x - MARKER_SIZE_HALF,
                                obj->y - MARKER_SIZE_HALF,
                                2 * MARKER_SIZE_HALF, 2 * MARKER_SIZE_HALF);
                cairo_fill(cr);
            }

            /* Draw satellite label shadow */
            if (show_label && obj->nickname)
            {
                rgba_to_cairo(satmap->col_shadow, &r, &g, &b, &a);
                cairo_set_source_rgba(cr, 0, 0, 0, a);
                pango_layout_set_text(layout, obj->nickname, -1);
                pango_layout_get_pixel_size(layout, &tw, &th);

                if (obj->x < 50)
                    cairo_move_to(cr, obj->x + 3 + 1, obj->y + 1);
                else if ((satmap->width - obj->x) < 50)
                    cairo_move_to(cr, obj->x - 3 - tw + 1, obj->y + 1);
                else if ((satmap->height - obj->y) < 25)
                    cairo_move_to(cr, obj->x - tw / 2 + 1, obj->y - 2 - th + 1);
                else
                    cairo_move_to(cr, obj->x - tw / 2 + 1, obj->y + 2 + 1);
                pango_cairo_show_layout(cr, layout);
            }

            /* Draw satellite label */
            if (show_label && obj->nickname)
            {
                if (obj->selected)
                    rgba_to_cairo(satmap->col_sat_sel, &r, &g, &b, &a);
                else
                    rgba_to_cairo(satmap->col_sat, &r, &g, &b, &a);
                cairo_set_source_rgba(cr, r, g, b, a);
                pango_layout_set_text(layout, obj->nickname, -1);
                pango_layout_get_pixel_size(layout, &tw, &th);

                if (obj->x < 50)
                    cairo_move_to(cr, obj->x + 3, obj->y);
                else if ((satmap->width - obj->x) < 50)
                    cairo_move_to(cr, obj->x - 3 - tw, obj->y);
                else if ((satmap->height - obj->y) < 25)
                    cairo_move_to(cr, obj->x - tw / 2, obj->y - 2 - th);
                else
                    cairo_move_to(cr, obj->x - tw / 2, obj->y + 2);
                pango_cairo_show_layout(cr, layout);
            }
        }
    }

    /* Draw info text elements */
    rgba_to_cairo(satmap->col_info, &r, &g, &b, &a);
    cairo_set_source_rgba(cr, r, g, b, a);

    /* QTH info (top-left) */
    if (satmap->qthinfo && satmap->locnam_text)
    {
        pango_layout_set_markup(layout, satmap->locnam_text, -1);
        cairo_move_to(cr, (gdouble)satmap->x0 + 2, (gdouble)satmap->y0 + 1);
        pango_cairo_show_layout(cr, layout);
    }

    /* Next event (top-right) */
    if (satmap->eventinfo && satmap->next_text)
    {
        pango_layout_set_markup(layout, satmap->next_text, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, (gdouble)(satmap->x0 + satmap->width - 2 - tw),
                      (gdouble)satmap->y0 + 1);
        pango_cairo_show_layout(cr, layout);
    }

    /* Cursor tracking (bottom-left) */
    if (satmap->cursinfo && satmap->curs_text)
    {
        pango_layout_set_markup(layout, satmap->curs_text, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, (gdouble)satmap->x0 + 2,
                      (gdouble)(satmap->y0 + satmap->height - 1 - th));
        pango_cairo_show_layout(cr, layout);
    }

    /* Selected satellite info (bottom-right) */
    if (satmap->sel_text)
    {
        pango_layout_set_markup(layout, satmap->sel_text, -1);
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, (gdouble)(satmap->x0 + satmap->width - 2 - tw),
                      (gdouble)(satmap->y0 + satmap->height - 1 - th));
        pango_cairo_show_layout(cr, layout);
    }

    pango_font_description_free(font_desc);
    g_object_unref(layout);

    return FALSE;
}

/** Find satellite object at given position */
static sat_map_obj_t *find_sat_at_pos(GtkSatMap * satmap, gfloat mx, gfloat my)
{
    GHashTableIter  iter;
    gpointer        key, value;
    sat_map_obj_t  *obj;
    gfloat          dx, dy;
    const gfloat    hit_radius = 10.0;

    if (satmap->obj == NULL)
        return NULL;

    g_hash_table_iter_init(&iter, satmap->obj);
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
        obj = SAT_MAP_OBJ(value);
        dx = mx - obj->x;
        dy = my - obj->y;
        if (dx * dx + dy * dy < hit_radius * hit_radius)
            return obj;
    }
    return NULL;
}

static void size_allocate_cb(GtkWidget * widget, GtkAllocation * allocation,
                             gpointer data)
{
    (void)widget;
    (void)allocation;
    GTK_SAT_MAP(data)->resize = TRUE;
}

static void update_map_size(GtkSatMap * satmap)
{
    GtkAllocation   allocation;
    GdkPixbuf      *pbuf;
    gfloat          ratio;

    if (gtk_widget_get_realized(GTK_WIDGET(satmap)))
    {
        gtk_widget_get_allocation(GTK_WIDGET(satmap), &allocation);

        if (satmap->keepratio)
        {
            ratio = (gfloat)gdk_pixbuf_get_width(satmap->origmap) /
                (gfloat)gdk_pixbuf_get_height(satmap->origmap);

            gfloat size = MIN(allocation.width, ratio * allocation.height);

            satmap->width = (guint)size;
            satmap->height = (guint)(size / ratio);

            satmap->x0 = (allocation.width - satmap->width) / 2;
            satmap->y0 = (allocation.height - satmap->height) / 2;

            pbuf = gdk_pixbuf_scale_simple(satmap->origmap,
                                           satmap->width,
                                           satmap->height,
                                           GDK_INTERP_BILINEAR);
        }
        else
        {
            satmap->x0 = 0;
            satmap->y0 = 0;
            satmap->width = allocation.width;
            satmap->height = allocation.height;

            pbuf = gdk_pixbuf_scale_simple(satmap->origmap,
                                           satmap->width,
                                           satmap->height,
                                           GDK_INTERP_BILINEAR);
        }

        if (satmap->map)
            g_object_unref(satmap->map);
        satmap->map = pbuf;

        if (satmap->show_terminator)
            redraw_terminator(satmap);

        g_hash_table_foreach(satmap->sats, update_sat, satmap);
        satmap->resize = FALSE;

        gtk_widget_queue_draw(satmap->canvas);
    }
}

void gtk_sat_map_update(GtkWidget * widget)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(widget);
    sat_t          *sat = NULL;
    gdouble         number, now;
    gchar          *buff;
    gint           *catnr;
    guint           h, m, s;
    gchar          *ch, *cm, *cs;

    /* check whether there are any pending resize requests */
    if (satmap->resize)
        update_map_size(satmap);

    /* check refresh rate and refresh sats/qth if time */
    /* FIXME add location check */
    if (satmap->counter < satmap->refresh)
    {
        satmap->counter++;
    }
    else
    {
        /* reset data */
        satmap->counter = 1;
        satmap->naos = 0.0;
        satmap->ncat = 0;

        g_hash_table_foreach(satmap->sats, update_sat, satmap);

        /* Update the Solar Terminator if necessary */
        if (satmap->show_terminator &&
            fabs(satmap->tstamp - satmap->terminator_last_tstamp) >
            TERMINATOR_UPDATE_INTERVAL)
        {
            satmap->terminator_last_tstamp = satmap->tstamp;
            redraw_terminator(satmap);
        }

        if (satmap->eventinfo)
        {
            if (satmap->ncat > 0)
            {
                catnr = g_try_new0(gint, 1);
                *catnr = satmap->ncat;
                sat = SAT(g_hash_table_lookup(satmap->sats, catnr));
                g_free(catnr);

                /* last desperate sanity check */
                if (sat != NULL)
                {
                    now = satmap->tstamp;       //get_current_daynum ();
                    number = satmap->naos - now;

                    /* convert julian date to seconds */
                    s = (guint)(number * 86400);

                    /* extract hours */
                    h = (guint)floor(s / 3600);
                    s -= 3600 * h;

                    /* leading zero */
                    if ((h > 0) && (h < 10))
                        ch = g_strdup("0");
                    else
                        ch = g_strdup("");

                    /* extract minutes */
                    m = (guint)floor(s / 60);
                    s -= 60 * m;

                    /* leading zero */
                    if (m < 10)
                        cm = g_strdup("0");
                    else
                        cm = g_strdup("");

                    /* leading zero */
                    if (s < 10)
                        cs = g_strdup(":0");
                    else
                        cs = g_strdup(":");

                    g_free(satmap->next_text);
                    if (h > 0)
                        buff = g_markup_printf_escaped(
                            _("<span background=\"#%s\"> "
                              "Next: %s in %s%d:%s%d%s%d </span>"),
                            satmap->infobgd,
                            sat->nickname, ch, h, cm, m, cs, s);
                    else
                        buff = g_markup_printf_escaped(
                            _("<span background=\"#%s\"> "
                              "Next: %s in %s%d%s%d </span>"),
                            satmap->infobgd,
                            sat->nickname, cm, m, cs, s);

                    satmap->next_text = buff;

                    g_free(ch);
                    g_free(cm);
                    g_free(cs);
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Can not find NEXT satellite."),
                                __func__);
                    g_free(satmap->next_text);
                    satmap->next_text = g_strdup(_("Next: ERR"));
                }
            }
            else
            {
                g_free(satmap->next_text);
                satmap->next_text = g_strdup(_("Next: N/A"));
            }
        }
        else
        {
            g_free(satmap->next_text);
            satmap->next_text = NULL;
        }

        gtk_widget_queue_draw(satmap->canvas);
    }
}

static void lonlat_to_xy(GtkSatMap * p, gdouble lon, gdouble lat, gfloat * x,
                         gfloat * y)
{
    *x = p->x0 + (lon - p->left_side_lon) * p->width / 360.0;
    *y = p->y0 + (90.0 - lat) * p->height / 180.0;
    while (*x < 0)
        *x += p->width;
    while (*x > p->width)
        *x -= p->width;
}

static void xy_to_lonlat(GtkSatMap * p, gfloat x, gfloat y, gfloat * lon,
                         gfloat * lat)
{
    *lat = 90.0 - (180.0 / p->height) * (y - p->y0);
    *lon = (360.0 / p->width) * (x - p->x0) + p->left_side_lon;
    while (*lon > 180)
        *lon -= 360;
    while (*lon < -180)
        *lon += 360;
}

static gboolean on_motion_notify(GtkWidget * widget, GdkEventMotion * event,
                                 gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    gfloat          lat, lon;

    (void)widget;

    if (satmap->cursinfo)
    {
        xy_to_lonlat(satmap, event->x, event->y, &lon, &lat);

        g_free(satmap->curs_text);
        satmap->curs_text = g_strdup_printf(
            "<span background=\"#%s\"> "
            "LON:%.0f\302\260 LAT:%.0f\302\260 </span>",
            satmap->infobgd, lon, lat);

        gtk_widget_queue_draw(satmap->canvas);
    }

    return TRUE;
}

static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event,
                                gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    sat_map_obj_t  *obj;
    gint           *catpoint = NULL;
    sat_t          *sat = NULL;

    (void)widget;

    obj = find_sat_at_pos(satmap, event->x, event->y);

    if (obj == NULL)
        return FALSE;

    switch (event->button)
    {
    case 1:
        if (event->type == GDK_2BUTTON_PRESS)
        {
            catpoint = g_try_new0(gint, 1);
            *catpoint = obj->catnum;

            sat = SAT(g_hash_table_lookup(satmap->sats, catpoint));
            if (sat != NULL)
            {
                show_sat_info(sat, gtk_widget_get_toplevel(GTK_WIDGET(data)));
            }
            g_free(catpoint);
        }
        break;

    case 3:
        catpoint = g_try_new0(gint, 1);
        *catpoint = obj->catnum;
        sat = SAT(g_hash_table_lookup(satmap->sats, catpoint));
        if (sat != NULL)
        {
            gtk_sat_map_popup_exec(sat, satmap->qth, satmap, event,
                                   gtk_widget_get_toplevel(GTK_WIDGET(satmap)));
        }
        g_free(catpoint);
        break;
    default:
        break;
    }

    return TRUE;
}

static gboolean on_button_release(GtkWidget * widget, GdkEventButton * event,
                                  gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    sat_map_obj_t  *obj = NULL;
    gint           *catpoint;

    (void)widget;

    if (event->button != 1)
        return FALSE;

    obj = find_sat_at_pos(satmap, event->x, event->y);

    if (obj == NULL)
        return FALSE;

    obj->selected = !obj->selected;

    catpoint = g_try_new0(gint, 1);
    *catpoint = obj->catnum;

    if (!obj->selected)
    {
        g_free(satmap->sel_text);
        satmap->sel_text = NULL;
        *catpoint = 0;
    }

    g_hash_table_foreach(satmap->obj, clear_selection, catpoint);
    g_hash_table_foreach(satmap->sats, update_sat, satmap);

    g_free(catpoint);

    gtk_widget_queue_draw(satmap->canvas);

    return TRUE;
}

static void clear_selection(gpointer key, gpointer val, gpointer data)
{
    gint           *old = key;
    gint           *new = data;
    sat_map_obj_t  *obj = SAT_MAP_OBJ(val);

    if ((*old != *new) && (obj->selected))
    {
        obj->selected = FALSE;
    }
}

void gtk_sat_map_select_sat(GtkWidget * satmap, gint catnum)
{
    GtkSatMap      *smap = GTK_SAT_MAP(satmap);
    gint           *catpoint = NULL;
    sat_map_obj_t  *obj = NULL;

    catpoint = g_try_new0(gint, 1);
    *catpoint = catnum;

    obj = SAT_MAP_OBJ(g_hash_table_lookup(smap->obj, catpoint));
    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Can not find clicked object (%d) in hash table"),
                    __func__, catnum);
    }
    else
    {
        obj->selected = TRUE;
        g_hash_table_foreach(smap->obj, clear_selection, catpoint);
        g_hash_table_foreach(smap->sats, update_sat, smap);
        gtk_widget_queue_draw(smap->canvas);
    }

    g_free(catpoint);
}

void gtk_sat_map_reconf(GtkWidget * widget, GKeyFile * cfgdat)
{
    (void)widget;
    (void)cfgdat;
}

static void load_map_file(GtkSatMap * satmap, float clon)
{
    gchar          *buff;
    gchar          *mapfile;
    GError         *error = NULL;
    GdkPixbuf      *tmpbuf;

    buff = mod_cfg_get_str(satmap->cfgdata,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_FILE, SAT_CFG_STR_MAP_FILE);

    if (g_path_is_absolute(buff))
    {
        mapfile = g_strdup(buff);
    }
    else
    {
        mapfile = map_file_name(buff);
    }
    g_free(buff);

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%d: Loading map file %s"), __FILE__, __LINE__, mapfile);

    if (g_file_test(mapfile, G_FILE_TEST_EXISTS))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%d: Map file found"), __FILE__, __LINE__);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Could not find map file %s"),
                    __FILE__, __LINE__, mapfile);

        g_free(mapfile);
        mapfile = sat_cfg_get_str_def(SAT_CFG_STR_MAP_FILE);

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Using default map: %s"),
                    __FILE__, __LINE__, mapfile);
    }

    tmpbuf = gdk_pixbuf_new_from_file(mapfile, &error);

    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Error loading map file (%s)"),
                    __FILE__, __LINE__, error->message);
        g_clear_error(&error);

        tmpbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 400, 200);
        gdk_pixbuf_fill(tmpbuf, 0x0F0F0F0F);
    }

    satmap->origmap = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                     FALSE,
                                     gdk_pixbuf_get_bits_per_sample(tmpbuf),
                                     gdk_pixbuf_get_width(tmpbuf),
                                     gdk_pixbuf_get_height(tmpbuf));

    map_tools_shift_center(tmpbuf, satmap->origmap, clon);
    g_object_unref(tmpbuf);
    g_free(mapfile);

    satmap->left_side_lon = -180.0;
    if (clon > 0.0)
        satmap->left_side_lon += clon;
    else if (clon < 0.0)
        satmap->left_side_lon = 180.0 + clon;
}

static gdouble arccos(gdouble x, gdouble y)
{
    if (x && y)
    {
        if (y > 0.0)
            return acos(x / y);
        else if (y < 0.0)
            return pi + acos(x / y);
    }

    return 0.0;
}

static gboolean pole_is_covered(sat_t * sat)
{
    if (north_pole_is_covered(sat) || south_pole_is_covered(sat))
        return TRUE;
    else
        return FALSE;
}

static gboolean north_pole_is_covered(sat_t * sat)
{
    int             ret1;
    gdouble         qrb1, az1;

    ret1 = qrb(sat->ssplon, sat->ssplat, 0.0, 90.0, &qrb1, &az1);
    if (ret1 != RIG_OK)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Bad data measuring distance to North Pole %f %f."),
                    __func__, sat->ssplon, sat->ssplat);
    }
    if (qrb1 <= 0.5 * sat->footprint)
    {
        return TRUE;
    }
    return FALSE;
}

static gboolean south_pole_is_covered(sat_t * sat)
{
    int             ret1;
    gdouble         qrb1, az1;

    ret1 = qrb(sat->ssplon, sat->ssplat, 0.0, -90.0, &qrb1, &az1);
    if (ret1 != RIG_OK)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Bad data measuring distance to South Pole %f %f."),
                    __func__, sat->ssplon, sat->ssplat);
    }
    if (qrb1 <= 0.5 * sat->footprint)
    {
        return TRUE;
    }
    return FALSE;
}

static gboolean mirror_lon(sat_t * sat, gdouble rangelon, gdouble * mlon,
                           gdouble mapbreak)
{
    gdouble         diff;
    gboolean        warped = FALSE;

    diff = (sat->ssplon - rangelon);
    while (diff < 0)
        diff += 360;
    while (diff > 360)
        diff -= 360;

    *mlon = sat->ssplon + fabs(diff);
    while (*mlon > 180)
        *mlon -= 360;
    while (*mlon < -180)
        *mlon += 360;

    if (((sat->ssplon >= mapbreak) && (sat->ssplon < mapbreak + 180)) ||
        ((sat->ssplon < mapbreak - 180) && (sat->ssplon >= mapbreak - 360)))
    {
        if (((rangelon >= mapbreak) && (rangelon < mapbreak + 180)) ||
            ((rangelon < mapbreak - 180) && (rangelon >= mapbreak - 360)))
        {
        }
        else
        {
            warped = TRUE;
        }
    }
    else
    {
        if (((*mlon >= mapbreak) && (*mlon < mapbreak + 180)) ||
            ((*mlon < mapbreak - 180) && (*mlon >= mapbreak - 360)))
        {
            warped = TRUE;
        }
    }

    return warped;
}

static guint calculate_footprint(GtkSatMap * satmap, sat_t * sat,
                                 sat_map_obj_t * obj)
{
    guint           azi;
    gfloat          sx, sy, msx, msy, ssx, ssy;
    gdouble         ssplat, ssplon, beta, azimuth, num, dem;
    gdouble         rangelon, rangelat, mlon;
    gboolean        warped = FALSE;
    guint           numrc = 1;
    gint            n1, n2;

    ssplat = sat->ssplat * de2ra;
    ssplon = sat->ssplon * de2ra;
    beta = (0.5 * sat->footprint) / xkmper;

    for (azi = 0; azi < 180; azi++)
    {
        azimuth = de2ra * (double)azi;
        rangelat = asin(sin(ssplat) * cos(beta) + cos(azimuth) *
                        sin(beta) * cos(ssplat));
        num = cos(beta) - (sin(ssplat) * sin(rangelat));
        dem = cos(ssplat) * cos(rangelat);

        if (azi == 0 && north_pole_is_covered(sat))
            rangelon = ssplon + pi;
        else if (fabs(num / dem) > 1.0)
            rangelon = ssplon;
        else
        {
            if ((180.0 - azi) >= 0)
                rangelon = ssplon - arccos(num, dem);
            else
                rangelon = ssplon + arccos(num, dem);
        }

        while (rangelon < -pi)
            rangelon += twopi;

        while (rangelon > (pi))
            rangelon -= twopi;

        rangelat = rangelat / de2ra;
        rangelon = rangelon / de2ra;

        if (mirror_lon(sat, rangelon, &mlon, satmap->left_side_lon))
            warped = TRUE;

        lonlat_to_xy(satmap, rangelon, rangelat, &sx, &sy);
        lonlat_to_xy(satmap, mlon, rangelat, &msx, &msy);

        temp_points1[2 * azi] = sx;
        temp_points1[2 * azi + 1] = sy;

        temp_points1[718 - 2 * azi] = msx;
        temp_points1[719 - 2 * azi] = msy;
    }

    if (pole_is_covered(sat))
    {
        sort_points_x(satmap, sat, temp_points1, 360);
        numrc = 1;
        n1 = 360;
        n2 = 0;
    }
    else if (warped == TRUE)
    {
        lonlat_to_xy(satmap, sat->ssplon, sat->ssplat, &ssx, &ssy);
        n1 = 360;
        n2 = 360;
        split_points(satmap, sat, ssx, temp_points1, &n1, temp_points2, &n2);
        numrc = 2;
    }
    else
    {
        numrc = 1;
        n1 = 360;
        n2 = 0;
    }

    g_free(obj->range1_points);
    obj->range1_points = g_new(gdouble, n1 * 2);
    memcpy(obj->range1_points, temp_points1, n1 * 2 * sizeof(gdouble));
    obj->range1_count = n1;

    if (numrc == 2 && n2 > 0)
    {
        g_free(obj->range2_points);
        obj->range2_points = g_new(gdouble, n2 * 2);
        memcpy(obj->range2_points, temp_points2, n2 * 2 * sizeof(gdouble));
        obj->range2_count = n2;
    }
    else
    {
        g_free(obj->range2_points);
        obj->range2_points = NULL;
        obj->range2_count = 0;
    }

    return numrc;
}

static void split_points(GtkSatMap * satmap, sat_t * sat, gdouble sspx,
                         gdouble * points1, gint * n1,
                         gdouble * points2, gint * n2)
{
    gdouble        *tps1, *tps2;
    gint            n, np1, np2, ns, i, j, k;

    n = 360;
    np1 = 0;
    np2 = 0;
    i = 0;
    j = 0;
    k = 0;
    ns = 0;
    tps1 = g_new0(gdouble, n * 2);
    tps2 = g_new0(gdouble, n * 2);

    if ((sat->ssplon >= 179.4) || (sat->ssplon <= -179.4))
    {
        for (i = 0; i < n; i++)
        {
            if (points1[2 * i] > (satmap->x0 + satmap->width / 2))
            {
                tps1[2 * np1] = points1[2 * i];
                tps1[2 * np1 + 1] = points1[2 * i + 1];
                np1++;
            }
            else
            {
                tps2[2 * np2] = points1[2 * i];
                tps2[2 * np2 + 1] = points1[2 * i + 1];
                np2++;
            }
        }

        sort_points_y(satmap, sat, tps1, np1);
        sort_points_y(satmap, sat, tps2, np2);
    }
    else if (sspx < (satmap->x0 + satmap->width / 2))
    {
        while (points1[2 * i] <= sspx)
        {
            i++;
        }
        ns = i - 1;

        while (points1[2 * i] > (satmap->x0 + satmap->width / 2))
        {
            tps2[2 * j] = points1[2 * i];
            tps2[2 * j + 1] = points1[2 * i + 1];
            i++;
            j++;
            np2++;
        }

        while (i < n)
        {
            tps1[2 * k] = points1[2 * i];
            tps1[2 * k + 1] = points1[2 * i + 1];
            i++;
            k++;
            np1++;
        }

        for (i = 0; i <= ns; i++)
        {
            tps1[2 * k] = points1[2 * i];
            tps1[2 * k + 1] = points1[2 * i + 1];
            k++;
            np1++;
        }
    }
    else
    {
        i = n - 1;
        while (points1[2 * i] >= sspx)
        {
            i--;
        }
        ns = i + 1;

        while (points1[2 * i] < (satmap->x0 + satmap->width / 2))
        {
            tps2[2 * j] = points1[2 * i];
            tps2[2 * j + 1] = points1[2 * i + 1];
            i--;
            j++;
            np2++;
        }

        while (i >= 0)
        {
            tps1[2 * k] = points1[2 * i];
            tps1[2 * k + 1] = points1[2 * i + 1];
            i--;
            k++;
            np1++;
        }

        for (i = n - 1; i >= ns; i--)
        {
            tps1[2 * k] = points1[2 * i];
            tps1[2 * k + 1] = points1[2 * i + 1];
            k++;
            np1++;
        }
    }

    for (i = 0; i < np1; i++)
    {
        points1[2 * i] = tps1[2 * i];
        points1[2 * i + 1] = tps1[2 * i + 1];
    }
    *n1 = np1;

    for (i = 0; i < np2; i++)
    {
        points2[2 * i] = tps2[2 * i];
        points2[2 * i + 1] = tps2[2 * i + 1];
    }
    *n2 = np2;

    g_free(tps1);
    g_free(tps2);

    if (np1 > 0 && np2 > 0)
    {
        if (points1[0] > (satmap->x0 + satmap->width / 2))
        {
            points1[0] = satmap->x0 + satmap->width;
            points1[2 * (np1 - 1)] = satmap->x0 + satmap->width;
            points2[0] = satmap->x0;
            points2[2 * (np2 - 1)] = satmap->x0;
        }
        else
        {
            points2[0] = satmap->x0 + satmap->width;
            points2[2 * (np2 - 1)] = satmap->x0 + satmap->width;
            points1[0] = satmap->x0;
            points1[2 * (np1 - 1)] = satmap->x0;
        }
    }
}

static void sort_points_x(GtkSatMap * satmap, sat_t * sat,
                          gdouble * points, gint num)
{
    gsize           size = 2 * sizeof(double);

#if GLIB_CHECK_VERSION(2, 82, 0)
    g_sort_array(points, num, size, compare_coordinates_x, NULL);
#else
    g_qsort_with_data(points, num, size, compare_coordinates_x, NULL);
#endif

    points[2] = satmap->x0;
    points[3] = points[1];

    points[716] = satmap->x0 + satmap->width;
    points[717] = points[719];

    if (sat->ssplat > 0.0)
    {
        points[0] = satmap->x0;
        points[1] = satmap->y0;

        points[718] = satmap->x0 + satmap->width;
        points[719] = satmap->y0;
    }
    else
    {
        points[0] = satmap->x0;
        points[1] = satmap->y0 + satmap->height;

        points[718] = satmap->x0 + satmap->width;
        points[719] = satmap->y0 + satmap->height;
    }
}

static void sort_points_y(GtkSatMap * satmap, sat_t * sat,
                          gdouble * points, gint num)
{
    gsize           size;

    (void)satmap;
    (void)sat;
    size = 2 * sizeof(double);

#if GLIB_CHECK_VERSION(2, 82, 0)
    g_sort_array(points, num, size, compare_coordinates_y, NULL);
#else
    g_qsort_with_data(points, num, size, compare_coordinates_y, NULL);
#endif
}

static gint compare_coordinates_x(gconstpointer a, gconstpointer b,
                                  gpointer data)
{
    double         *ea = (double *)a;
    double         *eb = (double *)b;

    (void)data;

    if (ea[0] < eb[0])
        return -1;
    else if (ea[0] > eb[0])
        return 1;

    return 0;
}

static gint compare_coordinates_y(gconstpointer a, gconstpointer b,
                                  gpointer data)
{
    double         *ea = (double *)a;
    double         *eb = (double *)b;

    (void)data;

    if (ea[1] < eb[1])
        return -1;
    else if (ea[1] > eb[1])
        return 1;

    return 0;
}

static void plot_sat(gpointer key, gpointer value, gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    sat_map_obj_t  *obj = NULL;
    sat_t          *sat = SAT(value);
    gint           *catnum;
    gfloat          x, y;
    gchar          *tooltip;

    (void)key;

    if (decayed(sat))
    {
        return;
    }

    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;

    lonlat_to_xy(satmap, sat->ssplon, sat->ssplat, &x, &y);

    obj = g_try_new0(sat_map_obj_t, 1);

    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Cannot allocate memory for satellite %d."),
                    __func__, sat->tle.catnr);
        return;
    }

    obj->selected = FALSE;

    if (!g_hash_table_lookup_extended(satmap->showtracks, catnum, NULL, NULL))
    {
        obj->showtrack = FALSE;
    }
    else
    {
        obj->showtrack = TRUE;
    }

    if (!g_hash_table_lookup_extended(satmap->hidecovs, catnum, NULL, NULL))
    {
        obj->showcov = TRUE;
    }
    else
    {
        obj->showcov = FALSE;
    }

    obj->istarget = FALSE;
    obj->oldrcnum = 0;
    obj->newrcnum = 0;
    obj->catnum = sat->tle.catnr;
    obj->track_data.latlon = NULL;
    obj->track_data.lines = NULL;
    obj->track_orbit = 0;

    obj->x = x;
    obj->y = y;

    obj->nickname = g_strdup(sat->nickname);

    tooltip = g_markup_printf_escaped("<b>%s</b>\n"
                                      "Lon: %5.1f\302\260\n"
                                      "Lat: %5.1f\302\260\n"
                                      " Az: %5.1f\302\260\n"
                                      " El: %5.1f\302\260",
                                      sat->nickname,
                                      sat->ssplon, sat->ssplat,
                                      sat->az, sat->el);
    obj->tooltip = tooltip;

    obj->range1_points = NULL;
    obj->range1_count = 0;
    obj->range2_points = NULL;
    obj->range2_count = 0;

    obj->newrcnum = calculate_footprint(satmap, sat, obj);
    obj->oldrcnum = obj->newrcnum;

    g_hash_table_insert(satmap->obj, catnum, obj);
}

static void free_sat_obj(gpointer key, gpointer value, gpointer data)
{
    sat_map_obj_t  *obj = SAT_MAP_OBJ(value);
    sat_t          *sat = NULL;
    GtkSatMap      *satmap = GTK_SAT_MAP(data);

    (void)key;

    if (obj->showtrack)
    {
        sat = SAT(g_hash_table_lookup(satmap->sats, &obj->catnum));
        ground_track_delete(satmap, sat, satmap->qth, obj, TRUE);
    }

    g_free(obj->nickname);
    obj->nickname = NULL;
    g_free(obj->tooltip);
    obj->tooltip = NULL;
    g_free(obj->range1_points);
    obj->range1_points = NULL;
    g_free(obj->range2_points);
    obj->range2_points = NULL;
}

static void update_sat(gpointer key, gpointer value, gpointer data)
{
    gint           *catnum;
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    sat_map_obj_t  *obj = NULL;
    sat_t          *sat = SAT(value);
    gfloat          x, y;
    gfloat          oldx, oldy;
    gdouble         now;
    gchar          *tooltip;
    gchar          *aosstr;

    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;

    now = satmap->tstamp;

    if (sat->aos > now)
    {
        if ((sat->aos < satmap->naos) || (satmap->naos == 0.0))
        {
            satmap->naos = sat->aos;
            satmap->ncat = sat->tle.catnr;
        }
    }

    obj = SAT_MAP_OBJ(g_hash_table_lookup(satmap->obj, catnum));

    if (decayed(sat) && obj != NULL)
    {
        free_sat_obj(NULL, obj, satmap);
        g_hash_table_remove(satmap->obj, catnum);
        g_free(catnum);
        return;
    }

    if (obj == NULL)
    {
        if (decayed(sat))
        {
            g_free(catnum);
            return;
        }
        else
        {
            plot_sat(key, value, data);
            g_free(catnum);
            return;
        }
    }

    if (obj->selected)
    {
        update_selected(satmap, sat);
    }

    g_free(obj->nickname);
    obj->nickname = g_strdup(sat->nickname);

    aosstr = aoslos_time_to_str(satmap, sat);
    g_free(obj->tooltip);
    tooltip = g_markup_printf_escaped("<b>%s</b>\n"
                                      "Lon: %5.1f\302\260\n"
                                      "Lat: %5.1f\302\260\n"
                                      " Az: %5.1f\302\260\n"
                                      " El: %5.1f\302\260\n"
                                      "%s",
                                      sat->nickname,
                                      sat->ssplon, sat->ssplat,
                                      sat->az, sat->el, aosstr);
    obj->tooltip = tooltip;
    g_free(aosstr);

    lonlat_to_xy(satmap, sat->ssplon, sat->ssplat, &x, &y);

    oldx = obj->x;
    oldy = obj->y;

    if ((fabs(oldx - x) >= 2 * MARKER_SIZE_HALF) ||
        (fabs(oldy - y) >= 2 * MARKER_SIZE_HALF))
    {
        obj->x = x;
        obj->y = y;

        obj->newrcnum = calculate_footprint(satmap, sat, obj);
        obj->oldrcnum = obj->newrcnum;
    }

    if (obj->showtrack)
    {
        if (obj->track_orbit != sat->orbit)
        {
            ground_track_update(satmap, sat, satmap->qth, obj, TRUE);
        }
        else if (satmap->resize)
        {
            ground_track_update(satmap, sat, satmap->qth, obj, FALSE);
        }
    }

    g_free(catnum);
}

static void update_selected(GtkSatMap * satmap, sat_t * sat)
{
    guint           h, m, s;
    gchar          *ch, *cm, *cs;
    gchar          *alsstr, *text;
    gdouble         number, now;
    gboolean        isgeo = FALSE;

    now = satmap->tstamp;

    if (sat->el > 0.0)
    {
        if (sat->los > 0.0)
        {
            number = sat->los - now;
            alsstr = g_strdup("LOS");
        }
        else
        {
            isgeo = TRUE;
        }
    }
    else
    {
        if (sat->aos > 0.0)
        {
            number = sat->aos - now;
            alsstr = g_strdup("AOS");
        }
        else
        {
            isgeo = TRUE;
        }
    }

    if (isgeo)
    {
        if (sat->el > 0.0)
        {
            text = g_markup_printf_escaped(
                "<span background=\"#%s\"> %s: Always in range </span>",
                satmap->infobgd, sat->nickname);
        }
        else
        {
            text = g_markup_printf_escaped(
                "<span background=\"#%s\"> %s: Always out of range </span>",
                satmap->infobgd, sat->nickname);
        }
    }
    else
    {
        s = (guint)(number * 86400);

        h = (guint)floor(s / 3600);
        s -= 3600 * h;

        if ((h > 0) && (h < 10))
            ch = g_strdup("0");
        else
            ch = g_strdup("");

        m = (guint)floor(s / 60);
        s -= 60 * m;

        if (m < 10)
            cm = g_strdup("0");
        else
            cm = g_strdup("");

        if (s < 10)
            cs = g_strdup(":0");
        else
            cs = g_strdup(":");

        if (h > 0)
        {
            text = g_markup_printf_escaped("<span background=\"#%s\"> "
                                           "%s %s in %s%d:%s%d%s%d </span>",
                                           satmap->infobgd, sat->nickname,
                                           alsstr, ch, h, cm, m, cs, s);
        }
        else
        {
            text = g_markup_printf_escaped("<span background=\"#%s\"> "
                                           "%s %s in %s%d%s%d </span>",
                                           satmap->infobgd, sat->nickname,
                                           alsstr, cm, m, cs, s);
        }

        g_free(ch);
        g_free(cm);
        g_free(cs);
        g_free(alsstr);
    }

    g_free(satmap->sel_text);
    satmap->sel_text = text;
}

static inline gdouble sgn(gdouble const t)
{
    return t < 0.0 ? -1.0 : 1.0;
}

static void redraw_terminator(GtkSatMap * satmap)
{
    int             longitude;
    gfloat          x, y;
    gdouble         lx, ly;
    geodetic_t      geodetic;
    vector_t        sun_;
    gdouble         sx, sy, sz;
    gdouble         rx, ry, rz;

    if (satmap->terminator_points == NULL)
    {
        satmap->terminator_points = g_new(gdouble, 363 * 2);
    }

    Calculate_Solar_Position(satmap->tstamp, &sun_);
    Calculate_LatLonAlt(satmap->tstamp, &sun_, &geodetic);

    sx = cos(geodetic.lat) * cos(geodetic.lon);
    sy = cos(geodetic.lat) * sin(-geodetic.lon);
    sz = sin(geodetic.lat);

    for (longitude = -180; longitude <= 180; ++longitude)
    {
        int centered_longitude = longitude + (satmap->left_side_lon - 180.0);
        lx = cos(de2ra * (centered_longitude + sgn(sz) * 90));
        ly = sin(de2ra * (centered_longitude + sgn(sz) * 90));

        rx = ly * sz;
        ry = -lx * sz;
        rz = -lx * sy - ly * sx;

        gdouble length = sqrt(rx * rx + ry * ry + rz * rz);

        lonlat_to_xy(satmap,
                     centered_longitude, asin(rz / length) * (1.0 / de2ra),
                     &x, &y);

        if (180 == longitude)
        {
            x = satmap->x0 + satmap->width;
        }

        satmap->terminator_points[2 * (longitude + 181)] = x;
        satmap->terminator_points[2 * (longitude + 181) + 1] = y;
    }

    satmap->terminator_points[0] = satmap->x0;
    satmap->terminator_points[1] = sz < 0.0 ? satmap->y0 :
        (satmap->y0 + satmap->height);

    satmap->terminator_points[724] = satmap->x0 + satmap->width;
    satmap->terminator_points[725] = sz < 0.0 ? satmap->y0 :
        (satmap->y0 + satmap->height);

    satmap->terminator_count = 363;
}

void gtk_sat_map_lonlat_to_xy(GtkSatMap * m,
                              gdouble lon, gdouble lat,
                              gdouble * x, gdouble * y)
{
    gfloat          fx, fy;

    fx = (gfloat)*x;
    fy = (gfloat)*y;

    lonlat_to_xy(m, lon, lat, &fx, &fy);

    *x = (gdouble)fx;
    *y = (gdouble)fy;
}

void gtk_sat_map_reload_sats(GtkWidget * satmap, GHashTable * sats)
{
    GTK_SAT_MAP(satmap)->sats = sats;
    GTK_SAT_MAP(satmap)->naos = 0.0;
    GTK_SAT_MAP(satmap)->ncat = 0;

    g_hash_table_foreach(GTK_SAT_MAP(satmap)->obj, reset_ground_track, NULL);
}

static void reset_ground_track(gpointer key, gpointer value,
                               gpointer user_data)
{
    sat_map_obj_t  *obj = (sat_map_obj_t *)value;
    (void)key;
    (void)user_data;

    obj->track_orbit = 0;
}

static gchar   *aoslos_time_to_str(GtkSatMap * satmap, sat_t * sat)
{
    guint           h, m, s;
    gdouble         number, now;
    gchar          *text = NULL;

    now = satmap->tstamp;
    if (sat->el > 0.0)
    {
        number = sat->los - now;
    }
    else
    {
        number = sat->aos - now;
    }

    s = (guint)(number * 86400);

    h = (guint)floor(s / 3600);
    s -= 3600 * h;

    m = (guint)floor(s / 60);
    s -= 60 * m;

    if (sat->el > 0.0)
        text = g_strdup_printf(_("LOS in %d minutes"), m + 60 * h);
    else
        text = g_strdup_printf(_("AOS in %d minutes"), m + 60 * h);

    return text;
}

static void gtk_sat_map_load_showtracks(GtkSatMap * satmap)
{
    mod_cfg_get_integer_list_boolean(satmap->cfgdata,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_SHOWTRACKS,
                                     satmap->showtracks);
}

static void gtk_sat_map_store_showtracks(GtkSatMap * satmap)
{
    mod_cfg_set_integer_list_boolean(satmap->cfgdata,
                                     satmap->showtracks,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_SHOWTRACKS);
}

static void gtk_sat_map_store_hidecovs(GtkSatMap * satmap)
{
    mod_cfg_set_integer_list_boolean(satmap->cfgdata,
                                     satmap->hidecovs,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_HIDECOVS);
}

static void gtk_sat_map_load_hide_coverages(GtkSatMap * satmap)
{
    mod_cfg_get_integer_list_boolean(satmap->cfgdata,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_HIDECOVS, satmap->hidecovs);
}
