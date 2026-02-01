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
#ifndef __GTK_SKY_GLANCE_H__
#define __GTK_SKY_GLANCE_H__ 1

#include <gdk/gdk.h>
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


#define GTK_SKY_GLANCE(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_sky_glance_get_type (), GtkSkyGlance)
#define GTK_SKY_GLANCE_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gtk_sky_glance_get_type (), GtkSkyGlanceClass)
#define GTK_IS_SKY_GLANCE(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sky_glance_get_type ())
#define GTK_TYPE_SKY_GLANCE          (gtk_sky_glance_get_type ())
#define IS_GTK_SKY_GLANCE(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sky_glance_get_type ())

typedef struct _GtkSkyGlance GtkSkyGlance;
typedef struct _GtkSkyGlanceClass GtkSkyGlanceClass;


/** Satellite object on graph. */
typedef struct {
    guint           catnum;     /* Catalog number of satellite */
    pass_t         *pass;       /* Details of the corresponding pass. */
    gdouble         x;          /* X position of box */
    gdouble         y;          /* Y position of box */
    gdouble         w;          /* Width of box */
    gdouble         h;          /* Height of box */
    guint32         bcol;       /* Border color */
    guint32         fcol;       /* Fill color */
} sky_pass_t;


#define SKY_PASS_T(obj) ((sky_pass_t *)obj)


/** GtkSkyGlance widget */
struct _GtkSkyGlance {
    GtkBox          vbox;

    GtkWidget      *canvas;     /*!< The drawing area widget */

    GHashTable     *sats;       /* Local copy of satellites. */
    qth_t          *qth;        /* Pointer to current location. */

    GSList         *passes;     /* List of sky_pass_t representing each pass. */
    GSList         *satlab;     /* List of satellite label data (name, color, position). */

    /* Satellite label data structure */
    /* Each element is a struct { gchar *name; guint32 color; gdouble x, y; } */

    guint           x0;
    guint           y0;
    guint           w;          /* width of the plot */
    guint           h;          /* height of the plot */
    guint           pps;        /* pixels per satellite */

    guint           numsat;     /* Number of satellites */
    guint           satcnt;     /* Counter to keep track of how many satellites we have
                                   plotted so far when creating the boxes.
                                   This is needed to ensure that we do not plot more
                                   than 10 satellites and to know which colour to fetch
                                   from sat-cfg.
                                 */
    gdouble         ts, te;     /* Start and end times (Julian date) */

    /* Time tick data */
    gint            num_ticks;  /* Number of time ticks */
    gdouble        *major_x;    /* X positions of major ticks */
    gdouble        *minor_x;    /* X positions of minor ticks */
    gchar         **tick_labels; /* Tick label strings */

    /* Cursor tracking */
    gdouble         cursor_x;   /* Current cursor X position */
    gchar          *time_label; /* Time label text at cursor */

    gchar          *font;       /*!< Default font name */

};

/** Satellite label structure for drawing */
typedef struct {
    gchar          *name;       /* Satellite name */
    guint32         color;      /* Label color */
    gdouble         x;          /* X position */
    gdouble         y;          /* Y position */
    gint            anchor;     /* Anchor type: 0=west, 1=east */
} sat_label_t;
struct _GtkSkyGlanceClass {
    GtkBoxClass     parent_class;
};


GType           gtk_sky_glance_get_type(void);
GtkWidget      *gtk_sky_glance_new(GHashTable * sats, qth_t * qth, gdouble ts);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* __GTK_SKY_GLANCE_H__ */
