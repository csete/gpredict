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
#ifndef __GTK_POLAR_PLOT_H__
#define __GTK_POLAR_PLOT_H__ 1

#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "predict-tools.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* *INDENT-ON* */

/** \brief Number of time ticks. */
#define TRACK_TICK_NUM 5

#define GTK_POLAR_PLOT(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_polar_plot_get_type (), GtkPolarPlot)
#define GTK_POLAR_PLOT_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gtk_polar_plot_get_type (), GtkPolarPlotClass)
#define GTK_IS_POLAR_PLOT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_polar_plot_get_type ())
#define GTK_TYPE_POLAR_PLOT          (gtk_polar_plot_get_type ())
#define IS_GTK_POLAR_PLOT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_polar_plot_get_type ())

typedef struct _GtkPolarPlot GtkPolarPlot;
typedef struct _GtkPolarPlotClass GtkPolarPlotClass;

/* graph orientation; start at 12
   o'clock and go clockwise */
typedef enum {
    POLAR_PLOT_NESW = 0,        /*!< Normal / usual */
    POLAR_PLOT_NWSE = 1,
    POLAR_PLOT_SENW = 2,
    POLAR_PLOT_SWNE = 3
} polar_plot_swap_t;

/* pole identifier */
typedef enum {
    POLAR_PLOT_POLE_N = 0,
    POLAR_PLOT_POLE_E = 1,
    POLAR_PLOT_POLE_S = 2,
    POLAR_PLOT_POLE_W = 3
} polar_plot_pole_t;

/** Time tick data for sky track */
typedef struct {
    gfloat          x;          /*!< X coordinate */
    gfloat          y;          /*!< Y coordinate */
    gchar           text[6];    /*!< Time string */
} polar_plot_tick_t;

struct _GtkPolarPlot {
    GtkBox          box;

    GtkWidget      *canvas;     /*!< The drawing area widget */

    /* Colors */
    guint32         col_bgd;    /*!< Background color */
    guint32         col_axis;   /*!< Axis color */
    guint32         col_tick;   /*!< Tick label color */
    guint32         col_info;   /*!< Info text color */
    guint32         col_sat;    /*!< Satellite color */
    guint32         col_track;  /*!< Track color */

    /* Text elements */
    gchar          *curs_text;  /*!< Cursor tracking text */

    pass_t         *pass;

    /* Track points */
    gdouble        *track_points; /*!< Array of track coordinates */
    gint            track_count;  /*!< Number of track points */
    polar_plot_tick_t trtick[TRACK_TICK_NUM]; /*!< Time ticks along the sky track */

    /* Target/controller/rotor positions */
    gdouble         target_az;  /*!< Target azimuth (-1 if hidden) */
    gdouble         target_el;  /*!< Target elevation (-1 if hidden) */
    gdouble         ctrl_az;    /*!< Controller azimuth (-1 if hidden) */
    gdouble         ctrl_el;    /*!< Controller elevation (-1 if hidden) */
    gdouble         rotor_az;   /*!< Rotor azimuth (-1 if hidden) */
    gdouble         rotor_el;   /*!< Rotor elevation (-1 if hidden) */

    qth_t          *qth;        /*!< Pointer to current location. */

    guint           cx;         /*!< center X */
    guint           cy;         /*!< center Y */
    guint           r;          /*!< radius */
    guint           size;       /*!< Size of the box = min(h,w) */


    polar_plot_swap_t swap;

    gboolean        qthinfo;    /*!< Show the QTH info. */
    gboolean        cursinfo;   /*!< Track the mouse cursor. */
    gboolean        extratick;  /*!< Show extra ticks */

    gchar          *font;       /*!< Default font name */
};

struct _GtkPolarPlotClass {
    GtkBoxClass     parent_class;
};

GType           gtk_polar_plot_get_type(void);
GtkWidget      *gtk_polar_plot_new(qth_t * qth, pass_t * pass);
void            gtk_polar_plot_set_pass(GtkPolarPlot * plot, pass_t * pass);
void            gtk_polar_plot_set_target_pos(GtkPolarPlot * plot, gdouble az,
                                              gdouble el);
void            gtk_polar_plot_set_ctrl_pos(GtkPolarPlot * plot, gdouble az,
                                            gdouble el);
void            gtk_polar_plot_set_rotor_pos(GtkPolarPlot * plot, gdouble az,
                                             gdouble el);
void            gtk_polar_plot_show_time_ticks(GtkPolarPlot * plot,
                                               gboolean show);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif /* __cplusplus */
/* *INDENT-ON* */

#endif /* __GTK_POLAR_PLOT_H__ */
