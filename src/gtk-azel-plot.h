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
#ifndef __GTK_AZEL_PLOT_H__
#define __GTK_AZEL_PLOT_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkvbox.h>
#include "gtk-sat-data.h"
#include "predict-tools.h"
#include <goocanvas.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* number of ticks excluding end points */
#define AZEL_PLOT_NUM_TICKS 5


#define GTK_AZEL_PLOT(obj)          GTK_CHECK_CAST (obj, gtk_azel_plot_get_type (), GtkAzelPlot)
#define GTK_AZEL_PLOT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_azel_plot_get_type (), GtkAzelPlotClass)
#define GTK_IS_AZEL_PLOT(obj)       GTK_CHECK_TYPE (obj, gtk_azel_plot_get_type ())
#define GTK_TYPE_AZEL_PLOT          (gtk_azel_plot_get_type ())
#define IS_GTK_AZEL_PLOT(obj)       GTK_CHECK_TYPE (obj, gtk_azel_plot_get_type ())

typedef struct _GtkAzelPlot        GtkAzelPlot;
typedef struct _GtkAzelPlotClass   GtkAzelPlotClass;



struct _GtkAzelPlot
{
     GtkVBox vbox;

     GtkWidget  *canvas;   /*!< The canvas widget */

     GooCanvasItemModel *curs;    /*!< cusor info */
     GooCanvasItemModel *frame;   /*!< frame */
     GooCanvasItemModel *azg;     /*!< Az graph */
     GooCanvasItemModel *elg;     /*!< El graph */
     GooCanvasItemModel *xticksb[AZEL_PLOT_NUM_TICKS];     /*!< x tick marks bottom */
     GooCanvasItemModel *xtickst[AZEL_PLOT_NUM_TICKS];     /*!< x tick marks top */
     GooCanvasItemModel *xlabels[AZEL_PLOT_NUM_TICKS];     /*!< x tick labels */
     GooCanvasItemModel *yticksl[AZEL_PLOT_NUM_TICKS];     /*!< x tick marks left */
     GooCanvasItemModel *yticksr[AZEL_PLOT_NUM_TICKS];     /*!< x tick marks right */
     GooCanvasItemModel *ylabelsl[AZEL_PLOT_NUM_TICKS];     /*!< left y tick labels */
     GooCanvasItemModel *ylabelsr[AZEL_PLOT_NUM_TICKS];     /*!< right y tick labels */

     GooCanvasItemModel *xlab[AZEL_PLOT_NUM_TICKS];     /*!< x tick labels */
     GooCanvasItemModel *azlab[AZEL_PLOT_NUM_TICKS];    /*!< Az tick labels */
     GooCanvasItemModel *ellab[AZEL_PLOT_NUM_TICKS];    /*!< El tick labels */
     GooCanvasItemModel *azleg,*elleg,*xleg;            /*!< Az and El legend */

     qth_t      *qth;      /*!< Pointer to current location. */
     pass_t     *pass;

     guint       width;     /*!< width of the box  */
     guint       height;    /*!< height of the box */

     guint       x0,y0,xmax,ymax;

     gdouble     maxaz;     /*!< max Az 360 or 180 */

     gboolean    qthinfo;     /*!< Show the QTH info. */
     gboolean    cursinfo;    /*!< Track the mouse cursor. */
     gboolean    extratick;   /*!< Show extra ticks */
};

struct _GtkAzelPlotClass
{
     GtkVBoxClass parent_class;
};



GtkType        gtk_azel_plot_get_type   (void);

GtkWidget*     gtk_azel_plot_new        (qth_t *qth, pass_t *pass);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_AZEL_PLOT_H__ */
