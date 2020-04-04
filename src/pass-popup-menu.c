/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.
 
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "config-keys.h"
#include "orbit-tools.h"
#include "pass-popup-menu.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pass-dialogs.h"
#include "sgpsdp/sgp4sdp4.h"

extern GtkWidget *app;


/**
 * Show pass details.
 *
 * @param menuitem The seelcted menuitem.
 * @param data Pointer to the toplevel window.
 *
 * This function is called when the user selects the "Show details" menu item
 * in the upcoming passes popup menu.
 */
static void show_pass_details(GtkWidget * menuitem, gpointer data)
{
    pass_t         *pass;
    qth_t          *qth;

    pass = PASS(g_object_get_data(G_OBJECT(menuitem), "pass"));
    qth = (qth_t *) g_object_get_data(G_OBJECT(menuitem), "qth");

    show_pass(pass->satname, qth, pass, data);
}

/* data = toplevel window */
static void polar_plot_pass_details(GtkWidget * menuitem, gpointer data)
{
    (void)menuitem;
    (void)data;

    g_print("FIXME: %s:%s not implemented!\n", __FILE__, __func__);
}

/* data = toplevel window */
static void azel_plot_pass_details(GtkWidget * menuitem, gpointer data)
{
    (void)menuitem;
    (void)data;

    g_print("FIXME: %s:%s not implemented!\n", __FILE__, __func__);
}

void pass_popup_menu_exec(qth_t * qth, pass_t * pass, GdkEventButton * event,
                          GtkWidget * toplevel)
{
    GtkWidget      *menu;
    GtkWidget      *menuitem;

    menu = gtk_menu_new();

    /* pass details */
    menuitem = gtk_menu_item_new_with_label(_("Show details"));
    g_object_set_data(G_OBJECT(menuitem), "pass", pass);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_signal_connect(menuitem, "activate",
                     G_CALLBACK(show_pass_details), toplevel);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* Polar plot pass */
    menuitem = gtk_menu_item_new_with_label(_("Polar plot"));
    g_object_set_data(G_OBJECT(menuitem), "pass", pass);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_signal_connect(menuitem, "activate",
                     G_CALLBACK(polar_plot_pass_details), toplevel);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_set_sensitive(menuitem, FALSE);

    /* Az/El plot pass */
    menuitem = gtk_menu_item_new_with_label(_("Az/El plot"));
    g_object_set_data(G_OBJECT(menuitem), "pass", pass);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_signal_connect(menuitem, "activate",
                     G_CALLBACK(azel_plot_pass_details), toplevel);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_set_sensitive(menuitem, FALSE);

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
