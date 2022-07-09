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
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-pref.h"
#include "sat-pref-general.h"
#include "sat-pref-interfaces.h"
#include "sat-pref-modules.h"
#include "sat-pref-predict.h"

/** Columns in the icon list */
enum {
    NBOOK_PAGE_GENERAL = 0,
    NBOOK_PAGE_MODULES,
    NBOOK_PAGE_INTERFACE,
    NBOOK_PAGE_PREDICT
};

const gchar    *WINDOW_TITLE[4] = {
    N_("GPREDICT Preferences :: General"),
    N_("GPREDICT Preferences :: Modules"),
    N_("GPREDICT Preferences :: Interfaces"),
    N_("GPREDICT Preferences :: Predict")
};

GtkWidget      *window;         /* dialog window */
extern GtkWidget *app;

static void     button_press_cb(GtkWidget * widget, gpointer nbook);

/**
 * Create and run preferences dialog.
 *
 * The preferences dialog contains a GtkNoteBook, which is used to group
 * the various configuration parts/modules (General, Modules, Interfaces, ...).
 * Each configuration part can contain several subgroups that are managed
 * by the module itself. As an example, consider the "Modules" tab which
 * could have the following sub-groups: General, List View, Map View and so on.
 * The tabs of the notebook are invisible, instead a vertical icon list
 * placed on the left of the notebook is used to navigate through the
 * notebook pages. The icon list is actually implemented using pixmap buttons
 * in a button box. Using something like the GtkIconView would have been better
 * but that seems to be rather useless when packed into a box.
 */
void sat_pref_run()
{
    GtkWidget      *nbook;      /* notebook widget */
    GtkWidget      *hbox;       /* horizontal box */
    GtkWidget      *butbox;
    GtkWidget      *genbut, *modbut, *ifbut, *predbut;
    gchar          *iconfile;
    gint            response;

    /* Create notebook and add individual pages.
       The individual pages will need the GKeyFile
     */
    nbook = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_general_create(),
                             gtk_label_new(_("General")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_modules_create(NULL),
                             gtk_label_new(_("Modules")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_interfaces_create(),
                             gtk_label_new(_("Interfaces")));
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook),
                             sat_pref_predict_create(),
                             gtk_label_new(_("Predict")));

    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(nbook), FALSE);

    /* create a button box and add the buttons one by one */
    genbut = gpredict_vpixmap_button("gpredict-sat-pref.png",
                                     _("General"), NULL);
    gtk_button_set_relief(GTK_BUTTON(genbut), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(genbut), "page",
                      GINT_TO_POINTER(NBOOK_PAGE_GENERAL));
    g_signal_connect(G_OBJECT(genbut), "clicked",
                     G_CALLBACK(button_press_cb), nbook);

    modbut = gpredict_vpixmap_button("gpredict-sat-list.png",
                                     _("Modules"), NULL);
    gtk_button_set_relief(GTK_BUTTON(modbut), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(modbut), "page",
                      GINT_TO_POINTER(NBOOK_PAGE_MODULES));
    g_signal_connect(G_OBJECT(modbut), "clicked",
                     G_CALLBACK(button_press_cb), nbook);

    ifbut = gpredict_vpixmap_button("gpredict-oscilloscope.png",
                                    _("Interfaces"), NULL);
    gtk_button_set_relief(GTK_BUTTON(ifbut), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(ifbut), "page",
                      GINT_TO_POINTER(NBOOK_PAGE_INTERFACE));
    g_signal_connect(G_OBJECT(ifbut), "clicked",
                     G_CALLBACK(button_press_cb), nbook);

    predbut = gpredict_vpixmap_button("gpredict-calendar.png",
                                      _("Predict"), NULL);
    gtk_button_set_relief(GTK_BUTTON(predbut), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(predbut), "page",
                      GINT_TO_POINTER(NBOOK_PAGE_PREDICT));
    g_signal_connect(G_OBJECT(predbut), "clicked",
                     G_CALLBACK(button_press_cb), nbook);

    butbox = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(butbox), GTK_BUTTONBOX_START);
    gtk_container_add(GTK_CONTAINER(butbox), genbut);
    gtk_container_add(GTK_CONTAINER(butbox), modbut);
    gtk_container_add(GTK_CONTAINER(butbox), ifbut);
    gtk_container_add(GTK_CONTAINER(butbox), predbut);

    /* create horizontal box which will contain the icon list on the left side
       and the notebook on the right side.
     */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), butbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), nbook, TRUE, TRUE, 0);
    gtk_widget_show_all(hbox);

    /* create and display preferences window */
    window = gtk_dialog_new_with_buttons(_("Gpredict Preferences :: General"),
                                         GTK_WINDOW(app),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         "_OK", GTK_RESPONSE_ACCEPT,
                                         NULL);
    iconfile = icon_file_name("gpredict-sat-pref.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(window), iconfile, NULL);
    g_free(iconfile);

    gtk_box_pack_start(GTK_BOX
                       (gtk_dialog_get_content_area(GTK_DIALOG(window))),
                       hbox, TRUE, TRUE, 0);
    gtk_box_set_spacing(GTK_BOX
                        (gtk_dialog_get_content_area(GTK_DIALOG(window))), 10);

    gtk_button_clicked(GTK_BUTTON(genbut));

    response = gtk_dialog_run(GTK_DIALOG(window));
    switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
        sat_pref_general_ok();
        sat_pref_modules_ok(NULL);
        sat_pref_interfaces_ok();
        sat_pref_predict_ok();
        sat_cfg_save();
        break;

    default:
        sat_pref_general_cancel();
        sat_pref_modules_cancel(NULL);
        sat_pref_interfaces_cancel();
        sat_pref_predict_cancel();
        break;
    }
    gtk_widget_destroy(window);
}

/** 
 * Handle button press events
 *
 * Basically consists of switching pages in the notebook. The page number is
 * received via the nbook parameter.
 */
static void button_press_cb(GtkWidget * widget, gpointer nbook)
{
    gint            page =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "page"));

    gtk_notebook_set_current_page(GTK_NOTEBOOK(nbook), page);

    gtk_window_set_title(GTK_WINDOW(window), _(WINDOW_TITLE[page]));
}
