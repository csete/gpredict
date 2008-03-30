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
#include "gtk-freq-ctrl.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif



static void gtk_freq_ctrl_class_init (GtkFreqCtrlClass *class);
static void gtk_freq_ctrl_init       (GtkFreqCtrl      *list);
static void gtk_freq_ctrl_destroy    (GtkObject       *object);


static GtkHBoxClass *parent_class = NULL;


GType
gtk_freq_ctrl_get_type ()
{
	static GType gtk_freq_ctrl_type = 0;

	if (!gtk_freq_ctrl_type) {

		static const GTypeInfo gtk_freq_ctrl_info = {
			sizeof (GtkFreqCtrlClass),
			NULL,  /* base_init */
			NULL,  /* base_finalize */
			(GClassInitFunc) gtk_freq_ctrl_class_init,
			NULL,  /* class_finalize */
			NULL,  /* class_data */
			sizeof (GtkFreqCtrl),
			5,     /* n_preallocs */
			(GInstanceInitFunc) gtk_freq_ctrl_init,
		};

		gtk_freq_ctrl_type = g_type_register_static (GTK_TYPE_VBOX,
												    "GtkFreqCtrl",
													&gtk_freq_ctrl_info,
													0);
	}

	return gtk_freq_ctrl_type;
}


static void
gtk_freq_ctrl_class_init (GtkFreqCtrlClass *class)
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

	object_class->destroy = gtk_freq_ctrl_destroy;
 
}



static void
gtk_freq_ctrl_init (GtkFreqCtrl *list)
{

    
}

static void
gtk_freq_ctrl_destroy (GtkObject *object)
{
	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}




GtkWidget *
gtk_freq_ctrl_new ()
{
    GtkWidget *widget;
    GtkWidget *table;


	widget = g_object_new (GTK_TYPE_FREQ_CTRL, NULL);



	gtk_widget_show_all (widget);

	return widget;
}


