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
/** \brief ROTOR control window.
 *  \ingroup widgets
 *
 * The master rotator control UI is implemented as a Gtk+ Widget in order
 * to allow multiple instances. The widget is created from the module
 * popup menu and each module can have several rotator control windows
 * attached to it.
 * 
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include "gtk-rot-ctrl.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif



static void gtk_rot_ctrl_class_init (GtkRotCtrlClass *class);
static void gtk_rot_ctrl_init       (GtkRotCtrl      *list);
static void gtk_rot_ctrl_destroy    (GtkObject       *object);


static GtkWidget *create_status_widgets (void);
static GtkWidget *create_setup_widgets (void);
static GtkWidget *create_control_widgets (void);


static GtkVBoxClass *parent_class = NULL;


GType
gtk_rot_ctrl_get_type ()
{
	static GType gtk_rot_ctrl_type = 0;

	if (!gtk_rot_ctrl_type) {

		static const GTypeInfo gtk_rot_ctrl_info = {
			sizeof (GtkRotCtrlClass),
			NULL,  /* base_init */
			NULL,  /* base_finalize */
			(GClassInitFunc) gtk_rot_ctrl_class_init,
			NULL,  /* class_finalize */
			NULL,  /* class_data */
			sizeof (GtkRotCtrl),
			5,     /* n_preallocs */
			(GInstanceInitFunc) gtk_rot_ctrl_init,
		};

		gtk_rot_ctrl_type = g_type_register_static (GTK_TYPE_VBOX,
												    "GtkRotCtrl",
													&gtk_rot_ctrl_info,
													0);
	}

	return gtk_rot_ctrl_type;
}


static void
gtk_rot_ctrl_class_init (GtkRotCtrlClass *class)
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

	object_class->destroy = gtk_rot_ctrl_destroy;
 
}



static void
gtk_rot_ctrl_init (GtkRotCtrl *ctrl)
{

    
}

static void
gtk_rot_ctrl_destroy (GtkObject *object)
{
	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



/** \brief Create a new rotor control widget.
 * \return A new rotor control window.
 * 
 */
GtkWidget *
gtk_rot_ctrl_new ()
{
    GtkWidget *widget;
    GtkWidget *table;
    guint i;

	widget = g_object_new (GTK_TYPE_ROT_CTRL, NULL);

    /* create contents */
    table = gtk_table_new (2, 2, FALSE);
    gtk_table_attach (GTK_TABLE (table), create_status_widgets (),
                      0, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_table_attach (GTK_TABLE (table), create_setup_widgets (),
                      0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_table_attach (GTK_TABLE (table), create_control_widgets (),
                      1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

    gtk_container_add (GTK_CONTAINER (widget), table);
    
	return widget;
}


/** \brief Create status widgets. */
static GtkWidget *create_status_widgets ()
{
    GtkWidget *frame,*table,*label,*ctrbut;
    
    table = gtk_table_new (3,7, TRUE);
    
    /* table header */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>SAT</b>");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>\316\224</b>");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>SET</b>");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 0, 1);
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>READ</b>");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 5, 6, 0, 1);
    
    /* Azimuth widgets */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>AZ:</b>");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
    
    /* Eevation widgets */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>EL:</b>");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
    
    frame = gtk_frame_new (_("STATUS"));
    gtk_container_add (GTK_CONTAINER (frame), table);
    gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
    
    return frame;
}


/** \brief Create setup widgets. */
static GtkWidget *create_setup_widgets ()
{
    GtkWidget *frame;
    
    frame = gtk_frame_new (_("SETUP"));
    gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
    
    return frame;
}

/** \brief Create control buttons. */
static GtkWidget *create_control_widgets ()
{
    GtkWidget *frame;
    
    frame = gtk_frame_new (_("CONTROL"));
    gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
    
    return frame;
}
