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
#include <goocanvas.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "mod-cfg-get-param.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "gtk-polar-view.h"
#include "time-tools.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-pass-dialogs.h"
#include "sat-info.h"
#include "gtk-polar-view-popup.h"


static void track_toggled (GtkCheckMenuItem *item, gpointer data);
/* static void target_toggled (GtkCheckMenuItem *item, gpointer data); */
static GooCanvasItemModel *create_time_tick (GtkPolarView *pv, gdouble time, gfloat x, gfloat y);
static void show_next_pass_cb       (GtkWidget *menuitem, gpointer data);
static void show_next_passes_cb     (GtkWidget *menuitem, gpointer data);


/** \brief Show satellite popup menu.
 *  \param sat Pointer to the satellite data.
 *  \param qth The current location.
 *  \param pview The GtkPolarView widget.
 *  \param event The mouse-click related event info
 *  \param toplevel The toplevel window or NULL.
 *
 */
void
        gtk_polar_view_popup_exec (sat_t *sat,
                                   qth_t *qth,
                                   GtkPolarView *pview,
                                   GdkEventButton *event,
                                   GtkWidget *toplevel)
{
    GtkWidget        *menu;
    GtkWidget        *menuitem;
    GtkWidget        *label;
    GtkWidget        *image;
    gchar            *buff;
    sat_obj_t  *obj = NULL;
    gint           *catnum;



    menu = gtk_menu_new ();

    /* first menu item is the satellite name, centered */
    menuitem = gtk_image_menu_item_new ();
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    buff = g_strdup_printf ("<b>%s</b>", sat->nickname);
    gtk_label_set_markup (GTK_LABEL (label), buff);
    g_free (buff);
    gtk_container_add (GTK_CONTAINER (menuitem), label);
    image = gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);

    /* attach data to menuitem and connect callback */
    g_object_set_data (G_OBJECT (menuitem), "sat", sat);
    g_object_set_data (G_OBJECT (menuitem), "qth", qth);
    g_signal_connect (menuitem, "activate", G_CALLBACK (show_sat_info_menu_cb), toplevel);

    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

    /* separator */
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

    /* next pass and predict passes */
    menuitem = gtk_image_menu_item_new_with_label (_("Show next pass"));
    image = gtk_image_new_from_stock (GTK_STOCK_JUSTIFY_FILL, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
    g_object_set_data (G_OBJECT (menuitem), "sat", sat);
    g_object_set_data (G_OBJECT (menuitem), "qth", qth);
    g_signal_connect (menuitem, "activate", G_CALLBACK (show_next_pass_cb), pview);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_image_menu_item_new_with_label (_("Future passes"));
    image = gtk_image_new_from_stock (GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
    g_object_set_data (G_OBJECT (menuitem), "sat", sat);
    g_object_set_data (G_OBJECT (menuitem), "qth", qth);
    g_signal_connect (menuitem, "activate", G_CALLBACK (show_next_passes_cb), pview);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

    /* separator */
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

    /* get sat obj since we'll need it for the remaining items */
    catnum = g_new0 (gint, 1);
    *catnum = sat->tle.catnr;
    obj = SAT_OBJ (g_hash_table_lookup (pview->obj, catnum));
    g_free (catnum);

    /* show track */
    menuitem = gtk_check_menu_item_new_with_label (_("Sky track"));
    g_object_set_data (G_OBJECT (menuitem), "sat", sat);
    g_object_set_data (G_OBJECT (menuitem), "qth", qth);
    g_object_set_data (G_OBJECT (menuitem), "obj", obj);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), obj->showtrack);
    g_signal_connect (menuitem, "activate", G_CALLBACK (track_toggled), pview);

    /* disable menu item if satellite is geostationary */
    if (sat->otype == ORBIT_TYPE_GEO)
        gtk_widget_set_sensitive (menuitem, FALSE);

    /* target */
    /*      menuitem = gtk_check_menu_item_new_with_label (_("Set as target")); */
    /*      g_object_set_data (G_OBJECT (menuitem), "sat", sat); */
    /*      g_object_set_data (G_OBJECT (menuitem), "qth", qth); */
    /*      g_object_set_data (G_OBJECT (menuitem), "obj", obj); */
    /*      gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem); */
    /*      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), obj->istarget); */
    /*      g_signal_connect (menuitem, "activate", G_CALLBACK (target_toggled), pview); */

    gtk_widget_show_all (menu);

    /* Note: event can be NULL here when called from view_onPopupMenu;
         *  gdk_event_get_time() accepts a NULL argument */
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                    (event != NULL) ? event->button : 0,
                    gdk_event_get_time ((GdkEvent*) event));


}



/** \brief Manage toggling of Ground Track.
 *  \param item The menu item that was toggled.
 *  \param data Pointer to the GtkPolarView structure.
 *
 */
static void
        track_toggled (GtkCheckMenuItem *item, gpointer data)
{
    GtkPolarView       *pv = GTK_POLAR_VIEW (data);
    sat_obj_t          *obj = NULL;
    sat_t              *sat;
    qth_t              *qth;
    gint               idx,i;
    GooCanvasItemModel *root;
    pass_detail_t      *detail;
    guint              num;
    GooCanvasPoints    *points;
    gfloat             x,y;
    guint32            col;
    guint              tres,ttidx;


    /* get satellite object */
    obj = SAT_OBJ(g_object_get_data (G_OBJECT (item), "obj"));
    sat = SAT(g_object_get_data (G_OBJECT (item), "sat"));
    qth = (qth_t *)(g_object_get_data (G_OBJECT (item), "qth"));

    if (obj == NULL) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s:%d: Failed to get satellite object."),
                     __FILE__, __LINE__);
        return;
    }

    /* toggle flag */
    obj->showtrack = !obj->showtrack;
    gtk_check_menu_item_set_active (item, obj->showtrack);

    root = goo_canvas_get_root_item_model (GOO_CANVAS (pv->canvas));

    if (obj->showtrack) {
        /* add sky track */

        /* create points */
        num = g_slist_length (obj->pass->details);
        if (num == 0) {
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%d: Pass has no details."),
                         __FILE__, __LINE__);
            return;
        }

        /* time resolution for time ticks; we need
                   3 additional points to AOS and LOS ticks.
                */
        tres = (num-2) / (TRACK_TICK_NUM-1);

        points = goo_canvas_points_new (num);

        /* first point should be (aos_az,0.0) */
        azel_to_xy (pv, obj->pass->aos_az, 0.0, &x, &y);
        points->coords[0] = (double) x;
        points->coords[1] = (double) y;
        obj->trtick[0] = create_time_tick (pv, obj->pass->aos, x, y);

        ttidx = 1;

        for (i = 1; i < num-1; i++) {
            detail = PASS_DETAIL(g_slist_nth_data (obj->pass->details, i));
            if (detail->el >=0.0)
                azel_to_xy (pv, detail->az, detail->el, &x, &y);
            points->coords[2*i] = (double) x;
            points->coords[2*i+1] = (double) y;

            if (!(i % tres)) {
                /* create a time tick */
                if (ttidx<TRACK_TICK_NUM)
                    obj->trtick[ttidx] = create_time_tick (pv, detail->time, x, y);
                ttidx++;
            }
        }

        /* last point should be (los_az, 0.0)  */
        azel_to_xy (pv, obj->pass->los_az, 0.0, &x, &y);
        points->coords[2*(num-1)] = (double) x;
        points->coords[2*(num-1)+1] = (double) y;

        /* create poly-line */
        col = mod_cfg_get_int (pv->cfgdata,
                               MOD_CFG_POLAR_SECTION,
                               MOD_CFG_POLAR_TRACK_COL,
                               SAT_CFG_INT_POLAR_TRACK_COL);

        obj->track = goo_canvas_polyline_model_new (root, FALSE, 0,
                                                    "points", points,
                                                    "line-width", 1.0,
                                                    "stroke-color-rgba", col,
                                                    "line-cap", CAIRO_LINE_CAP_SQUARE,
                                                    "line-join", CAIRO_LINE_JOIN_MITER,
                                                    NULL);
        goo_canvas_points_unref (points);

        /* put track on the bottom of the sack */
        goo_canvas_item_model_lower (obj->track, NULL);

    }
    else {
        /* delete sky track */
        idx = goo_canvas_item_model_find_child (root, obj->track);

        if (idx != -1) {
            goo_canvas_item_model_remove_child (root, idx);
        }

        for (i = 0; i < TRACK_TICK_NUM; i++) {
            idx = goo_canvas_item_model_find_child (root, obj->trtick[i]);

            if (idx != -1) {
                goo_canvas_item_model_remove_child (root, idx);
            }
        }
    }

}


#if 0
/** \brief Manage toggling of Set Target.
 *  \param item The menu item that was toggled.
 *  \param data Pointer to the GtkPolarView structure.
 *
 */
static void
        target_toggled (GtkCheckMenuItem *item, gpointer data)
{
    sat_obj_t *obj = NULL;



    /* get satellite object */
    obj = SAT_OBJ(g_object_get_data (G_OBJECT (item), "obj"));

    if (obj == NULL) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s:%d: Failed to get satellite object."),
                     __FILE__, __LINE__);
        return;
    }

    /* toggle flag */
    obj->istarget = !obj->istarget;
    gtk_check_menu_item_set_active (item, obj->istarget);
}
#endif


static GooCanvasItemModel *
        create_time_tick (GtkPolarView *pv, gdouble time, gfloat x, gfloat y)
{
    GooCanvasItemModel *item;
    time_t             t;
    gchar              buff[7];
    GtkAnchorType      anchor;
    GooCanvasItemModel *root;
    guint32            col;

    root = goo_canvas_get_root_item_model (GOO_CANVAS (pv->canvas));

    col = mod_cfg_get_int (pv->cfgdata,
                           MOD_CFG_POLAR_SECTION,
                           MOD_CFG_POLAR_TRACK_COL,
                           SAT_CFG_INT_POLAR_TRACK_COL);

    /* convert julian date to struct tm */
    t = (time - 2440587.5)*86400.;

    /* format either local time or UTC depending on check box */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
        strftime (buff, 8, "%H:%M", localtime (&t));
    else
        strftime (buff, 8, "%H:%M", gmtime (&t));

    buff[6]='\0';

    if (x > pv->cx) {
        anchor = GTK_ANCHOR_EAST;
        x -= 5;
    }
    else {
        anchor = GTK_ANCHOR_WEST;
        x += 5;
    }

    item = goo_canvas_text_model_new (root, buff,
                                      (gdouble) x, (gdouble) y,
                                      -1, anchor,
                                      "font", "Sans 7",
                                      "fill-color-rgba", col,
                                      NULL);

    goo_canvas_item_model_lower (item, NULL);

    return item;
}



static void
        show_next_pass_cb       (GtkWidget *menuitem, gpointer data)
{
    GtkPolarView *pv = GTK_POLAR_VIEW (data);
    sat_t        *sat;
    qth_t        *qth;
    pass_t       *pass;
    GtkWidget    *dialog;
    GtkWindow    *toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (data)));


    /* get next pass */
    sat = SAT(g_object_get_data (G_OBJECT (menuitem), "sat"));
    qth = (qth_t *) (g_object_get_data (G_OBJECT (menuitem), "qth"));

    /* check wheather sat actially has AOS */
    if ((sat->otype != ORBIT_TYPE_GEO) && (sat->otype != ORBIT_TYPE_DECAYED) &&
        has_aos (sat, qth)) {
        if (sat_cfg_get_bool(SAT_CFG_BOOL_PRED_USE_REAL_T0)) {
            pass = get_next_pass (sat, qth,
                                  sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD));
        }
        else {
            pass = get_pass (sat, qth, pv->tstamp,
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


static void show_next_passes_cb     (GtkWidget *menuitem, gpointer data)
{
    GtkPolarView *pv = GTK_POLAR_VIEW (data);
    GtkWidget *dialog;
    GtkWindow *toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (data)));
    GSList    *passes = NULL;
    sat_t     *sat;
    qth_t     *qth;


    sat = SAT(g_object_get_data (G_OBJECT (menuitem), "sat"));
    qth = (qth_t *) (g_object_get_data (G_OBJECT (menuitem), "qth"));

    /* check wheather sat actially has AOS */
    if ((sat->otype != ORBIT_TYPE_GEO) && (sat->otype != ORBIT_TYPE_DECAYED) &&
        has_aos (sat, qth)) {

        if (sat_cfg_get_bool(SAT_CFG_BOOL_PRED_USE_REAL_T0)) {
            passes = get_next_passes (sat, qth,
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_LOOK_AHEAD),
                                      sat_cfg_get_int (SAT_CFG_INT_PRED_NUM_PASS));
        }
        else {
            passes = get_passes (sat, qth, pv->tstamp,
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
