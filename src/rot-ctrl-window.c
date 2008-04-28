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
#include "sat-log.h"
#include "compat.h"

#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif

#include "rot-ctrl-window.h"


/** \brief Flag indicating whether the window is open or not. */
static gboolean window_is_open = FALSE;

/** \brief The rotor control window widget. */
static GtkWidget *win;


static GtkWidget *AzSat,*AzDelta,*AzLock,*AzSet,*AzRead;
static GtkWidget *ElSat,*ElDelta,*ElLock,*ElSet,*ElRead;


static gint     rot_win_delete  (GtkWidget *, GdkEvent *, gpointer);
static void     rot_win_destroy (GtkWidget *, gpointer);
static gboolean rot_win_config  (GtkWidget *, GdkEventConfigure *, gpointer);

static GtkWidget *create_status_widgets (void);
static GtkWidget *create_setup_widgets (void);
static GtkWidget *create_control_widgets (void);



/** \brief Open rotator control window */
void rot_ctrl_window_open ()
{
    GtkWidget *table;
    gchar     *icon;      /* icon file name */
    
    
    if (window_is_open) {
        /* window may be hidden so bring it to front */
        gtk_window_present (GTK_WINDOW (win));
        return;
    }
    
    /* create the window */
    win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (win), _("GPREDICT Rotator Control"));
    icon = icon_file_name ("gpredict-antenna.png");
    if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
        gtk_window_set_icon_from_file (GTK_WINDOW (win), icon, NULL);
    }
    g_free (icon);
    
    /* connect delete and destroy signals */
    g_signal_connect (G_OBJECT (win), "delete_event",
                      G_CALLBACK (rot_win_delete), NULL);
    g_signal_connect (G_OBJECT (win), "configure_event",
                      G_CALLBACK (rot_win_config), NULL);
    g_signal_connect (G_OBJECT (win), "destroy",
                      G_CALLBACK (rot_win_destroy), NULL);

    /* create contents */
    table = gtk_table_new (2, 2, FALSE);
    gtk_table_attach (GTK_TABLE (table), create_status_widgets (),
                      0, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_table_attach (GTK_TABLE (table), create_setup_widgets (),
                      0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_table_attach (GTK_TABLE (table), create_control_widgets (),
                      1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    
    gtk_container_add (GTK_CONTAINER (win), table);
    gtk_widget_show_all (win);
    
    window_is_open = TRUE;
}



/** \brief Close rotator control window */
void rot_ctrl_window_close ()
{
    if (window_is_open) {
        gtk_widget_destroy (win);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s: Rotor control window is not open!"),
                     __FUNCTION__);
    }
}




/** \brief Manage window delete events. */
static gint
rot_win_delete      (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    return FALSE;
}



/** \brief Manage destroy signals. */
static void
rot_win_destroy    (GtkWidget *widget, gpointer   data)
{
    /* cloase active hardware connections. */
    
    window_is_open = FALSE;
}


/** \brief Manage configure events. */
static gboolean
rot_win_config   (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    return FALSE;
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
      
