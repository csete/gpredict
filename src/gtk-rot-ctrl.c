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
#include "gtk-rot-ctrl.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif


#define FMTSTR "<span size='xx-large'>%c</span>"


static void gtk_rot_ctrl_class_init (GtkRotCtrlClass *class);
static void gtk_rot_ctrl_init       (GtkRotCtrl      *list);
static void gtk_rot_ctrl_destroy    (GtkObject       *object);

static void gtk_rot_ctrl_update     (GtkRotCtrl *ctrl);

static void button_clicked_cb (GtkWidget *button, gpointer data);


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
    guint i;

	widget = g_object_new (GTK_TYPE_ROT_CTRL, NULL);

    GTK_ROT_CTRL(widget)->min = min;
    GTK_ROT_CTRL(widget)->max = max;
    GTK_ROT_CTRL(widget)->value = val;
    
    /* create table */
    table = gtk_table_new (3, 7, FALSE);
    
    /* create buttons */
    /* +100 deg */
    GTK_ROT_CTRL(widget)->buttons[0] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[0]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[0]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[0]),
                       "delta", GINT_TO_POINTER(10000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[0],
                      1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[0], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +10 deg */
    GTK_ROT_CTRL(widget)->buttons[1] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[1]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[1]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[1]),
                       "delta", GINT_TO_POINTER(1000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[1],
                      2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[1], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +1 deg */
    GTK_ROT_CTRL(widget)->buttons[2] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[2]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[2]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[2]),
                       "delta", GINT_TO_POINTER(100));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[2],
                     3, 4, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[2], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +0.1 deg */
    GTK_ROT_CTRL(widget)->buttons[3] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[3]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[3]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[3]),
                       "delta", GINT_TO_POINTER(10));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[3],
                      5, 6, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[3], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* +0.01 deg */
    GTK_ROT_CTRL(widget)->buttons[4] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[4]),
                       gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[4]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[4]),
                       "delta", GINT_TO_POINTER(1));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[4],
                      6, 7, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[4], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -100 deg */
    GTK_ROT_CTRL(widget)->buttons[5] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[5]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[5]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[5]),
                       "delta", GINT_TO_POINTER(-10000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[5],
                      1, 2, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[5], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -10 deg */
    GTK_ROT_CTRL(widget)->buttons[6] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[6]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[6]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[6]),
                       "delta", GINT_TO_POINTER(-1000));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[6],
                      2, 3, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[6], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -1 deg */
    GTK_ROT_CTRL(widget)->buttons[7] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[7]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[7]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[7]),
                       "delta", GINT_TO_POINTER(-100));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[7],
                      3, 4, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[7], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -0.1 deg */
    GTK_ROT_CTRL(widget)->buttons[8] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[8]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[8]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[8]),
                       "delta", GINT_TO_POINTER(-10));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[8],
                      5, 6, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[8], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);
    
    /* -0.01 deg */
    GTK_ROT_CTRL(widget)->buttons[9] = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER(GTK_ROT_CTRL(widget)->buttons[9]),
                       gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE));
    gtk_button_set_relief (GTK_BUTTON(GTK_ROT_CTRL(widget)->buttons[9]),
                           GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (GTK_ROT_CTRL(widget)->buttons[9]),
                       "delta", GINT_TO_POINTER(-1));
    gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->buttons[9],
                      6, 7, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
    g_signal_connect (GTK_ROT_CTRL(widget)->buttons[9], "clicked",
                      G_CALLBACK (button_clicked_cb), widget);

    /* create labels */
    for (i = 0; i < 7; i++) {
        GTK_ROT_CTRL(widget)->digits[i] = gtk_label_new (NULL);
        gtk_table_attach (GTK_TABLE (table), GTK_ROT_CTRL(widget)->digits[i],
                          i, i+1, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
    }
    
    gtk_rot_ctrl_update (GTK_ROT_CTRL(widget));

    
    gtk_container_add (GTK_CONTAINER (widget), table);
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
 *  \param[in] ctrl The rotor control widget.
 * 
 */
static void
gtk_rot_ctrl_update     (GtkRotCtrl *ctrl)
{
    gchar b[7];
    gchar *buff;
    guint i;
    
    g_ascii_formatd (b, 8, "%6.2f", fabs(ctrl->value)); 
    
    /* set label markups */
    for (i = 0; i < 6; i++) {
        buff = g_strdup_printf (FMTSTR, b[i]);
        gtk_label_set_markup (GTK_LABEL(ctrl->digits[i+1]), buff);
        g_free (buff);
    }
    
    if (ctrl->value <= 0)
        buff = g_strdup_printf (FMTSTR, '-');
    else
        buff = g_strdup_printf (FMTSTR, ' ');
    
    gtk_label_set_markup (GTK_LABEL(ctrl->digits[0]), buff);
    g_free (buff);
}


/** \brief Button clicked event.
 * \param button The button that was clicked.
 * \param data Pointer to the GtkRotCtrl widget.
 * 
 */
static void
button_clicked_cb (GtkWidget *button, gpointer data)
{
    GtkRotCtrl *ctrl = GTK_ROT_CTRL (data);
    gfloat delta = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (button), "delta")) / 100.0;
    
    if ((delta > 0.0) && ((ctrl->value + delta) <= ctrl->max)) {
        ctrl->value += delta;
    }
    else if ((delta < 0.0) && ((ctrl->value + delta) >= ctrl->min)) {
        ctrl->value += delta;
    }
    
    gtk_rot_ctrl_update (ctrl);
    
    g_print ("VAL: %.2f\n", ctrl->value);
}
