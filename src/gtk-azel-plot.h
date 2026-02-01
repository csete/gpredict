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
#ifndef __GTK_AZEL_PLOT_H__
#define __GTK_AZEL_PLOT_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "predict-tools.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern          "C" {
#endif
/* *INDENT-ON* */

/* number of ticks excluding end points */
#define AZEL_PLOT_NUM_TICKS 5

#define GTK_AZEL_PLOT(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_azel_plot_get_type (), GtkAzelPlot)
#define GTK_AZEL_PLOT_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gtk_azel_plot_get_type (), GtkAzelPlotClass)
#define GTK_IS_AZEL_PLOT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_azel_plot_get_type ())
#define GTK_TYPE_AZEL_PLOT          (gtk_azel_plot_get_type ())
#define IS_GTK_AZEL_PLOT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_azel_plot_get_type ())

typedef struct _GtkAzelPlot GtkAzelPlot;
typedef struct _GtkAzelPlotClass GtkAzelPlotClass;

struct _GtkAzelPlot {
    GtkBox          box;

    GtkWidget      *canvas;     /*!< The drawing area widget */

    /* Colors */
    guint32         col_axis;   /*!< Axis/frame color */
    guint32         col_az;     /*!< Azimuth graph color */
    guint32         col_el;     /*!< Elevation graph color */

    /* Text elements */
    gchar          *curs_text;  /*!< Cursor info text */

    /* Graph data points */
    gdouble        *az_points;  /*!< Array of azimuth graph points */
    gdouble        *el_points;  /*!< Array of elevation graph points */
    gint            num_points; /*!< Number of data points */

    /* Tick labels */
    gchar          *xlabels[AZEL_PLOT_NUM_TICKS]; /*!< x tick label texts */
    gchar          *azlabels[AZEL_PLOT_NUM_TICKS]; /*!< Az tick label texts */
    gchar          *ellabels[AZEL_PLOT_NUM_TICKS]; /*!< El tick label texts */

    qth_t          *qth;        /*!< Pointer to current location. */
    pass_t         *pass;

    guint           width;      /*!< width of the box  */
    guint           height;     /*!< height of the box */

    guint           x0, y0, xmax, ymax;

    gdouble         maxaz;      /*!< max Az 360 or 180 */

    gboolean        qthinfo;    /*!< Show the QTH info. */
    gboolean        cursinfo;   /*!< Track the mouse cursor. */
    gboolean        extratick;  /*!< Show extra ticks */

    gchar          *font;       /*!< Default font name */
};

struct _GtkAzelPlotClass {
    GtkBoxClass     parent_class;
};

GType           gtk_azel_plot_get_type(void);
GtkWidget      *gtk_azel_plot_new(qth_t * qth, pass_t * pass);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* __GTK_AZEL_PLOT_H__ */
