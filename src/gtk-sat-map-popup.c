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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "gtk-sat-data.h"
#include "gtk-sat-map.h"
#include "gtk-sat-map-popup.h"
#include "gtk-sat-map-ground-track.h"
#include "gtk-sat-popup-common.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-info.h"
#include "sat-pass-dialogs.h"
#include "sgpsdp/sgp4sdp4.h"

static void     coverage_toggled(GtkCheckMenuItem * item, gpointer data);
static void     track_toggled(GtkCheckMenuItem * item, gpointer data);

/* static void target_toggled (GtkCheckMenuItem *item, gpointer data); */

/**
 * Show satellite popup menu.
 *
 * @param sat Pointer to the satellite data.
 * @param qth The current location.
 * @param event The mouse-click related event info
 * @param toplevel Pointer to toplevel window.
 *
 */
void gtk_sat_map_popup_exec(sat_t * sat, qth_t * qth,
                            GtkSatMap * satmap,
                            GdkEventButton * event, GtkWidget * toplevel)
{
    GtkWidget      *menu;
    GtkWidget      *menuitem;
    sat_map_obj_t  *obj = NULL;
    gint           *catnum;

    menu = gtk_menu_new();

    /* first menu item is the satellite name, centered */
    menuitem = gtk_menu_item_new();
    gtk_menu_item_set_label(GTK_MENU_ITEM(menuitem), _("Satellite info"));

    /* attach data to menuitem and connect callback */
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_signal_connect(menuitem, "activate", G_CALLBACK(show_sat_info_menu_cb),
                     toplevel);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* add the menu items for current,next, and future passes. */
    add_pass_menu_items(menu, sat, qth, &satmap->tstamp, GTK_WIDGET(satmap));

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* get sat obj since we'll need it for the remaining items */
    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;
    obj = SAT_MAP_OBJ(g_hash_table_lookup(satmap->obj, catnum));
    g_free(catnum);

    /* highlight cov. area */
    menuitem = gtk_check_menu_item_new_with_label(_("Highlight footprint"));
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "obj", obj);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                   obj->showcov);
    g_signal_connect(menuitem, "activate", G_CALLBACK(coverage_toggled),
                     satmap);

    /* show track */
    menuitem = gtk_check_menu_item_new_with_label(_("Ground Track"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                   obj->showtrack);
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_object_set_data(G_OBJECT(menuitem), "obj", obj);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                   obj->showtrack);
    g_signal_connect(menuitem, "activate", G_CALLBACK(track_toggled), satmap);

#if 0
    /* target */
    menuitem = gtk_check_menu_item_new_with_label(_("Set Target"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                   obj->istarget);
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_object_set_data(G_OBJECT(menuitem), "obj", obj);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                   obj->istarget);
    g_signal_connect(menuitem, "activate", G_CALLBACK(target_toggled), satmap);
    gtk_widget_set_sensitive(menuitem, FALSE);
#endif

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

/**
 * Manage toggling of Area Coverage.
 *
 * @param item The menu item that was toggled.
 * @param data Pointer to the GtkSatMap structure.
 *
 */
static void coverage_toggled(GtkCheckMenuItem * item, gpointer data)
{
    sat_map_obj_t  *obj = NULL;
    sat_t          *sat;
    GtkSatMap      *satmap = GTK_SAT_MAP(data);
    guint32         covcol;

    /* get satellite object */
    obj = SAT_MAP_OBJ(g_object_get_data(G_OBJECT(item), "obj"));
    sat = SAT(g_object_get_data(G_OBJECT(item), "sat"));

    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to get satellite object."),
                    __FILE__, __LINE__);
        return;
    }

    /* toggle flag */
    obj->showcov = !obj->showcov;
    gtk_check_menu_item_set_active(item, obj->showcov);

    if (obj->showcov)
    {
        /* remove it from the storage structure */
        g_hash_table_remove(satmap->hidecovs, &(sat->tle.catnr));

    }
    else
    {
        g_hash_table_insert(satmap->hidecovs,
                            &(sat->tle.catnr), (gpointer) 0x1);
    }

    /* set or clear coverage colour */
    if (obj->showcov)
    {
        covcol = mod_cfg_get_int(satmap->cfgdata,
                                 MOD_CFG_MAP_SECTION,
                                 MOD_CFG_MAP_SAT_COV_COL,
                                 SAT_CFG_INT_MAP_SAT_COV_COL);
    }
    else
    {
        covcol = 0x00000000;
    }

    g_object_set(obj->range1, "fill-color-rgba", covcol, NULL);

    if (obj->newrcnum == 2)
    {
        g_object_set(obj->range2, "fill-color-rgba", covcol, NULL);
    }
}

/**
 * Manage toggling of Ground Track.
 *
 * @param item The menu item that was toggled.
 * @param data Pointer to the GtkSatMap structure.
 *
 */
static void track_toggled(GtkCheckMenuItem * item, gpointer data)
{
    sat_map_obj_t  *obj = NULL;
    sat_t          *sat = NULL;
    qth_t          *qth = NULL;
    GtkSatMap      *satmap = GTK_SAT_MAP(data);

    /* get satellite object */
    obj = SAT_MAP_OBJ(g_object_get_data(G_OBJECT(item), "obj"));
    sat = SAT(g_object_get_data(G_OBJECT(item), "sat"));
    qth = (qth_t *) (g_object_get_data(G_OBJECT(item), "qth"));

    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to get satellite object."),
                    __FILE__, __LINE__);
        return;
    }

    /* toggle flag */
    obj->showtrack = !obj->showtrack;
    gtk_check_menu_item_set_active(item, obj->showtrack);

    if (obj->showtrack)
    {
        /* create ground track */
        ground_track_create(satmap, sat, qth, obj);

        /* add it to the storage structure */
        g_hash_table_insert(satmap->showtracks,
                            &(sat->tle.catnr), (gpointer) 0x1);

    }
    else
    {
        /* delete ground track with clear_ssp = TRUE */
        ground_track_delete(satmap, sat, qth, obj, TRUE);

        /* remove it from the storage structure */
        g_hash_table_remove(satmap->showtracks, &(sat->tle.catnr));
    }
}

#if 0
/**
 * Manage toggling of Set Target.
 *
 * @param item The menu item that was toggled.
 * @param data Pointer to the GtkSatMap structure.
 *
 */
static void target_toggled(GtkCheckMenuItem * item, gpointer data)
{
    sat_map_obj_t  *obj = NULL;

    /* get satellite object */
    obj = SAT_MAP_OBJ(g_object_get_data(G_OBJECT(item), "obj"));

    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to get satellite object."),
                    __FILE__, __LINE__);
        return;
    }

    /* toggle flag */
    obj->istarget = !obj->istarget;
    gtk_check_menu_item_set_active(item, obj->istarget);
}
#endif
