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
/** \brief Azel Plot Widget.
 *
 * More info...
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "time-tools.h"
#include "gtk-sat-data.h"
#include "gpredict-utils.h"
#include "gtk-azel-plot.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include <goocanvas.h>



#define POLV_DEFAULT_SIZE 300
#define POLV_X_MARGIN 40
#define POLV_Y_MARGIN 40
 

/* extra size for line outside 0 deg circle (inside margin) */
#define POLV_LINE_EXTRA 5

#define MARKER_SIZE 5


static void gtk_azel_plot_class_init    (GtkAzelPlotClass *class);
static void gtk_azel_plot_init          (GtkAzelPlot *polview);
static void gtk_azel_plot_destroy       (GtkObject *object);
static void size_allocate_cb            (GtkWidget *widget,
                                                   GtkAllocation *allocation,
                                                   gpointer data);
static gboolean on_motion_notify        (GooCanvasItem *item,
                                                   GooCanvasItem *target,
                                                   GdkEventMotion *event,
                                                   gpointer data);
static void on_item_created             (GooCanvas *canvas,
                                                   GooCanvasItem *item,
                                                   GooCanvasItemModel *model,
                                                   gpointer data);
static void on_canvas_realized          (GtkWidget *canvas, gpointer data);
static GooCanvasItemModel* create_canvas_model (GtkAzelPlot *polv);
static void get_canvas_bg_color         (GtkAzelPlot *polv, GdkColor *color);
static void xy_to_graph (GtkAzelPlot *p, gfloat x, gfloat y, gdouble *t, gdouble *az, gdouble *el);
static void az_to_xy (GtkAzelPlot *p, gdouble t, gdouble az, gdouble *x, gdouble *y);
static void el_to_xy (GtkAzelPlot *p, gdouble t, gdouble el, gdouble *x, gdouble *y);


static GtkVBoxClass *parent_class = NULL;


GtkType
gtk_azel_plot_get_type ()
{
     static GType gtk_azel_plot_type = 0;

     if (!gtk_azel_plot_type) {
          static const GTypeInfo gtk_azel_plot_info = {
               sizeof (GtkAzelPlotClass),
               NULL, /* base init */
               NULL, /* base finalise */
               (GClassInitFunc) gtk_azel_plot_class_init,
               NULL, /* class finalise */
               NULL, /* class data */
               sizeof (GtkAzelPlot),
               5,  /* n_preallocs */
               (GInstanceInitFunc) gtk_azel_plot_init,
          };

          gtk_azel_plot_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                   "GtkAzelPlot",
                                                                   &gtk_azel_plot_info,
                                                                   0);
     }

     return gtk_azel_plot_type;
}


static void
gtk_azel_plot_class_init (GtkAzelPlotClass *class)
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

     object_class->destroy = gtk_azel_plot_destroy;
     //widget_class->size_allocate = gtk_azel_plot_size_allocate;
}


static void
gtk_azel_plot_init (GtkAzelPlot *polview)
{
     polview->qth       = NULL;
     polview->width     = 0;
     polview->height    = 0;
     polview->x0        = 0;
     polview->y0        = 0;
     polview->xmax      = 0;
     polview->ymax      = 0;
     polview->maxaz     = 0.0;
     polview->qthinfo   = FALSE;
     polview->cursinfo  = FALSE;
     polview->extratick = FALSE;
}


static void
gtk_azel_plot_destroy (GtkObject *object)
{

     (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/** \brief Create a new GtkAzelPlot widget.
 *  \param cfgdata The configuration data of the parent module.
 *  \param sats Pointer to the hash table containing the asociated satellites.
 *  \param qth Pointer to the ground station data.
 */
GtkWidget*
gtk_azel_plot_new (qth_t *qth, pass_t *pass)
{
     GtkWidget *polv;
     GooCanvasItemModel *root;
     GdkColor bg_color = {0, 0xFFFF, 0xFFFF, 0xFFFF};
     guint i,n;
     pass_detail_t *detail;

     polv = g_object_new (GTK_TYPE_AZEL_PLOT, NULL);

     GTK_AZEL_PLOT (polv)->qth  = qth;
     GTK_AZEL_PLOT (polv)->pass = pass;

     /* get settings */
     GTK_AZEL_PLOT (polv)->qthinfo = sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_QTH_INFO);
     GTK_AZEL_PLOT (polv)->extratick = sat_cfg_get_bool (SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS);
     GTK_AZEL_PLOT (polv)->cursinfo = TRUE;

     /* check maximum Az */
     n = g_slist_length (pass->details);
     for (i = 0; i < n; i++) {
          detail = PASS_DETAIL (g_slist_nth_data (pass->details, i));

          if (detail->az > GTK_AZEL_PLOT (polv)->maxaz) {
               GTK_AZEL_PLOT (polv)->maxaz = detail->az;
          }
     }

     if (GTK_AZEL_PLOT (polv)->maxaz > 180.0)
          GTK_AZEL_PLOT (polv)->maxaz = 360.0;
     else
          GTK_AZEL_PLOT (polv)->maxaz = 180.0;

     /* create the canvas */
     GTK_AZEL_PLOT (polv)->canvas = goo_canvas_new ();
     get_canvas_bg_color (GTK_AZEL_PLOT (polv), &bg_color);
     gtk_widget_modify_base (GTK_AZEL_PLOT (polv)->canvas, GTK_STATE_NORMAL, &bg_color);
     gtk_widget_set_size_request (GTK_AZEL_PLOT (polv)->canvas,
                                         POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);
     goo_canvas_set_bounds (GOO_CANVAS (GTK_AZEL_PLOT (polv)->canvas), 0, 0,
                                        POLV_DEFAULT_SIZE, POLV_DEFAULT_SIZE);

     /* connect size-request signal */
     g_signal_connect (GTK_AZEL_PLOT (polv)->canvas, "size-allocate",
                           G_CALLBACK (size_allocate_cb), polv);
     g_signal_connect (GTK_AZEL_PLOT (polv)->canvas, "item_created",
                           (GtkSignalFunc) on_item_created, polv);
     g_signal_connect_after (GTK_AZEL_PLOT (polv)->canvas, "realize",
                                   (GtkSignalFunc) on_canvas_realized, polv);

     gtk_widget_show (GTK_AZEL_PLOT (polv)->canvas);


     /* Create the canvas model */
     root = create_canvas_model (GTK_AZEL_PLOT (polv));
     goo_canvas_set_root_item_model (GOO_CANVAS (GTK_AZEL_PLOT (polv)->canvas), root);

     g_object_unref (root);

     gtk_container_add (GTK_CONTAINER (polv), GTK_AZEL_PLOT (polv)->canvas);

     return polv;
}


static GooCanvasItemModel *
create_canvas_model (GtkAzelPlot *polv)
{
     GooCanvasItemModel *root;
     guint32 col;
     guint i;
     gdouble xstep,ystep;
     gdouble t,az,el;
     time_t tim;
     gchar  buff[7];
     gchar *txt;

     root = goo_canvas_group_model_new (NULL, NULL);

     /* graph dimensions */
     polv->width = POLV_DEFAULT_SIZE;
     polv->height = POLV_DEFAULT_SIZE;

     /* update coordinate system */
     polv->x0 = POLV_X_MARGIN;
     polv->xmax = polv->width - POLV_X_MARGIN;
     polv->y0 = polv->height - POLV_Y_MARGIN;
     polv->ymax = POLV_Y_MARGIN;
     
     col = sat_cfg_get_int (SAT_CFG_INT_POLAR_AXIS_COL);

     /* frame */
     polv->frame = goo_canvas_rect_model_new (root,
                                                        (gdouble) polv->x0,
                                                        (gdouble) polv->ymax,
                                                        (gdouble) (polv->xmax - polv->x0),
                                                        (gdouble) (polv->y0 - POLV_Y_MARGIN),
                                                        "stroke-color-rgba", 0x000000FF,
                                                        "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                        "line-join", CAIRO_LINE_JOIN_MITER,
                                                        "line-width", 1.0,
                                                        NULL);

     xstep = (polv->xmax - polv->x0) / (AZEL_PLOT_NUM_TICKS+1); 
     ystep = (polv->y0 - polv->ymax) / (AZEL_PLOT_NUM_TICKS+1);

     /* tick marks */
     for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++) {

          /* bottom x tick marks */
          polv->xticksb[i] = goo_canvas_polyline_model_new_line (root,
                                                                             (gdouble) (polv->x0 + (i+1) * xstep),
                                                                             (gdouble) polv->y0,
                                                                             (gdouble) (polv->x0 + (i+1) * xstep),
                                                                             (gdouble) (polv->y0 - MARKER_SIZE),
                                                                             "fill-color-rgba", col,
                                                                             "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                                             "line-join", CAIRO_LINE_JOIN_MITER,
                                                                             "line-width", 1.0,
                                                                             NULL);

          /* top x tick marks */
          polv->xtickst[i] = goo_canvas_polyline_model_new_line (root,
                                                                              (gdouble) (polv->x0 + (i+1) * xstep),
                                                                              (gdouble) polv->ymax,
                                                                              (gdouble) (polv->x0 + (i+1) * xstep),
                                                                              (gdouble) (polv->ymax + MARKER_SIZE),
                                                                              "fill-color-rgba", col,
                                                                              "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                                              "line-join", CAIRO_LINE_JOIN_MITER,
                                                                              "line-width", 1.0,
                                                                              NULL);

          /* x tick labels */
          /* get time */
          xy_to_graph (polv, polv->x0 + (i+1) * xstep, 0.0, &t, &az, &el);

          /* convert julian date to struct tm */
          tim = (t - 2440587.5)*86400.0;

          /* format either local time or UTC depending on check box */
          if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
               strftime (buff, 7, "%H:%M", localtime (&tim));
          else
               strftime (buff, 7, "%H:%M", gmtime (&tim));
          
          buff[6]='\0';

          polv->xlab[i] = goo_canvas_text_model_new (root, buff,
                                                               (gfloat) (polv->x0 + (i+1) * xstep),
                                                               (gfloat) (polv->y0 + 5),
                                                               -1,
                                                               GTK_ANCHOR_N,
                                                               "font", "Sans 8",
                                                               "fill-color-rgba", 0x000000FF,
                                                               NULL);

          /* left y tick marks */
          polv->yticksl[i] = goo_canvas_polyline_model_new_line (root,
                                                                              (gdouble) polv->x0,
                                                                              (gdouble) (polv->y0 - (i+1) * ystep),
                                                                              (gdouble) (polv->x0 + MARKER_SIZE),
                                                                              (gdouble) (polv->y0 - (i+1) * ystep),
                                                                              "fill-color-rgba", col,
                                                                              "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                                              "line-join", CAIRO_LINE_JOIN_MITER,
                                                                              "line-width", 1.0,
                                                                              NULL);

          /* right y tick marks */
          polv->yticksr[i] = goo_canvas_polyline_model_new_line (root,
                                                                              (gdouble) polv->xmax,
                                                                              (gdouble) (polv->y0 - (i+1) * ystep),
                                                                              (gdouble) (polv->xmax - MARKER_SIZE),
                                                                              (gdouble) (polv->y0 - (i+1) * ystep),
                                                                              "fill-color-rgba", col,
                                                                              "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                                              "line-join", CAIRO_LINE_JOIN_MITER,
                                                                              "line-width", 1.0,
                                                                              NULL);

          /* Az tick labels */
          txt = g_strdup_printf ("%.0f\302\260", polv->maxaz / (AZEL_PLOT_NUM_TICKS+1) * (i+1));
          polv->azlab[i] = goo_canvas_text_model_new (root, txt,
                                                                 (gfloat) (polv->x0 - 5),
                                                                 (gfloat) (polv->y0 - (i+1) * ystep),
                                                                 -1,
                                                                 GTK_ANCHOR_E,
                                                                 "font", "Sans 8",
                                                                 "fill-color-rgba", 0x0000BFFF,
                                                                 NULL);
          g_free (txt);

          /* El tick labels */
          txt = g_strdup_printf ("%.0f\302\260", 90.0 / (AZEL_PLOT_NUM_TICKS+1) * (i+1));
          polv->ellab[i] = goo_canvas_text_model_new (root, txt,
                                                                 (gfloat) (polv->xmax + 5),
                                                                 (gfloat) (polv->y0 - (i+1) * ystep),
                                                                 -1,
                                                                 GTK_ANCHOR_W,
                                                                 "font", "Sans 8",
                                                                 "fill-color-rgba", 0xBF0000FF,
                                                                 NULL);
          g_free (txt);

     }

     /* x legend */
     if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME)) {
          polv->xleg = goo_canvas_text_model_new (root, _("Local Time"),
                                                            (gfloat) (polv->x0 + (polv->xmax - polv->x0) / 2),
                                                            (gfloat) (polv->height - 5),
                                                            -1,
                                                            GTK_ANCHOR_S,
                                                            "font", "Sans 9",
                                                            "fill-color-rgba", 0x000000FF,
                                                            NULL);
     }
     else {
          polv->xleg = goo_canvas_text_model_new (root, _("UTC"),
                                                            (gfloat) (polv->x0 + (polv->xmax - polv->x0) / 2),
                                                            (gfloat) (polv->height - 5),
                                                            -1,
                                                            GTK_ANCHOR_S,
                                                            "font", "Sans 9",
                                                            "fill-color-rgba", 0x000000FF,
                                                            NULL);
     }

     /* cursor text in upper right corner */
     polv->curs = goo_canvas_text_model_new (root, "",
                                                       (gfloat) (polv->x0 + (polv->xmax - polv->x0) / 2),
                                                       5.0,
                                                       -1,
                                                       GTK_ANCHOR_N,
                                                       "font", "Sans 8",
                                                       "fill-color-rgba", 0x000000FF,
                                                       NULL);

     /* Az legend */
     polv->azleg = goo_canvas_text_model_new (root, _("Az"),
                                                        (gfloat) (polv->x0 - 7),
                                                        (gfloat) polv->ymax,
                                                        -1,
                                                        GTK_ANCHOR_NE,
                                                        "font", "Sans 9",
                                                        "fill-color-rgba", 0x0000BFFF,
                                                        NULL);

     /* El legend */
     polv->elleg = goo_canvas_text_model_new (root, _("El"),
                                                        (gfloat) (polv->xmax + 7),
                                                        (gfloat) polv->ymax,
                                                        -1,
                                                        GTK_ANCHOR_NW,
                                                        "font", "Sans 9",
                                                        "fill-color-rgba", 0xBF0000FF,
                                                        NULL);

     /* Az graph */
     polv->azg = goo_canvas_polyline_model_new_line (root, 0, 0, 10, 10,
                                                                 "stroke-color-rgba", 0x0000BFFF,
                                                                 "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                                 "line-join", CAIRO_LINE_JOIN_MITER,
                                                                 "line-width", 1.0,
                                                                 NULL);

     /* El graph */
     polv->elg = goo_canvas_polyline_model_new_line (root, 30, 30, 40, 40,
                                                                 "stroke-color-rgba", 0xBF0000FF,
                                                                 "fill-color-rgba", 0xBF00001A,
                                                                 "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                                 "line-join", CAIRO_LINE_JOIN_MITER,
                                                                 "line-width", 1.0,
                                                                 NULL);

     return root;
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
     GtkAzelPlot *polv;
     GooCanvasPoints *pts;
     gdouble dx,dy;
     gdouble xstep,ystep;
     guint i,n;
     pass_detail_t *detail;


     if (GTK_WIDGET_REALIZED (widget)) {

          /* get graph dimensions */
          polv = GTK_AZEL_PLOT (data);

          polv->width = allocation->width;
          polv->height = allocation->height;

        goo_canvas_set_bounds (GOO_CANVAS (GTK_AZEL_PLOT (polv)->canvas), 0, 0,
                               polv->width, polv->height);
        
          /* update coordinate system */
          polv->x0 = POLV_X_MARGIN;
          polv->xmax = polv->width - POLV_X_MARGIN;
          polv->y0 = polv->height - POLV_Y_MARGIN;
          polv->ymax = POLV_Y_MARGIN;

          /* frame */
          g_object_set (polv->frame,
                           "x", (gdouble) polv->x0,
                           "y", (gdouble) polv->ymax,
                           "width", (gdouble) (polv->xmax - polv->x0), 
                           "height", (gdouble) (polv->y0 - POLV_Y_MARGIN),
                           NULL);


          xstep = (polv->xmax - polv->x0) / (AZEL_PLOT_NUM_TICKS+1); 
          ystep = (polv->y0 - polv->ymax) / (AZEL_PLOT_NUM_TICKS+1);

          /* tick marks */
          for (i = 0; i < AZEL_PLOT_NUM_TICKS; i++) {

               pts = goo_canvas_points_new (2);

               /* bottom x tick marks */
               pts->coords[0] = (gdouble) (polv->x0 + (i+1) * xstep);
               pts->coords[1] = (gdouble) polv->y0;
               pts->coords[2] = (gdouble) (polv->x0 + (i+1) * xstep);
               pts->coords[3] = (gdouble) (polv->y0 - MARKER_SIZE);
               g_object_set (polv->xticksb[i], "points", pts, NULL);

               /* top x tick marks */
               pts->coords[0] = (gdouble) (polv->x0 + (i+1) * xstep);
               pts->coords[1] = (gdouble) polv->ymax;
               pts->coords[2] = (gdouble) (polv->x0 + (i+1) * xstep);
               pts->coords[3] = (gdouble) (polv->ymax + MARKER_SIZE);
               g_object_set (polv->xtickst[i], "points", pts, NULL);

               /* x tick labels */
               g_object_set (polv->xlab[i],
                                "x", (gfloat) (polv->x0 + (i+1) * xstep),
                                "y", (gfloat) (polv->y0 + 5.0),
                                NULL);
          
               /* left y tick marks */
               pts->coords[0] = (gdouble) (gdouble) polv->x0;
               pts->coords[1] = (gdouble) (gdouble) (polv->y0 - (i+1) * ystep);
               pts->coords[2] = (gdouble) (gdouble) (polv->x0 + MARKER_SIZE);
               pts->coords[3] = (gdouble) (gdouble) (polv->y0 - (i+1) * ystep);
               g_object_set (polv->yticksl[i], "points", pts, NULL);
               
               /* right y tick marks */
               pts->coords[0] = (gdouble) polv->xmax;
               pts->coords[1] = (gdouble) (polv->y0 - (i+1) * ystep);
               pts->coords[2] = (gdouble) (polv->xmax - MARKER_SIZE);
               pts->coords[3] = (gdouble) (polv->y0 - (i+1) * ystep);
               g_object_set (polv->yticksr[i], "points", pts, NULL);
               
               goo_canvas_points_unref (pts);

               /* Az tick labels */
               g_object_set (polv->azlab[i],
                                "x", (gfloat) (polv->x0 - 5),
                                "y", (gfloat) (polv->y0 - (i+1) * ystep),
                                NULL);

               /* El tick labels */
               g_object_set (polv->ellab[i],
                                "x", (gfloat) (polv->xmax + 5),
                                "y", (gfloat) (polv->y0 - (i+1) * ystep),
                                NULL);

          }

          /* Az legend */
          g_object_set (polv->azleg,
                           "x", (gfloat) (polv->x0 - 7),
                           "y", (gfloat) (polv->ymax),
                           NULL);

          /* El legend */
          g_object_set (polv->elleg,
                           "x", (gfloat) (polv->xmax + 7),
                           "y", (gfloat) (polv->ymax),
                           NULL);

          /* x legend */
          g_object_set (polv->xleg,
                           "x", (gfloat) (polv->x0 + (polv->xmax - polv->x0) / 2),
                           "y", (gfloat) (polv->height - 5),
                           NULL);

          /* Az graph */
          n = g_slist_length (polv->pass->details);
          pts = goo_canvas_points_new (n);

          for (i = 0; i < n; i++) {
               detail = PASS_DETAIL (g_slist_nth_data (polv->pass->details, i));
               az_to_xy (polv, detail->time, detail->az, &dx, &dy);
               pts->coords[2*i] = dx;
               pts->coords[2*i+1] = dy;
          }
          pts->coords[0] = polv->x0;
          pts->coords[2*n-2] = polv->xmax;
          g_object_set (polv->azg, "points", pts, NULL);
          goo_canvas_points_unref (pts);

          /* El graph */
          n = g_slist_length (polv->pass->details);
          pts = goo_canvas_points_new (n);

          for (i = 0; i < n; i++) {
               detail = PASS_DETAIL (g_slist_nth_data (polv->pass->details, i));
               el_to_xy (polv, detail->time, detail->el, &dx, &dy);
               pts->coords[2*i] = dx;
               pts->coords[2*i+1] = dy;
          }
          pts->coords[1] = polv->y0;
          pts->coords[2*n-1] = polv->y0;
          g_object_set (polv->elg, "points", pts, NULL);
          goo_canvas_points_unref (pts);
          
          /* cursor track */
          g_object_set (polv->curs,
                           "x", (gfloat) (polv->x0 + (polv->xmax - polv->x0) / 2),
                           "y", (gfloat) 5.0,
                           NULL);



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


/** \brief Convert Az/El to canvas based XY coordinates. */
/* static void */
/* graph_to_xy (GtkAzelPlot *p, gdouble t, gdouble az, gdouble el, gfloat *x, gfloat *y) */
/* { */
/*      gdouble rel; */


/* } */


/** \brief Convert canvas based coordinates to Az/El. */
static void
xy_to_graph    (GtkAzelPlot *p, gfloat x, gfloat y, gdouble *t, gdouble *az, gdouble *el)
{
     gdouble tpp;  /* time per pixel */
     gdouble dpp;  /* degrees per pixel */

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


static void
az_to_xy (GtkAzelPlot *p, gdouble t, gdouble az, gdouble *x, gdouble *y)
{
     gdouble tpp;  /* time per pixel */
     gdouble dpp;  /* degrees per pixel */

     /* time */
     tpp = (p->pass->los - p->pass->aos) / (p->xmax - p->x0);
     *x = p->x0 + (t - p->pass->aos) / tpp;

     /* Az */
     dpp = (gdouble) (p->maxaz / (p->y0 - p->ymax));
     *y = (gdouble) (p->y0 - az / dpp);
}


static void
el_to_xy (GtkAzelPlot *p, gdouble t, gdouble el, gdouble *x, gdouble *y)
{
     gdouble tpp;  /* time per pixel */
     gdouble dpp;  /* degrees per pixel */

     /* time */
     tpp = (p->pass->los - p->pass->aos) / (p->xmax - p->x0);
     *x = p->x0 + (t - p->pass->aos) / tpp;

     /* Az */
     dpp = (gdouble) (90.0 / (p->y0 - p->ymax));
     *y = (gdouble) (p->y0 - el / dpp);
}


/** \brief Manage mouse motion events. */
static gboolean
on_motion_notify (GooCanvasItem *item,
                      GooCanvasItem *target,
                      GdkEventMotion *event,
                      gpointer data)
{
     GtkAzelPlot *polv = GTK_AZEL_PLOT (data);
     gdouble t,az,el;
     time_t tim;
     gfloat x,y;
     gchar *text;
     gchar  buff[10];

     if (polv->cursinfo) {

          /* get (x,y) */
          x = event->x;
          y = event->y;

          /* get (t,az,el) */

          /* show vertical line at that time */

          /* print (t,az,el) */
          if ((x > polv->x0) && (x < polv->xmax) &&
               (y > polv->ymax) && (y < polv->y0)) {

               xy_to_graph (polv, x, y, &t, &az, &el);

               /* convert julian date to struct tm */
               tim = (t - 2440587.5)*86400.0;

               /* format either local time or UTC depending on check box */
               if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
                    strftime (buff, 10, "%H:%M:%S", localtime (&tim));
               else
                    strftime (buff, 10, "%H:%M:%S", gmtime (&tim));
               
               buff[8]='\0';

               /* cursor track */
               text = g_strdup_printf ("T: %s, AZ: %.0f\302\260, EL: %.0f\302\260", buff, az, el);
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
 *  \param data Pointer to the GtkAzelPlot object.
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
     if (!goo_canvas_item_model_get_parent (model)) {
          /* root item / canvas */
          g_signal_connect (item, "motion_notify_event",
                                (GtkSignalFunc) on_motion_notify, data);
     }

}


/** \brief Retrieve background color.
 *
 * This function retrieves the canvas background color, which is in 0xRRGGBBAA
 * format and converts it to GdkColor style. Besides extractibg the RGB components
 * we also need to scale from [0;255] to [0;65535], i.e. multiply by 257.
 */
static void
get_canvas_bg_color (GtkAzelPlot *polv, GdkColor *color)
{
     guint32 col,tmp;
     guint16 r,g,b;


     col = sat_cfg_get_int (SAT_CFG_INT_POLAR_BGD_COL);

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
