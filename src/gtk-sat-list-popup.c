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




void show_next_pass_cb     (GtkWidget *menuitem, gpointer data);
void show_future_passes_cb (GtkWidget *menuitem, gpointer data);


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
    buff = g_strdup_printf ("<b>%s</b>", sat->nickname);
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

     /* next pass and predict passes */
     menuitem = gtk_image_menu_item_new_with_label (_("Show next pass"));
     image = gtk_image_new_from_stock (GTK_STOCK_JUSTIFY_FILL,
                           GTK_ICON_SIZE_MENU);
     gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
     g_object_set_data (G_OBJECT (menuitem), "sat", sat);
     g_object_set_data (G_OBJECT (menuitem), "qth", qth);
     g_signal_connect (menuitem, "activate",
                      G_CALLBACK (show_next_pass_cb),
                      list);
     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
          
     menuitem = gtk_image_menu_item_new_with_label (_("Future passes"));
     image = gtk_image_new_from_stock (GTK_STOCK_INDEX,
                           GTK_ICON_SIZE_MENU);
     gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
     g_object_set_data (G_OBJECT (menuitem), "sat", sat);
     g_object_set_data (G_OBJECT (menuitem), "qth", qth);
     g_signal_connect (menuitem, "activate",
                      G_CALLBACK (show_future_passes_cb),
                      list);
     gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);


     gtk_widget_show_all (menu);

     /* Note: event can be NULL here when called from view_onPopupMenu;
      *  gdk_event_get_time() accepts a NULL argument */
     gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
               (event != NULL) ? event->button : 0,
               gdk_event_get_time ((GdkEvent*) event));
          

}





/** \brief Show details of the next pass.
 *
 */
void
show_next_pass_cb     (GtkWidget *menuitem, gpointer data)
{
     sat_t      *sat;
     qth_t      *qth;
     pass_t     *pass;
     GtkWidget  *dialog;
     GtkWindow  *toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (data)));
    GtkSatList *list = GTK_SAT_LIST (data);


     /* get next pass */
     sat = SAT(g_object_get_data (G_OBJECT (menuitem), "sat"));
     qth = (qth_t *) (g_object_get_data (G_OBJECT (menuitem), "qth"));

     /* check wheather sat actially has AOS */
     if ((sat->otype != ORBIT_TYPE_GEO) && (sat->otype != ORBIT_TYPE_DECAYED) &&
         has_aos (sat, qth)) {

        if (sat_cfg_get_bool (SAT_CFG_BOOL_PRED_USE_REAL_T0)) {
            pass = get_next_pass (sat, qth,
                                  sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD));
        }
        else {
            pass = get_pass (sat, qth, list->tstamp,
                             sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD));
        }

          if (pass != NULL) {
            show_pass (sat->nickname, qth, pass, GTK_WIDGET (toplevel));
          }
          else {
               /* show dialog that there are no passes within time frame */
               dialog = gtk_message_dialog_new (toplevel,
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    _("Satellite %s has no passes\n"\
                                      "within the next %d days"),
                             sat->nickname,
                                    sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD));

               gtk_dialog_run (GTK_DIALOG (dialog));
               gtk_widget_destroy (dialog);
          }
     }
     else {
          /* show dialog telling that this sat never reaches AOS*/
          dialog = gtk_message_dialog_new (toplevel,
                               GTK_DIALOG_MODAL |
                               GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_ERROR,
                               GTK_BUTTONS_OK,
                               _("Satellite %s has no passes for\n"\
                                 "the current ground station!\n\n"\
                                 "This can be because the satellite\n"\
                                 "is geostationary, decayed or simply\n"\
                                 "never comes above the horizon"),
                         sat->nickname);

          gtk_dialog_run (GTK_DIALOG (dialog));
          gtk_widget_destroy (dialog);
     }

}


void
show_future_passes_cb (GtkWidget *menuitem, gpointer data)
{
     GtkWidget *dialog;
     GtkWindow *toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (data)));
    GtkSatList *list = GTK_SAT_LIST (data);
     GSList    *passes = NULL;
     sat_t     *sat;
     qth_t     *qth;



     sat = SAT(g_object_get_data (G_OBJECT (menuitem), "sat"));
     qth = (qth_t *) (g_object_get_data (G_OBJECT (menuitem), "qth"));

     /* check wheather sat actially has AOS */
     if ((sat->otype != ORBIT_TYPE_GEO) && (sat->otype != ORBIT_TYPE_DECAYED) &&
         has_aos (sat, qth)) {

        if (sat_cfg_get_bool (SAT_CFG_BOOL_PRED_USE_REAL_T0)) {
            passes = get_next_passes (sat, qth,
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD),
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_NUM_PASS));
        }
        else {
            passes = get_passes (sat, qth, list->tstamp,
                                 sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD),
                                 sat_cfg_get_int (SAT_CFG_INT_PRED_NUM_PASS));
        }
        
          if (passes != NULL) {
            show_passes (sat->nickname, qth, passes, GTK_WIDGET (toplevel));
          }
          else {
               /* show dialog that there are no passes within time frame */
               dialog = gtk_message_dialog_new (toplevel,
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    _("Satellite %s has no passes\n"\
                                      "within the next %d days"),
                             sat->nickname,
                                    sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD));

               gtk_dialog_run (GTK_DIALOG (dialog));
               gtk_widget_destroy (dialog);
          }

     }
     else {
          /* show dialog */
          GtkWidget *dialog;

          dialog = gtk_message_dialog_new (toplevel,
                               GTK_DIALOG_MODAL |
                               GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_ERROR,
                               GTK_BUTTONS_OK,
                               _("Satellite %s has no passes for\n"\
                                 "the current ground station!"),
                         sat->nickname);

          gtk_dialog_run (GTK_DIALOG (dialog));
          gtk_widget_destroy (dialog);
     }
}



