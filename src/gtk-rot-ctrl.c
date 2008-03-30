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
/** \brief ROTOR control.
 *
 * More info...
 * 
 * \bug This should be a generic widget, not just rotor specific
 * 
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gtk-rot-ctrl.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif



static void gtk_rot_ctrl_class_init (GtkRotCtrlClass *class);
static void gtk_rot_ctrl_init       (GtkRotCtrl      *list);
static void gtk_rot_ctrl_destroy    (GtkObject       *object);

static void gtk_rot_ctrl_update     (GtkRotCtrl *ctrl);


static GtkHBoxClass *parent_class = NULL;


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
 * \param[in] min The lower limit in decimal degrees.
 * \param[in] max The upper limit in decimal degrees.
 * \param[in] val The initial value of the control.
 * \return A new rotor control widget.
 * 
 */
GtkWidget *
gtk_rot_ctrl_new (gfloat min, gfloat max, gfloat val)
{
    GtkWidget *widget;
    GtkWidget *table;


	widget = g_object_new (GTK_TYPE_ROT_CTRL, NULL);

    GTK_ROT_CTRL(widget)->min = min;
    GTK_ROT_CTRL(widget)->max = max;
    GTK_ROT_CTRL(widget)->value = val;
    
    gtk_rot_ctrl_update (GTK_ROT_CTRL(widget));

	gtk_widget_show_all (widget);

    
    
	return widget;
}


/** \brief Set the value of the rotor control widget.
 * \param[in] ctrl The rotor control widget.
 * \param[in] val The new value.
 * 
 */
void
gtk_rot_ctrl_set_value (GtkRotCtrl *ctrl, gfloat val)
{
    /* set the new value */
    if ((val >= ctrl->min) && (val <= ctrl->max))
        ctrl->value = val;
    
    /* update the display */
    gtk_rot_ctrl_update (ctrl);
}


/** \brief Get the current value of the rotor control widget.
 *  \param[in] ctrl The rotor control widget.
 *  \return The current value.
 * 
 * Hint: For reading the value you can also access ctrl->value.
 * 
 */
gfloat
gtk_rot_ctrl_get_value (GtkRotCtrl *ctrl)
{
    return ctrl->value;
}



/** \brief Update rotor display widget.
 *  \param[in] ctrl The rottor control widget.
 * 
 */
static void
gtk_rot_ctrl_update     (GtkRotCtrl *ctrl)
{
    
}
