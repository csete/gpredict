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

#include <goocanvas.h>
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
static gboolean on_motion_notify(GooCanvasItem * item, GooCanvasItem * target,
                                 GdkEventMotion * event, gpointer data);
static void     on_item_created(GooCanvas * canvas, GooCanvasItem * item,
                                GooCanvasItemModel * model, gpointer data);
static void     on_canvas_realized(GtkWidget * canvas, gpointer data);
static gboolean on_button_press(GooCanvasItem * item,
                                GooCanvasItem * target,
                                GdkEventButton * event, gpointer data);
static gboolean on_button_release(GooCanvasItem * item,
                                  GooCanvasItem * target,
                                  GdkEventButton * event, gpointer data);
static void     clear_selection(gpointer key, gpointer val, gpointer data);
static void     load_map_file(GtkSatMap * satmap, float clon);
static GooCanvasItemModel *create_canvas_model(GtkSatMap * satmap);
static gdouble  arccos(gdouble, gdouble);
static gboolean pole_is_covered(sat_t * sat);
static gboolean north_pole_is_covered(sat_t * sat);
static gboolean south_pole_is_covered(sat_t * sat);
static gboolean mirror_lon(sat_t * sat, gdouble rangelon, gdouble * mlon,
                           gdouble mapbreak);
static guint    calculate_footprint(GtkSatMap * satmap, sat_t * sat);
static void     split_points(GtkSatMap * satmap, sat_t * sat, gdouble sspx);
static void     sort_points_x(GtkSatMap * satmap, sat_t * sat,
                              GooCanvasPoints * points, gint num);
static void     sort_points_y(GtkSatMap * satmap, sat_t * sat,
                              GooCanvasPoints * points, gint num);
static gint     compare_coordinates_x(gconstpointer a, gconstpointer b,
                                      gpointer data);
static gint     compare_coordinates_y(gconstpointer a, gconstpointer b,
                                      gpointer data);
static void     update_selected(GtkSatMap * satmap, sat_t * sat);
static void     draw_grid_lines(GtkSatMap * satmap, GooCanvasItemModel * root);
static void     redraw_grid_lines(GtkSatMap * satmap);
static void     draw_terminator(GtkSatMap * satmap, GooCanvasItemModel * root);
static void     redraw_terminator(GtkSatMap * satmap);
static gchar   *aoslos_time_to_str(GtkSatMap * satmap, sat_t * sat);
static void     gtk_sat_map_load_showtracks(GtkSatMap * map);
static void     gtk_sat_map_store_showtracks(GtkSatMap * satmap);
static void     gtk_sat_map_load_hide_coverages(GtkSatMap * map);
static void     gtk_sat_map_store_hidecovs(GtkSatMap * satmap);
static void     reset_ground_track(gpointer key, gpointer value,
                                   gpointer user_data);

static GtkVBoxClass *parent_class = NULL;
static GooCanvasPoints *points1;
static GooCanvasPoints *points2;


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
    satmap->show_terminator = FALSE;
    satmap->qthinfo = FALSE;
    satmap->eventinfo = FALSE;
    satmap->cursinfo = FALSE;
    satmap->showgrid = FALSE;
    satmap->keepratio = FALSE;
    satmap->resize = FALSE;
}

static void gtk_sat_map_destroy(GtkWidget * widget)
{
    GtkSatMap          *satmap = GTK_SAT_MAP(widget);
    GooCanvasItemModel *root;
    gint                idx;
    guint               i;

    /* check widget isn't already destroyed */
    if (satmap->obj) {
        /* save config */
        gtk_sat_map_store_showtracks(GTK_SAT_MAP(widget));
        gtk_sat_map_store_hidecovs(GTK_SAT_MAP(widget));

        /* sat objects need a reference to the widget to free all alocations */
        g_hash_table_foreach(satmap->obj, free_sat_obj, satmap);
        g_hash_table_destroy(satmap->obj);
        satmap->obj = NULL;

        /* these objects destruct themselves cleanly */
        g_object_unref(satmap->origmap);
        satmap->origmap = NULL;
        g_hash_table_destroy(satmap->showtracks);
        satmap->showtracks = NULL;
        g_hash_table_destroy(satmap->hidecovs);
        satmap->hidecovs = NULL;

        root = goo_canvas_get_root_item_model(GOO_CANVAS(satmap->canvas));

        idx = goo_canvas_item_model_find_child(root, satmap->qthmark);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->qthmark = NULL;

        idx = goo_canvas_item_model_find_child(root, satmap->qthlabel);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->qthlabel = NULL;

        idx = goo_canvas_item_model_find_child(root, satmap->locnam);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->locnam = NULL;

        idx = goo_canvas_item_model_find_child(root, satmap->curs);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->curs = NULL;

        idx = goo_canvas_item_model_find_child(root, satmap->next);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->next = NULL;

        idx = goo_canvas_item_model_find_child(root, satmap->sel);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->sel = NULL;

        idx = goo_canvas_item_model_find_child(root, satmap->terminator);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->terminator = NULL;

        for (i = 0; i < 5; i++)
        {
            idx = goo_canvas_item_model_find_child(root, satmap->gridh[i]);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);
            satmap->gridh[i] = NULL;

            idx = goo_canvas_item_model_find_child(root, satmap->gridhlab[i]);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);
            satmap->gridhlab[i] = NULL;
        }

        for (i = 0; i < 11; i++)
        {
            idx = goo_canvas_item_model_find_child(root, satmap->gridv[i]);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);
            satmap->gridv[i] = NULL;

            idx = goo_canvas_item_model_find_child(root, satmap->gridvlab[i]);
            if (idx != -1)
                goo_canvas_item_model_remove_child(root, idx);
            satmap->gridvlab[i] = NULL;
        }

        idx = goo_canvas_item_model_find_child(root, satmap->map);
        if (idx != -1)
            goo_canvas_item_model_remove_child(root, idx);
        satmap->map = NULL;
    }
    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

GtkWidget      *gtk_sat_map_new(GKeyFile * cfgdata, GHashTable * sats,
                                qth_t * qth)
{
    GtkSatMap      *satmap;
    GooCanvasItemModel *root;
    guint32         col;

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

    satmap->canvas = goo_canvas_new();
    g_object_set(G_OBJECT(satmap->canvas), "has-tooltip", TRUE, NULL);

    float           clon = (float)mod_cfg_get_int(cfgdata,
                                                  MOD_CFG_MAP_SECTION,
                                                  MOD_CFG_MAP_CENTER,
                                                  SAT_CFG_INT_MAP_CENTER);

    load_map_file(satmap, clon);

    /* Initial size request should be based on map size
       but if we do this we can not shrink the canvas below this size
       even though the map shrinks => better to set default size of
       main container window.
     */
    /*  gtk_widget_set_size_request (satmap->canvas, */
    /*  gdk_pixbuf_get_width (satmap->origmap), */
    /*  gdk_pixbuf_get_height (satmap->origmap)); */

    goo_canvas_set_bounds(GOO_CANVAS(satmap->canvas), 0, 0,
                          gdk_pixbuf_get_width(satmap->origmap),
                          gdk_pixbuf_get_height(satmap->origmap));

    g_signal_connect(satmap->canvas, "size-allocate",
                     G_CALLBACK(size_allocate_cb), satmap);
    g_signal_connect(satmap->canvas, "item_created",
                     (GCallback) on_item_created, satmap);
    g_signal_connect_after(satmap->canvas, "realize",
                           (GCallback) on_canvas_realized, satmap);

    gtk_widget_show(satmap->canvas);

    root = create_canvas_model(satmap);
    goo_canvas_set_root_item_model(GOO_CANVAS(satmap->canvas), root);
    g_object_unref(root);

    gtk_sat_map_load_showtracks(satmap);
    gtk_sat_map_load_hide_coverages(satmap);
    g_hash_table_foreach(satmap->sats, plot_sat, satmap);

    gtk_box_pack_start(GTK_BOX(satmap), satmap->canvas, TRUE, TRUE, 0);

    return GTK_WIDGET(satmap);
}

static GooCanvasItemModel *create_canvas_model(GtkSatMap * satmap)
{
    GooCanvasItemModel *root;
    gchar          *buff;
    gfloat          x, y;
    guint32         col;

    satmap->terminator = NULL;
    root = goo_canvas_group_model_new(NULL, NULL);

    /* map dimensions */
    satmap->width = 200;        // was: gdk_pixbuf_get_width (satmap->origmap);
    satmap->height = 100;       // was: gdk_pixbuf_get_height (satmap->origmap);
    satmap->x0 = 0;
    satmap->y0 = 0;

    /* background map */
    satmap->map = goo_canvas_image_model_new(root,
                                             satmap->origmap,
                                             satmap->x0, satmap->y0, NULL);

    goo_canvas_item_model_lower(satmap->map, NULL);
    draw_grid_lines(satmap, root);

    if (satmap->show_terminator)
        draw_terminator(satmap, root);

    /* QTH mark */
    col = mod_cfg_get_int(satmap->cfgdata,
                          MOD_CFG_MAP_SECTION,
                          MOD_CFG_MAP_QTH_COL, SAT_CFG_INT_MAP_QTH_COL);
    lonlat_to_xy(satmap, satmap->qth->lon, satmap->qth->lat, &x, &y);
    satmap->qthmark = goo_canvas_rect_model_new(root,
                                                x - MARKER_SIZE_HALF,
                                                y - MARKER_SIZE_HALF,
                                                2 * MARKER_SIZE_HALF,
                                                2 * MARKER_SIZE_HALF,
                                                "fill-color-rgba", col,
                                                "stroke-color-rgba", col,
                                                NULL);
    satmap->qthlabel = goo_canvas_text_model_new(root, satmap->qth->name,
                                                 x, y + 2, -1,
                                                 GOO_CANVAS_ANCHOR_NORTH,
                                                 "font", "Sans 8",
                                                 "fill-color-rgba", col, NULL);

    /* QTH info */
    col = mod_cfg_get_int(satmap->cfgdata,
                          MOD_CFG_MAP_SECTION,
                          MOD_CFG_MAP_INFO_COL, SAT_CFG_INT_MAP_INFO_COL);

    satmap->locnam = goo_canvas_text_model_new(root, "",
                                               satmap->x0 + 2, satmap->y0 + 1,
                                               -1,
                                               GOO_CANVAS_ANCHOR_NORTH_WEST,
                                               "font", "Sans 8",
                                               "fill-color-rgba", col,
                                               "use-markup", TRUE, NULL);

    /* set text only if QTH info is enabled */
    if (satmap->qthinfo)
    {
        /* For now only QTH name and location.
           It would be nice with coordinates (remember NWSE setting)
           and Maidenhead locator, when using hamlib.

           Note: I used pango markup to set the background color, I didn't find any
           other obvious ways to get the text height in pixels to draw rectangle.
         */
        buff =
            g_strdup_printf("<span background=\"#%s\"> %s \302\267 %s </span>",
                            satmap->infobgd, satmap->qth->name,
                            satmap->qth->loc);
        g_object_set(satmap->locnam, "text", buff, NULL);
        g_free(buff);
    }

    /* next event */
    satmap->next = goo_canvas_text_model_new(root, "",
                                             satmap->x0 + satmap->width - 2,
                                             satmap->y0 + 1, -1,
                                             GOO_CANVAS_ANCHOR_NORTH_EAST,
                                             "font", "Sans 8",
                                             "fill-color-rgba", col,
                                             "use-markup", TRUE, NULL);

    /* set text only if QTH info is enabled */
    if (satmap->eventinfo)
    {
        buff = g_strdup_printf("<span background=\"#%s\"> ... </span>",
                               satmap->infobgd);
        g_object_set(satmap->next, "text", buff, NULL);
        g_free(buff);
    }

    /* cursor track */
    satmap->curs = goo_canvas_text_model_new(root, "",
                                             satmap->x0 + 2,
                                             satmap->y0 + satmap->height - 1,
                                             -1,
                                             GOO_CANVAS_ANCHOR_SOUTH_WEST,
                                             "font", "Sans 8",
                                             "fill-color-rgba", col,
                                             "use-markup", TRUE, NULL);

    /* info about a selected satellite */
    satmap->sel = goo_canvas_text_model_new(root, "",
                                            satmap->x0 + satmap->width - 2,
                                            satmap->y0 + satmap->height - 1,
                                            -1,
                                            GOO_CANVAS_ANCHOR_SOUTH_EAST,
                                            "font", "Sans 8",
                                            "fill-color-rgba", col,
                                            "use-markup", TRUE, NULL);

    return root;
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
    gfloat          x, y;
    gfloat          ratio;      /* ratio between map width and height */
    gfloat          size;       /* size = min (alloc.w, ratio*alloc.h) */

    if (gtk_widget_get_realized(GTK_WIDGET(satmap)))
    {
        /* get graph dimensions */
        gtk_widget_get_allocation(GTK_WIDGET(satmap), &allocation);

        if (satmap->keepratio)
        {
            /* Use allocation->width and allocation->height to calculate
             *  new X0 Y0 width and height. Map proportions must be kept.
             */
            ratio = gdk_pixbuf_get_width(satmap->origmap) /
                gdk_pixbuf_get_height(satmap->origmap);

            size = MIN(allocation.width, ratio * allocation.height);

            satmap->width = (guint) size;
            satmap->height = (guint) (size / ratio);

            satmap->x0 = (allocation.width - satmap->width) / 2;
            satmap->y0 = (allocation.height - satmap->height) / 2;

            /* rescale pixbuf */
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

            /* rescale pixbuf */
            pbuf = gdk_pixbuf_scale_simple(satmap->origmap,
                                           satmap->width,
                                           satmap->height,
                                           GDK_INTERP_BILINEAR);
        }

        /* set canvas bounds to match new size */
        goo_canvas_set_bounds(GOO_CANVAS(GTK_SAT_MAP(satmap)->canvas), 0, 0,
                              satmap->width, satmap->height);


        /* redraw static elements */
        g_object_set(satmap->map,
                     "pixbuf", pbuf,
                     "x", (gdouble) satmap->x0,
                     "y", (gdouble) satmap->y0, NULL);
        g_object_unref(pbuf);

        redraw_grid_lines(satmap);

        if (satmap->show_terminator)
            redraw_terminator(satmap);

        lonlat_to_xy(satmap, satmap->qth->lon, satmap->qth->lat, &x, &y);
        g_object_set(satmap->qthmark,
                     "x", x - MARKER_SIZE_HALF,
                     "y", y - MARKER_SIZE_HALF, NULL);
        g_object_set(satmap->qthlabel, "x", x, "y", y + 2, NULL);

        g_object_set(satmap->locnam,
                     "x", (gdouble) satmap->x0 + 2,
                     "y", (gdouble) satmap->y0 + 1, NULL);

        g_object_set(satmap->next,
                     "x", (gdouble) satmap->x0 + satmap->width - 2,
                     "y", (gdouble) satmap->y0 + 1, NULL);

        g_object_set(satmap->curs,
                     "x", (gdouble) satmap->x0 + 2,
                     "y", (gdouble) satmap->y0 + satmap->height - 1, NULL);

        g_object_set(satmap->sel,
                     "x", (gdouble) satmap->x0 + satmap->width - 2,
                     "y", (gdouble) satmap->y0 + satmap->height - 1, NULL);

        g_hash_table_foreach(satmap->sats, update_sat, satmap);
        satmap->resize = FALSE;
    }
}

static void on_canvas_realized(GtkWidget * canvas, gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);

    (void)canvas;

    goo_canvas_item_model_raise(satmap->sel, NULL);
    goo_canvas_item_model_raise(satmap->locnam, NULL);
    goo_canvas_item_model_raise(satmap->next, NULL);
    goo_canvas_item_model_raise(satmap->curs, NULL);
}

/* Update the GtkSatMap widget. Called periodically from GtkSatModule. */
void gtk_sat_map_update(GtkWidget * widget)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(widget);
    sat_t          *sat = NULL;
    gdouble         number, now;
    gchar          *buff;
    gint           *catnr;
    guint           h, m, s;
    gchar          *ch, *cm, *cs;
    gfloat          x, y;
    gdouble         oldx, oldy;

    /* check whether there are any pending resize requests */
    if (satmap->resize)
        update_map_size(satmap);

    /* check if qth has moved significantly, if so move it */
    lonlat_to_xy(satmap, satmap->qth->lon, satmap->qth->lat, &x, &y);
    g_object_get(satmap->qthmark, "x", &oldx, "y", &oldy, NULL);

    if ((fabs(oldx - x) >= 2 * MARKER_SIZE_HALF) ||
        (fabs(oldy - y) >= 2 * MARKER_SIZE_HALF))
    {
        g_object_set(satmap->qthmark,
                     "x", x - MARKER_SIZE_HALF,
                     "y", y - MARKER_SIZE_HALF, NULL);
        g_object_set(satmap->qthlabel, "x", x, "y", y + 2, NULL);
        g_object_set(satmap->locnam,
                     "x", (gdouble) satmap->x0 + 2,
                     "y", (gdouble) satmap->y0 + 1, NULL);
        satmap->counter = satmap->refresh;
    }

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

        lonlat_to_xy(satmap, satmap->qth->lon, satmap->qth->lat, &x, &y);
        g_object_set(satmap->qthmark,
                     "x", x - MARKER_SIZE_HALF,
                     "y", y - MARKER_SIZE_HALF, NULL);
        g_object_set(satmap->qthlabel, "x", x, "y", y + 2, NULL);
        g_object_set(satmap->locnam,
                     "x", (gdouble) satmap->x0 + 2,
                     "y", (gdouble) satmap->y0 + 1, NULL);

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
                    s = (guint) (number * 86400);

                    /* extract hours */
                    h = (guint) floor(s / 3600);
                    s -= 3600 * h;

                    /* leading zero */
                    if ((h > 0) && (h < 10))
                        ch = g_strdup("0");
                    else
                        ch = g_strdup("");

                    /* extract minutes */
                    m = (guint) floor(s / 60);
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

                    if (h > 0)
                        buff =
                            g_markup_printf_escaped(_
                                                    ("<span background=\"#%s\"> "
                                                     "Next: %s in %s%d:%s%d%s%d </span>"),
                                                    satmap->infobgd,
                                                    sat->nickname, ch, h, cm,
                                                    m, cs, s);
                    else
                        buff =
                            g_markup_printf_escaped(_
                                                    ("<span background=\"#%s\"> "
                                                     "Next: %s in %s%d%s%d </span>"),
                                                    satmap->infobgd,
                                                    sat->nickname, cm, m, cs,
                                                    s);

                    g_object_set(satmap->next, "text", buff, NULL);

                    g_free(buff);
                    g_free(ch);
                    g_free(cm);
                    g_free(cs);
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Can not find NEXT satellite."),
                                __func__);
                    g_object_set(satmap->next, "text", _("Next: ERR"), NULL);
                }
            }
            else
            {
                g_object_set(satmap->next, "text", _("Next: N/A"), NULL);
            }
        }
        else
        {
            g_object_set(satmap->next, "text", "", NULL);
        }
    }
}

/* Assumes that -180 <= lon <= 180 and -90 <= lat <= 90 */
static void lonlat_to_xy(GtkSatMap * p, gdouble lon, gdouble lat, gfloat * x,
                         gfloat * y)
{
    *x = p->x0 + (lon - p->left_side_lon) * p->width / 360.0;
    *y = p->y0 + (90.0 - lat) * p->height / 180.0;
    while (*x < 0)
    {
        *x += p->width;
    }
    while (*x > p->width)
    {
        *x -= p->width;
    }
}

static void xy_to_lonlat(GtkSatMap * p, gfloat x, gfloat y, gfloat * lon,
                         gfloat * lat)
{
    *lat = 90.0 - (180.0 / p->height) * (y - p->y0);
    *lon = (360.0 / p->width) * (x - p->x0) + p->left_side_lon;
    while (*lon > 180)
    {
        *lon -= 360;
    }
    while (*lon < -180)
    {
        *lon += 360;
    }
}

static gboolean on_motion_notify(GooCanvasItem * item,
                                 GooCanvasItem * target,
                                 GdkEventMotion * event, gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    gfloat          lat, lon;
    gchar          *text;

    (void)target;
    (void)item;

    /* set text only if QTH info is enabled */
    if (satmap->cursinfo)
    {
        xy_to_lonlat(satmap, event->x, event->y, &lon, &lat);

        /* cursor track */
        text = g_strdup_printf("<span background=\"#%s\"> "
                               "LON:%.0f\302\260 LAT:%.0f\302\260 </span>",
                               satmap->infobgd, lon, lat);

        g_object_set(satmap->curs, "text", text, NULL);
        g_free(text);
    }

    return TRUE;
}

/*
 * Finish canvas item setup.
 *
 * This function is called when a canvas item is created. Its purpose is to connect
 * the corresponding signals to the created items.
 *
 * The root item, ie the background is connected to motion notify event, while other
 * items (sats) are connected to mouse click events.
 *
 * @bug Should filter out QTH or do we want to click on the QTH?
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
                         (GCallback) on_motion_notify, data);
    }
    else if (!g_object_get_data(G_OBJECT(item), "skip-signal-connection"))
    {
        g_signal_connect(item, "button_press_event",
                         (GCallback) on_button_press, data);
        g_signal_connect(item, "button_release_event",
                         (GCallback) on_button_release, data);
    }
}

static gboolean on_button_press(GooCanvasItem * item,
                                GooCanvasItem * target, GdkEventButton * event,
                                gpointer data)
{
    GooCanvasItemModel *model = goo_canvas_item_get_model(item);
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    gint            catnum =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(model), "catnum"));
    gint           *catpoint = NULL;
    sat_t          *sat = NULL;

    (void)target;

    switch (event->button)
    {
        /* double-left-click */
    case 1:
        if (event->type == GDK_2BUTTON_PRESS)
        {
            catpoint = g_try_new0(gint, 1);
            *catpoint = catnum;

            sat = SAT(g_hash_table_lookup(satmap->sats, catpoint));
            if (sat != NULL)
            {
                show_sat_info(sat, gtk_widget_get_toplevel(GTK_WIDGET(data)));
            }
            else
            {
                /* double-clicked on map */
            }
        }
        g_free(catpoint);
        break;

        /* pop-up menu */
    case 3:
        catpoint = g_try_new0(gint, 1);
        *catpoint = catnum;
        sat = SAT(g_hash_table_lookup(satmap->sats, catpoint));
        if (sat != NULL)
        {
            gtk_sat_map_popup_exec(sat, satmap->qth, satmap, event,
                                   gtk_widget_get_toplevel(GTK_WIDGET
                                                           (satmap)));
        }
        else
        {
            /* clicked on map -> map pop-up in the future */
        }
        g_free(catpoint);
        break;
    default:
        break;
    }

    return TRUE;
}

static gboolean on_button_release(GooCanvasItem * item,
                                  GooCanvasItem * target,
                                  GdkEventButton * event, gpointer data)
{
    GooCanvasItemModel *model = goo_canvas_item_get_model(item);
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    gint            catnum =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(model), "catnum"));
    gint           *catpoint = NULL;
    sat_map_obj_t  *obj = NULL;
    guint32         col;

    (void)target;

    catpoint = g_try_new0(gint, 1);
    *catpoint = catnum;

    switch (event->button)
    {
        /* Select / de-select satellite */
    case 1:
        obj = SAT_MAP_OBJ(g_hash_table_lookup(satmap->obj, catpoint));
        if (obj == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _
                        ("%s:%d: Can not find clicked object (%d) in hash table"),
                        __FILE__, __LINE__, catnum);
        }
        else
        {
            obj->selected = !obj->selected;

            if (obj->selected)
            {
                col = mod_cfg_get_int(satmap->cfgdata,
                                      MOD_CFG_MAP_SECTION,
                                      MOD_CFG_MAP_SAT_SEL_COL,
                                      SAT_CFG_INT_MAP_SAT_SEL_COL);
            }
            else
            {
                col = mod_cfg_get_int(satmap->cfgdata,
                                      MOD_CFG_MAP_SECTION,
                                      MOD_CFG_MAP_SAT_COL,
                                      SAT_CFG_INT_MAP_SAT_COL);
                *catpoint = 0;

                g_object_set(satmap->sel, "text", "", NULL);
            }

            g_object_set(obj->marker,
                         "fill-color-rgba", col,
                         "stroke-color-rgba", col, NULL);
            g_object_set(obj->label,
                         "fill-color-rgba", col,
                         "stroke-color-rgba", col, NULL);
            g_object_set(obj->range1, "stroke-color-rgba", col, NULL);

            if (obj->oldrcnum == 2)
                g_object_set(obj->range2, "stroke-color-rgba", col, NULL);

            /* clear other selections */
            g_hash_table_foreach(satmap->obj, clear_selection, catpoint);
        }
        break;
    default:
        break;
    }

    g_free(catpoint);

    return TRUE;
}

static void clear_selection(gpointer key, gpointer val, gpointer data)
{
    gint           *old = key;
    gint           *new = data;
    sat_map_obj_t  *obj = SAT_MAP_OBJ(val);
    guint32         col;

    if ((*old != *new) && (obj->selected))
    {
        obj->selected = FALSE;

        /** FIXME: this is only global default; need the satmap here! */
        col = sat_cfg_get_int(SAT_CFG_INT_MAP_SAT_COL);

        g_object_set(obj->marker,
                     "fill-color-rgba", col, "stroke-color-rgba", col, NULL);
        g_object_set(obj->label,
                     "fill-color-rgba", col, "stroke-color-rgba", col, NULL);
        g_object_set(obj->range1, "stroke-color-rgba", col, NULL);

        if (obj->oldrcnum == 2)
            g_object_set(obj->range2, "stroke-color-rgba", col, NULL);
    }
}

void gtk_sat_map_select_sat(GtkWidget * satmap, gint catnum)
{
    GtkSatMap      *smap = GTK_SAT_MAP(satmap);
    gint           *catpoint = NULL;
    sat_map_obj_t  *obj = NULL;
    guint32         col;

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

        col = mod_cfg_get_int(smap->cfgdata,
                              MOD_CFG_MAP_SECTION,
                              MOD_CFG_MAP_SAT_SEL_COL,
                              SAT_CFG_INT_MAP_SAT_SEL_COL);

        g_object_set(obj->marker,
                     "fill-color-rgba", col, "stroke-color-rgba", col, NULL);
        g_object_set(obj->label,
                     "fill-color-rgba", col, "stroke-color-rgba", col, NULL);
        g_object_set(obj->range1, "stroke-color-rgba", col, NULL);

        if (obj->oldrcnum == 2)
            g_object_set(obj->range2, "stroke-color-rgba", col, NULL);

        /* clear other selections */
        g_hash_table_foreach(smap->obj, clear_selection, catpoint);
    }

    g_free(catpoint);
}

/*
 * Reconfigure map.
 *
 * This function should eventually reload all configuration for the GtkSatMap.
 * Currently this function is not implemented for any of the views. Reconfiguration
 * is done by recreating the whole module.
 */
void gtk_sat_map_reconf(GtkWidget * widget, GKeyFile * cfgdat)
{
    (void)widget;
    (void)cfgdat;
}

/*
 * Safely load a map file.
 *
 * @param satmap The GtkSatMap widget
 * @param clon The longitude that should be the center of the map
 *
 * This function is called shortly after the canvas has been created. Its purpose
 * is to load a mapfile into satmap->origmap.
 *
 * The function ensures that satmap->origmap will contain a valid GdkPixpuf, by
 * using the following logic:
 *
 *   - Get either module specific or global map file using mod_cfg_get_str
 *   - If the returned file does not exist try sat_cfg_get_str_def
 *   - If loading of default map does not succeed, create a dummy GdkPixbuf
 *     (and raise all possible alarms)
 *
 * @note satmap->cfgdata should contain a valid GKeyFile.
 *
 */
static void load_map_file(GtkSatMap * satmap, float clon)
{
    gchar          *buff;
    gchar          *mapfile;
    GError         *error = NULL;
    GdkPixbuf      *tmpbuf;

    /* get local, global or default map file */
    buff = mod_cfg_get_str(satmap->cfgdata,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_FILE, SAT_CFG_STR_MAP_FILE);

    if (g_path_is_absolute(buff))
    {
        /* map is user specific, ie. in $HOME/.gpredict2/maps/ */
        mapfile = g_strdup(buff);
    }
    else
    {
        /* build complete path */
        mapfile = map_file_name(buff);
    }
    g_free(buff);

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%d: Loading map file %s"), __FILE__, __LINE__, mapfile);

    /* check that file exists, if not get the default */
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

        /* get default map file */
        g_free(mapfile);
        mapfile = sat_cfg_get_str_def(SAT_CFG_STR_MAP_FILE);

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Using default map: %s"),
                    __FILE__, __LINE__, mapfile);
    }

    /* try to load the map file */
    tmpbuf = gdk_pixbuf_new_from_file(mapfile, &error);

    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Error loading map file (%s)"),
                    __FILE__, __LINE__, error->message);
        g_clear_error(&error);

        /* create a dummy GdkPixbuf to avoid crash */
        tmpbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 400, 200);
        gdk_pixbuf_fill(tmpbuf, 0x0F0F0F0F);
    }

    /* create a blank map with same parameters as tmpbuf */
    satmap->origmap = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                     FALSE,
                                     gdk_pixbuf_get_bits_per_sample(tmpbuf),
                                     gdk_pixbuf_get_width(tmpbuf),
                                     gdk_pixbuf_get_height(tmpbuf));

    map_tools_shift_center(tmpbuf, satmap->origmap, clon);
    g_object_unref(tmpbuf);
    g_free(mapfile);

    /* Calculate longitude at the left side (-180 deg if center is at 0 deg longitude) */
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

/* Check whether the footprint covers the North or South pole. */
static gboolean pole_is_covered(sat_t * sat)
{
    if (north_pole_is_covered(sat) || south_pole_is_covered(sat))
        return TRUE;
    else
        return FALSE;
}

/* Check whether the footprint covers the North pole. */
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

/* Check whether the footprint covers the South pole. */
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

/* Mirror the footprint longitude. */
static gboolean mirror_lon(sat_t * sat, gdouble rangelon, gdouble * mlon,
                           gdouble mapbreak)
{
    gdouble         diff;
    gboolean        warped = FALSE;

    /* make it so rangelon is on left of ssplon */
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
    //printf("Something %s %f %f %f\n",sat->nickname, sat->ssplon, rangelon,mapbreak);
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
            //printf ("sat %s warped for first \n",sat->nickname);
        }
    }
    else
    {
        if (((*mlon >= mapbreak) && (*mlon < mapbreak + 180)) ||
            ((*mlon < mapbreak - 180) && (*mlon >= mapbreak - 360)))
        {
            warped = TRUE;
            //printf ("sat %s warped for second \n",sat->nickname);
        }
    }

    return warped;
}

/**
 * Calculate satellite footprint and coverage area.
 *
 * @param satmap TheGtkSatMap widget.
 * @param sat The satellite.
 * @param points1 Initialised GooCanvasPoints structure with 360 points.
 * @param points2 Initialised GooCanvasPoints structure with 360 points.
 * @return The number of range circle parts.
 *
 * This function calculates the "left" side of the range circle and mirrors
 * the points in longitude to create the "right side of the range circle, too.
 * In order to be able to use the footprint points to create a set of subsequent
 * lines connected to each other (poly-lines) the function may have to perform
 * one of the following three actions:
 *
 * 1. If the footprint covers the North or South pole, we need to sort the points
 *    and add two extra points: One to begin the range circle (e.g. -180,90) and
 *    one to end the range circle (e.g. 180,90). This is necessary to create a
 *    complete and consistent set of points suitable for a polyline. The addition
 *    of the extra points is done by the sort_points function.
 *
 * 2. Else if parts of the range circle is on one side of the map, while parts of
 *    it is on the right side of the map, i.e. the range circle runs off the border
 *    of the map, it calls the split_points function to split the points into two
 *    complete and consistent sets of points that are suitable to create two 
 *    poly-lines.
 *
 * 3. Else nothing needs to be done since the points are already suitable for
 *    a polyline.
 *
 * The function will re-initialise points1 and points2 according to its needs. The
 * total number of points will always be 360, even with the addition of the two
 * extra points. 
 */
static guint calculate_footprint(GtkSatMap * satmap, sat_t * sat)
{
    guint           azi;
    gfloat          sx, sy, msx, msy, ssx, ssy;
    gdouble         ssplat, ssplon, beta, azimuth, num, dem;
    gdouble         rangelon, rangelat, mlon;
    gboolean        warped = FALSE;
    guint           numrc = 1;

    /* Range circle calculations.
     * Borrowed from gsat 0.9.0 by Xavier Crehueras, EB3CZS
     * who borrowed from John Magliacane, KD2BD.
     * Optimized by Alexandru Csete and William J Beksi.
     */
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

        /* mirror longitude */
        if (mirror_lon(sat, rangelon, &mlon, satmap->left_side_lon))
            warped = TRUE;

        lonlat_to_xy(satmap, rangelon, rangelat, &sx, &sy);
        lonlat_to_xy(satmap, mlon, rangelat, &msx, &msy);

        points1->coords[2 * azi] = sx;
        points1->coords[2 * azi + 1] = sy;

        /* Add mirrored point */
        points1->coords[718 - 2 * azi] = msx;
        points1->coords[719 - 2 * azi] = msy;
    }

    /* points1 now contains 360 pairs of map-based XY coordinates.
       Check whether actions 1, 2 or 3 have to be performed.
     */

    /* pole is covered => sort points1 and add additional points */
    if (pole_is_covered(sat))
    {

        sort_points_x(satmap, sat, points1, 360);
        numrc = 1;
    }

    /* pole not covered but range circle has been warped
       => split points */
    else if (warped == TRUE)
    {

        lonlat_to_xy(satmap, sat->ssplon, sat->ssplat, &ssx, &ssy);
        split_points(satmap, sat, ssx);
        numrc = 2;

    }
    else
    {
        /* the nominal condition => points1 is adequate */
        numrc = 1;
    }

    return numrc;
}

/**
 * Split and sort polyline points.
 *
 * @param satmap The GtkSatMap structure.
 * @param points1 GooCanvasPoints containing the footprint points.
 * @param points2 A GooCanvasPoints structure containing the second set of points.
 * @param sspx Canvas based x-coordinate of SSP.
 * @bug We should ensure that the endpoints in points1 have x=x0, while in
 *      the endpoints in points2 should have x=x0+width (TBC).
 *
 * @note This function works on canvas-based coordinates rather than lat/lon
 * @note DO NOT USE this function when the footprint covers one of the poles
 *       (the end result may freeze the X-server requiring a hard-reset!)
 */
static void split_points(GtkSatMap * satmap, sat_t * sat, gdouble sspx)
{
    GooCanvasPoints *tps1, *tps2;
    gint            n, n1, n2, ns, i, j, k;

    /* initialize parameters */
    n = points1->num_points;
    n1 = 0;
    n2 = 0;
    i = 0;
    j = 0;
    k = 0;
    ns = 0;
    tps1 = goo_canvas_points_new(n);
    tps2 = goo_canvas_points_new(n);

    //if ((sspx >= (satmap->x0 + satmap->width - 0.6)) ||
    //    (sspx >= (satmap->x0 - 0.6))) {
    //if ((sspx == (satmap->x0 + satmap->width)) ||
    //    (sspx == (satmap->x0))) {
    if ((sat->ssplon >= 179.4) || (sat->ssplon <= -179.4))
    {
        /* sslon = +/-180 deg.
           - copy points with (x > satmap->x0+satmap->width/2) to tps1
           - copy points with (x < satmap->x0+satmap->width/2) to tps2
           - sort tps1 and tps2
         */
        for (i = 0; i < n; i++)
        {
            if (points1->coords[2 * i] > (satmap->x0 + satmap->width / 2))
            {
                tps1->coords[2 * n1] = points1->coords[2 * i];
                tps1->coords[2 * n1 + 1] = points1->coords[2 * i + 1];
                n1++;
            }
            else
            {
                tps2->coords[2 * n2] = points1->coords[2 * i];
                tps2->coords[2 * n2 + 1] = points1->coords[2 * i + 1];
                n2++;
            }
        }

        sort_points_y(satmap, sat, tps1, n1);
        sort_points_y(satmap, sat, tps2, n2);
    }
    else if (sspx < (satmap->x0 + satmap->width / 2))
    {
        /* We are on the left side of the map.
           Scan through points1 until we get to x > sspx (i=ns):

           - copy the points forwards until x < (x0+w/2) => tps2
           - continue to copy until the end => tps1
           - copy the points from i=0 to i=ns => tps1.

           Copy tps1 => points1 and tps2 => points2
         */
        while (points1->coords[2 * i] <= sspx)
        {
            i++;
        }
        ns = i - 1;

        while (points1->coords[2 * i] > (satmap->x0 + satmap->width / 2))
        {
            tps2->coords[2 * j] = points1->coords[2 * i];
            tps2->coords[2 * j + 1] = points1->coords[2 * i + 1];
            i++;
            j++;
            n2++;
        }

        while (i < n)
        {
            tps1->coords[2 * k] = points1->coords[2 * i];
            tps1->coords[2 * k + 1] = points1->coords[2 * i + 1];
            i++;
            k++;
            n1++;
        }

        for (i = 0; i <= ns; i++)
        {
            tps1->coords[2 * k] = points1->coords[2 * i];
            tps1->coords[2 * k + 1] = points1->coords[2 * i + 1];
            k++;
            n1++;
        }
    }
    else
    {
        /* We are on the right side of the map.
           Scan backwards through points1 until x < sspx (i=ns):

           - copy the points i=ns,i-- until x >= x0+w/2  => tps2
           - copy the points until we reach i=0          => tps1
           - copy the points from i=n to i=ns            => tps1

         */
        i = n - 1;
        while (points1->coords[2 * i] >= sspx)
        {
            i--;
        }
        ns = i + 1;

        while (points1->coords[2 * i] < (satmap->x0 + satmap->width / 2))
        {
            tps2->coords[2 * j] = points1->coords[2 * i];
            tps2->coords[2 * j + 1] = points1->coords[2 * i + 1];
            i--;
            j++;
            n2++;
        }

        while (i >= 0)
        {
            tps1->coords[2 * k] = points1->coords[2 * i];
            tps1->coords[2 * k + 1] = points1->coords[2 * i + 1];
            i--;
            k++;
            n1++;
        }

        for (i = n - 1; i >= ns; i--)
        {
            tps1->coords[2 * k] = points1->coords[2 * i];
            tps1->coords[2 * k + 1] = points1->coords[2 * i + 1];
            k++;
            n1++;
        }
    }

    //g_print ("NS:%d  N1:%d  N2:%d\n", ns, n1, n2);

    /* free points and copy new contents */
    goo_canvas_points_unref(points1);
    goo_canvas_points_unref(points2);

    points1 = goo_canvas_points_new(n1);
    for (i = 0; i < n1; i++)
    {
        points1->coords[2 * i] = tps1->coords[2 * i];
        points1->coords[2 * i + 1] = tps1->coords[2 * i + 1];
    }
    goo_canvas_points_unref(tps1);

    points2 = goo_canvas_points_new(n2);
    for (i = 0; i < n2; i++)
    {
        points2->coords[2 * i] = tps2->coords[2 * i];
        points2->coords[2 * i + 1] = tps2->coords[2 * i + 1];
    }
    goo_canvas_points_unref(tps2);

    /* stretch end points to map borders */
    if (points1->coords[0] > (satmap->x0 + satmap->width / 2))
    {
        points1->coords[0] = satmap->x0 + satmap->width;
        points1->coords[2 * (n1 - 1)] = satmap->x0 + satmap->width;
        points2->coords[0] = satmap->x0;
        points2->coords[2 * (n2 - 1)] = satmap->x0;
    }
    else
    {
        points2->coords[0] = satmap->x0 + satmap->width;
        points2->coords[2 * (n2 - 1)] = satmap->x0 + satmap->width;
        points1->coords[0] = satmap->x0;
        points1->coords[2 * (n1 - 1)] = satmap->x0;
    }
}

/**
 * Sort points according to X coordinates.
 *
 * @param satmap The GtkSatMap structure.
 * @param sat The satellite data structure.
 * @param points The points to sort.
 * @param num The number of points. By specifying it as parameter we can
 *            sort incomplete arrays.
 *
 * This function sorts the points in ascending order with respect
 * to their x value. After sorting the function adds two extra points
 * to the array using the following algorithms:
 *
 *   move point at position 0 to position 1
 *   move point at position N to position N-1
 *   if (ssplat > 0)
 *         insert (x0,y0) into position 0
 *         insert (x0+width,y0) into position N
 *   else
 *         insert (x0,y0+height) into position 0
 *         insert (x0+width,y0+height) into position N
 *
 * This way we loose the points at position 1 and N-1, but that does not
 * make any big difference anyway, since we have 360 points in total.
 *
 */
static void sort_points_x(GtkSatMap * satmap, sat_t * sat,
                          GooCanvasPoints * points, gint num)
{
    gsize           size = 2 * sizeof(double);

    /* call g_qsort_with_data, which warps the qsort function
       from stdlib */
    g_qsort_with_data(points->coords, num, size, compare_coordinates_x, NULL);

    /* move point at position 0 to position 1 */
    points->coords[2] = satmap->x0;
    points->coords[3] = points->coords[1];

    /* move point at position N to position N-1 */
    points->coords[716] = satmap->x0 + satmap->width;   //points->coords[718];
    points->coords[717] = points->coords[719];

    if (sat->ssplat > 0.0)
    {
        /* insert (x0-1,y0) into position 0 */
        points->coords[0] = satmap->x0;
        points->coords[1] = satmap->y0;

        /* insert (x0+width,y0) into position N */
        points->coords[718] = satmap->x0 + satmap->width;
        points->coords[719] = satmap->y0;
    }
    else
    {
        /* insert (x0,y0+height) into position 0 */
        points->coords[0] = satmap->x0;
        points->coords[1] = satmap->y0 + satmap->height;

        /* insert (x0+width,y0+height) into position N */
        points->coords[718] = satmap->x0 + satmap->width;
        points->coords[719] = satmap->y0 + satmap->height;
    }
}

/**
 * Sort points according to Y coordinates.
 *
 * @param satmap The GtkSatMap structure.
 * @param sat The satellite data structure.
 * @param points The points to sort.
 * @param num The number of points. By specifying it as parameter we can
 *            sort incomplete arrays.
 *
 * This function sorts the points in ascending order with respect
 * to their y value.
 */
static void sort_points_y(GtkSatMap * satmap, sat_t * sat,
                          GooCanvasPoints * points, gint num)
{
    gsize           size;

    (void)satmap;
    (void)sat;
    size = 2 * sizeof(double);

    /* call g_qsort_with_data, which warps the qsort function
       from stdlib */
    g_qsort_with_data(points->coords, num, size, compare_coordinates_y, NULL);
}

/**
 * Compare two X coordinates.
 *
 * @param a Pointer to one coordinate (x,y) both double.
 * @param b Pointer to the second coordinate (x,y) both double.
 * @param data User data; always NULL.
 * @return Negative value if a < b; zero if a = b; positive value if a > b. 
 *
 * This function is used by the g_qsort_with_data function to compare two
 * elements in the coordinate array of the GooCanvasPoints structure. We have
 * to remember that GooCanvasPoints->coords is an array of XY values, i.e.
 * [X0,Y0,X1,Y1,...,Xn,Yn], but we only want to sort according to the X
 * coordinate. Therefore, we let the g_qsort_with_data believe that the whole
 * pair is one element (which is OK even according to the API docs) but we
 * cast the pointers to an array with two elements and compare only the first
 * one (x).
 *
 */
static gint compare_coordinates_x(gconstpointer a, gconstpointer b,
                                  gpointer data)
{
    double         *ea = (double *)a;
    double         *eb = (double *)b;

    (void)data;

    if (ea[0] < eb[0])
    {
        return -1;
    }
    else if (ea[0] > eb[0])
    {
        return 1;
    }

    return 0;
}

/**
 * Compare two Y coordinates.
 *
 * @param a Pointer to one coordinate (x,y) both double.
 * @param b Pointer to the second coordinate (x,y) both double.
 * @param data User data; always NULL.
 * @return Negative value if a < b; zero if a = b; positive value if a > b. 
 *
 * This function is used by the g_qsort_with_data function to compare two
 * elements in the coordinate array of the GooCanvasPoints structure. We have
 * to remember that GooCanvasPoints->coords is an array of XY values, i.e.
 * [X0,Y0,X1,Y1,...,Xn,Yn], but we only want to sort according to the Y
 * coordinate. Therefore, we let the g_qsort_with_data believe that the whole
 * pair is one element (which is OK even according to the API docs) but we
 * cast the pointers to an array with two elements and compare only the second
 * one (y).
 *
 */
static gint compare_coordinates_y(gconstpointer a, gconstpointer b,
                                  gpointer data)
{
    double         *ea = (double *)a;
    double         *eb = (double *)b;

    (void)data;

    if (ea[1] < eb[1])
    {
        return -1;
    }

    else if (ea[1] > eb[1])
    {
        return 1;
    }

    return 0;
}

/**
 * Plot a satellite.
 *
 * @param key The hash table key.
 * @param value Pointer to the satellite.
 * @param data Pointer to the GtkSatMap widget.
 *
 * This function creates and initializes the canvas objects (rectangle, label,
 * footprint) for a satellite. The function is called as a g_hash_table_foreach
 * callback.
 */
static void plot_sat(gpointer key, gpointer value, gpointer data)
{
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    sat_map_obj_t  *obj = NULL;
    sat_t          *sat = SAT(value);
    GooCanvasItemModel *root;
    gint           *catnum;
    guint32         col, covcol, shadowcol;
    gfloat          x, y;
    gchar          *tooltip;

    (void)key;

    if (decayed(sat))
    {
        return;
    }
    /* get satellite and SSP */
    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;

    lonlat_to_xy(satmap, sat->ssplon, sat->ssplat, &x, &y);

    /* create and initialize a sat object */
    obj = g_try_new(sat_map_obj_t, 1);

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
    obj->range2 = NULL;
    obj->catnum = sat->tle.catnr;
    obj->track_data.latlon = NULL;
    obj->track_data.lines = NULL;
    obj->track_orbit = 0;

    root = goo_canvas_get_root_item_model(GOO_CANVAS(satmap->canvas));

    /* satellite color */
    col = mod_cfg_get_int(satmap->cfgdata,
                          MOD_CFG_MAP_SECTION,
                          MOD_CFG_MAP_SAT_COL, SAT_CFG_INT_MAP_SAT_COL);

    /* area coverage colour */
    covcol = mod_cfg_get_int(satmap->cfgdata,
                             MOD_CFG_MAP_SECTION,
                             MOD_CFG_MAP_SAT_COV_COL,
                             SAT_CFG_INT_MAP_SAT_COV_COL);
    /* coverage color */
    if (obj->showcov)
    {
        covcol = mod_cfg_get_int(satmap->cfgdata,
                                 MOD_CFG_MAP_SECTION,
                                 MOD_CFG_MAP_SAT_COV_COL,
                                 SAT_CFG_INT_MAP_SAT_COV_COL);
    }
    else
    {
        covcol = 0x00000000;
    }


    /* shadow colour (only alpha channel) */
    shadowcol = mod_cfg_get_int(satmap->cfgdata,
                                MOD_CFG_MAP_SECTION,
                                MOD_CFG_MAP_SHADOW_ALPHA,
                                SAT_CFG_INT_MAP_SHADOW_ALPHA);

    /* create tooltip */
    tooltip = g_markup_printf_escaped("<b>%s</b>\n"
                                      "Lon: %5.1f\302\260\n"
                                      "Lat: %5.1f\302\260\n"
                                      " Az: %5.1f\302\260\n"
                                      " El: %5.1f\302\260",
                                      sat->nickname,
                                      sat->ssplon, sat->ssplat,
                                      sat->az, sat->el);

    /* create satellite marker and label + shadows. We create shadows first */
    obj->shadowm = goo_canvas_rect_model_new(root,
                                             x - MARKER_SIZE_HALF + 1,
                                             y - MARKER_SIZE_HALF + 1,
                                             2 * MARKER_SIZE_HALF,
                                             2 * MARKER_SIZE_HALF,
                                             "fill-color-rgba", 0x00,
                                             "stroke-color-rgba", shadowcol,
                                             NULL);
    obj->marker = goo_canvas_rect_model_new(root,
                                            x - MARKER_SIZE_HALF,
                                            y - MARKER_SIZE_HALF,
                                            2 * MARKER_SIZE_HALF,
                                            2 * MARKER_SIZE_HALF,
                                            "fill-color-rgba", col,
                                            "stroke-color-rgba", col,
                                            "tooltip", tooltip, NULL);

    obj->shadowl = goo_canvas_text_model_new(root, sat->nickname,
                                             x + 1,
                                             y + 3,
                                             -1,
                                             GOO_CANVAS_ANCHOR_NORTH,
                                             "font", "Sans 8",
                                             "fill-color-rgba", shadowcol,
                                             NULL);
    obj->label = goo_canvas_text_model_new(root, sat->nickname,
                                           x,
                                           y + 2,
                                           -1,
                                           GOO_CANVAS_ANCHOR_NORTH,
                                           "font", "Sans 8",
                                           "fill-color-rgba", col,
                                           "tooltip", tooltip, NULL);

    g_free(tooltip);

    g_object_set_data(G_OBJECT(obj->marker), "catnum",
                      GINT_TO_POINTER(*catnum));
    g_object_set_data(G_OBJECT(obj->label), "catnum",
                      GINT_TO_POINTER(*catnum));

    /* initialize points for footprint */
    points1 = goo_canvas_points_new(360);
    points2 = goo_canvas_points_new(360);

    /* calculate footprint */
    obj->newrcnum = calculate_footprint(satmap, sat);
    obj->oldrcnum = obj->newrcnum;

    /* invisible footprint for decayed sats (STS fix) */
    /*     if (sat->otype == ORBIT_TYPE_DECAYED) { */
    /*         col = 0x00000000; */
    /*     } */

    /* always create first part of range circle */
    obj->range1 = goo_canvas_polyline_model_new(root, FALSE, 0,
                                                "points", points1,
                                                "line-width", 1.0,
                                                "fill-color-rgba", covcol,
                                                "stroke-color-rgba", col,
                                                "line-cap",
                                                CAIRO_LINE_CAP_SQUARE,
                                                "line-join",
                                                CAIRO_LINE_JOIN_MITER, NULL);
    g_object_set_data(G_OBJECT(obj->range1), "catnum",
                      GINT_TO_POINTER(*catnum));

    /* create second part if available */
    if (obj->newrcnum == 2)
    {
        //g_print ("N1:%d  N2:%d\n", points1->num_points, points2->num_points);

        obj->range2 = goo_canvas_polyline_model_new(root, FALSE, 0,
                                                    "points", points2,
                                                    "line-width", 1.0,
                                                    "fill-color-rgba", covcol,
                                                    "stroke-color-rgba", col,
                                                    "line-cap",
                                                    CAIRO_LINE_CAP_SQUARE,
                                                    "line-join",
                                                    CAIRO_LINE_JOIN_MITER,
                                                    NULL);
        g_object_set_data(G_OBJECT(obj->range2), "catnum",
                          GINT_TO_POINTER(*catnum));

    }

    goo_canvas_points_unref(points1);
    goo_canvas_points_unref(points2);

    /* add sat to hash table */
    g_hash_table_insert(satmap->obj, catnum, obj);
}

/**
 * Free a satellite object.
 *
 * @param key The hash table key.
 * @param value Pointer to the satellite.
 * @param data Pointer to the GtkSatMap widget.
 *
 * This function removes the canvas objects allocated by `plot_sat`. It needs
 * access the the GtkSatMap widget to remove the elements from the canvas and
 * to pass on to `ground_track_delete`. The function must be called as a
 * g_hash_table_foreach callback in order to pass in the GtkSatMap.
 */
static void free_sat_obj(gpointer key, gpointer value, gpointer data)
{
    sat_map_obj_t      *obj = SAT_MAP_OBJ(value);
    sat_t              *sat = NULL;
    GtkSatMap          *satmap = GTK_SAT_MAP(data);
    GooCanvasItemModel *root;
    gint                idx;

    (void)key;

    root = goo_canvas_get_root_item_model(GOO_CANVAS(satmap->canvas));

    idx = goo_canvas_item_model_find_child(root, obj->marker);
    if (idx != -1)
        goo_canvas_item_model_remove_child(root, idx);;
    obj->marker = NULL;

    idx = goo_canvas_item_model_find_child(root, obj->shadowm);
    if (idx != -1)
        goo_canvas_item_model_remove_child(root, idx);;
    obj->shadowm = NULL;

    idx = goo_canvas_item_model_find_child(root, obj->label);
    if (idx != -1)
        goo_canvas_item_model_remove_child(root, idx);
    obj->label = NULL;

    idx = goo_canvas_item_model_find_child(root, obj->shadowl);
    if (idx != -1)
        goo_canvas_item_model_remove_child(root, idx);
    obj->shadowl = NULL;

    idx = goo_canvas_item_model_find_child(root, obj->range1);
    if (idx != -1)
        goo_canvas_item_model_remove_child(root, idx);
    obj->range1 = NULL;

    idx = goo_canvas_item_model_find_child(root, obj->range2);
    if (idx != -1)
        goo_canvas_item_model_remove_child(root, idx);
    obj->range2 = NULL;

    if (obj->showtrack)
    {
        sat = SAT(g_hash_table_lookup(satmap->sats, &obj->catnum));
        ground_track_delete(satmap, sat, satmap->qth, obj, TRUE);
    }
}

/** Update a given satellite. */
static void update_sat(gpointer key, gpointer value, gpointer data)
{
    gint           *catnum;
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    sat_map_obj_t  *obj = NULL;
    sat_t          *sat = SAT(value);
    gfloat          x, y;
    gdouble         oldx, oldy;
    gdouble         now;        // = get_current_daynum ();
    GooCanvasItemModel *root;
    gint            idx;
    guint32         col, covcol;
    gchar          *tooltip;
    gchar          *aosstr;

    //gdouble sspla,ssplo;

    root = goo_canvas_get_root_item_model(GOO_CANVAS(satmap->canvas));

    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;

    now = satmap->tstamp;

    /* update next AOS */
    if (sat->aos > now)
    {
        if ((sat->aos < satmap->naos) || (satmap->naos == 0.0))
        {
            satmap->naos = sat->aos;
            satmap->ncat = sat->tle.catnr;
        }
    }

    obj = SAT_MAP_OBJ(g_hash_table_lookup(satmap->obj, catnum));

    /* get rid of a decayed satellite */
    if (decayed(sat) && obj != NULL)
    {
        free_sat_obj(NULL, obj, satmap);
        g_hash_table_remove(satmap->obj, catnum);
        return;
    }

    if (obj == NULL)
    {
        if (decayed(sat))
        {
            return;
        }
        else
        {
            /* satellite was decayed now is visible
               time controller backed up time */
            plot_sat(key, value, data);
            return;
        }
    }

    if (obj->selected)
    {
        /* update satmap->sel */
        update_selected(satmap, sat);
    }

    g_object_set(obj->label, "text", sat->nickname, NULL);
    g_object_set(obj->shadowl, "text", sat->nickname, NULL);

    aosstr = aoslos_time_to_str(satmap, sat);
    tooltip = g_markup_printf_escaped("<b>%s</b>\n"
                                      "Lon: %5.1f\302\260\n"
                                      "Lat: %5.1f\302\260\n"
                                      " Az: %5.1f\302\260\n"
                                      " El: %5.1f\302\260\n"
                                      "%s",
                                      sat->nickname,
                                      sat->ssplon, sat->ssplat,
                                      sat->az, sat->el, aosstr);
    g_object_set(obj->marker, "tooltip", tooltip, NULL);
    g_object_set(obj->label, "tooltip", tooltip, NULL);
    g_free(tooltip);
    g_free(aosstr);

    lonlat_to_xy(satmap, sat->ssplon, sat->ssplat, &x, &y);

    /* update only if satellite has moved at least
       2 * MARKER_SIZE_HALF (no need to drain CPU all the time)
     */
    g_object_get(obj->marker, "x", &oldx, "y", &oldy, NULL);

    if ((fabs(oldx - x) >= 2 * MARKER_SIZE_HALF) ||
        (fabs(oldy - y) >= 2 * MARKER_SIZE_HALF))
    {
        g_object_set(obj->marker,
                     "x", (gdouble) (x - MARKER_SIZE_HALF),
                     "y", (gdouble) (y - MARKER_SIZE_HALF), NULL);
        g_object_set(obj->shadowm,
                     "x", (gdouble) (x - MARKER_SIZE_HALF + 1),
                     "y", (gdouble) (y - MARKER_SIZE_HALF + 1), NULL);

        if (x < 50)
        {
            g_object_set(obj->label,
                         "x", (gdouble) (x + 3),
                         "y", (gdouble) (y), "anchor", GOO_CANVAS_ANCHOR_WEST,
                         NULL);
            g_object_set(obj->shadowl, "x", (gdouble) (x + 3 + 1), "y",
                         (gdouble) (y + 1), "anchor", GOO_CANVAS_ANCHOR_WEST,
                         NULL);
        }
        else if ((satmap->width - x) < 50)
        {
            g_object_set(obj->label,
                         "x", (gdouble) (x - 3),
                         "y", (gdouble) (y), "anchor", GOO_CANVAS_ANCHOR_EAST,
                         NULL);
            g_object_set(obj->shadowl, "x", (gdouble) (x - 3 + 1), "y",
                         (gdouble) (y + 1), "anchor", GOO_CANVAS_ANCHOR_EAST,
                         NULL);
        }
        else if ((satmap->height - y) < 25)
        {
            g_object_set(obj->label,
                         "x", (gdouble) (x),
                         "y", (gdouble) (y - 2),
                         "anchor", GOO_CANVAS_ANCHOR_SOUTH, NULL);
            g_object_set(obj->shadowl,
                         "x", (gdouble) (x + 1),
                         "y", (gdouble) (y - 2 + 1),
                         "anchor", GOO_CANVAS_ANCHOR_SOUTH, NULL);
        }
        else
        {
            g_object_set(obj->label,
                         "x", (gdouble) (x),
                         "y", (gdouble) (y + 2),
                         "anchor", GOO_CANVAS_ANCHOR_NORTH, NULL);
            g_object_set(obj->shadowl,
                         "x", (gdouble) (x + 1),
                         "y", (gdouble) (y + 2 + 1),
                         "anchor", GOO_CANVAS_ANCHOR_NORTH, NULL);
        }

        /* initialize points for footprint */
        points1 = goo_canvas_points_new(360);
        points2 = goo_canvas_points_new(360);

        /* calculate footprint */
        obj->newrcnum = calculate_footprint(satmap, sat);

        /* always update first part */
        g_object_set(obj->range1, "points", points1, NULL);

        if (obj->newrcnum == 2)
        {
            if (obj->oldrcnum == 1)
            {
                /* we need to create the second part */
                if (obj->selected)
                {
                    col = mod_cfg_get_int(satmap->cfgdata,
                                          MOD_CFG_MAP_SECTION,
                                          MOD_CFG_MAP_SAT_SEL_COL,
                                          SAT_CFG_INT_MAP_SAT_SEL_COL);
                }
                else
                {
                    col = mod_cfg_get_int(satmap->cfgdata,
                                          MOD_CFG_MAP_SECTION,
                                          MOD_CFG_MAP_SAT_COL,
                                          SAT_CFG_INT_MAP_SAT_COL);
                }
                /* coverage color */
                if (obj->showcov)
                {
                    covcol = mod_cfg_get_int(satmap->cfgdata,
                                             MOD_CFG_MAP_SECTION,
                                             MOD_CFG_MAP_SAT_COV_COL,
                                             SAT_CFG_INT_MAP_SAT_COV_COL);
                }
                else
                {
                    covcol = 0x00000000;
                }
                obj->range2 = goo_canvas_polyline_model_new(root, FALSE, 0,
                                                            "points", points2,
                                                            "line-width", 1.0,
                                                            "fill-color-rgba",
                                                            covcol,
                                                            "stroke-color-rgba",
                                                            col, "line-cap",
                                                            CAIRO_LINE_CAP_SQUARE,
                                                            "line-join",
                                                            CAIRO_LINE_JOIN_MITER,
                                                            NULL);
                g_object_set_data(G_OBJECT(obj->range2), "catnum",
                                  GINT_TO_POINTER(*catnum));
            }
            else
            {
                /* just update the second part */
                g_object_set(obj->range2, "points", points2, NULL);
            }
        }
        else
        {
            if (obj->oldrcnum == 2)
            {
                /* remove second part */
                idx = goo_canvas_item_model_find_child(root, obj->range2);
                if (idx != -1)
                {
                    goo_canvas_item_model_remove_child(root, idx);
                }
            }
        }

        /* update rc-number */
        obj->oldrcnum = obj->newrcnum;

        goo_canvas_points_unref(points1);
        goo_canvas_points_unref(points2);
    }

    /* if ground track is visible check whether we have passed into a
       new orbit, in which case we need to recalculate the ground track
     */
    if (obj->showtrack)
    {
        if (obj->track_orbit != sat->orbit)
        {
            ground_track_update(satmap, sat, satmap->qth, obj, TRUE);
        }
        /* otherwise we may be in a map rescale process */
        else if (satmap->resize)
        {
            ground_track_update(satmap, sat, satmap->qth, obj, FALSE);
        }
    }

    g_free(catnum);
}

/**
 * Update information about the selected satellite.
 *
 * @param satmap Pointer to the GtkSatMap widget.
 * @param sat Pointer to the selected satellite
 */
static void update_selected(GtkSatMap * satmap, sat_t * sat)
{
    guint           h, m, s;
    gchar          *ch, *cm, *cs;
    gchar          *alsstr, *text;
    gdouble         number, now;
    gboolean        isgeo = FALSE;      /* set to TRUE if satellite appears to be GEO */

    now = satmap->tstamp;       //get_current_daynum ();

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

    /* if satellite appears to be GEO don't attempt to show AOS/LOS */
    if (isgeo)
    {
        if (sat->el > 0.0)
        {
            text =
                g_markup_printf_escaped
                ("<span background=\"#%s\"> %s: Always in range </span>",
                 satmap->infobgd, sat->nickname);
        }
        else
        {
            text =
                g_markup_printf_escaped
                ("<span background=\"#%s\"> %s: Always out of range </span>",
                 satmap->infobgd, sat->nickname);
        }
    }
    else
    {
        /* convert julian date to seconds */
        s = (guint) (number * 86400);

        /* extract hours */
        h = (guint) floor(s / 3600);
        s -= 3600 * h;

        /* leading zero */
        if ((h > 0) && (h < 10))
            ch = g_strdup("0");
        else
            ch = g_strdup("");

        /* extract minutes */
        m = (guint) floor(s / 60);
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

    /* update info text */
    g_object_set(satmap->sel, "text", text, NULL);
    g_free(text);
}

static void draw_grid_lines(GtkSatMap * satmap, GooCanvasItemModel * root)
{
    gdouble         xstep, ystep;
    gfloat          lon, lat;
    guint32         col;
    guint           i;
    gchar          *buf, hmf = ' ';

    /* initialize algo parameters */
    col = mod_cfg_get_int(satmap->cfgdata,
                          MOD_CFG_MAP_SECTION,
                          MOD_CFG_MAP_GRID_COL, SAT_CFG_INT_MAP_GRID_COL);

    xstep = (gdouble) (30.0 * satmap->width / 360.0);
    ystep = (gdouble) (30.0 * satmap->height / 180.0);

#define MKLINE goo_canvas_polyline_model_new_line

    /* horizontal grid */
    for (i = 0; i < 5; i++)
    {
        /* line */
        satmap->gridh[i] = MKLINE(root,
                                  (gdouble) satmap->x0,
                                  (gdouble) (satmap->y0 + (i + 1) * ystep),
                                  (gdouble) (satmap->x0 + satmap->width),
                                  (gdouble) (satmap->y0 + (i + 1) * ystep),
                                  "stroke-color-rgba", col,
                                  "line-cap", CAIRO_LINE_CAP_SQUARE,
                                  "line-join", CAIRO_LINE_JOIN_MITER,
                                  "line-width", 0.5, NULL);

        /* FIXME: Use dotted line pattern? */

        /* label */
        xy_to_lonlat(satmap, satmap->x0, satmap->y0 + (i + 1) * ystep, &lon,
                     &lat);
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
        satmap->gridhlab[i] = goo_canvas_text_model_new(root,
                                                        buf,
                                                        (gdouble) (satmap->x0 +
                                                                   15),
                                                        (gdouble) (satmap->y0 +
                                                                   (i +
                                                                    1) *
                                                                   ystep), -1,
                                                        GOO_CANVAS_ANCHOR_NORTH,
                                                        "font", "Sans 8",
                                                        "fill-color-rgba", col,
                                                        NULL);
        g_free(buf);

        /* lower items to be just above the background map
           or below it if lines are invisible
         */
        if (satmap->showgrid)
        {
            goo_canvas_item_model_raise(satmap->gridh[i], satmap->map);
            goo_canvas_item_model_raise(satmap->gridhlab[i], satmap->map);
        }
        else
        {
            goo_canvas_item_model_lower(satmap->gridh[i], satmap->map);
            goo_canvas_item_model_lower(satmap->gridhlab[i], satmap->map);
        }
    }

    /* vertical grid */
    for (i = 0; i < 11; i++)
    {
        /* line */
        satmap->gridv[i] = MKLINE(root,
                                  (gdouble) (satmap->x0 + (i + 1) * xstep),
                                  (gdouble) satmap->y0,
                                  (gdouble) (satmap->x0 + (i + 1) * xstep),
                                  (gdouble) (satmap->y0 + satmap->height),
                                  "stroke-color-rgba", col,
                                  "line-cap", CAIRO_LINE_CAP_SQUARE,
                                  "line-join", CAIRO_LINE_JOIN_MITER,
                                  "line-width", 0.5, NULL);

        /* label */
        xy_to_lonlat(satmap, satmap->x0 + (i + 1) * xstep, satmap->y0, &lon,
                     &lat);
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
        satmap->gridvlab[i] = goo_canvas_text_model_new(root,
                                                        buf,
                                                        (gdouble) (satmap->x0 +
                                                                   (i +
                                                                    1) *
                                                                   xstep),
                                                        (gdouble) (satmap->y0 +
                                                                   satmap->height
                                                                   - 5), -1,
                                                        GOO_CANVAS_ANCHOR_EAST,
                                                        "font", "Sans 8",
                                                        "fill-color-rgba", col,
                                                        NULL);
        g_free(buf);

        /* lower items to be just above the background map
           or below it if lines are invisible
         */
        if (satmap->showgrid)
        {
            goo_canvas_item_model_raise(satmap->gridv[i], satmap->map);
            goo_canvas_item_model_raise(satmap->gridvlab[i], satmap->map);
        }
        else
        {
            goo_canvas_item_model_lower(satmap->gridv[i], satmap->map);
            goo_canvas_item_model_lower(satmap->gridvlab[i], satmap->map);
        }
    }
}

static void draw_terminator(GtkSatMap * satmap, GooCanvasItemModel * root)
{
    guint32         terminator_col;
    guint32         globe_shadow_col;

    /* initialize algo parameters */
    terminator_col = mod_cfg_get_int(satmap->cfgdata,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_TERMINATOR_COL,
                                     SAT_CFG_INT_MAP_TERMINATOR_COL);

    globe_shadow_col = mod_cfg_get_int(satmap->cfgdata,
                                       MOD_CFG_MAP_SECTION,
                                       MOD_CFG_MAP_GLOBAL_SHADOW_COL,
                                       SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL);

    /* We do not set any polygon vertices here, but trust that the redraw_terminator
       will be called in due course to do the job. */

    satmap->terminator = goo_canvas_polyline_model_new(root, FALSE, 0,
                                                       "line-width", 1.0,
                                                       "fill-color-rgba",
                                                       globe_shadow_col,
                                                       "stroke-color-rgba",
                                                       terminator_col,
                                                       "line-cap",
                                                       CAIRO_LINE_CAP_SQUARE,
                                                       "line-join",
                                                       CAIRO_LINE_JOIN_MITER,
                                                       NULL);

    goo_canvas_item_model_raise(satmap->terminator, satmap->map);

    satmap->terminator_last_tstamp = satmap->tstamp;
}

static void redraw_grid_lines(GtkSatMap * satmap)
{
    GooCanvasPoints *line;
    gdouble         xstep, ystep;
    guint           i;

    xstep = (gdouble) 30.0 *((gdouble) satmap->width) / 360.0;
    ystep = (gdouble) 30.0 *((gdouble) satmap->height) / 180.0;

    /* horizontal grid */
    for (i = 0; i < 5; i++)
    {

        /* update line */
        line = goo_canvas_points_new(2);
        line->coords[0] = (gdouble) satmap->x0;
        line->coords[1] = (gdouble) (satmap->y0 + (i + 1) * ystep);
        line->coords[2] = (gdouble) (satmap->x0 + satmap->width);
        line->coords[3] = (gdouble) (satmap->y0 + (i + 1) * ystep);
        g_object_set(satmap->gridh[i], "points", line, NULL);
        goo_canvas_points_unref(line);

        /* update label */
        g_object_set(satmap->gridhlab[i],
                     "x", (gdouble) (satmap->x0 + 15),
                     "y", (gdouble) (satmap->y0 + (i + 1) * ystep), NULL);

        /* lower items to be just above the background map
           or below it if lines are invisible
         */
        if (satmap->showgrid)
        {
            goo_canvas_item_model_raise(satmap->gridh[i], satmap->map);
            goo_canvas_item_model_raise(satmap->gridhlab[i], satmap->map);
        }
        else
            goo_canvas_item_model_lower(satmap->gridh[i], satmap->map);
    }

    /* vertical grid */
    for (i = 0; i < 11; i++)
    {
        /* update line */
        line = goo_canvas_points_new(2);
        line->coords[0] = (gdouble) (satmap->x0 + (i + 1) * xstep);
        line->coords[1] = (gdouble) satmap->y0;
        line->coords[2] = (gdouble) (satmap->x0 + (i + 1) * xstep);
        line->coords[3] = (gdouble) (satmap->y0 + satmap->height);
        g_object_set(satmap->gridv[i], "points", line, NULL);
        goo_canvas_points_unref(line);

        /* update label */
        g_object_set(satmap->gridvlab[i],
                     "x", (gdouble) (satmap->x0 + (i + 1) * xstep),
                     "y", (gdouble) (satmap->y0 + satmap->height - 5), NULL);

        /* lower items to be just above the background map
           or below it if lines are invisible
         */
        if (satmap->showgrid)
        {
            goo_canvas_item_model_raise(satmap->gridv[i], satmap->map);
            goo_canvas_item_model_raise(satmap->gridvlab[i], satmap->map);
        }
        else
        {
            goo_canvas_item_model_lower(satmap->gridv[i], satmap->map);
            goo_canvas_item_model_lower(satmap->gridvlab[i], satmap->map);
        }
    }
}

static inline gdouble sgn(gdouble const t)
{
    return t < 0.0 ? -1.0 : 1.0;
}

static void redraw_terminator(GtkSatMap * satmap)
{
    /* Set of (x, y) points along the terminator, one on each line of longitude in
       increments of longitudinal degrees. */
    GooCanvasPoints *line;

    /* A variable which iterates over the longitudes. */
    int             longitude;

    /* Working variables in the computation of the terminator coordinates. */
    gfloat          x, y;

    /* Vector normal to plane containing a line of longitude (z component is
       always zero). */
    /* Note: our coordinates have z along the Earth's axis, x pointing through the
       intersection of the Greenwich Meridian and the Equator, and y right-handedly
       perpendicular to both. */
    gdouble         lx, ly;

    /* The position of the sun as latitude, longitude. */
    geodetic_t      geodetic;

    /* Vector which points from the centre of the Earth to the Sun in inertial
       coordinates. */
    vector_t        sun_;

    /* The same vector in geodesic coordinates. */
    gdouble         sx, sy, sz;

    /* Vector cross-product of (lx,ly,lz) and sun vector. */
    gdouble         rx, ry, rz;

    line = goo_canvas_points_new(363);

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
        /* lz = 0.0; */

        rx = ly * sz /* -lz*sy */ ;
        ry = /* lz*sx */ -lx * sz;
        rz = -lx * sy - ly * sx;

        gdouble         length = sqrt(rx * rx + ry * ry + rz * rz);

        lonlat_to_xy(satmap,
                     centered_longitude, asin(rz / length) * (1.0 / de2ra), &x, &y);

        if( 180 == longitude )
        {
           // make sure the last point is on the right side
           x = satmap->x0 + satmap->width;
        }

        line->coords[2 * (longitude + 181)] = x;
        line->coords[2 * (longitude + 181) + 1] = y;
    }

    line->coords[0] = satmap->x0;
    line->coords[1] = sz < 0.0 ? satmap->y0 : ( satmap->y0 + satmap->height );

    line->coords[724] = satmap->x0 + satmap->width;
    line->coords[725] = sz < 0.0 ? satmap->y0 : ( satmap->y0 + satmap->height );

    g_object_set(satmap->terminator, "points", line, NULL);
    goo_canvas_points_unref(line);
}

void gtk_sat_map_lonlat_to_xy(GtkSatMap * m,
                              gdouble lon, gdouble lat,
                              gdouble * x, gdouble * y)
{
    gfloat          fx, fy;

    fx = (gfloat) * x;
    fy = (gfloat) * y;

    lonlat_to_xy(m, lon, lat, &fx, &fy);

    *x = (gdouble) fx;
    *y = (gdouble) fy;
}

void gtk_sat_map_reload_sats(GtkWidget * satmap, GHashTable * sats)
{
    GTK_SAT_MAP(satmap)->sats = sats;
    GTK_SAT_MAP(satmap)->naos = 0.0;
    GTK_SAT_MAP(satmap)->ncat = 0;

    /* reset ground track orbit to force repaint */
    g_hash_table_foreach(GTK_SAT_MAP(satmap)->obj, reset_ground_track, NULL);
}

static void reset_ground_track(gpointer key, gpointer value,
                               gpointer user_data)
{
    sat_map_obj_t  *obj = (sat_map_obj_t *) value;
    (void) key;
    (void) user_data;

    obj->track_orbit = 0;
}

static gchar   *aoslos_time_to_str(GtkSatMap * satmap, sat_t * sat)
{
    guint           h, m, s;
    gdouble         number, now;
    gchar          *text = NULL;


    now = satmap->tstamp;       //get_current_daynum ();
    if (sat->el > 0.0)
    {
        number = sat->los - now;
    }
    else
    {
        number = sat->aos - now;
    }

    /* convert julian date to seconds */
    s = (guint) (number * 86400);

    /* extract hours */
    h = (guint) floor(s / 3600);
    s -= 3600 * h;

    /* extract minutes */
    m = (guint) floor(s / 60);
    s -= 60 * m;

    if (sat->el > 0.0)
        text = g_strdup_printf(_("LOS in %d minutes"), m + 60 * h);
    else
        text = g_strdup_printf(_("AOS in %d minutes"), m + 60 * h);

    return text;
}

/** Load the satellites that we should show tracks for */
static void gtk_sat_map_load_showtracks(GtkSatMap * satmap)
{
    mod_cfg_get_integer_list_boolean(satmap->cfgdata,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_SHOWTRACKS,
                                     satmap->showtracks);

}

/** Save the satellites that we should not show ground tracks */
static void gtk_sat_map_store_showtracks(GtkSatMap * satmap)
{
    mod_cfg_set_integer_list_boolean(satmap->cfgdata,
                                     satmap->showtracks,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_SHOWTRACKS);
}

/** Save the satellites that we should not highlight coverage */
static void gtk_sat_map_store_hidecovs(GtkSatMap * satmap)
{
    mod_cfg_set_integer_list_boolean(satmap->cfgdata,
                                     satmap->hidecovs,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_HIDECOVS);
}

/** Load the satellites that we should not highlight coverage */
static void gtk_sat_map_load_hide_coverages(GtkSatMap * satmap)
{
    mod_cfg_get_integer_list_boolean(satmap->cfgdata,
                                     MOD_CFG_MAP_SECTION,
                                     MOD_CFG_MAP_HIDECOVS, satmap->hidecovs);
}
