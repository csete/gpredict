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
#ifndef __GTK_FREQ_KNOB_H__
#define __GTK_FREQ_KNOB_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* *INDENT-OO* */


#define GTK_TYPE_FREQ_KNOB          (gtk_freq_knob_get_type ())
#define GTK_FREQ_KNOB(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                    gtk_freq_knob_get_type (),\
                                    GtkFreqKnob)

#define GTK_FREQ_KNOB_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                    gtk_freq_knob_get_type (),\
                                    GtkFreqKnobClass)

#define IS_GTK_FREQ_KNOB(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_freq_knob_get_type ())


typedef struct _gtk_freq_knob      GtkFreqKnob;
typedef struct _GtkFreqKnobClass   GtkFreqKnobClass;

struct _gtk_freq_knob
{
     GtkVBox vbox;
    
    GtkWidget *digits[10];   /*!< Labels for the digits */
    GtkWidget *evtbox[10];   /*!< Event boxes to catch mouse events over the digits */
    GtkWidget *buttons[20];  /*!< Buttons; 0..9 up; 10..19 down */
     
    gdouble min;
    gdouble max;
    gdouble value;
};

struct _GtkFreqKnobClass
{
     GtkVBoxClass parent_class;
};

GType    gtk_freq_knob_get_type  (void);
GtkWidget* gtk_freq_knob_new       (gdouble val, gboolean buttons);
void       gtk_freq_knob_set_value (GtkFreqKnob *knob, gdouble val);
gdouble    gtk_freq_knob_get_value (GtkFreqKnob *knob);


/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif /* __cplusplus */
/* *INDENT-ON* */

#endif /* __GTK_FREQ_KNOB_H__ */
