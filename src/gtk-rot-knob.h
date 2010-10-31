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
#ifndef __GTK_ROT_KNOB_H__
#define __GTK_ROT_KNOB_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




#define GTK_TYPE_ROT_KNOB          (gtk_rot_knob_get_type ())
#define GTK_ROT_KNOB(obj)          GTK_CHECK_CAST (obj,\
                                       gtk_rot_knob_get_type (),\
                                         GtkRotKnob)

#define GTK_ROT_KNOB_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass,\
                                    gtk_rot_knob_get_type (),\
                                    GtkRotKnobClass)

#define IS_GTK_ROT_KNOB(obj)       GTK_CHECK_TYPE (obj, gtk_rot_knob_get_type ())


typedef struct _gtk_rot_knob      GtkRotKnob;
typedef struct _GtkRotKnobClass   GtkRotKnobClass;



struct _gtk_rot_knob
{
     GtkVBox vbox;
    
    GtkWidget *digits[7];   /*!< Labels for the digits */
    GtkWidget *buttons[10]; /*!< Buttons; 0..4 up; 5..9 down */
     
    gdouble min;
    gdouble max;
    gdouble value;
};

struct _GtkRotKnobClass
{
     GtkVBoxClass parent_class;
};



GtkType    gtk_rot_knob_get_type  (void);
GtkWidget* gtk_rot_knob_new       (gdouble min, gdouble max, gdouble val);
void       gtk_rot_knob_set_value (GtkRotKnob *knob, gdouble val);
gdouble    gtk_rot_knob_get_value (GtkRotKnob *knob);
void       gtk_rot_knob_set_max   (GtkRotKnob *knob, gdouble max);
gdouble    gtk_rot_knob_get_max   (GtkRotKnob *knob);
gdouble    gtk_rot_knob_get_min   (GtkRotKnob *knob);
void       gtk_rot_knob_set_min   (GtkRotKnob *knob, gdouble min);
void       gtk_rot_knob_set_max   (GtkRotKnob *knob, gdouble max);
void       gtk_rot_knob_set_range (GtkRotKnob *knob, gdouble min, gdouble max);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_ROT_knob_H__ */
