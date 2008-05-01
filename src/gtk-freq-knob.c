/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>

  Comments, questions and bugreports should be submitted via
  http://sourceforge.net/projects/groundstation/
  More details can be found at the project home page:

  http://groundstation.sourceforge.net/
 
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
/** \brief FREQ control.
 *
 * More info...
 * 
 * \bug This should be a generic widget, not just frequency specific
 * 
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gtk-freq-knob.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif



static void gtk_freq_knob_class_init (GtkFreqKnobClass *class);
static void gtk_freq_knob_init       (GtkFreqKnob      *list);
static void gtk_freq_knob_destroy    (GtkObject       *object);

static void gtk_freq_knob_update     (GtkFreqKnob *knob);


static GtkHBoxClass *parent_class = NULL;


GType
gtk_freq_knob_get_type ()
{
	static GType gtk_freq_knob_type = 0;

	if (!gtk_freq_knob_type) {

		static const GTypeInfo gtk_freq_knob_info = {
			sizeof (GtkFreqKnobClass),
			NULL,  /* base_init */
			NULL,  /* base_finalize */
			(GClassInitFunc) gtk_freq_knob_class_init,
			NULL,  /* class_finalize */
			NULL,  /* class_data */
			sizeof (GtkFreqKnob),
			5,     /* n_preallocs */
			(GInstanceInitFunc) gtk_freq_knob_init,
		};

		gtk_freq_knob_type = g_type_register_static (GTK_TYPE_VBOX,
												    "GtkFreqKnob",
													&gtk_freq_knob_info,
													0);
	}

	return gtk_freq_knob_type;
}


static void
gtk_freq_knob_class_init (GtkFreqKnobClass *class)
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

	object_class->destroy = gtk_freq_knob_destroy;
 
}



static void
gtk_freq_knob_init (GtkFreqKnob *knob)
{

    
}

static void
gtk_freq_knob_destroy (GtkObject *object)
{
	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



/** \brief Create a new Frequency control widget.
 * \param[in] val The initial value of the control.
 * \return A new frequency control widget.
 * 
 */
GtkWidget *
gtk_freq_knob_new (gdouble val)
{
    GtkWidget *widget;
    GtkWidget *table;


	widget = g_object_new (GTK_TYPE_FREQ_KNOB, NULL);

    GTK_FREQ_KNOB(widget)->value = val;
    gtk_freq_knob_update (GTK_FREQ_KNOB(widget));

	gtk_widget_show_all (widget);

	return widget;
}


/** \brief Set the value of the frequency control widget.
 * \param[in] knob THe frequency control widget.
 * \param[in] val The new value.
 * 
 */
void
gtk_freq_knob_set_value (GtkFreqKnob *knob, gdouble val)
{
    /* set the new value */
    knob->value = val;
    
    /* update the display */
    gtk_freq_knob_update (knob);
}


/** \brief Get the current value of the frequency control widget.
 *  \param[in] knob The frequency control widget.
 *  \return The current value.
 * 
 * Hint: For reading the value you can also access knob->value.
 * 
 */
gdouble
gtk_freq_knob_get_value (GtkFreqKnob *knob)
{
    return knob->value;
}



/** \brief Update frequency display widget.
 *  \param[in] knob The frequency control widget.
 * 
 */
static void
gtk_freq_knob_update     (GtkFreqKnob *knob)
{
    
}
