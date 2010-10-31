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
/** \brief Pop-up menu used by GtkSatList, GtkSatMap, etc.
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "compat.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-pass-dialogs.h"
#include "pass-popup-menu.h"


/* wee need this as toplevel, because gtk_widget_get_toplevel
   does not work very good with menuitems :-L */
extern GtkWidget *app;


static void show_pass_details       (GtkWidget *menuitem, gpointer data);
static void polar_plot_pass_details (GtkWidget *menuitem, gpointer data);
static void azel_plot_pass_details  (GtkWidget *menuitem, gpointer data);


/** \brief Show popup menu for a pass.
 *
 *
 */
void
pass_popup_menu_exec (qth_t *qth, pass_t *pass, GdkEventButton *event, GtkWidget *toplevel)
{
     GtkWidget        *menu;
     GtkWidget        *menuitem;
     GtkWidget        *image;
     gchar            *buff;


     menu = gtk_menu_new ();

     /* pass details */
     menuitem = gtk_image_menu_item_new_with_label (_("Show details"));
     image = gtk_image_new_from_stock (GTK_STOCK_JUSTIFY_FILL,
                           GTK_ICON_SIZE_MENU);
     gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
     g_object_set_data (G_OBJECT (menuitem), "pass", pass);
     g_object_set_data (G_OBJECT (menuitem), "qth", qth);
     g_signal_connect (menuitem, "activate",
                 G_CALLBACK (show_pass_details),
                 toplevel);
     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
     
     /* separator */
     menuitem = gtk_separator_menu_item_new ();
     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);


     /* Polar plot pass */
     menuitem = gtk_image_menu_item_new_with_label (_("Polar plot"));
     buff = icon_file_name ("gpredict-polar-small.png");
     image = gtk_image_new_from_file (buff);
     g_free (buff);
     gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
     g_object_set_data (G_OBJECT (menuitem), "pass", pass);
     g_object_set_data (G_OBJECT (menuitem), "qth", qth);
     g_signal_connect (menuitem, "activate",
                 G_CALLBACK (polar_plot_pass_details),
                 toplevel);
     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
     gtk_widget_set_sensitive (menuitem, FALSE);
          
     /* Az/El plot pass */
     menuitem = gtk_image_menu_item_new_with_label (_("Az/El plot"));
     buff = icon_file_name ("gpredict-azel-small.png");
     image = gtk_image_new_from_file (buff);
     g_free (buff);
     gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
     g_object_set_data (G_OBJECT (menuitem), "pass", pass);
     g_object_set_data (G_OBJECT (menuitem), "qth", qth);
     g_signal_connect (menuitem, "activate",
                 G_CALLBACK (azel_plot_pass_details),
                 toplevel);
     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
     gtk_widget_set_sensitive (menuitem, FALSE);


     gtk_widget_show_all (menu);

     /* Note: event can be NULL here when called from view_onPopupMenu;
      *  gdk_event_get_time() accepts a NULL argument */
     gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
               (event != NULL) ? event->button : 0,
               gdk_event_get_time ((GdkEvent*) event));
          

}



/** \brief Show pass details.
 *  \param menuitem The seelcted menuitem.
 *  \param data Pointer to the toplevel window.
 *
 * This function is called when the user selects the "Show details" menu item
 * in the upcoming passes popup menu.
 */
static void
show_pass_details       (GtkWidget *menuitem, gpointer data)
{
     pass_t *pass;
     qth_t *qth;

     //pass = copy_pass (PASS (g_object_get_data (G_OBJECT (menuitem), "pass")));
     pass = PASS (g_object_get_data (G_OBJECT (menuitem), "pass"));
     qth = (qth_t *) g_object_get_data (G_OBJECT (menuitem), "qth");

     show_pass (pass->satname, qth, pass, data);
}

/* data = toplevel window */
static void
polar_plot_pass_details (GtkWidget *menuitem, gpointer data)
{
     g_print ("FIXME: %s:%s not implemented!\n", __FILE__, __FUNCTION__);
}


/* data = toplevel window */
static void
azel_plot_pass_details  (GtkWidget *menuitem, gpointer data)
{
     g_print ("FIXME: %s:%s not implemented!\n", __FILE__, __FUNCTION__);
}

