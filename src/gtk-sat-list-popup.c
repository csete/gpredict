/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "config-keys.h"
#include "gtk-sat-list-popup.h"
#include "gtk-sat-popup-common.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-info.h"
#include "sat-log.h"
#include "sat-pass-dialogs.h"
#include "sgpsdp/sgp4sdp4.h"

/**
 * Show satellite popup menu.
 *
 * @param sat Pointer to the satellite data.
 * @param qth The current location.
 * @param event The mouse-click related event info.
 * @param toplevel The top level window.
 */
void gtk_sat_list_popup_exec(sat_t * sat, qth_t * qth, GdkEventButton * event,
                             GtkSatList * list)
{
    GtkWidget      *menu;
    GtkWidget      *menuitem;

    menu = gtk_menu_new();

    /* first menu item is the satellite name, centered */
    menuitem = gtk_menu_item_new();
    gtk_menu_item_set_label(GTK_MENU_ITEM(menuitem), _("Satellite info"));

    /* attach data to menuitem and connect callback */
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_signal_connect(menuitem, "activate",
                     G_CALLBACK(show_sat_info_menu_cb),
                     gtk_widget_get_toplevel(GTK_WIDGET(list)));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* add the menu items for current,next, and future passes. */
    add_pass_menu_items(menu, sat, qth, &list->tstamp, GTK_WIDGET(list));

    gtk_widget_show_all(menu);

    /* gtk_menu_popup got deprecated in 3.22, first available in Ubuntu 18.04 */
#if GTK_MINOR_VERSION < 22
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent *) event));
#else
    (void) event;
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
#endif
}
