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
/** \brief Pop-up menu used by GtkSatList.
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
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-pass-dialogs.h"
#include "gtk-sat-list-popup.h"
#include "sat-info.h"
#include "gtk-sat-popup-common.h"

/** \brief Show satellite popup menu.
 *  \param sat Pointer to the satellite data.
 *  \param qth The current location.
 *  \param event The mouse-click related event info.
 *  \param toplevel The top level window.
 */
void
gtk_sat_list_popup_exec (sat_t *sat, qth_t *qth, GdkEventButton *event, GtkSatList *list)
{
     GtkWidget        *menu;
     GtkWidget        *menuitem;
     GtkWidget        *label;
     GtkWidget        *image;
     gchar            *buff;



     menu = gtk_menu_new ();

     /* first menu item is the satellite name, centered */
     menuitem = gtk_image_menu_item_new ();
     label = gtk_label_new (NULL);
     gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
     buff = g_markup_printf_escaped ("<b>%s</b>", sat->nickname);
     gtk_label_set_markup (GTK_LABEL (label), buff);
     g_free (buff);
     gtk_container_add (GTK_CONTAINER (menuitem), label);
     image = gtk_image_new_from_stock (GTK_STOCK_INFO,
                           GTK_ICON_SIZE_MENU);
     gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);

     /* attach data to menuitem and connect callback */
     g_object_set_data (G_OBJECT (menuitem), "sat", sat);
     g_object_set_data (G_OBJECT (menuitem), "qth", qth);
     g_signal_connect (menuitem, "activate",
                      G_CALLBACK (show_sat_info_menu_cb),
                      gtk_widget_get_toplevel (GTK_WIDGET (list)));

     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

     /* separator */
     menuitem = gtk_separator_menu_item_new ();
     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

     /* add the menu items for current,next, and future passes. */
     add_pass_menu_items(menu,sat,qth,&list->tstamp,GTK_WIDGET(list));

     gtk_widget_show_all (menu);

     /* Note: event can be NULL here when called from view_onPopupMenu;
      *  gdk_event_get_time() accepts a NULL argument */
     gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
               (event != NULL) ? event->button : 0,
               gdk_event_get_time ((GdkEvent*) event));
          

}

