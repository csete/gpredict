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
/** \brief ROTOR control.
 *  \ingroup widgets
 *
 * More info...
 * 
 * \bug This should be a generic widget, not just rotor specific
 * 
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include "gtk-rot-knob.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif


#define FMTSTR "<span size='xx-large'>%c</span>"


static void gtk_rot_knob_class_init (GtkRotKnobClass *class);
static void gtk_rot_knob_init       (GtkRotKnob      *list);
static void gtk_rot_knob_destroy    (GtkObject       *object);

static void gtk_rot_knob_update     (GtkRotKnob *knob);

static void button_clicked_cb (GtkWidget *button, gpointer data);


static GtkHBoxClass *parent_class = NULL;


        
        

GType
gtk_rot_knob_get_type ()
{
     static GType gtk_rot_knob_type = 0;

     if (!gtk_rot_knob_type) {

          static const GTypeInfo gtk_rot_knob_info = {
               sizeof (GtkRotKnobClass),
               NULL,  /* base_init */
               NULL,  /* base_finalize */
               (GClassInitFunc) gtk_rot_knob_class_init,
               NULL,  /* class_finalize */
               NULL,  /* class_data */
               sizeof (GtkRotKnob),
               5,     /* n_preallocs */
               (GInstanceInitFunc) gtk_rot_knob_init,
          };

          gtk_rot_knob_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                "GtkRotKnob",
                                                                 &gtk_rot_knob_info,
                                                                 0);
     }

     return gtk_rot_knob_type;
}


static void
gtk_rot_knob_class_init (GtkRotKnobClass *class)
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

     object_class->destroy = gtk_rot_knob_destroy;
 
}



static void
gtk_rot_knob_init (GtkRotKnob *knob)
{

    
}

static void
gtk_rot_knob_destroy (GtkObject *object)
{
     (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



/** \brief Create a new rotor control widget.
 * \param[in] min The lower limit in decimal degrees.
 * \param[in] max The upper limit in decimal degrees.
 * \param[in] val The initial value of the control.
 * \return A new rotor control widget.
 * 
 */
GtkWidget *
gtk_rot_knob_new (gdouble min, gdouble max, gdouble val)
{
    GtkWidget *widget;
    GtkWidget *table;
    GtkWidget *label;
    guint i;

     widget = g_object_new (GTK_TYPE_ROT_KNOB, NULL);

    GTK_ROT_KNOB(widget)->min = min;
    GTK_ROT_KNOB(widget)->max = max;
    GTK_ROT_KNOB(widget)->value = val;
    
    /* create table */
    table = gtk_table_new (3, 8, FALSE);
    
    /* create buttons */
    /* +100 deg */
    GTK_ROT_KNOB(widget)->buttons[0] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[0]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[0]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[0]),
                       "delta", GINT_TO_POINTER(10000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[0],
                      1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[0], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +10 deg */
    GTK_ROT_KNOB(widget)->buttons[1] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[1]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[1]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[1]),
                       "delta", GINT_TO_POINTER(1000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[1],
                      2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[1], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +1 deg */
    GTK_ROT_KNOB(widget)->buttons[2] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[2]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[2]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[2]),
                       "delta", GINT_TO_POINTER(100));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[2],
                     3, 4, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[2], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +0.1 deg */
    GTK_ROT_KNOB(widget)->buttons[3] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[3]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[3]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[3]),
                       "delta", GINT_TO_POINTER(10));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[3],
                      5, 6, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[3], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +0.01 deg */
    GTK_ROT_KNOB(widget)->buttons[4] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[4]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[4]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[4]),
                       "delta", GINT_TO_POINTER(1));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[4],
                      6, 7, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[4], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -100 deg */
    GTK_ROT_KNOB(widget)->buttons[5] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[5]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[5]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[5]),
                       "delta", GINT_TO_POINTER(-10000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[5],
                      1, 2, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[5], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -10 deg */
    GTK_ROT_KNOB(widget)->buttons[6] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[6]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[6]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[6]),
                       "delta", GINT_TO_POINTER(-1000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[6],
                      2, 3, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[6], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -1 deg */
    GTK_ROT_KNOB(widget)->buttons[7] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[7]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[7]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[7]),
                       "delta", GINT_TO_POINTER(-100));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[7],
                      3, 4, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[7], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -0.1 deg */
    GTK_ROT_KNOB(widget)->buttons[8] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[8]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[8]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[8]),
                       "delta", GINT_TO_POINTER(-10));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[8],
                      5, 6, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[8], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -0.01 deg */
    GTK_ROT_KNOB(widget)->buttons[9] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_KNOB(widget)->buttons[9]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_KNOB(widget)->buttons[9]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_KNOB(widget)->buttons[9]),
                       "delta", GINT_TO_POINTER(-1));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->buttons[9],
                      6, 7, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_KNOB(widget)->buttons[9], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);

    /* create labels */
    for (i = 0; i < 7; i++) {
        GTK_ROT_KNOB(widget)->digits[i] = gtk_label_new (NULL);
        gtk_table_attach (GTK_TABLE (table), GTK_ROT_KNOB(widget)->digits[i],
                          i, i+1, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
    }
    
    /* degree sign */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<span size='xx-large'>\302\260</span>");
    gtk_table_attach (GTK_TABLE (table), label, 7, 8, 1, 2,
                      GTK_SHRINK, GTK_SHRINK, 0, 0);
    
    gtk_rot_knob_update (GTK_ROT_KNOB(widget));
    
    gtk_container_add (GTK_CONTAINER (widget), table);
     gtk_widget_show_all (widget);

    
     return widget;
}


/** \brief Set the value of the rotor control widget.
 * \param[in] knob The rotor control widget.
 * \param[in] val The new value.
 * 
 */
void
gtk_rot_knob_set_value (GtkRotKnob *knob, gdouble val)
{
    /* set the new value */
    if (val <= knob->min)
        knob->value = knob->min;
    else if (val >= knob->max)
        knob->value = knob->max;
    else
        knob->value = val;
    
    /* update the display */
    gtk_rot_knob_update (knob);
}


/** \brief Get the current value of the rotor control widget.
 *  \param[in] knob The rotor control widget.
 *  \return The current value.
 * 
 * Hint: For reading the value you can also access knob->value.
 * 
 */
gdouble
gtk_rot_knob_get_value (GtkRotKnob *knob)
{
    return knob->value;
}


/** \brief Get the upper limit of the control widget
 * \param[in] knob The rotor control widget.
 * \return The upper limit of the control widget.
 */
gdouble
gtk_rot_knob_get_max   (GtkRotKnob *knob)
{
    return knob->max;
}


/** \brief Get the lower limit of the control widget
 * \param[in] knob The rotor control widget.
 * \return The lower limit of the control widget.
 */
gdouble
gtk_rot_knob_get_min   (GtkRotKnob *knob)
{
    return knob->min;
}


/** \brief Set the lower limit of the control widget
 * \param[in] knob The rotor control widget.
 * \param[in] min The new lower limit of the control widget.
 */
void
gtk_rot_knob_set_min   (GtkRotKnob *knob, gdouble min)
{
    /* just som sanity check we have only 3 digits */
    if (min < 1000) {
        knob->min = min;
        
        /* ensure that current value is within range */
        if (knob->value < knob->min) {
            knob->value = knob->min;
            gtk_rot_knob_update (knob);
        }
    }
}

/** \brief Set the upper limit of the control widget
 * \param[in] knob The rotor control widget.
 * \param[in] min The new upper limit of the control widget.
 */
void
gtk_rot_knob_set_max (GtkRotKnob *knob, gdouble max)
{
    /* just som sanity check we have only 3 digits */
    if (max < 1000) {
        knob->max = max;
        
        /* ensure that current value is within range */
        if (knob->value > knob->max) {
            knob->value = knob->max;
            gtk_rot_knob_update (knob);
        }
    }
}


/** \brief Set the range of the control widget
 * \param[in] knob The rotor control widget.
 * \param[in] min The new lower limit of the control widget.
 * \param[in] max The new upper limit of the control widget.
 */
void
gtk_rot_knob_set_range (GtkRotKnob *knob, gdouble min, gdouble max)
{
    gtk_rot_knob_set_min (knob, min);
    gtk_rot_knob_set_max (knob, max);
}


/** \brief Update rotor display widget.
 *  \param[in] knob The rotor control widget.
 * 
 */
static void
gtk_rot_knob_update     (GtkRotKnob *knob)
{
    gchar b[7];
    gchar *buff;
    guint i;
    
    g_ascii_formatd (b, 8, "%6.2f", fabs(knob->value)); 
    
    /* set label markups */
    for (i = 0; i < 6; i++) {
        buff = g_strdup_printf (FMTSTR, b[i]);
        gtk_label_set_markup (GTK_LABEL(knob->digits[i+1]), buff);
        g_free (buff);
    }
    
    if (knob->value < 0)
        buff = g_strdup_printf (FMTSTR, '-');
    else
        buff = g_strdup_printf (FMTSTR, ' ');
    
    gtk_label_set_markup (GTK_LABEL(knob->digits[0]), buff);
    g_free (buff);
}


/** \brief Button clicked event.
 * \param button The button that was clicked.
 * \param data Pointer to the GtkRotKnob widget.
 * 
 */
static void
button_clicked_cb (GtkWidget *button, gpointer data)
{
    GtkRotKnob *knob = GTK_ROT_KNOB (data);
    gdouble delta = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (button), "delta")) / 100.0;
    
    if ((delta > 0.0) && ((knob->value + delta) <= knob->max+.005)) {
        knob->value += delta;
          if (knob->value>knob->max){
               knob->value=knob->max;
          }
    }
    else if ((delta < 0.0) && ((knob->value + delta) >= knob->min-.005)) {
        knob->value += delta;
          if (knob->value<knob->min){
               knob->value=knob->min;
          }
    } else {
        //g_print("Val: %.2f %.2f %.10f\n",knob->value,delta,knob->value+delta);
     }
    
    gtk_rot_knob_update (knob);
    
    /*g_print ("VAL: %.2f\n", knob->value);*/
}


