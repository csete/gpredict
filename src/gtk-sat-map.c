/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2008  Alexandru Csete, OZ9AEC.
  Copyright (C)  2006-2007  William J Beksi, KC2EXL.

  Authors: Alexandru Csete <oz9aec@gmail.com>
  William J Beksi <wjbeksi@users.sourceforge.net>

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

/** \brief Satellite Map Widget.
 *
 * The satellite map widget is responsible for plotting and updating information
 * relevent to each satellite. This information includes the satellite position,
 * label, and footprint. Satellites are stored in a hash table by their catalog 
 * number. The satellite map widget is also responsible for loading and resizing
 * the map.
 *
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "mod-cfg-get-param.h"
#include "gtk-sat-data.h"
#include "gpredict-utils.h"
#include "compat.h"
#include "gtk-sat-map-popup.h"
#include "gtk-sat-map-ground-track.h"
#include "gtk-sat-map.h"
#include "locator.h"
#include "sat-debugger.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include <goocanvas.h>

#define MARKER_SIZE_HALF    1

static void gtk_sat_map_class_init (GtkSatMapClass *class);
static void gtk_sat_map_init       (GtkSatMap *polview);
static void gtk_sat_map_destroy    (GtkObject *object);
static void size_allocate_cb       (GtkWidget *widget, GtkAllocation *allocation, gpointer data);
static void update_map_size        (GtkSatMap *satmap);
static void update_sat             (gpointer key, gpointer value, gpointer data);
static void plot_sat               (gpointer key, gpointer value, gpointer data);
static void lonlat_to_xy           (GtkSatMap *m, gdouble lon, gdouble lat, gfloat *x, gfloat *y);
static void xy_to_lonlat           (GtkSatMap *m, gfloat x, gfloat y, gfloat *lon, gfloat *lat);
static gboolean on_motion_notify   (GooCanvasItem *item,
                                    GooCanvasItem *target,
                                    GdkEventMotion *event,
                                    gpointer data);
static void on_item_created        (GooCanvas *canvas,
                                    GooCanvasItem *item,
                                    GooCanvasItemModel *model,
                                    gpointer data);
static void on_canvas_realized     (GtkWidget *canvas, gpointer data);
static gboolean on_button_press    (GooCanvasItem *item,
                                    GooCanvasItem *target,
                                    GdkEventButton *event,
                                    gpointer data);
static gboolean on_button_release  (GooCanvasItem *item,
                                    GooCanvasItem *target,
                                    GdkEventButton *event,
                                    gpointer data);
static void clear_selection        (gpointer key, gpointer val, gpointer data);
static void load_map_file          (GtkSatMap *satmap);
static GooCanvasItemModel*         create_canvas_model (GtkSatMap *satmap);
static gdouble arccos              (gdouble, gdouble);
static gboolean pole_is_covered    (sat_t *sat);
static gboolean mirror_lon         (sat_t *sat, gdouble rangelon, gdouble *mlon);
static guint calculate_footprint   (GtkSatMap *satmap, sat_t *sat);
static void  split_points          (GtkSatMap *satmap, sat_t *sat, gdouble sspx);
static void sort_points_x          (GtkSatMap *satmap, sat_t *sat, GooCanvasPoints *points, gint num);
static void sort_points_y          (GtkSatMap *satmap, sat_t *sat, GooCanvasPoints *points, gint num);
static gint compare_coordinates_x  (gconstpointer a, gconstpointer b, gpointer data);
static gint compare_coordinates_y  (gconstpointer a, gconstpointer b, gpointer data);
static void update_selected        (GtkSatMap *satmap, sat_t *sat);
static void draw_grid_lines        (GtkSatMap *satmap, GooCanvasItemModel *root);
static void redraw_grid_lines      (GtkSatMap *satmap);

static GtkVBoxClass *parent_class = NULL;
static GooCanvasPoints *points1;
static GooCanvasPoints *points2;


/** \brief Register the satellite map widget. */
GtkType
gtk_sat_map_get_type ()
{
    static GType gtk_sat_map_type = 0;

    if (!gtk_sat_map_type) {
        static const GTypeInfo gtk_sat_map_info = {
            sizeof (GtkSatMapClass),
            NULL, /* base init */
            NULL, /* base finalize */
            (GClassInitFunc) gtk_sat_map_class_init,
            NULL, /* class finalize */
            NULL, /* class data */
            sizeof (GtkSatMap),
            5,    /* n_preallocs */
            (GInstanceInitFunc) gtk_sat_map_init,
        };

        gtk_sat_map_type = g_type_register_static (GTK_TYPE_VBOX,
                                                   "GtkSatMap",
                                                   &gtk_sat_map_info,
                                                   0);
    }

    return gtk_sat_map_type;
}


/** \brief Initialize a GtkSatMapClass object. */
static void
gtk_sat_map_class_init (GtkSatMapClass *class)
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

    object_class->destroy = gtk_sat_map_destroy;
    /* widget_class->size_allocate = gtk_sat_map_size_allocate; */
}


/** \brief Initialize a newly created GtkSatMap widget. */
static void
gtk_sat_map_init (GtkSatMap *satmap)
{
    satmap->sats      = NULL;
    satmap->qth       = NULL;
    satmap->obj       = NULL;
    satmap->naos      = 2458849.5;
    satmap->ncat      = 0;
    satmap->tstamp    = 2458849.5;
    satmap->x0        = 0;
    satmap->y0        = 0;
    satmap->width     = 0;
    satmap->height    = 0;
    satmap->refresh   = 0;
    satmap->counter   = 0;
    satmap->qthinfo   = FALSE;
    satmap->eventinfo = FALSE;
    satmap->cursinfo  = FALSE;
    satmap->showgrid  = FALSE;
    satmap->keepratio = FALSE;
    satmap->resize    = FALSE;
}


/** \brief Destroy a GtkSatMap widget. */
static void
gtk_sat_map_destroy (GtkObject *object)
{
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/** \brief Initialize and plot each satellite on a canvas model. 
 *  
 *  This function creates a canvas model and loads the background map
 *  onto the canvas. Each satellite is then plotted on the map.
 *
 */
GtkWidget*
gtk_sat_map_new (GKeyFile *cfgdata, GHashTable *sats, qth_t *qth)
{
    GtkWidget *satmap;
    GooCanvasItemModel *root;
    guint32 col;

    satmap = g_object_new (GTK_TYPE_SAT_MAP, NULL);

    GTK_SAT_MAP (satmap)->cfgdata = cfgdata;
    GTK_SAT_MAP (satmap)->sats = sats;
    GTK_SAT_MAP (satmap)->qth  = qth;

    GTK_SAT_MAP (satmap)->obj  = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);

    /* get settings */
    GTK_SAT_MAP (satmap)->refresh = mod_cfg_get_int (cfgdata,
                                                     MOD_CFG_MAP_SECTION,
                                                     MOD_CFG_MAP_REFRESH,
                                                     SAT_CFG_INT_MAP_REFRESH);
    GTK_SAT_MAP (satmap)->counter = 1;

    GTK_SAT_MAP (satmap)->qthinfo = mod_cfg_get_bool (cfgdata,
                                                      MOD_CFG_MAP_SECTION,
                                                      MOD_CFG_MAP_SHOW_QTH_INFO,
                                                      SAT_CFG_BOOL_MAP_SHOW_QTH_INFO);

    GTK_SAT_MAP (satmap)->eventinfo = mod_cfg_get_bool (cfgdata,
                                                        MOD_CFG_MAP_SECTION,
                                                        MOD_CFG_MAP_SHOW_NEXT_EVENT,
                                                        SAT_CFG_BOOL_MAP_SHOW_NEXT_EV);

    GTK_SAT_MAP (satmap)->cursinfo = mod_cfg_get_bool (cfgdata,
                                                       MOD_CFG_MAP_SECTION,
                                                       MOD_CFG_MAP_SHOW_CURS_TRACK,
                                                       SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK);

    GTK_SAT_MAP (satmap)->showgrid = mod_cfg_get_bool (cfgdata,
                                                       MOD_CFG_MAP_SECTION,
                                                       MOD_CFG_MAP_SHOW_GRID,
                                                       SAT_CFG_BOOL_MAP_SHOW_GRID);

    GTK_SAT_MAP (satmap)->keepratio = mod_cfg_get_bool (cfgdata,
                                                        MOD_CFG_MAP_SECTION,
                                                        MOD_CFG_MAP_KEEP_RATIO,
                                                        SAT_CFG_BOOL_MAP_KEEP_RATIO);
    col = mod_cfg_get_int (cfgdata,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_INFO_BGD_COL,
                           SAT_CFG_INT_MAP_INFO_BGD_COL);

    GTK_SAT_MAP (satmap)->infobgd = rgba2html (col);

    /* create the canvas */
    GTK_SAT_MAP (satmap)->canvas = goo_canvas_new ();

    /* safely load a background map */
    load_map_file (GTK_SAT_MAP (satmap));

    /* Initial size request should be based on map size
       but if we do this we can not shrink the canvas below this size
       even though the map shrinks => better to set default size of
       main container window.
    */
    /*  gtk_widget_set_size_request (GTK_SAT_MAP (satmap)->canvas, */
    /*  gdk_pixbuf_get_width (GTK_SAT_MAP (satmap)->origmap), */
    /*  gdk_pixbuf_get_height (GTK_SAT_MAP (satmap)->origmap)); */

    goo_canvas_set_bounds (GOO_CANVAS (GTK_SAT_MAP (satmap)->canvas), 0, 0,
                           gdk_pixbuf_get_width (GTK_SAT_MAP (satmap)->origmap),
                           gdk_pixbuf_get_height (GTK_SAT_MAP (satmap)->origmap));
    
                           
    /* connect size-request signal */
    g_signal_connect (GTK_SAT_MAP (satmap)->canvas, "size-allocate",
                      G_CALLBACK (size_allocate_cb), satmap);
    g_signal_connect (GTK_SAT_MAP (satmap)->canvas, "item_created",
                      (GtkSignalFunc) on_item_created, satmap);
    g_signal_connect_after (GTK_SAT_MAP (satmap)->canvas, "realize",
                            (GtkSignalFunc) on_canvas_realized, satmap);

    gtk_widget_show (GTK_SAT_MAP (satmap)->canvas);

    /* create the canvas model */
    root = create_canvas_model (GTK_SAT_MAP (satmap));
    goo_canvas_set_root_item_model (GOO_CANVAS (GTK_SAT_MAP (satmap)->canvas), root);

    g_object_unref (root);

    /* plot each sat on the canvas */
    g_hash_table_foreach (GTK_SAT_MAP (satmap)->sats, plot_sat, GTK_SAT_MAP (satmap));

    /* gtk_box_pack_start (GTK_BOX (satmap), GTK_SAT_MAP (satmap)->swin, TRUE, TRUE, 0); */
    gtk_container_add (GTK_CONTAINER (satmap), GTK_SAT_MAP (satmap)->canvas);

    return satmap;
}


/** \brief Create the map items on the canvas. 
 *  
 *  This function intializes the map dimensions and sets up the cursor track. 
 *  It creates and sets all of the static map items (QTH, grid lines, etc.)
 *  on the canvas.
 *
 */
static GooCanvasItemModel *
create_canvas_model (GtkSatMap *satmap)
{
    GooCanvasItemModel *root;
    gchar *buff;
    gfloat x,y;
    guint32 col;

    root = goo_canvas_group_model_new (NULL, NULL);

    /* map dimensions */
    satmap->width = gdk_pixbuf_get_width (satmap->origmap);
    satmap->height = gdk_pixbuf_get_height (satmap->origmap);
    satmap->x0 = 0;
    satmap->y0 = 0;

    /* background map */
    satmap->map = goo_canvas_image_model_new (root,
                                              satmap->origmap,
                                              satmap->x0,
                                              satmap->y0,
                                              NULL);

    goo_canvas_item_model_lower (satmap->map, NULL);

    /* grid lines */
    draw_grid_lines (satmap, root);

    /* QTH mark */
    col = mod_cfg_get_int (satmap->cfgdata,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_QTH_COL,
                           SAT_CFG_INT_MAP_QTH_COL);

    lonlat_to_xy (satmap, satmap->qth->lon, satmap->qth->lat, &x, &y);
    
    satmap->qthmark = goo_canvas_rect_model_new (root,
                                                 x - MARKER_SIZE_HALF,
                                                 y - MARKER_SIZE_HALF,
                                                 2 * MARKER_SIZE_HALF,
                                                 2 * MARKER_SIZE_HALF,
                                                 "fill-color-rgba", col,
                                                 "stroke-color-rgba", col,
                                                 NULL);

    satmap->qthlabel = goo_canvas_text_model_new (root, satmap->qth->name,
                                                  x, y+2, -1,
                                                  GTK_ANCHOR_NORTH,
                                                  "font", "Sans 8",
                                                  "fill-color-rgba", col,
                                                  NULL);

    /* QTH info */
    col = mod_cfg_get_int (satmap->cfgdata,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_INFO_COL,
                           SAT_CFG_INT_MAP_INFO_COL);

    satmap->locnam = goo_canvas_text_model_new (root, "",
                                                satmap->x0 + 2, satmap->y0 + 1, -1,
                                                GTK_ANCHOR_NORTH_WEST,
                                                "font", "Sans 8",
                                                "fill-color-rgba", col,
                                                "use-markup", TRUE,
                                                NULL);

    /* set text only if QTH info is enabled */
    if (satmap->qthinfo) {
        /* For now only QTH name and location.
           It would be nice with coordinates (remember NWSE setting)
           and Maidenhead locator, when using hamlib.
         
           Note: I used pango markup to set the background color, I didn't find any
           other obvious ways to get the text height in pixels to draw rectangle.
        */
        buff = g_strdup_printf ("<span background=\"#%s\"> %s \302\267 %s </span>",
                                satmap->infobgd,
                                satmap->qth->name,
                                satmap->qth->loc);
        g_object_set (satmap->locnam, "text", buff, NULL);
        g_free (buff);
    }

    /* next event */
    satmap->next = goo_canvas_text_model_new (root, "",
                                              satmap->x0+satmap->width - 2,
                                              satmap->y0 + 1, -1,
                                              GTK_ANCHOR_NORTH_EAST,
                                              "font", "Sans 8",
                                              "fill-color-rgba", col,
                                              "use-markup", TRUE,
                                              NULL);

    /* set text only if QTH info is enabled */
    if (satmap->eventinfo) {
        buff = g_strdup_printf ("<span background=\"#%s\"> ... </span>",
                                satmap->infobgd);
        g_object_set (satmap->next, "text", buff, NULL);
        g_free (buff);
    }

    /* cursor track */
    satmap->curs = goo_canvas_text_model_new (root, "",
                                              satmap->x0 + 2,
                                              satmap->y0 + satmap->height - 1,
                                              -1,
                                              GTK_ANCHOR_SOUTH_WEST,
                                              "font", "Sans 8",
                                              "fill-color-rgba", col,
                                              "use-markup", TRUE,
                                              NULL);

    /* info about a selected satellite */
    satmap->sel = goo_canvas_text_model_new (root, "",
                                             satmap->x0 + satmap->width - 2,
                                             satmap->y0 + satmap->height - 1,
                                             -1,
                                             GTK_ANCHOR_SOUTH_EAST,
                                             "font", "Sans 8",
                                             "fill-color-rgba", col,
                                             "use-markup", TRUE,
                                             NULL);

    return root;
}


/** \brief Manage new size allocation.
 *
 * This function is called when the canvas receives a new size allocation,
 * e.g. when the container is re-sized. The function sets the resize flag of
 * the GtkSatMap widget to TRUE to indicate that the map sizes should be
 * recalculated during the next timeout cycle.
 *
 * \note We could also do the calculation here, but that can be very CPU
 *       intensive because repetitive size allocation will also occur while
 *       the user resizes the window or the GtkPaned layout containers.
 */
static void
size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
    GTK_SAT_MAP (data)->resize = TRUE;
}


/** \brief Update map size.
 *  \param widget Pointer to a GtkSatMap widget.
 *
 * This function is used to recalculate the GtkSatMap dimensions based on
 * its size allocations. It is normally called by the cyclic timeout handler
 * when the satmap->resize flag is set to TRUE.
 *
 * If the aspect ratio of the map is to be kept, we must use the following
 * algorithm to calculate the map dimensions:
 *
 *   ratio = origmap.w / origmap.h
 *   size = min (alloc.w, ratio*alloc.h)
 *   map.w = size
 *   map.h = size / ratio
 *
 * otherwise we can simply calculate using allocation->width and height.
 *
 */
static void
update_map_size (GtkSatMap *satmap)
{
    GtkAllocation allocation;
    GdkPixbuf *pbuf;
    gfloat x, y;
    gfloat ratio;   /* ratio between map width and height */
    gfloat size;    /* size = min (alloc.w, ratio*alloc.h) */

    if (GTK_WIDGET_REALIZED (satmap)) {
        /* get graph dimensions */
        allocation.width = GTK_WIDGET (satmap)->allocation.width;
        allocation.height = GTK_WIDGET (satmap)->allocation.height;

        if (satmap->keepratio) {
            /* Use allocation->width and allocation->height to calculate
             *  new X0 Y0 width and height. Map proportions must be kept.
             */
            ratio = gdk_pixbuf_get_width (satmap->origmap) /
                gdk_pixbuf_get_height (satmap->origmap);

            size = MIN(allocation.width, ratio*allocation.height);

            satmap->width = (guint) size;
            satmap->height = (guint) (size / ratio);
            
            satmap->x0 = (allocation.width - satmap->width) / 2;
            satmap->y0 = (allocation.height - satmap->height) / 2;

            /* rescale pixbuf */
            pbuf = gdk_pixbuf_scale_simple (satmap->origmap,
                                            satmap->width,
                                            satmap->height,
                                            GDK_INTERP_BILINEAR);
        }
        else {
            satmap->x0 = 0;
            satmap->y0 = 0;
            satmap->width = allocation.width;
            satmap->height =allocation.height;

            /* rescale pixbuf */
            pbuf = gdk_pixbuf_scale_simple (satmap->origmap,
                                            satmap->width,
                                            satmap->height,
                                            GDK_INTERP_BILINEAR);
        }

        /* set canvas bounds to match new size */
        goo_canvas_set_bounds (GOO_CANVAS (GTK_SAT_MAP (satmap)->canvas), 0, 0,
                                           satmap->width, satmap->height);


        /* redraw static elements */
        g_object_set (satmap->map,
                      "pixbuf", pbuf,
                      "x", (gdouble) satmap->x0,
                      "y", (gdouble) satmap->y0,
                      NULL);
        g_object_unref (pbuf);

        /* grid lines */
        redraw_grid_lines (satmap);
        
        /* QTH */
        lonlat_to_xy (satmap,  satmap->qth->lon, satmap->qth->lat, &x, &y);
        g_object_set (satmap->qthmark,
                      "x", x - MARKER_SIZE_HALF,
                      "y", y - MARKER_SIZE_HALF,
                      NULL);
        g_object_set (satmap->qthlabel,
                      "x", x,
                      "y", y+2,
                      NULL);

        /* QTH info */
        g_object_set (satmap->locnam,
                      "x", (gdouble) satmap->x0 + 2,
                      "y", (gdouble) satmap->y0 + 1,
                      NULL);

        /* next event */
        g_object_set (satmap->next,
                      "x", (gdouble) satmap->x0 + satmap->width - 2,
                      "y", (gdouble) satmap->y0 + 1,
                      NULL);

        /* cursor info */
        g_object_set (satmap->curs,
                      "x", (gdouble) satmap->x0 + 2,
                      "y", (gdouble) satmap->y0 + satmap->height - 1,
                      NULL);

        /* selected sat info */
        g_object_set (satmap->sel,
                      "x", (gdouble) satmap->x0 + satmap->width - 2,
                      "y", (gdouble) satmap->y0 + satmap->height - 1,
                      NULL);


        /* update satellites */
        g_hash_table_foreach (satmap->sats, update_sat, satmap);

        satmap->resize = FALSE;
    }
}


/** \brief Manage canvas realize signals.
 *
 * The function is used to re-order the canvas items (they can not be
 * re-ordered at creation, since the canvas is not visible).
 */
static void
on_canvas_realized (GtkWidget *canvas, gpointer data)
{
    GtkSatMap *satmap = GTK_SAT_MAP(data);

    /* raise info items */
    goo_canvas_item_model_raise (satmap->sel, NULL);
    goo_canvas_item_model_raise (satmap->locnam, NULL);
    goo_canvas_item_model_raise (satmap->next, NULL);
    goo_canvas_item_model_raise (satmap->curs, NULL);
}


/** \brief Update the GtkSatMap widget
 *
 * Called periodically from GtkSatModule.
 */
void
gtk_sat_map_update (GtkWidget  *widget)
{
    GtkSatMap *satmap = GTK_SAT_MAP (widget);
    sat_t *sat = NULL;
    gdouble number, now;
    gchar *buff;
    gint *catnr;
    guint h, m, s;
    gchar *ch, *cm, *cs;

    /* check whether there are any pending resize requests */
    if (satmap->resize) 
        update_map_size (satmap);

    /* check refresh rate and refresh sats if time */
    if (satmap->counter < satmap->refresh) {
        satmap->counter++;
    }
    else {
        /* reset data */
        satmap->counter = 1;
        satmap->naos = 2458849.5;

        /* update sats */
        g_hash_table_foreach (satmap->sats, update_sat, satmap);

        /* update countdown to NEXT AOS label */
        if (satmap->eventinfo) {

            if (satmap->ncat > 0) {

                catnr = g_try_new0 (gint, 1);
                *catnr = satmap->ncat;
                sat = SAT(g_hash_table_lookup (satmap->sats, catnr));
                g_free (catnr);

                /* last desperate sanity check */
                if (sat != NULL) {

                    now = satmap->tstamp;//get_current_daynum ();
                    number = satmap->naos - now;

                    /* convert julian date to seconds */
                    s = (guint) (number * 86400);

                    /* extract hours */
                    h = (guint) floor (s/3600);
                    s -= 3600*h;

                    /* leading zero */
                    if ((h > 0) && (h < 10))
                        ch = g_strdup ("0");
                    else
                        ch = g_strdup ("");

                    /* extract minutes */
                    m = (guint) floor (s/60);
                    s -= 60*m;

                    /* leading zero */
                    if (m < 10)
                        cm = g_strdup ("0");
                    else
                        cm = g_strdup ("");

                    /* leading zero */
                    if (s < 10)
                        cs = g_strdup (":0");
                    else
                        cs = g_strdup (":");

                    if (h > 0) 
                        buff = g_strdup_printf (_("<span background=\"#%s\"> "\
                                                "Next: %s in %s%d:%s%d%s%d </span>"),
                                                satmap->infobgd,
                                                sat->nickname,
                                                ch, h, cm, m, cs, s);
                    else 
                        buff = g_strdup_printf (_("<span background=\"#%s\"> " \
                                                "Next: %s in %s%d%s%d </span>"),
                                                satmap->infobgd,
                                                sat->nickname,
                                                cm, m, cs, s);

                    g_object_set (satmap->next,
                                  "text", buff,
                                  NULL);

                    g_free (buff);
                    g_free (ch);
                    g_free (cm);
                    g_free (cs);
                }
                else {
                    sat_log_log (SAT_LOG_LEVEL_BUG,
                                 _("%s: Can not find NEXT satellite."),
                                 __FUNCTION__);
                    g_object_set (satmap->next,
                                  "text", _("Next: ERR"),
                                  NULL);
                }
            }
            else {
                g_object_set (satmap->next,
                              "text", _("Next: N/A"),
                              NULL);
            }
        }
        else {
            g_object_set (satmap->next,
                          "text", "",
                          NULL);
        }
    }
}


/** \brief Convert latitude and longitude to screen coordinates.
 *  \param m The GtkSatMap widget on which the conversion should be done.
 *  \param lat The latitude in decimal degrees North.
 *  \param lon The longitude in decimal degrees East.
 *  \param x The X coordinate on the screen (left to right)
 *  \param y The Y coordinate on the screen (top to bottom)
 *
 */
static void
lonlat_to_xy (GtkSatMap *p, gdouble lon, gdouble lat, gfloat *x, gfloat *y)
{
    *x = p->x0 + (180.0 - lon) * p->width / 360.0;
    if (*x < 0.0) /* west longitude */
        *x *= -1;
    else  /* east longitude */
        *x = p->x0 + (180.0 + lon) * p->width / 360.0;

    *y = p->y0 + (90.0 - lat) * p->height / 180.0;;
}


/** \brief Convert screen coordinates to latitude and longitude.
 *  \param m The GtkSatMap widget on which the conversion should be done.
 *  \param x The X coordinate on the screen (left to right)
 *  \param y The Y coordinate on the screen (top to bottom)
 *  \param lat The latitude in decimal degrees North.
 *  \param lon The longitude in decimal degrees East.
 *
 * The function will return nonsense when the cursor is off the map
 * so it is the responsibility of the consumer to check whether returned
 * values are in between valid ranges.
 */
static void
xy_to_lonlat (GtkSatMap *p, gfloat x, gfloat y, gfloat *lon, gfloat *lat)
{
    *lat = 90.0 - (180.0 / p->height) * (y - p->y0);
    *lon = (360.0 / p->width) * (x - p->x0) - 180.0;
}


/** \brief Manage map motion events.
 *
 * This function is called every time the mouse is moving on the map. Its purpose
 * is to update the cursor text with lat and lon coordinates.
 */
static gboolean
on_motion_notify (GooCanvasItem *item,
                  GooCanvasItem *target,
                  GdkEventMotion *event,
                  gpointer data)
{
    GtkSatMap *satmap = GTK_SAT_MAP (data);
    gfloat lat,lon;
    gchar *text;

    /* set text only if QTH info is enabled */
    if (satmap->cursinfo) {

        xy_to_lonlat (satmap, event->x, event->y, &lon, &lat);

        /*** FIXME:
             - Add QRA?
        */
        /* cursor track */
        text = g_strdup_printf ("<span background=\"#%s\"> "\
                                "LON:%.0f\302\260 LAT:%.0f\302\260 </span>",
                                satmap->infobgd, lon, lat);

        g_object_set (satmap->curs, "text", text, NULL);
        g_free (text);
    }

    return TRUE;
}


/** \brief Finish canvas item setup.
 *  \param canvas 
 *  \param item
 *  \param model 
 *  \param data Pointer to the GtkSatMap object.
 *
 * This function is called when a canvas item is created. Its purpose is to connect
 * the corresponding signals to the created items.
 *
 * The root item, ie the background is connected to motion notify event, while other
 * items (sats) are connected to mouse click events.
 *
 * \bug Should filter out QTH or do we want to click on the QTH?
 */
static void
on_item_created (GooCanvas *canvas,
                 GooCanvasItem *item,
                 GooCanvasItemModel *model,
                 gpointer data)
{
    if (!goo_canvas_item_model_get_parent (model)) {
        /* root item / canvas */
        g_signal_connect (item, "motion_notify_event",
                          (GtkSignalFunc) on_motion_notify, data);
    }

    else if (!g_object_get_data (G_OBJECT (item), "skip-signal-connection")) {
        g_signal_connect (item, "button_press_event",
                          (GtkSignalFunc) on_button_press, data);
        g_signal_connect (item, "button_release_event",
                          (GtkSignalFunc) on_button_release, data);
    }
}


/** \brief Manage button press events
 *
 * This function is called when a mouse button is pressed on a satellite object.
 * If the pressed button is #3 (right button) the satellite popup menu will be
 * created and executed.
 */
static gboolean
on_button_press (GooCanvasItem *item,
                 GooCanvasItem *target,
                 GdkEventButton *event,
                 gpointer data)
{
    GooCanvasItemModel *model = goo_canvas_item_get_model (item);
    GtkSatMap *satmap = GTK_SAT_MAP (data);
    gint catnum = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "catnum"));
    gint *catpoint = NULL;
    sat_t *sat = NULL;

    switch (event->button) {

        /* pop-up menu */
    case 3:
        catpoint = g_try_new0 (gint, 1);
        *catpoint = catnum;

        sat = SAT (g_hash_table_lookup (satmap->sats, catpoint));

        if (sat != NULL) {
            gtk_sat_map_popup_exec (sat, satmap->qth, satmap, event,
                                    gtk_widget_get_toplevel (GTK_WIDGET (satmap)));
        }
        else {
            /* clicked on map -> map pop-up in the future */
        }

        g_free (catpoint);

        break;

    default:
        break;
    }

    return TRUE;
}


/** \brief Manage button release events.
 *
 * This function is called when the mouse button is released above
 * a satellite object. It will act as a button click and if the relesed
 * button is the left one, the clock will correspond to selecting or
 * deselecting a satellite
 */
static gboolean
on_button_release (GooCanvasItem *item,
                   GooCanvasItem *target,
                   GdkEventButton *event,
                   gpointer data)
{
    GooCanvasItemModel *model = goo_canvas_item_get_model (item);
    GtkSatMap *satmap = GTK_SAT_MAP (data);
    gint catnum = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "catnum"));
    gint *catpoint = NULL;
    sat_map_obj_t *obj = NULL;
    guint32  col;

    catpoint = g_try_new0 (gint, 1);
    *catpoint = catnum;

    switch (event->button) {
        /* Select / de-select satellite */
    case 1:
        obj = SAT_MAP_OBJ (g_hash_table_lookup (satmap->obj, catpoint));
        if (obj == NULL) {
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%d: Can not find clicked object (%d) in hash table"),
                         __FILE__, __LINE__, catnum);
        }
        else {
            obj->selected = !obj->selected;

            if (obj->selected) {
                col = mod_cfg_get_int (satmap->cfgdata,
                                       MOD_CFG_MAP_SECTION,
                                       MOD_CFG_MAP_SAT_SEL_COL,
                                       SAT_CFG_INT_MAP_SAT_SEL_COL);
            }
            else {
                col = mod_cfg_get_int (satmap->cfgdata,
                                       MOD_CFG_MAP_SECTION,
                                       MOD_CFG_MAP_SAT_COL,
                                       SAT_CFG_INT_MAP_SAT_COL);
                *catpoint = 0;

                g_object_set (satmap->sel, "text", "", NULL);
            }

            g_object_set (obj->marker,
                          "fill-color-rgba", col,
                          "stroke-color-rgba", col,
                          NULL);
            g_object_set (obj->label,
                          "fill-color-rgba", col,
                          "stroke-color-rgba", col,
                          NULL);
            g_object_set (obj->range1,
                          "stroke-color-rgba", col,
                          NULL);

            if (obj->oldrcnum == 2)
                g_object_set (obj->range2,
                              "stroke-color-rgba", col,
                              NULL);

            /* clear other selections */
            g_hash_table_foreach (satmap->obj, clear_selection, catpoint);
        }
        break;
    default:
        break;
    }

    g_free (catpoint);
    
    return TRUE;
}


/** \brief Clear selection.
 *
 * This function is used to clear the old selection when a new satellite
 * is selected.
 */
static void
clear_selection (gpointer key, gpointer val, gpointer data)
{
    gint *old = key;
    gint *new = data;
    sat_map_obj_t *obj = SAT_MAP_OBJ (val);
    guint32 col;

    if ((*old != *new) && (obj->selected)) {
        obj->selected = FALSE;

        /** FIXME: this is only global default; need the satmap here! */
        col = sat_cfg_get_int (SAT_CFG_INT_MAP_SAT_COL);

        g_object_set (obj->marker,
                      "fill-color-rgba", col,
                      "stroke-color-rgba", col,
                      NULL);
        g_object_set (obj->label,
                      "fill-color-rgba", col,
                      "stroke-color-rgba", col,
                      NULL);
        g_object_set (obj->range1,
                      "stroke-color-rgba", col,
                      NULL);

        if (obj->oldrcnum == 2)
            g_object_set (obj->range2,
                          "stroke-color-rgba", col,
                          NULL);
    }
}


/** \brief Reconfigure map.
 *
 * This function should eventually reload all configuration for the GtkSatMap.
 * Currently this function is not implemented for any of the views. Reconfiguration
 * is done by recreating the whole module.
 */
void
gtk_sat_map_reconf (GtkWidget  *widget, GKeyFile *cfgdat)
{
}


/** \brief Safely load a map file.
 *  \param satmap The GtkSatMap widget
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
 * \note satmap->cfgdata cshould contain a valid GKeyFile.
 *
 */
static void
load_map_file (GtkSatMap *satmap)
{
    gchar *buff;
    gchar *mapfile;
    GError *error = NULL;

    /* get local, global or default map file */
    buff = mod_cfg_get_str (satmap->cfgdata,
                            MOD_CFG_MAP_SECTION,
                            MOD_CFG_MAP_FILE,
                            SAT_CFG_STR_MAP_FILE);

    if (g_path_is_absolute (buff)) {
        /* map is user specific, ie. in $HOME/.gpredict2/maps/ */
        mapfile = g_strdup (buff);
    }
    else {
        /* build complete path */
        mapfile = map_file_name (buff);
    }
    g_free (buff);

    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                 _("%s:%d: Loading map file %s"),
                 __FILE__, __LINE__, mapfile);

    /* check that file exists, if not get the default */
    if (g_file_test (mapfile, G_FILE_TEST_EXISTS)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%d: Map file found"),
                     __FILE__, __LINE__);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Could not find map file %s"),
                     __FILE__, __LINE__, mapfile);

        /* get default map file */
        g_free (mapfile);
        mapfile = sat_cfg_get_str_def (SAT_CFG_STR_MAP_FILE);

        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Using default map: %s"),
                     __FILE__, __LINE__, mapfile);
    }

    /* try to load the map file */
    satmap->origmap = gdk_pixbuf_new_from_file (mapfile, &error);

    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Error loading map file (%s)"),
                     __FILE__, __LINE__, error->message);
        g_clear_error (&error);

        /* create a dummy GdkPixbuf to avoid crash */
        satmap->origmap = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                          FALSE,
                                          8, 400, 200);
        gdk_pixbuf_fill (satmap->origmap, 0x0F0F0F0F);
    }
                
    g_free (mapfile);
}


/** \brief Arccosine implementation. 
 *
 * Returns a value between zero and two pi.
 * Borrowed from gsat 0.9 by Xavier Crehueras, EB3CZS.
 * Optimized by Alexandru Csete.
 */
static gdouble
arccos (gdouble x, gdouble y)
{
    if (x && y) {
        if (y > 0.0)
            return acos (x/y);
        else if (y < 0.0)
            return pi + acos (x/y);
    }

    return 0.0;
}


/** \brief Check whether the footprint covers the North or South pole. */
static gboolean
pole_is_covered   (sat_t *sat)
{
    int ret1,ret2;
    gdouble qrb1, qrb2, az1, az2;

    ret1 = qrb (sat->ssplon, sat->ssplat, 0.0, 90.0, &qrb1, &az1);
    ret2 = qrb (sat->ssplon, sat->ssplat, 0.0, -90.0, &qrb2, &az2);

    if ((qrb1 <= 0.5*sat->footprint) || (qrb2 <= 0.5*sat->footprint))
        return TRUE;
    
    return FALSE;
}


/** \brief Mirror the footprint longitude. */
static gboolean
mirror_lon (sat_t *sat, gdouble rangelon, gdouble *mlon)
{
    gdouble diff;
    gboolean warped = FALSE;

    if (sat->ssplon < 0.0) {
        /* western longitude */
        if (rangelon < 0.0) {
            /* rangelon has not been warped over */
            *mlon = sat->ssplon + fabs (rangelon - sat->ssplon);
        }
        else {
            /* rangelon has been warped over */
            diff = 360.0 + sat->ssplon - rangelon;
            *mlon = sat->ssplon + diff;
            warped = TRUE;
        }
    }
    else {
        /* eastern longitude */
        *mlon = sat->ssplon + fabs (rangelon - sat->ssplon);
        
        if (*mlon > 180.0) {
            *mlon -= 360;
            warped = TRUE;
        }
    }

    return warped;
}


/** \brief Calculate satellite footprint and coverage area.
 *  \param satmap TheGtkSatMap widget.
 *  \param sat The satellite.
 *  \param points1 Initialised GooCanvasPoints structure with 360 points.
 *  \param points2 Initialised GooCanvasPoints structure with 360 points.
 *  \return The number of range circle parts.
 *
 * This function calculates the "left" side of the range circle and mirrors
 * the points in longitude to create the "right side of the range circle, too.
 * In order to be able to use the footprint points to create a set of subsequent
 * lines conencted to each other (poly-lines) the function may have to perform
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
static guint
calculate_footprint (GtkSatMap *satmap, sat_t *sat)
{
    guint azi;
    gfloat sx, sy, msx, msy, ssx, ssy;
    gdouble ssplat, ssplon, beta, azimuth, num, dem;
    gdouble rangelon, rangelat, mlon;
    gboolean warped = FALSE;
    guint numrc = 1;

    /* Range circle calculations.
     * Borrowed from gsat 0.9.0 by Xavier Crehueras, EB3CZS
     * who borrowed from John Magliacane, KD2BD.
     * Optimized by Alexandru Csete and William J Beksi.
     */
    ssplat = sat->ssplat * de2ra;
    ssplon = sat->ssplon * de2ra;
    beta = (0.5 * sat->footprint) / xkmper;

    for (azi = 0; azi < 180; azi++)    {
        azimuth = de2ra * (double)azi;
        rangelat = asin (sin (ssplat) * cos (beta) + cos (azimuth) *
                         sin (beta) * cos (ssplat));
        num = cos (beta) - (sin (ssplat) * sin (rangelat));
        dem = cos (ssplat) * cos (rangelat);
            
        if (azi == 0 && (beta > pio2 - ssplat))
            rangelon = ssplon + pi;
            
        else if (azi == 180 && (beta > pio2 + ssplat))
            rangelon = ssplon + pi;
                
        else if (fabs (num / dem) > 1.0)
            rangelon = ssplon;
                
        else {
            if ((180 - azi) >= 0)
                rangelon = ssplon - arccos (num, dem);
            else
                rangelon = ssplon + arccos (num, dem);
        }
                
        while (rangelon < -pi)
            rangelon += twopi;
        
        while (rangelon > (pi))
            rangelon -= twopi;
                
        rangelat = rangelat / de2ra;
        rangelon = rangelon / de2ra;

        /* mirror longitude */
        if (mirror_lon (sat, rangelon, &mlon))
            warped = TRUE;

        lonlat_to_xy (satmap, rangelon, rangelat, &sx, &sy);
        lonlat_to_xy (satmap, mlon, rangelat, &msx, &msy);

        points1->coords[2*azi] = sx;
        points1->coords[2*azi+1] = sy;
    
        /* Add mirrored point */
        points1->coords[718-2*azi] = msx;
        points1->coords[719-2*azi] = msy;
    }

    /* points1 ow contains 360 pairs of map-based XY coordinates.
       Check whether actions 1, 2 or 3 have to be performed.
    */

    /* pole is covered => sort points1 and add additional points */
    if (pole_is_covered (sat)) {

        sort_points_x (satmap, sat, points1, 360);
        numrc = 1;

    }

    /* pole not covered but range circle has been warped
       => split points */
    else if (warped == TRUE) {

        lonlat_to_xy (satmap, sat->ssplon, sat->ssplat, &ssx, &ssy);
        split_points (satmap, sat, ssx);
        numrc = 2;

    }

    /* the nominal condition => points1 is adequate */
    else {

        numrc = 1;

    }

    return numrc;
}


/** \brief Split and sort polyline points.
 *  \param satmap The GtkSatMap structure.
 *  \param points1 GooCanvasPoints containing the footprint points.
 *  \param points2 A GooCanvasPoints structure containing the second set of points.
 *  \param sspx Canvas based x-coordinate of SSP.
 *  \bug We should ensure that the endpoints in points1 have x=x0, while in
 *       the endpoints in points2 should have x=x0+width (TBC).
 *
 * \note This function works on canvas-based coordinates rather than lat/lon
 * \note DO NOT USE this function when the footprint covers one of the poles
 *       (the end result may freeze the X-server requiring a hard-reset!)
 */
static void
split_points (GtkSatMap *satmap, sat_t *sat, gdouble sspx)
{
    GooCanvasPoints *tps1,*tps2;
    gint n,n1,n2,ns,i,j,k;

    /* initialize parameters */
    n = points1->num_points;
    n1 = 0;
    n2 = 0;
    i = 0;
    j = 0;
    k = 0;
    ns = 0;
    tps1 = goo_canvas_points_new (n);
    tps2 = goo_canvas_points_new (n);

    //if ((sspx >= (satmap->x0 + satmap->width - 0.6)) ||
    //    (sspx >= (satmap->x0 - 0.6))) {
    //if ((sspx == (satmap->x0 + satmap->width)) ||
    //    (sspx == (satmap->x0))) {
    if ((sat->ssplon >= 179.4) || (sat->ssplon <= -179.4)) {
        /* sslon = +/-180 deg.
           - copy points with (x > satmap->x0+satmap->width/2) to tps1
           - copy points with (x < satmap->x0+satmap->width/2) to tps2
           - sort tps1 and tps2
        */
        for (i = 0; i < n; i++) {
            
            if (points1->coords[2*i] > (satmap->x0 + satmap->width/2)) {
                tps1->coords[2*n1] = points1->coords[2*i];
                tps1->coords[2*n1+1] = points1->coords[2*i+1];
                n1++;
            }
            else {
                tps2->coords[2*n2] = points1->coords[2*i];
                tps2->coords[2*n2+1] = points1->coords[2*i+1];
                n2++;
            }
        }

        sort_points_y (satmap, sat, tps1, n1);
        sort_points_y (satmap, sat, tps2, n2);
    }

    else if (sspx < (satmap->x0 + satmap->width / 2)) {
        /* We are on the left side of the map.
           Scan through points1 until we get to x > sspx (i=ns):

           - copy the points forwards until x < (x0+w/2) => tps2
           - continue to copy until the end => tps1
           - copy the points from i=0 to i=ns => tps1.

           Copy tps1 => points1 and tps2 => points2
        */
        while (points1->coords[2*i] <= sspx) {
            i++;
        }
        ns = i-1;

        while (points1->coords[2*i] > (satmap->x0 + satmap->width/2)) {
            tps2->coords[2*j] = points1->coords[2*i];
            tps2->coords[2*j+1] = points1->coords[2*i+1];
            i++;
            j++;
            n2++;
        }

        while (i < n) {
            tps1->coords[2*k] = points1->coords[2*i];
            tps1->coords[2*k+1] = points1->coords[2*i+1];
            i++;
            k++;
            n1++;
        }

        for (i = 0; i <= ns; i++) {
            tps1->coords[2*k] = points1->coords[2*i];
            tps1->coords[2*k+1] = points1->coords[2*i+1];
            k++;
            n1++;
        }
    }
    else {
        /* We are on the right side of the map.
           Scan backwards through points1 until x < sspx (i=ns):

           - copy the points i=ns,i-- until x >= x0+w/2  => tps2
           - copy the points until we reach i=0          => tps1
           - copy the points from i=n to i=ns            => tps1

        */
        i = n-1;
        while (points1->coords[2*i] >= sspx) {
            i--;
        }
        ns = i+1;

        while (points1->coords[2*i] < (satmap->x0 + satmap->width/2)) {
            tps2->coords[2*j] = points1->coords[2*i];
            tps2->coords[2*j+1] = points1->coords[2*i+1];
            i--;
            j++;
            n2++;
        }

        while (i >= 0) {
            tps1->coords[2*k] = points1->coords[2*i];
            tps1->coords[2*k+1] = points1->coords[2*i+1];
            i--;
            k++;
            n1++;
        }

        for (i = n-1; i >= ns; i--) {
            tps1->coords[2*k] = points1->coords[2*i];
            tps1->coords[2*k+1] = points1->coords[2*i+1];
            k++;
            n1++;
        }
    }

    //g_print ("NS:%d  N1:%d  N2:%d\n", ns, n1, n2);

    /* free points and copy new contents */
    goo_canvas_points_unref (points1);
    goo_canvas_points_unref (points2);

    points1 = goo_canvas_points_new (n1);
    for (i = 0; i < n1; i++) {
        points1->coords[2*i] = tps1->coords[2*i];
        points1->coords[2*i+1] = tps1->coords[2*i+1];
    }
    goo_canvas_points_unref (tps1);
    
    points2 = goo_canvas_points_new (n2);
    for (i = 0; i < n2; i++) {
        points2->coords[2*i] = tps2->coords[2*i];
        points2->coords[2*i+1] = tps2->coords[2*i+1];
    }
    goo_canvas_points_unref (tps2);

    /* stretch end points to map borders */
    if (points1->coords[0] > (satmap->x0+satmap->width/2)) {
        points1->coords[0] = satmap->x0+satmap->width;
        points1->coords[2*(n1-1)] = satmap->x0+satmap->width;
        points2->coords[0] = satmap->x0;
        points2->coords[2*(n2-1)] = satmap->x0;
    }
    else {
        points2->coords[0] = satmap->x0+satmap->width;
        points2->coords[2*(n2-1)] = satmap->x0+satmap->width;
        points1->coords[0] = satmap->x0;
        points1->coords[2*(n1-1)] = satmap->x0;
    }
}


/** \brief Sort points according to X coordinates.
 *  \param satmap The GtkSatMap structure.
 *  \param sat The satellite data structure.
 *  \param points The points to sort.
 *  \param num The number of points. By specifying it as parameter we can
 *             sort incomplete arrays.
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
static void
sort_points_x (GtkSatMap *satmap, sat_t *sat, GooCanvasPoints *points, gint num)
{
    gsize size = 2*sizeof(double);

    /* call g_qsort_with_data, which warps the qsort function
       from stdlib */
    g_qsort_with_data (points->coords, num, size,
                       compare_coordinates_x, NULL);

    /* move point at position 0 to position 1 */
    points->coords[2] = satmap->x0;
    points->coords[3] = points->coords[1];

    /* move point at position N to position N-1 */
    points->coords[716] = satmap->x0+satmap->width;//points->coords[718];
    points->coords[717] = points->coords[719];

    if (sat->ssplat > 0.0) {
        /* insert (x0-1,y0) into position 0 */
        points->coords[0] = satmap->x0;
        points->coords[1] = satmap->y0;
        
        /* insert (x0+width,y0) into position N */
        points->coords[718] = satmap->x0 + satmap->width;
        points->coords[719] = satmap->y0;
    }
    else {
        /* insert (x0,y0+height) into position 0 */
        points->coords[0] = satmap->x0;
        points->coords[1] = satmap->y0 + satmap->height;

        /* insert (x0+width,y0+height) into position N */
        points->coords[718] = satmap->x0 + satmap->width;
        points->coords[719] = satmap->y0 + satmap->height;
    }
}


/** \brief Sort points according to Y coordinates.
 *  \param satmap The GtkSatMap structure.
 *  \param sat The satellite data structure.
 *  \param points The points to sort.
 *  \param num The number of points. By specifying it as parameter we can
 *             sort incomplete arrays.
 *
 * This function sorts the points in ascending order with respect
 * to their y value.
 *
 */
static void
sort_points_y (GtkSatMap *satmap, sat_t *sat, GooCanvasPoints *points, gint num)
{
    gsize size;

    size = 2*sizeof(double);

    /* call g_qsort_with_data, which warps the qsort function
       from stdlib */
    g_qsort_with_data (points->coords, num, size,
                       compare_coordinates_y, NULL);
}


/** \brief Compare two X coordinates.
 *  \param a Pointer to one coordinate (x,y) both double.
 *  \param b Pointer to the second coordinate (x,y) both double.
 *  \param data User data; always NULL.
 *  \return Negative value if a < b; zero if a = b; positive value if a > b. 
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
static gint
compare_coordinates_x (gconstpointer a, gconstpointer b, gpointer data)
{
    double *ea = (double *)a;
    double *eb = (double *)b;
    
    if (ea[0] < eb[0]) {
        return -1;
    }

    else if (ea[0] > eb[0]) {
        return 1;
    }

    return 0;
}


/** \brief Compare two Y coordinates.
 *  \param a Pointer to one coordinate (x,y) both double.
 *  \param b Pointer to the second coordinate (x,y) both double.
 *  \param data User data; always NULL.
 *  \return Negative value if a < b; zero if a = b; positive value if a > b. 
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
static gint
compare_coordinates_y (gconstpointer a, gconstpointer b, gpointer data)
{
    double *ea = (double *)a;
    double *eb = (double *)b;
    
    if (ea[1] < eb[1]) {
        return -1;
    }

    else if (ea[1] > eb[1]) {
        return 1;
    }

    return 0;
}


/** \brief Plot a satellite.
 *  \param key The hash table key.
 *  \param value Pointer to the satellite.
 *  \param data Pointer to the GtkSatMap widget.
 *
 * This function creates and initializes the canvas objects (rectangle, label,
 * footprint) for a satellite. The function is called as a g_hash_table_foreach
 * callback.
 */
static void
plot_sat (gpointer key, gpointer value, gpointer data)
{
    GtkSatMap *satmap = GTK_SAT_MAP (data);
    sat_map_obj_t *obj = NULL;
    sat_t *sat = SAT(value);
    GooCanvasItemModel *root;
    gint *catnum;
    guint32 col,covcol;
    gfloat x,y;

    /* get satellite and SSP */
    catnum = g_new0 (gint, 1);
    *catnum = sat->tle.catnr;

    lonlat_to_xy (satmap, sat->ssplon, sat->ssplat, &x, &y);
    
    /* create and initialize a sat object */
    obj = g_try_new (sat_map_obj_t, 1);

    obj->selected = FALSE;
    obj->showtrack = FALSE;
    obj->showcov = TRUE;
    obj->istarget = FALSE;
    obj->oldrcnum = 0;
    obj->newrcnum = 0;
    obj->track_data.latlon = NULL;
    obj->track_data.lines = NULL;
    obj->track_orbit = 0;

    root = goo_canvas_get_root_item_model (GOO_CANVAS (satmap->canvas));

    /* satellite color */
    col = mod_cfg_get_int (satmap->cfgdata,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_SAT_COL,
                           SAT_CFG_INT_MAP_SAT_COL);

    /* area coverage colour */
    /*     if ((obj->showcov) && (sat->otype != ORBIT_TYPE_DECAYED)) { */
    covcol = mod_cfg_get_int (satmap->cfgdata,
                              MOD_CFG_MAP_SECTION,
                              MOD_CFG_MAP_SAT_COV_COL,
                              SAT_CFG_INT_MAP_SAT_COV_COL);
    /*     } */
    /*     else { */
    /*         covcol = 0x00000000; */
    /*     } */

    obj->marker = goo_canvas_rect_model_new (root,
                                             x - MARKER_SIZE_HALF,
                                             y - MARKER_SIZE_HALF,
                                             2 * MARKER_SIZE_HALF,
                                             2 * MARKER_SIZE_HALF,
                                             "fill-color-rgba", col,
                                             "stroke-color-rgba", col,
                                             NULL);

    obj->label = goo_canvas_text_model_new (root, sat->nickname,
                                            x, 
                                            y+2,
                                            -1,
                                            GTK_ANCHOR_NORTH,
                                            "font", "Sans 8",
                                            "fill-color-rgba", col,
                                            NULL);

    g_object_set_data (G_OBJECT (obj->marker), "catnum", GINT_TO_POINTER (*catnum));
    g_object_set_data (G_OBJECT (obj->label), "catnum", GINT_TO_POINTER (*catnum));

    /* initialize points for footprint */
    points1 = goo_canvas_points_new (360);
    points2 = goo_canvas_points_new (360);

    /* calculate footprint */
    obj->newrcnum = calculate_footprint (satmap, sat);
    obj->oldrcnum = obj->newrcnum;

    /* invisible footprint for decayed sats (STS fix) */
    /*     if (sat->otype == ORBIT_TYPE_DECAYED) { */
    /*         col = 0x00000000; */
    /*     } */

    /* always create first part of range circle */
    obj->range1 = goo_canvas_polyline_model_new (root, FALSE, 0,
                                                 "points", points1,
                                                 "line-width", 1.0,
                                                 "fill-color-rgba", covcol,
                                                 "stroke-color-rgba", col,
                                                 "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                 "line-join", CAIRO_LINE_JOIN_MITER,
                                                 NULL);
    g_object_set_data (G_OBJECT (obj->range1), "catnum", GINT_TO_POINTER (*catnum));
    
    /* create second part if available */
    if (obj->newrcnum == 2) {
        //g_print ("N1:%d  N2:%d\n", points1->num_points, points2->num_points);

        obj->range2 = goo_canvas_polyline_model_new (root, FALSE, 0,
                                                     "points", points2,
                                                     "line-width", 1.0,
                                                     "fill-color-rgba", covcol,
                                                     "stroke-color-rgba", col,
                                                     "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                     "line-join", CAIRO_LINE_JOIN_MITER,
                                                     NULL);
        g_object_set_data (G_OBJECT (obj->range2), "catnum", GINT_TO_POINTER (*catnum));

    }

    goo_canvas_points_unref (points1);
    goo_canvas_points_unref (points2);

    /* add sat to hash table */
    g_hash_table_insert (satmap->obj, catnum, obj);
}


/** \brief Update a given satellite.
 */
static void
update_sat (gpointer key, gpointer value, gpointer data)
{
    gint               *catnum;
    GtkSatMap          *satmap = GTK_SAT_MAP (data);
    sat_map_obj_t      *obj = NULL;
    sat_t              *sat = SAT(value);
    gfloat             x, y;
    gdouble            oldx, oldy; 
    gdouble            now;  // = get_current_daynum ();
    GooCanvasItemModel *root;
    gint               idx;
    guint32            col,covcol;

    //gdouble sspla,ssplo;

    root = goo_canvas_get_root_item_model (GOO_CANVAS (satmap->canvas));

    catnum = g_new0 (gint, 1);
    *catnum = sat->tle.catnr;

    now = satmap->tstamp;

    /* update next AOS */
    if ((sat->aos > now) && (sat->aos < satmap->naos)) {
        satmap->naos = sat->aos;
        satmap->ncat = sat->tle.catnr;
    }

    obj = SAT_MAP_OBJ (g_hash_table_lookup (satmap->obj, catnum));

    if (obj->selected) {
        /* update satmap->sel */
        update_selected (satmap, sat);
    }

    //sat_debugger_get_ssp (&ssplo,&sspla);
    //sat->ssplon = ssplo;
    //sat->ssplat = sspla;

    lonlat_to_xy (satmap, sat->ssplon, sat->ssplat, &x, &y);

    /* update only if satellite has moved at least
       2 * MARKER_SIZE_HALF (no need to drain CPU all the time)
    */
    g_object_get (obj->marker,
                  "x", &oldx,
                  "y", &oldy,
                  NULL);

    if ((fabs (oldx-x) >= 2*MARKER_SIZE_HALF) ||
        (fabs (oldy-y) >= 2*MARKER_SIZE_HALF)) {

        /* update sat mark */
        g_object_set (obj->marker,
                      "x", (gdouble) (x - MARKER_SIZE_HALF),
                      "y", (gdouble) (y - MARKER_SIZE_HALF),
                      NULL);

        /* update sat label */
        if (x < 50) {
            g_object_set (obj->label,
                          "x", (gdouble) (x+3),
                          "y", (gdouble) (y),
                          "anchor", GTK_ANCHOR_WEST,
                          NULL);
        } else if ((satmap->width - x ) < 50) {
            g_object_set (obj->label,
                          "x", (gdouble) (x),
                          "y", (gdouble) (y),
                          "anchor", GTK_ANCHOR_EAST,
                          NULL);
        } else if ((satmap->height - y) < 25) {
            g_object_set (obj->label,
                          "x", (gdouble) (x),
                          "y", (gdouble) (y-2),
                          "anchor", GTK_ANCHOR_SOUTH,
                          NULL);
        } else {
            g_object_set (obj->label,
                          "x", (gdouble) (x),
                          "y", (gdouble) (y+2),
                          "anchor", GTK_ANCHOR_NORTH,
                          NULL);
        }

        /* initialize points for footprint */
        points1 = goo_canvas_points_new (360);
        points2 = goo_canvas_points_new (360);

        /* calculate footprint */
        obj->newrcnum = calculate_footprint (satmap, sat);

        /* always update first part */
        g_object_set (obj->range1,
                      "points", points1,
                      NULL);

        if (obj->newrcnum == 2) {
            if (obj->oldrcnum == 1) {
                /* we need to create the second part */
                if (obj->selected) {
                    col = mod_cfg_get_int (satmap->cfgdata,
                                           MOD_CFG_MAP_SECTION,
                                           MOD_CFG_MAP_SAT_SEL_COL,
                                           SAT_CFG_INT_MAP_SAT_SEL_COL);
                }
                else {
                    col = mod_cfg_get_int (satmap->cfgdata,
                                           MOD_CFG_MAP_SECTION,
                                           MOD_CFG_MAP_SAT_COL,
                                           SAT_CFG_INT_MAP_SAT_COL);
                }
                /* coverage color */
                if (obj->showcov) {
                    covcol = mod_cfg_get_int (satmap->cfgdata,
                                              MOD_CFG_MAP_SECTION,
                                              MOD_CFG_MAP_SAT_COV_COL,
                                              SAT_CFG_INT_MAP_SAT_COV_COL);
                }
                else {
                    covcol = 0x00000000;
                }
                obj->range2 = goo_canvas_polyline_model_new (root, FALSE, 0,
                                                             "points", points2,
                                                             "line-width", 1.0,
                                                             "fill-color-rgba", covcol,
                                                             "stroke-color-rgba", col,
                                                             "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                             "line-join", CAIRO_LINE_JOIN_MITER,
                                                             NULL);
                g_object_set_data (G_OBJECT (obj->range2), "catnum",
                                   GINT_TO_POINTER (*catnum));
            }
            else {
                /* just update the second part */
                g_object_set (obj->range2,
                              "points", points2,
                              NULL);
            }
        }
        else {
            if (obj->oldrcnum == 2) {
                /* remove second part */
                idx = goo_canvas_item_model_find_child (root, obj->range2);
                if (idx != -1) {
                    goo_canvas_item_model_remove_child (root, idx);
                }
            }
        }

        /* update rc-number */
        obj->oldrcnum = obj->newrcnum;

        goo_canvas_points_unref (points1);
        goo_canvas_points_unref (points2);
    }

    /* if ground track is visible check whether we have passed into a
       new orbit, in which case we need to recalculate the ground track
    */
    if (obj->showtrack) {
        if (obj->track_orbit != sat->orbit) {
            ground_track_update (satmap, sat, satmap->qth, obj, TRUE);
        }
        /* otherwise we may be in a map rescale process */
        else if (satmap->resize) {
            ground_track_update (satmap, sat, satmap->qth, obj, FALSE);
        }
    }

    g_free (catnum);
}


/** \brief Update information about the selected satellite.
 *  \param satmap Pointer to the GtkSatMap widget.
 *  \param sat Pointer to the selected satellite
 *
 * 
 */
static void
update_selected (GtkSatMap *satmap, sat_t *sat)
{
    guint         h,m,s;
    gchar        *ch,*cm,*cs;
    gchar        *alsstr,*text;
    gdouble       number, now;
    gboolean      isgeo = FALSE;  /* set to TRUE if satellite appears to be GEO */

    now = satmap->tstamp;//get_current_daynum ();

    if (sat->el > 0.0) {
        if (sat->los > 0.0) {
            number = sat->los - now;
            alsstr = g_strdup ("LOS");
        }
        else {
            isgeo = TRUE;
        }
    }
    else {
        if (sat->aos > 0.0) {
            number = sat->aos - now;
            alsstr = g_strdup ("AOS");
        }
        else {
            isgeo = TRUE;
        }
    }

    /* if satellite appears to be GEO don't attempt to show AOS/LOS */
    if (isgeo) {
        if (sat->el > 0.0) {
            text = g_strdup_printf ("<span background=\"#%s\"> %s: Always in range </span>",
                                    satmap->infobgd, sat->nickname);
        }
        else {
            text = g_strdup_printf ("<span background=\"#%s\"> %s: Always out of range </span>",
                                    satmap->infobgd, sat->nickname);
        }
    }
    else {
        /* convert julian date to seconds */
        s = (guint) (number * 86400);

        /* extract hours */
        h = (guint) floor (s/3600);
        s -= 3600*h;

        /* leading zero */
        if ((h > 0) && (h < 10))
            ch = g_strdup ("0");
        else
            ch = g_strdup ("");

        /* extract minutes */
        m = (guint) floor (s/60);
        s -= 60*m;

        /* leading zero */
        if (m < 10)
            cm = g_strdup ("0");
        else
            cm = g_strdup ("");

        /* leading zero */
        if (s < 10)
            cs = g_strdup (":0");
        else
            cs = g_strdup (":");

        if (h > 0) {
            text = g_strdup_printf ("<span background=\"#%s\"> "\
                                    "%s %s in %s%d:%s%d%s%d </span>",
                                    satmap->infobgd, sat->nickname,
                                    alsstr, ch, h, cm, m, cs, s);
        }
        else {
            text = g_strdup_printf ("<span background=\"#%s\"> "\
                                    "%s %s in %s%d%s%d </span>",
                                    satmap->infobgd, sat->nickname,
                                    alsstr, cm, m, cs, s);
        }

        g_free (ch);
        g_free (cm);
        g_free (cs);
        g_free (alsstr);
    }

    /* update info text */
    g_object_set (satmap->sel, "text", text, NULL);
    g_free (text);
}


/** \brief Add grid lines and labels to the map. */
static void
draw_grid_lines (GtkSatMap *satmap, GooCanvasItemModel *root)
{
    gdouble        xstep, ystep;
    gfloat        lon, lat;
    guint32        col;
    guint        i;
    gchar        *buf, hmf = ' ';

    /* initialize algo parameters */
    col = mod_cfg_get_int (satmap->cfgdata,
                           MOD_CFG_MAP_SECTION,
                           MOD_CFG_MAP_GRID_COL,
                           SAT_CFG_INT_MAP_GRID_COL);

    xstep = (gdouble) (30.0 * satmap->width / 360.0);
    ystep = (gdouble) (30.0 * satmap->height / 180.0);

    /* horizontal grid */
    for (i = 0; i < 5; i++) {
        /* line */
        satmap->gridh[i] = goo_canvas_polyline_model_new_line (root,
                                                               (gdouble) satmap->x0,
                                                               (gdouble) (satmap->y0 + (i+1)*ystep),
                                                               (gdouble) (satmap->x0 + satmap->width),
                                                               (gdouble) (satmap->y0 + (i+1)*ystep),
                                                               "stroke-color-rgba", col,
                                                               "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                               "line-join", CAIRO_LINE_JOIN_MITER,
                                                               "line-width", 0.5,
                                                               NULL);

        /* FIXME: Use dotted line pattern? */

        /* label */
        xy_to_lonlat (satmap, satmap->x0, satmap->y0 + (i+1)*ystep, &lon, &lat);
        if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_NSEW)) {
            if (lat < 0.00) {
                lat = -lat;
                hmf = 'S';
            }
            else {
                hmf = 'N';
            }
        }
        buf = g_strdup_printf ("%.0f\302\260%c", lat, hmf);
        satmap->gridhlab[i] = goo_canvas_text_model_new (root, 
                                                         buf,
                                                         (gdouble) (satmap->x0 + 15),
                                                         (gdouble) (satmap->y0 + (i+1)*ystep),
                                                         -1, GTK_ANCHOR_NORTH,
                                                         "font", "Sans 8", "fill-color-rgba", col,
                                                         NULL);
        g_free (buf);

        /* lower items to be just above the background map
           or below it if lines are invisible
        */
        if (satmap->showgrid) {
            goo_canvas_item_model_raise (satmap->gridh[i], satmap->map);
            goo_canvas_item_model_raise (satmap->gridhlab[i], satmap->map);
        }
        else {
            goo_canvas_item_model_lower (satmap->gridh[i], satmap->map);
            goo_canvas_item_model_lower (satmap->gridhlab[i], satmap->map);
        }
    }

    /* vertical grid */
    for (i = 0; i < 11; i++) {
        /* line */
        satmap->gridv[i] = goo_canvas_polyline_model_new_line (root,
                                                               (gdouble) (satmap->x0 + (i+1)*xstep),
                                                               (gdouble) satmap->y0,
                                                               (gdouble) (satmap->x0 + (i+1)*xstep),
                                                               (gdouble) (satmap->y0 + satmap->height),
                                                               "stroke-color-rgba", col,
                                                               "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                               "line-join", CAIRO_LINE_JOIN_MITER,
                                                               "line-width", 0.5,
                                                               NULL);

        /* label */
        xy_to_lonlat (satmap, satmap->x0 + (i+1)*xstep, satmap->y0, &lon, &lat);
        if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_NSEW)) {
            if (lon < 0.00) {
                lon = -lon;
                hmf = 'W';
            }
            else {
                hmf = 'E';
            }
        }
        buf = g_strdup_printf ("%.0f\302\260%c", lon, hmf);
        satmap->gridvlab[i] = goo_canvas_text_model_new (root, 
                                                         buf,
                                                         (gdouble) (satmap->x0 + (i+1)*xstep),
                                                         (gdouble) (satmap->y0 + satmap->height - 5),
                                                         -1, GTK_ANCHOR_EAST,
                                                         "font", "Sans 8", "fill-color-rgba", col,
                                                         NULL);
        g_free (buf);

        /* lower items to be just above the background map
           or below it if lines are invisible
        */
        if (satmap->showgrid) {
            goo_canvas_item_model_raise (satmap->gridv[i], satmap->map);
            goo_canvas_item_model_raise (satmap->gridvlab[i], satmap->map);
        }
        else {
            goo_canvas_item_model_lower (satmap->gridv[i], satmap->map);
            goo_canvas_item_model_lower (satmap->gridvlab[i], satmap->map);
        }
    }
}


/** \brief Redraw grid lines and labels. */
static void redraw_grid_lines (GtkSatMap *satmap)
{
    GooCanvasPoints *line;
    gdouble xstep, ystep;
    guint i;

    xstep = (gdouble) 30.0 * ((gdouble) satmap->width) / 360.0;
    ystep = (gdouble) 30.0 * ((gdouble) satmap->height) / 180.0;

    /* horizontal grid */
    for (i = 0; i < 5; i++) {

        /* update line */
        line = goo_canvas_points_new (2);
        line->coords[0] = (gdouble) satmap->x0;
        line->coords[1] = (gdouble) (satmap->y0 + (i+1)*ystep);
        line->coords[2] = (gdouble) (satmap->x0 + satmap->width);
        line->coords[3] = (gdouble) (satmap->y0 + (i+1)*ystep);
        g_object_set (satmap->gridh[i], "points", line, NULL);
        goo_canvas_points_unref (line);

        /* update label */
        g_object_set (satmap->gridhlab[i],
                      "x", (gdouble) (satmap->x0 + 15),
                      "y", (gdouble) (satmap->y0 + (i+1)*ystep),
                      NULL);

        /* lower items to be just above the background map
           or below it if lines are invisible
        */
        if (satmap->showgrid) {
            goo_canvas_item_model_raise (satmap->gridh[i], satmap->map);
            goo_canvas_item_model_raise (satmap->gridhlab[i], satmap->map);
        }
        else
            goo_canvas_item_model_lower (satmap->gridh[i], satmap->map);
    }

    /* vertical grid */
    for (i = 0; i < 11; i++) {
        /* update line */
        line = goo_canvas_points_new (2);
        line->coords[0] = (gdouble) (satmap->x0 + (i+1)*xstep);
        line->coords[1] = (gdouble) satmap->y0;
        line->coords[2] = (gdouble) (satmap->x0 + (i+1)*xstep);
        line->coords[3] = (gdouble) (satmap->y0 + satmap->height);
        g_object_set (satmap->gridv[i], "points", line, NULL);
        goo_canvas_points_unref (line);

        /* update label */
        g_object_set (satmap->gridvlab[i],
                      "x", (gdouble) (satmap->x0 + (i+1)*xstep),
                      "y", (gdouble) (satmap->y0 + satmap->height - 5),
                      NULL);

        /* lower items to be just above the background map
           or below it if lines are invisible
        */
        if (satmap->showgrid) {
            goo_canvas_item_model_raise (satmap->gridv[i], satmap->map);
            goo_canvas_item_model_raise (satmap->gridvlab[i], satmap->map);
        }
        else {
            goo_canvas_item_model_lower (satmap->gridv[i], satmap->map);
            goo_canvas_item_model_lower (satmap->gridvlab[i], satmap->map);
        }
    }
}


/** \brief  Public wrapper for private conversion function. */
void gtk_sat_map_lonlat_to_xy (GtkSatMap *m,
                               gdouble lon, gdouble lat,
                               gdouble *x, gdouble *y)
{
    gfloat fx,fy;

    fx = (gfloat) *x;
    fy = (gfloat) *y;

    lonlat_to_xy (m, lon, lat, &fx, &fy);

    *x = (gdouble) fx;
    *y = (gdouble) fy;
}


/** \brief Reload reference to satellites (e.g. after TLE update). */
void
gtk_sat_map_reload_sats (GtkWidget *satmap, GHashTable *sats)
{
    GTK_SAT_MAP (satmap)->sats = sats;
    GTK_SAT_MAP (satmap)->naos = 0.0;
    GTK_SAT_MAP (satmap)->ncat = 0;
}

