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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sat-cfg.h"
#include "sat-log.h"
#include "tle-update.h"
#include "compat.h"
#include "config-keys.h"
#include "rdv.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif


static gboolean running = FALSE;
static GtkWidget *win;


static gboolean rdv_win_create  (void);
static gint     rdv_win_delete  (GtkWidget *, GdkEvent *, gpointer);
static void     rdv_win_destroy (GtkWidget *, gpointer);
static gboolean rdv_win_config  (GtkWidget *, GdkEventConfigure *, gpointer);


void
rdv_open  ()
{
    if (!running) {
		if (rdv_win_create ()) {
			running = TRUE;
			gtk_widget_show_all (win);
		}
    }

    else {
        /* RDV tool already running; bring window to front */
		gtk_window_present (GTK_WINDOW (win));
    }

}



void
rdv_close ()
{
    if (running) {
        /* shut down RDV Tool */
        running = FALSE;
    }
    else {
        /* RDV Tool is not running! */
    }
}



static gboolean
rdv_win_create ()
{
	gchar     *title;     /* window title */
	gchar     *icon;      /* icon file name */
	
	/* create window title and file name for window icon  */
	title = g_strdup (_("Gpredict Space Shuttle Tool"));
	icon = icon_file_name ("gpredict-shuttle.png");

	/* ceate window, add title and icon, restore size and position */
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title (GTK_WINDOW (win), title);

	//	gtk_container_add (GTK_CONTAINER (win),
	//				   gui_create (app));

	if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
		gtk_window_set_icon_from_file (GTK_WINDOW (win), icon, NULL);
	}

	g_free (title);
	g_free (icon);

	/* connect delete and destroy signals */
	g_signal_connect (G_OBJECT (win), "delete_event",
					  G_CALLBACK (rdv_win_delete), NULL);    
	g_signal_connect (G_OBJECT (win), "configure_event",
					  G_CALLBACK (rdv_win_config), NULL);    
	g_signal_connect (G_OBJECT (win), "destroy",
					  G_CALLBACK (rdv_win_destroy), NULL);


	return TRUE;
}


static gint
rdv_win_delete      (GtkWidget *widget,
					 GdkEvent  *event,
					 gpointer   data)
{
	return FALSE;
}



static void
rdv_win_destroy    (GtkWidget *widget,
					gpointer   data)
{
	running = FALSE;
}



static gboolean
rdv_win_config   (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
#if 0
	gint x, y;

	/* data is only useful when window is visible */
	if (GTK_WIDGET_VISIBLE (widget))
 		gtk_window_get_position (GTK_WINDOW (widget), &x, &y);
	else
 		return FALSE; /* carry on normally */
 
#ifdef G_OS_WIN32
	/* Workaround for GTK+ bug # 169811 - "configure_event" is fired
	   when the window is being maximized */
 	if (gdk_window_get_state (widget->window) & GDK_WINDOW_STATE_MAXIMIZED) {
 		return FALSE;
	}
#endif

	/* don't save off-screen positioning */
   	if (x + event->width < 0 || y + event->height < 0 ||
		x > gdk_screen_width() || y > gdk_screen_height()) {

 		return FALSE; /* carry on normally */
	}

 	/* store the position and size */
	sat_cfg_set_int (SAT_CFG_INT_WINDOW_POS_X, x);
	sat_cfg_set_int (SAT_CFG_INT_WINDOW_POS_Y, y);
	sat_cfg_set_int (SAT_CFG_INT_WINDOW_WIDTH, event->width);
	sat_cfg_set_int (SAT_CFG_INT_WINDOW_HEIGHT, event->height);
#endif
	/* continue to handle event normally */
	return FALSE;
}
