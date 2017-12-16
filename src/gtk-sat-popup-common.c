/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.
                      2011  Charles Suprin, AA1VS

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

#include "gtk-sat-popup-common.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-pass-dialogs.h"


void add_pass_menu_items(GtkWidget * menu, sat_t * sat, qth_t * qth,
                         gdouble * tstamp, GtkWidget * widget)
{
    GtkWidget      *menuitem;

    /* next pass and predict passes */
    if (sat->el > 0.0)
    {
        menuitem = gtk_menu_item_new_with_label(_("Show current pass"));
        g_object_set_data(G_OBJECT(menuitem), "sat", sat);
        g_object_set_data(G_OBJECT(menuitem), "qth", qth);
        g_object_set_data(G_OBJECT(menuitem), "tstamp", tstamp);
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(show_current_pass_cb), widget);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    /* the next pass menu item */
    menuitem = gtk_menu_item_new_with_label(_("Show next pass"));
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_object_set_data(G_OBJECT(menuitem), "tstamp", tstamp);
    g_signal_connect(menuitem, "activate", G_CALLBACK(show_next_pass_cb),
                     widget);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* finally the future pass menu item */
    menuitem = gtk_menu_item_new_with_label(_("Future passes"));
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_object_set_data(G_OBJECT(menuitem), "tstamp", tstamp);
    g_signal_connect(menuitem, "activate", G_CALLBACK(show_future_passes_cb),
                     widget);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

void show_current_pass_cb(GtkWidget * menuitem, gpointer data)
{
    sat_t          *sat;
    qth_t          *qth;
    gdouble        *tstamp;
    GtkWindow      *toplevel =
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data)));

    sat = SAT(g_object_get_data(G_OBJECT(menuitem), "sat"));
    qth = (qth_t *) (g_object_get_data(G_OBJECT(menuitem), "qth"));
    tstamp = (gdouble *) (g_object_get_data(G_OBJECT(menuitem), "tstamp"));

    if (sat->el > 0.0)
        show_next_pass_dialog(sat, qth, *tstamp, toplevel);
}

void show_next_pass_cb(GtkWidget * menuitem, gpointer data)
{
    sat_t          *sat;
    qth_t          *qth;
    gdouble        *tstamp;
    GtkWindow      *toplevel =
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data)));

    sat = SAT(g_object_get_data(G_OBJECT(menuitem), "sat"));
    qth = (qth_t *) (g_object_get_data(G_OBJECT(menuitem), "qth"));
    tstamp = (gdouble *) (g_object_get_data(G_OBJECT(menuitem), "tstamp"));

    if (sat->el < 0)
        show_next_pass_dialog(sat, qth, *tstamp, toplevel);
    else
        /*if the satellite is currently visible
           go to end of pass and then add 10 minutes */
        show_next_pass_dialog(sat, qth, sat->los + 0.007, toplevel);
}

void show_future_passes_cb(GtkWidget * menuitem, gpointer data)
{
    sat_t          *sat;
    qth_t          *qth;
    gdouble        *tstamp;
    GtkWindow      *toplevel =
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data)));

    sat = SAT(g_object_get_data(G_OBJECT(menuitem), "sat"));
    qth = (qth_t *) (g_object_get_data(G_OBJECT(menuitem), "qth"));
    tstamp = (gdouble *) (g_object_get_data(G_OBJECT(menuitem), "tstamp"));

    show_future_passes_dialog(sat, qth, *tstamp, toplevel);
}

void show_next_pass_dialog(sat_t * sat, qth_t * qth, gdouble tstamp,
                           GtkWindow * toplevel)
{
    GtkWidget      *dialog;
    pass_t         *pass;

    /* check whether sat actually has AOS */
    if (has_aos(sat, qth))
    {
        if (sat_cfg_get_bool(SAT_CFG_BOOL_PRED_USE_REAL_T0))
        {
            pass = get_next_pass(sat, qth,
                                 sat_cfg_get_int(SAT_CFG_INT_PRED_LOOK_AHEAD));
        }
        else
        {
            pass = get_pass(sat, qth, tstamp,
                            sat_cfg_get_int(SAT_CFG_INT_PRED_LOOK_AHEAD));
        }

        if (pass != NULL)
        {
            show_pass(sat->nickname, qth, pass, GTK_WIDGET(toplevel));
        }
        else
        {
            /* show dialog that there are no passes within time frame */
            dialog = gtk_message_dialog_new(toplevel,
                                            GTK_DIALOG_MODAL |
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_OK,
                                            _("Satellite %s has no passes\n"
                                              "within the next %d days"),
                                            sat->nickname,
                                            sat_cfg_get_int
                                            (SAT_CFG_INT_PRED_LOOK_AHEAD));

            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }
    else
    {
        /* show dialog telling that this sat never reaches AOS */
        dialog = gtk_message_dialog_new(toplevel,
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        _("Satellite %s has no passes for\n"
                                          "the current ground station!\n\n"
                                          "This can be because the satellite\n"
                                          "is geostationary, decayed or simply\n"
                                          "never comes above the horizon"),
                                        sat->nickname);

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

}


void show_future_passes_dialog(sat_t * sat, qth_t * qth, gdouble tstamp,
                               GtkWindow * toplevel)
{
    GSList         *passes = NULL;
    GtkWidget      *dialog;

    /* check wheather sat actially has AOS */
    if (has_aos(sat, qth))
    {

        if (sat_cfg_get_bool(SAT_CFG_BOOL_PRED_USE_REAL_T0))
        {
            passes = get_next_passes(sat, qth,
                                     sat_cfg_get_int
                                     (SAT_CFG_INT_PRED_LOOK_AHEAD),
                                     sat_cfg_get_int
                                     (SAT_CFG_INT_PRED_NUM_PASS));
        }
        else
        {
            passes = get_passes(sat, qth, tstamp,
                                sat_cfg_get_int(SAT_CFG_INT_PRED_LOOK_AHEAD),
                                sat_cfg_get_int(SAT_CFG_INT_PRED_NUM_PASS));

        }


        if (passes != NULL)
        {
            show_passes(sat->nickname, qth, passes, GTK_WIDGET(toplevel));
        }
        else
        {
            /* show dialog that there are no passes within time frame */
            dialog = gtk_message_dialog_new(toplevel,
                                            GTK_DIALOG_MODAL |
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_OK,
                                            _("Satellite %s has no passes\n"
                                              "within the next %d days"),
                                            sat->nickname,
                                            sat_cfg_get_int
                                            (SAT_CFG_INT_PRED_LOOK_AHEAD));

            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }

    }
    else
    {
        /* show dialog */
        GtkWidget      *dialog;

        dialog = gtk_message_dialog_new(toplevel,
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        _("Satellite %s has no passes for\n"
                                          "the current ground station!"),
                                        sat->nickname);

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}
