/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
/** \brief Polar View Widget.
 *
 * More info...
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "mod-cfg-get-param.h"
#include "gtk-sat-data.h"
#include "gpredict-utils.h"
#include "gtk-polar-view-popup.h"
#include "gtk-polar-view.h"
#include "sat-info.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include <goocanvas.h>



#define POLV_DEFAULT_SIZE 100
#define POLV_DEFAULT_MARGIN 25

/* extra size for line outside 0 deg circle (inside margin) */
#define POLV_LINE_EXTRA 5

#define MARKER_SIZE_HALF 2


static void gtk_polar_view_class_init  (GtkPolarViewClass *class);
static void gtk_polar_view_init        (GtkPolarView *polview);
static void gtk_polar_view_destroy     (GtkObject *object);
static void size_allocate_cb           (GtkWidget *widget,
                                        GtkAllocation *allocation,
                                        gpointer data);
static void update_sat                 (gpointer key, gpointer value, gpointer data);
static void update_track               (gpointer key, gpointer value, gpointer data);
static void create_track               (GtkPolarView *pv, sat_obj_t *obj, sat_t *sat);
static void correct_pole_coor          (GtkPolarView *polv, polar_view_pole_t pole,
                                        gfloat *x, gfloat *y, GtkAnchorType *anch);
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
static void clear_selection            (gpointer key, gpointer val, gpointer data);
static GooCanvasItemModel* create_canvas_model (GtkPolarView *polv);
static void get_canvas_bg_color        (GtkPolarView *polv, GdkColor *color);
static gchar *los_time_to_str (GtkPolarView *polv, sat_t *sat);


static GtkVBoxClass *parent_class = NULL;


GtkType
gtk_polar_view_get_type ()
{
    static GType gtk_polar_view_type = 0;

    if (!gtk_polar_view_type) {
        static const GTypeInfo gtk_polar_view_info = {
            sizeof (GtkPolarViewClass),
            NULL, /* base init */
            NULL, /* base finalise */
            (GClassInitFunc) gtk_polar_view_class_init,
            NULL, /* class finalise */
            NULL, /* class data */
            sizeof (GtkPolarView),
            5,  /* n_preallocs */
            (GInstanceInitFunc) gtk_polar_view_init,
        };

        gtk_polar_view_type = g_type_register_static (GTK_TYPE_VBOX,
                                                      "GtkPolarView",
                                                      &gtk_polar_view_info,
                                                      0);
    }

    return gtk_polar_view_type;
}


static void
gtk_polar_view_class_init (GtkPolarViewClass *class)
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

    object_class->destroy = gtk_polar_view_destroy;
    //widget_class->size_allocate = gtk_polar_view_size_allocate;
}


static void
gtk_polar_view_init (GtkPolarView *polview)
{
    polview->sats      = NULL;
    polview->qth       = NULL;
    polview->obj       = NULL;
    polview->naos      = 2458849.5;
    polview->ncat      = 0;
    polview->size      = 0;
    polview->r         = 0;
    polview->cx        = 0;
    polview->cy        = 0;
    polview->refresh   = 0;
    polview->counter   = 0;
    polview->swap      = 0;
    polview->qthinfo   = FALSE;
    polview->eventinfo = FALSE;
    polview->cursinfo  = FALSE;
    polview->extratick = FALSE;
    polview->resize    = FALSE;
}


static void
gtk_polar_view_destroy (GtkObject *object)
{

    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/** \brief Create a new GtkPolarView widget.
 *  \param cfgdata The configuration data of the parent module.
 *  \param sats Pointer to the hash table containing the asociated satellites.
 *  \param qth Pointer to the ground station data.
 */
GtkWidget *
gtk_polar_view_new (GKeyFile *cfgdata, GHashTable *sats, qth_t *qth)
{
    GtkWidget *polv;
    GooCanvasItemModel *root;
    GdkColor bg_color = {0, 0xFFFF, 0xFFFF, 0xFFFF};

    polv = g_object_new (GTK_TYPE_POLAR_VIEW, NULL);

    GTK_POLAR_VIEW (polv)->cfgdata = cfgdata;
    GTK_POLAR_VIEW (polv)->sats = sats;
    GTK_POLAR_VIEW (polv)->qth  = qth;


    GTK_POLAR_VIEW (polv)->obj  = g_hash_table_new_full (g_int_hash,
                                                         g_int_equal,
                                                         g_free,
                                                         NULL);

    /* get settings */
    GTK_POLAR_VIEW (polv)->refresh = mod_cfg_get_int (cfgdata,
                                                      MOD_CFG_POLAR_SECTION,
                                                      MOD_CFG_POLAR_REFRESH,
                                                      SAT_CFG_INT_POLAR_REFRESH);

    GTK_POLAR_VIEW (polv)->showtrack = mod_cfg_get_bool (cfgdata,
                                                         MOD_CFG_POLAR_SECTION,
                                                         MOD_CFG_POLAR_SHOW_TRACK_AUTO,
                                                         SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO);


    GTK_POLAR_VIEW (polv)->counter = 1;

    GTK_POLAR_VIEW (polv)->swap = mod_cfg_get_int (cfgdata,
                                                   MOD_CFG_POLAR_SECTION,
                                                   MOD_CFG_POLAR_ORIENTATION,
                                                   SAT_CFG_INT_POLAR_ORIENTATION);

    GTK_POLAR_VIEW (polv)->qthinfo = mod_cfg_get_bool (cfgdata,
                                                       MOD_CFG_POLAR_SECTION,
                                                       MOD_CFG_POLAR_SHOW_QTH_INFO,
                                                       SAT_CFG_BOOL_POL_SHOW_QTH_INFO);

    GTK_POLAR_VIEW (polv)->eventinfo = mod_cfg_get_bool (cfgdata,
                                                         MOD_CFG_POLAR_SECTION,
                                                         MOD_CFG_POLAR_SHOW_NEXT_EVENT,
                                                         SAT_CFG_BOOL_POL_SHOW_NEXT_EV);

    GTK_POLAR_VIEW (polv)->cursinfo = mod_cfg_get_bool (cfgdata,
                                                        MOD_CFG_POLAR_SECTION,
                                                        MOD_CFG_POLAR_SHOW_CURS_TRACK,
                                                        SAT_CFG_BOOL_POL_SHOW_CURS_TRACK);

    GTK_POLAR_VIEW (polv)->extratick = mod_cfg_get_bool (cfgdata,
                                                         MOD_CFG_POLAR_SECTION,
                                                         MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS,
                                                         SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);

    /* create the canvas */
    GTK_POLAR_VIEW (polv)->canvas = goo_canvas_new ();
    g_object_set (G_OBJECT (GTK_POLAR_VIEW (polv)->canvas), "has-tooltip", TRUE, NULL);
    get_canvas_bg_color (GTK_POLAR_VIEW (polv), &bg_color);
    gtk_widget_modify_base (GTK_POLAR_VIEW (polv)->canvas, GTK_STATE_NORMAL, &bg_color);
    gtk_widget_set_size_request (GTK_POLAR_VIEW (polv)->canvas,
                                 POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);
    goo_canvas_set_bounds (GOO_CANVAS (GTK_POLAR_VIEW (polv)->canvas), 0, 0,
                           POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);


    /* connect size-request signal */
    g_signal_connect (GTK_POLAR_VIEW (polv)->canvas, "size-allocate",
                      G_CALLBACK (size_allocate_cb), polv);
    g_signal_connect (GTK_POLAR_VIEW (polv)->canvas, "item_created",
                      (GtkSignalFunc) on_item_created, polv);
    g_signal_connect_after (GTK_POLAR_VIEW (polv)->canvas, "realize",
                            (GtkSignalFunc) on_canvas_realized, polv);

    gtk_widget_show (GTK_POLAR_VIEW (polv)->canvas);

    /* Create the canvas model */
    root = create_canvas_model (GTK_POLAR_VIEW (polv));
    goo_canvas_set_root_item_model (GOO_CANVAS (GTK_POLAR_VIEW (polv)->canvas), root);

    g_object_unref (root);

    //gtk_box_pack_start (GTK_BOX (polv), GTK_POLAR_VIEW (polv)->swin, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (polv), GTK_POLAR_VIEW (polv)->canvas);

    return polv;
}


static GooCanvasItemModel *
create_canvas_model (GtkPolarView *polv)
{
    GooCanvasItemModel *root;
    gfloat x,y;
    guint32 col;
    GtkAnchorType anch = GTK_ANCHOR_CENTER;

    root = goo_canvas_group_model_new (NULL, NULL);

    /* graph dimensions */
    polv->size = POLV_DEFAULT_SIZE;
    polv->r = (polv->size / 2) - POLV_DEFAULT_MARGIN;
    polv->cx = POLV_DEFAULT_SIZE / 2;
    polv->cy = POLV_DEFAULT_SIZE / 2;

    col = mod_cfg_get_int (polv->cfgdata,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_AXIS_COL,
                           SAT_CFG_INT_POLAR_AXIS_COL);


    /* Add elevation circles at 0, 30 and 60 deg */
    polv->C00 = goo_canvas_ellipse_model_new (root,
                                              polv->cx, polv->cy,
                                              polv->r, polv->r,
                                              "line-width", 1.0,
                                              "stroke-color-rgba", col,
                                              NULL);

    polv->C30 = goo_canvas_ellipse_model_new (root,
                                              polv->cx, polv->cy,
                                              0.6667 * polv->r, 0.6667 * polv->r,
                                              "line-width", 1.0,
                                              "stroke-color-rgba", col,
                                              NULL);

    polv->C60 = goo_canvas_ellipse_model_new (root,
                                              polv->cx, polv->cy,
                                              0.333 * polv->r, 0.3333 * polv->r,
                                              "line-width", 1.0,
                                              "stroke-color-rgba", col,
                                              NULL);

    /* add horixontal and vertical guidance lines */
    polv->hl = goo_canvas_polyline_model_new_line (root,
                                                   polv->cx - polv->r - POLV_LINE_EXTRA,
                                                   polv->cy,
                                                   polv->cx + polv->r + POLV_LINE_EXTRA,
                                                   polv->cy,
                                                   "stroke-color-rgba", col,
                                                   "line-width", 1.0,
                                                   NULL);

    polv->vl = goo_canvas_polyline_model_new_line (root,
                                                   polv->cx,
                                                   polv->cy - polv->r - POLV_LINE_EXTRA,
                                                   polv->cx,
                                                   polv->cy + polv->r + POLV_LINE_EXTRA,
                                                   "stroke-color-rgba", col,
                                                   "line-width", 1.0,
                                                   NULL);

    /* N, S, E and W labels.  */
    col = mod_cfg_get_int (polv->cfgdata,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_TICK_COL,
                           SAT_CFG_INT_POLAR_TICK_COL);
    azel_to_xy (polv, 0.0, 0.0, &x, &y);
    correct_pole_coor (polv, POLAR_VIEW_POLE_N, &x, &y, &anch);
    polv->N = goo_canvas_text_model_new (root, _("N"),
                                         x,
                                         y,
                                         -1,
                                         anch,
                                         "font", "Sans 10",
                                         "fill-color-rgba", col,
                                         NULL);

    azel_to_xy (polv, 180.0, 0.0, &x, &y);
    correct_pole_coor (polv, POLAR_VIEW_POLE_S, &x, &y, &anch);
    polv->S = goo_canvas_text_model_new (root, _("S"),
                                         x,
                                         y,
                                         -1,
                                         anch,
                                         "font", "Sans 10",
                                         "fill-color-rgba", col,
                                         NULL);

    azel_to_xy (polv, 90.0, 0.0, &x, &y);
    correct_pole_coor (polv, POLAR_VIEW_POLE_E, &x, &y, &anch);
    polv->E = goo_canvas_text_model_new (root, _("E"),
                                         x,
                                         y,
                                         -1,
                                         anch,
                                         "font", "Sans 10",
                                         "fill-color-rgba", col,
                                         NULL);

    azel_to_xy (polv, 270.0, 0.0, &x, &y);
    correct_pole_coor (polv, POLAR_VIEW_POLE_W, &x, &y, &anch);
    polv->W = goo_canvas_text_model_new (root, _("W"),
                                         x,
                                         y,
                                         -1,
                                         anch,
                                         "font", "Sans 10",
                                         "fill-color-rgba", col,
                                         NULL);

    /* cursor text */
    col = mod_cfg_get_int (polv->cfgdata,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_INFO_COL,
                           SAT_CFG_INT_POLAR_INFO_COL);
    polv->curs = goo_canvas_text_model_new (root, "",
                                            polv->cx - polv->r - 2*POLV_LINE_EXTRA,
                                            polv->cy + polv->r + POLV_LINE_EXTRA,
                                            -1,
                                            GTK_ANCHOR_W,
                                            "font", "Sans 8",
                                            "fill-color-rgba", col,
                                            NULL);

    /* location info */
    polv->locnam = goo_canvas_text_model_new (root, polv->qth->name,
                                              polv->cx - polv->r - 2*POLV_LINE_EXTRA,
                                              polv->cy - polv->r - POLV_LINE_EXTRA,
                                              -1,
                                              GTK_ANCHOR_SW,
                                              "font", "Sans 8",
                                              "fill-color-rgba", col,
                                              NULL);

    /* next event */
    polv->next = goo_canvas_text_model_new (root, "",
                                            polv->cx + polv->r + 2*POLV_LINE_EXTRA,
                                            polv->cy - polv->r - POLV_LINE_EXTRA,
                                            -1,
                                            GTK_ANCHOR_E,
                                            "font", "Sans 8",
                                            "fill-color-rgba", col,
                                            "alignment", PANGO_ALIGN_RIGHT,
                                            NULL);

    /* selected satellite text */
    polv->sel = goo_canvas_text_model_new (root, "",
                                           polv->cx + polv->r + 2*POLV_LINE_EXTRA,
                                           polv->cy + polv->r + POLV_LINE_EXTRA,
                                           -1,
                                           GTK_ANCHOR_E,
                                           "font", "Sans 8",
                                           "fill-color-rgba", col,
                                           "alignment", PANGO_ALIGN_RIGHT,
                                           NULL);

    return root;
}


/** \brief Transform pole coordinates.
 *
 * This function transforms the pols coordinates (x,y) taking into account
 * the orientation of the polar plot.
 */
static void
correct_pole_coor          (GtkPolarView *polv,
                            polar_view_pole_t pole,
                            gfloat *x, gfloat *y,
                            GtkAnchorType *anch)
{

    switch (pole) {

    case POLAR_VIEW_POLE_N:
        if ((polv->swap == POLAR_VIEW_SENW) ||
            (polv->swap == POLAR_VIEW_SWNE)) {
            /* North and South are swapped */
            *y = *y + POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_NORTH;
        }
        else {
            *y = *y - POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_SOUTH;
        }

        break;

    case POLAR_VIEW_POLE_E:
        if ((polv->swap == POLAR_VIEW_NWSE) ||
            (polv->swap == POLAR_VIEW_SWNE)) {
            /* East and West are swapped */
            *x = *x - POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_EAST;
        }
        else {
            *x = *x + POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_WEST;
        }
        break;

    case POLAR_VIEW_POLE_S:
        if ((polv->swap == POLAR_VIEW_SENW) ||
            (polv->swap == POLAR_VIEW_SWNE)) {
            /* North and South are swapped */
            *y = *y - POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_SOUTH;
        }
        else {
            *y = *y + POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_NORTH;
        }
        break;

    case POLAR_VIEW_POLE_W:
        if ((polv->swap == POLAR_VIEW_NWSE) ||
            (polv->swap == POLAR_VIEW_SWNE)) {
            /* East and West are swapped */
            *x = *x + POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_WEST;
        }
        else {
            *x = *x - POLV_LINE_EXTRA;
            *anch = GTK_ANCHOR_EAST;
        }
        break;

    default:
        /* FIXME: bug */
        break;
    }
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
    GTK_POLAR_VIEW (data)->resize = TRUE;
}


static void
update_polv_size (GtkPolarView *polv)
{
    //GtkPolarView *polv;
    GtkAllocation allocation;
    GooCanvasPoints *prec;
    gfloat x,y;
    GtkAnchorType anch = GTK_ANCHOR_CENTER;


    if (GTK_WIDGET_REALIZED (polv)) {
        
        /* get graph dimensions */
        allocation.width = GTK_WIDGET (polv)->allocation.width;
        allocation.height = GTK_WIDGET (polv)->allocation.height;

        polv->size = MIN(allocation.width, allocation.height);
        polv->r = (polv->size / 2) - POLV_DEFAULT_MARGIN;
        polv->cx = allocation.width / 2;
        polv->cy = allocation.height / 2;

        /* update canvas bounds to match new size */
        goo_canvas_set_bounds (GOO_CANVAS (GTK_POLAR_VIEW (polv)->canvas), 0, 0,
                                allocation.width, allocation.height);

        
        /* update coordinate system */
        g_object_set (polv->C00,
                      "center-x", (gdouble) polv->cx,
                      "center-y", (gdouble) polv->cy,
                      "radius-x", (gdouble) polv->r,
                      "radius-y", (gdouble) polv->r,
                      NULL);
        g_object_set (polv->C30,
                      "center-x", (gdouble) polv->cx,
                      "center-y", (gdouble) polv->cy,
                      "radius-x", (gdouble) 0.6667*polv->r,
                      "radius-y", (gdouble) 0.6667*polv->r,
                      NULL);
        g_object_set (polv->C60,
                      "center-x", (gdouble) polv->cx,
                      "center-y", (gdouble) polv->cy,
                      "radius-x", (gdouble) 0.333*polv->r,
                      "radius-y", (gdouble) 0.333*polv->r,
                      NULL);

        /* horizontal line */
        prec = goo_canvas_points_new (2);
        prec->coords[0] = polv->cx - polv->r - POLV_LINE_EXTRA;
        prec->coords[1] = polv->cy;
        prec->coords[2] = polv->cx + polv->r + POLV_LINE_EXTRA;
        prec->coords[3] = polv->cy;
        g_object_set (polv->hl,
                      "points", prec,
                      NULL);

        /* vertical line */
        prec->coords[0] = polv->cx;
        prec->coords[1] = polv->cy - polv->r - POLV_LINE_EXTRA;
        prec->coords[2] = polv->cx;
        prec->coords[3] = polv->cy + polv->r + POLV_LINE_EXTRA;
        g_object_set (polv->vl,
                      "points", prec,
                      NULL);

        /* free memory */
        goo_canvas_points_unref (prec);
        
        /* N/E/S/W */
        azel_to_xy (polv, 0.0, 0.0, &x, &y);
        correct_pole_coor (polv, POLAR_VIEW_POLE_N, &x, &y, &anch);
        g_object_set (polv->N,
                      "x", x,
                      "y", y,
                      NULL);

        azel_to_xy (polv, 90.0, 0.0, &x, &y);
        correct_pole_coor (polv, POLAR_VIEW_POLE_E, &x, &y, &anch);
        g_object_set (polv->E,
                      "x", x,
                      "y", y,
                      NULL);

        azel_to_xy (polv, 180.0, 0.0, &x, &y);
        correct_pole_coor (polv, POLAR_VIEW_POLE_S, &x, &y, &anch);
        g_object_set (polv->S,
                      "x", x,
                      "y", y,
                      NULL);

        azel_to_xy (polv, 270.0, 0.0, &x, &y);
        correct_pole_coor (polv, POLAR_VIEW_POLE_W, &x, &y, &anch);
        g_object_set (polv->W,
                      "x", x,
                      "y", y,
                      NULL);

        /* cursor track */
        g_object_set (polv->curs,
                      "x", (gfloat) (polv->cx - polv->r - 2*POLV_LINE_EXTRA),
                      "y", (gfloat) (polv->cy + polv->r + POLV_LINE_EXTRA),
                      NULL);

        /* location name */
        g_object_set (polv->locnam,
                      "x", (gfloat) (polv->cx - polv->r - 2*POLV_LINE_EXTRA),
                      "y", (gfloat) (polv->cy - polv->r - POLV_LINE_EXTRA),
                      NULL);

        /* next event */
        g_object_set (polv->next,
                      "x", (gfloat) (polv->cx + polv->r + 2*POLV_LINE_EXTRA),
                      "y", (gfloat) (polv->cy - polv->r - POLV_LINE_EXTRA),
                      NULL);

        /* selection info */
        g_object_set (polv->sel,
                      "x", (gfloat) polv->cx + polv->r + 2*POLV_LINE_EXTRA,
                      "y", (gfloat) polv->cy + polv->r + POLV_LINE_EXTRA,
                      NULL);

        g_hash_table_foreach (polv->sats, update_sat, polv);

        /* sky tracks */
        g_hash_table_foreach (polv->obj, update_track, polv);

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


void
gtk_polar_view_update          (GtkWidget  *widget)
{
    GtkPolarView *polv = GTK_POLAR_VIEW (widget);
    gdouble       number, now;
    gchar        *buff;
    guint         h,m,s;
    gchar        *ch,*cm,*cs;
    sat_t        *sat = NULL;
    gint         *catnr;

    if (polv->resize) {
        update_polv_size (polv);
        polv->resize = FALSE;
    }
    
    /* check refresh rate and refresh sats if time */
    if (polv->counter < polv->refresh) {
        polv->counter++;
    }
    else {
        /* reset data */
        polv->counter = 1;
        polv->naos = 2458849.5;

        /* update sats */
        g_hash_table_foreach (polv->sats, update_sat, polv);

        /* update countdown to NEXT AOS label */
        if (polv->eventinfo) {

            if (polv->ncat > 0) {
                catnr = g_try_new0 (gint, 1);
                *catnr = polv->ncat;
                sat = SAT(g_hash_table_lookup (polv->sats, catnr));
                g_free (catnr);

                /* last desperate sanity check */
                if (sat != NULL) {

                    now = polv->tstamp; //get_current_daynum ();
                    number = polv->naos - now;

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
                        buff = g_strdup_printf (_("Next: %s\nin %s%d:%s%d%s%d"),
                                                sat->nickname, ch, h, cm, m, cs, s);
                    else
                        buff = g_strdup_printf (_("Next: %s\nin %s%d%s%d"),
                                                sat->nickname, cm, m, cs, s);
                    
                    
                    g_object_set (polv->next,
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
                    g_object_set (polv->next,
                                  "text", _("Next: ERR"),
                                  NULL);
                }
            }
            else {
                g_object_set (polv->next,
                              "text", _("Next: N/A"),
                              NULL);
            }
        }
        else {
            g_object_set (polv->next,
                          "text", "",
                          NULL);
        }

    }
}


static void
update_sat    (gpointer key, gpointer value, gpointer data)
{
    gint               *catnum;
    sat_t              *sat = SAT(value);
    GtkPolarView       *polv = GTK_POLAR_VIEW (data);
    sat_obj_t          *obj = NULL;
    gfloat             x,y;
    GooCanvasItemModel *root;
    gint               idx,i;
    gdouble            now;// = get_current_daynum ();
    gchar             *text;
    gchar             *losstr;
    gchar             *tooltip;
    guint32            colour;


    catnum = g_new0 (gint, 1);
    *catnum = sat->tle.catnr;

    now = polv->tstamp;

    /* update next AOS */
    if ((sat->aos > now) && (sat->aos < polv->naos)) {
        polv->naos = sat->aos;
        polv->ncat = sat->tle.catnr;
    }

    /* if sat is out of range */
    if (sat->el < 0.00) {

        obj = SAT_OBJ(g_hash_table_lookup (polv->obj, catnum));

        /* if sat is on canvas */
        if (obj != NULL) {

            /* remove sat from canvas */
            root = goo_canvas_get_root_item_model (GOO_CANVAS (polv->canvas));

            idx = goo_canvas_item_model_find_child (root, obj->marker);
            if (idx != -1) {
                goo_canvas_item_model_remove_child (root, idx);
            }

            idx = goo_canvas_item_model_find_child (root, obj->label);
            if (idx != -1) {
                goo_canvas_item_model_remove_child (root, idx);
            }

            /* remove sky track */
            if (obj->showtrack) {
                idx = goo_canvas_item_model_find_child (root, obj->track);
                if (idx != -1)
                    goo_canvas_item_model_remove_child (root, idx);

                for (i = 0; i < TRACK_TICK_NUM; i++) {
                    idx = goo_canvas_item_model_find_child (root, obj->trtick[i]);
                    if (idx != -1)
                        goo_canvas_item_model_remove_child (root, idx);
                }
            }
            
            /* free pass info */
            free_pass (obj->pass);

            /* if this was the selected satellite we need to
               clear the info text
            */
            if (obj->selected) {
                g_object_set (polv->sel, "text", "", NULL);
            }

            g_free (obj);

            /* remove sat object from hash table */ 
            g_hash_table_remove (polv->obj, catnum);

            /* FIXME: remove track from chart */

        }

        g_free (catnum);
    }

    /* sat is within range */
    else {

        obj = SAT_OBJ (g_hash_table_lookup (polv->obj, catnum));
        azel_to_xy (polv, sat->az, sat->el, &x, &y);

        /* if sat is already on canvas */
        if (obj != NULL) {

            /* update LOS count down */
            if (sat->los > 0.0) {
                losstr = los_time_to_str(polv, sat);
            }
            else {
                losstr = g_strdup_printf (_("%s\nAlways in range"), sat->nickname);
            }

            /* update tooltip */
            tooltip = g_strdup_printf("<big><b>%s</b>\n</big>"\
                                      "<tt>Az: %5.1f\302\260\n" \
                                      "El: %5.1f\302\260\n" \
                                      "%s</tt>",
                                      sat->nickname,
                                      sat->az, sat->el,
                                      losstr);

            g_object_set (obj->marker,
                          "x", x - MARKER_SIZE_HALF,
                          "y", y - MARKER_SIZE_HALF,
                          "tooltip", tooltip,
                          NULL);
            g_object_set (obj->label,
                          "x", x,
                          "y", y+2,
                          "tooltip", tooltip,
                          NULL);

            g_free (tooltip);

            /* update selection info if satellite is
               selected
            */
            if (obj->selected) {
                text = g_strdup_printf ("%s\n%s",sat->nickname, losstr);
                g_object_set (polv->sel, "text", text, NULL);
                g_free (text);
            }

            g_free (losstr);
            g_free (catnum);  // FIXME: why free here, what about else?
        }
        else {
            /* add sat to canvas */
            obj = g_try_new (sat_obj_t, 1);
            obj->selected = FALSE;
            obj->showtrack = polv->showtrack;
            obj->istarget = FALSE;

            root = goo_canvas_get_root_item_model (GOO_CANVAS (polv->canvas));

            colour = mod_cfg_get_int (polv->cfgdata,
                                      MOD_CFG_POLAR_SECTION,
                                      MOD_CFG_POLAR_SAT_COL,
                                      SAT_CFG_INT_POLAR_SAT_COL);

            /* create tooltip */
            tooltip = g_strdup_printf("<big><b>%s</b>\n</big>"\
                                      "<tt>Az: %5.1f\302\260\n" \
                                      "El: %5.1f\302\260\n" \
                                      "</tt>",
                                      sat->nickname,
                                      sat->az, sat->el);

            obj->marker = goo_canvas_rect_model_new (root,
                                                     x - MARKER_SIZE_HALF,
                                                     y - MARKER_SIZE_HALF,
                                                     2*MARKER_SIZE_HALF,
                                                     2*MARKER_SIZE_HALF,
                                                     "fill-color-rgba", colour,
                                                     "stroke-color-rgba", colour,
                                                     "tooltip", tooltip,
                                                     NULL);
            obj->label = goo_canvas_text_model_new (root, sat->nickname,
                                                    x,
                                                    y+2,
                                                    -1,
                                                    GTK_ANCHOR_NORTH,
                                                    "font", "Sans 8",
                                                    "fill-color-rgba", colour,
                                                    "tooltip", tooltip,
                                                    NULL);

            g_free (tooltip);
            
            goo_canvas_item_model_raise (obj->marker, NULL);
            goo_canvas_item_model_raise (obj->label, NULL);

            g_object_set_data (G_OBJECT (obj->marker), "catnum", GINT_TO_POINTER(*catnum));
            g_object_set_data (G_OBJECT (obj->label), "catnum", GINT_TO_POINTER(*catnum));

            /* get info about the current pass */
            obj->pass = get_current_pass (sat, polv->qth, now);

            /* add sat to hash table */
            g_hash_table_insert (polv->obj, catnum, obj);

            /* Finally, create the sky track if necessary */
            if (obj->showtrack)
                create_track (polv, obj, sat);
        }

    }

}


/** \brief Update sky track drawing after size allocate. */
static void
update_track (gpointer key, gpointer value, gpointer data)
{
    sat_obj_t       *obj = SAT_OBJ(value);;
    GtkPolarView    *pv = GTK_POLAR_VIEW (data);
    guint            num,i;
    GooCanvasPoints *points;
    gfloat           x,y;
    pass_detail_t   *detail;
    guint            tres,ttidx;


    if (obj->showtrack) {
          if (obj->pass == NULL) {
               sat_log_log (SAT_LOG_LEVEL_BUG,
                               _("%s:%d: Failed to get satellite pass."),
                               __FILE__, __LINE__);
               return;
          }
          
        /* create points */
        num = g_slist_length (obj->pass->details);
        if (num == 0) {
               sat_log_log (SAT_LOG_LEVEL_BUG,
                               _("%s:%d: Pass had no points in it."),
                               __FILE__, __LINE__);
               return;
          }

        points = goo_canvas_points_new (num);

        /* first point should be (aos_az,0.0) */
        azel_to_xy (pv, obj->pass->aos_az, 0.0, &x, &y);
        points->coords[0] = (double) x;
        points->coords[1] = (double) y;

        /* time tick 0 */
        g_object_set (obj->trtick[0], "x", (gdouble) x, "y", (gdouble) y, NULL);

        /* time resolution for time ticks; we need
           3 additional points to AOS and LOS ticks.
        */
        tres = (num-2) / (TRACK_TICK_NUM-1);
        ttidx = 1;

        for (i = 1; i < num-1; i++) {
            detail = PASS_DETAIL(g_slist_nth_data (obj->pass->details, i));
            if (detail->el>=0)
                azel_to_xy (pv, detail->az, detail->el, &x, &y);
            points->coords[2*i] = (double) x;
            points->coords[2*i+1] = (double) y;

            if (!(i % tres)) {
                /* update time tick */
                if (ttidx<TRACK_TICK_NUM)
                    g_object_set (obj->trtick[ttidx],
                                  "x", (gdouble) x, "y", (gdouble) y,
                                  NULL);
                ttidx++;
            }
        }

        /* last point should be (los_az, 0.0)  */
        azel_to_xy (pv, obj->pass->los_az, 0.0, &x, &y);
        points->coords[2*(num-1)] = (double) x;
        points->coords[2*(num-1)+1] = (double) y;

        g_object_set (obj->track, "points", points, NULL);

        goo_canvas_points_unref (points);


    }
}


/**** FIXME: DUPLICATE from gtk-polar-view-popup.c - needed by create_track  ******/
static GooCanvasItemModel *create_time_tick (GtkPolarView *pv, gdouble time, gfloat x, gfloat y)
{
    GooCanvasItemModel *item;
    time_t             t;
    gchar              buff[7];
    GtkAnchorType      anchor;
    GooCanvasItemModel *root;
    guint32            col;

    root = goo_canvas_get_root_item_model (GOO_CANVAS (pv->canvas));

    col = mod_cfg_get_int (pv->cfgdata,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_TRACK_COL,
                           SAT_CFG_INT_POLAR_TRACK_COL);

    /* convert julian date to struct tm */
    t = (time - 2440587.5)*86400.;

    /* format either local time or UTC depending on check box */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
        strftime (buff, 8, "%H:%M", localtime (&t));
    else
        strftime (buff, 8, "%H:%M", gmtime (&t));

    buff[6]='\0';

    if (x > pv->cx) {
        anchor = GTK_ANCHOR_EAST;
        x -= 5;
    }
    else {
        anchor = GTK_ANCHOR_WEST;
        x += 5;
    }

    item = goo_canvas_text_model_new (root, buff,
                                      (gdouble) x, (gdouble) y,
                                      -1, anchor,
                                      "font", "Sans 7",
                                      "fill-color-rgba", col,
                                      NULL);

    goo_canvas_item_model_lower (item, NULL);

    return item;
}

/** \brief Create a sky track for a satellite.
 *  \param pv Pointer to the GtkPolarView object.
 *  \param obj Pointer to the sat_obj_t object.
 *  \param sat Pointer to the sat_t object.
 *
 * Note: This function is only used when the the satellite comes within range
 *       and the ALWAYS_SHOW_SKY_TRACK option is TRUE.
 */
static void create_track (GtkPolarView *pv, sat_obj_t *obj, sat_t *sat)
{
    gint               i;
    GooCanvasItemModel *root;
    pass_detail_t      *detail;
    guint              num;
    GooCanvasPoints    *points;
    gfloat             x,y;
    guint32            col;
    guint              tres,ttidx;


    /* get satellite object */
    /*obj = SAT_OBJ(g_object_get_data (G_OBJECT (item), "obj"));
    sat = SAT(g_object_get_data (G_OBJECT (item), "sat"));
    qth = (qth_t *)(g_object_get_data (G_OBJECT (item), "qth"));*/

    if (obj == NULL) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s:%d: Failed to get satellite object."),
                     __FILE__, __LINE__);
        return;
    }

    if (obj->pass == NULL) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s:%d: Failed to get satellite pass."),
                     __FILE__, __LINE__);
        return;
    }

    root = goo_canvas_get_root_item_model (GOO_CANVAS (pv->canvas));

    /* add sky track */

    /* create points */
    num = g_slist_length (obj->pass->details);
    if (num == 0) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s:%d: Pass had no points in it."),
                     __FILE__, __LINE__);
        return;
    }

    /* time resolution for time ticks; we need
                   3 additional points to AOS and LOS ticks.
                */
    tres = (num-2) / (TRACK_TICK_NUM-1);

    points = goo_canvas_points_new (num);
    

    
    /* first point should be (aos_az,0.0) */
    azel_to_xy (pv, obj->pass->aos_az, 0.0, &x, &y);
    points->coords[0] = (double) x;
    points->coords[1] = (double) y;
    obj->trtick[0] = create_time_tick (pv, obj->pass->aos, x, y);

    ttidx = 1;
    
    for (i = 1; i < num-1; i++) {
        detail = PASS_DETAIL(g_slist_nth_data (obj->pass->details, i));
        if (detail->el >= 0.0 )
            azel_to_xy (pv, detail->az, detail->el, &x, &y);
        points->coords[2*i] = (double) x;
        points->coords[2*i+1] = (double) y;

        if (!(i % tres)) {
            /* create a time tick */
            if (ttidx<TRACK_TICK_NUM)
                obj->trtick[ttidx] = create_time_tick (pv, detail->time, x, y);
            ttidx++;
        }
    }

    /* last point should be (los_az, 0.0)  */
    azel_to_xy (pv, obj->pass->los_az, 0.0, &x, &y);
    points->coords[2*(num-1)] = (double) x;
    points->coords[2*(num-1)+1] = (double) y;

    /* create poly-line */
    col = mod_cfg_get_int (pv->cfgdata,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_TRACK_COL,
                           SAT_CFG_INT_POLAR_TRACK_COL);

    obj->track = goo_canvas_polyline_model_new (root, FALSE, 0,
                                                "points", points,
                                                "line-width", 1.0,
                                                "stroke-color-rgba", col,
                                                "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                "line-join", CAIRO_LINE_JOIN_MITER,
                                                NULL);
    goo_canvas_points_unref (points);

    /* put track on the bottom of the sack */
    goo_canvas_item_model_lower (obj->track, NULL);

}





/** \brief Convert Az/El to canvas based XY coordinates. */
void
azel_to_xy (GtkPolarView *p, gdouble az, gdouble el, gfloat *x, gfloat *y)
{
    gdouble rel;


    if (el < 0.0) {
        /* FIXME: generate bug report */

        *x = 0.0;
        *y = 0.0;

        return;
    }

    /* convert angles to radians */
    az = de2ra*az;
    el = de2ra*el;

    /* radius @ el */
    rel = p->r - (2*p->r*el)/M_PI;

    switch (p->swap) {

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


/** \brief Convert canvas based coordinates to Az/El. */
void
xy_to_azel    (GtkPolarView *p, gfloat x, gfloat y, gfloat *az, gfloat *el)
{
    gfloat rel;

    /* distance from center to cursor */
    rel = p->r - sqrt((x - p->cx) * (x - p->cx) + (y - p->cy) * (y - p->cy));

    /* scale according to p->r = 90 deg */
    *el = 90.0 * rel / p->r;

    if (x >= p->cx) {
        /* 1. and 2. quadrant */
        *az = atan2 (x-p->cx, p->cy - y) / de2ra;
    }
    else {
        /* 3 and 4. quadrant */
        *az = 360 + atan2 (x-p->cx, p->cy - y) / de2ra;
    }

    /* correct for orientation */
    switch (p->swap) {

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


/** \brief Manage mouse motion events. */
static gboolean
on_motion_notify (GooCanvasItem *item,
                  GooCanvasItem *target,
                  GdkEventMotion *event,
                  gpointer data)
{
    GtkPolarView *polv = GTK_POLAR_VIEW (data);
    gfloat az,el;
    gchar *text;

    if (polv->cursinfo) {

        xy_to_azel (polv, event->x, event->y, &az, &el);

        if (el > 0.0) {
            /* cursor track */
            text = g_strdup_printf ("AZ %.0f\302\260\nEL %.0f\302\260",az,el);
            g_object_set (polv->curs, "text", text, NULL);
            g_free (text);
        }
        else {
            g_object_set (polv->curs, "text", "", NULL);
        }
    }

    return TRUE;
}


/** \brief Finish canvas item setup.
 *  \param canvas 
 *  \param item
 *  \param model 
 *  \param data Pointer to the GtkPolarView object.
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
    if (!goo_canvas_item_model_get_parent (model))    {
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
    GtkPolarView  *polv = GTK_POLAR_VIEW (data);
    gint catnum = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "catnum"));
    gint *catpoint = NULL;
    sat_t *sat = NULL;

    switch (event->button) {

        /* double-left-click */
    case 1:
        if (event->type == GDK_2BUTTON_PRESS) {
            catpoint = g_try_new0 (gint, 1);
            *catpoint = catnum;

            sat = SAT (g_hash_table_lookup (polv->sats, catpoint));
            if (sat != NULL) {
                show_sat_info(sat, gtk_widget_get_toplevel (GTK_WIDGET (data)));
            }
            else {
                /* double-clicked on map */
            }
        }

        g_free (catpoint);

        break;


        /* pop-up menu */
    case 3:
        catpoint = g_try_new0 (gint, 1);
        *catpoint = catnum;

        sat = SAT (g_hash_table_lookup (polv->sats, catpoint));

        if (sat != NULL) {
            gtk_polar_view_popup_exec (sat, polv->qth,
                                       polv, event,
                                       gtk_widget_get_toplevel (GTK_WIDGET (polv)));
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%d: Could not find satellite (%d) in hash table"),
                         __FILE__, __LINE__, catnum);
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
 * a satellite object. It will act as a button click and if the released
 * button is the left one, the click will correspond to selecting or
 * deselecting a satellite
 */
static gboolean
on_button_release (GooCanvasItem *item,
                   GooCanvasItem *target,
                   GdkEventButton *event,
                   gpointer data)
{
    GooCanvasItemModel *model = goo_canvas_item_get_model (item);
    GtkPolarView *polv = GTK_POLAR_VIEW (data);
    gint catnum = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "catnum"));
    gint *catpoint = NULL;
    sat_obj_t *obj = NULL;
    guint32  color;

    catpoint = g_try_new0 (gint, 1);
    *catpoint = catnum;

    switch (event->button) {

        /* Select / de-select satellite */
    case 1:
        obj = SAT_OBJ (g_hash_table_lookup (polv->obj, catpoint));
        if (obj == NULL) {
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%d: Can not find clicked object (%d) in hash table"),
                         __FILE__, __LINE__, catnum);
        }
        else {
            obj->selected = !obj->selected;


            if (obj->selected) {
                color = mod_cfg_get_int (polv->cfgdata,
                                         MOD_CFG_POLAR_SECTION,
                                         MOD_CFG_POLAR_SAT_SEL_COL,
                                         SAT_CFG_INT_POLAR_SAT_SEL_COL);
            }
            else {
                color = mod_cfg_get_int (polv->cfgdata,
                                         MOD_CFG_POLAR_SECTION,
                                         MOD_CFG_POLAR_SAT_COL,
                                         SAT_CFG_INT_POLAR_SAT_COL);
                *catpoint = 0;

                g_object_set (polv->sel, "text", "", NULL);
            }

            g_object_set (obj->marker,
                          "fill-color-rgba", color,
                          "stroke-color-rgba", color,
                          NULL);
            g_object_set (obj->label,
                          "fill-color-rgba", color,
                          "stroke-color-rgba", color,
                          NULL);

            /* clear other selections */
            g_hash_table_foreach (polv->obj, clear_selection, catpoint);
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
    sat_obj_t *obj = SAT_OBJ (val);
    guint32 col;

    if ((*old != *new) && (obj->selected)) {
        obj->selected = FALSE;

        col = sat_cfg_get_int (SAT_CFG_INT_POLAR_SAT_COL);

        g_object_set (obj->marker,
                      "fill-color-rgba", col,
                      "stroke-color-rgba", col,
                      NULL);
        g_object_set (obj->label,
                      "fill-color-rgba", col,
                      "stroke-color-rgba", col,
                      NULL);
    }
}


void
gtk_polar_view_reconf          (GtkWidget  *widget, GKeyFile *cfgdat)
{
}


/** \brief Retrieve background color.
 *
 * This function retrieves the canvas background color, which is in 0xRRGGBBAA
 * format and converts it to GdkColor style. Besides extractibg the RGB components
 * we also need to scale from [0;255] to [0;65535], i.e. multiply by 257.
 */
static void
get_canvas_bg_color (GtkPolarView *polv, GdkColor *color)
{
    guint32 col,tmp;
    guint16 r,g,b;


    col = mod_cfg_get_int (polv->cfgdata,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_BGD_COL,
                           SAT_CFG_INT_POLAR_BGD_COL);

    /* red */
    tmp = col & 0xFF000000;
    r = (guint16) (tmp >> 24);

    /* green */
    tmp = col & 0x00FF0000;
    g = (guint16) (tmp >> 16);

    /* blue */
    tmp = col & 0x0000FF00;
    b = (guint16) (tmp >> 8);

    /* store colours */
    color->red   = 257 * r;
    color->green = 257 * g;
    color->blue  = 257 * b;
}


/** \brief Reload reference to satellites (e.g. after TLE update). */
void
gtk_polar_view_reload_sats (GtkWidget *polv, GHashTable *sats)
{

    GTK_POLAR_VIEW (polv)->sats = sats;

    GTK_POLAR_VIEW (polv)->naos = 0.0;
    GTK_POLAR_VIEW (polv)->ncat = 0;
}



/** \brief Convert LOS timestamp to human readable countdown string */
static gchar *los_time_to_str (GtkPolarView *polv, sat_t *sat)
{
    guint    h,m,s;
    gchar   *ch,*cm,*cs;
    gdouble  number, now;
    gchar   *text = NULL;


    now = polv->tstamp;//get_current_daynum ();
    number = sat->los - now;

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
        text = g_strdup_printf (_("LOS in %s%d:%s%d%s%d"), ch, h, cm, m, cs, s);
    }
    else {
        text = g_strdup_printf (_("LOS in %s%d%s%d"), cm, m, cs, s);
    }
    g_free (ch);
    g_free (cm);
    g_free (cs);

    return text;
}
