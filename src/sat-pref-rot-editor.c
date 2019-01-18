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
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <math.h>

#include "gpredict-utils.h"
#include "rotor-conf.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-rot-editor.h"


extern GtkWidget *window;       /* dialog window defined in sat-pref.c */
static GtkWidget *dialog;       /* dialog window */
static GtkWidget *name;         /* Configuration name */
static GtkWidget *host;         /* host name or IP */
static GtkWidget *port;         /* port number */
static GtkWidget *aztype;
static GtkWidget *minaz;
static GtkWidget *maxaz;
static GtkWidget *minel;
static GtkWidget *maxel;
static GtkWidget *azstoppos;

/* Update widgets from the currently selected row in the treeview */
static void update_widgets(rotor_conf_t * conf)
{
    /* configuration name */
    gtk_entry_set_text(GTK_ENTRY(name), conf->name);

    /* host */
    if (conf->host)
        gtk_entry_set_text(GTK_ENTRY(host), conf->host);

    /* port */
    if (conf->port > 1023)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), conf->port);
    else
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 4533); /* hamlib default? */

    gtk_combo_box_set_active(GTK_COMBO_BOX(aztype), conf->aztype);

    /* az and el limits */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minaz), conf->minaz);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxaz), conf->maxaz);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minel), conf->minel);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxel), conf->maxel);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(azstoppos), conf->azstoppos);
}

/* called when the user clicks on the CLEAR button */
static void clear_widgets()
{
    gtk_entry_set_text(GTK_ENTRY(name), "");
    gtk_entry_set_text(GTK_ENTRY(host), "localhost");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 4533);     /* hamlib default? */
    gtk_combo_box_set_active(GTK_COMBO_BOX(aztype), ROT_AZ_TYPE_360);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minaz), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxaz), 360);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minel), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxel), 90);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(azstoppos), 0);
}

/*
 * This function is called when the contents of the name entry changes.
 * The primary purpose of this function is to check whether the char length
 * of the name is greater than zero, if yes enable the OK button of the dialog.
 */
static void name_changed(GtkWidget * widget, gpointer data)
{
    const gchar    *text;
    gchar          *entry, *end, *j;
    gint            len, pos;

    (void)data;

    /* step 1: ensure that only valid characters are entered
       (stolen from xlog, tnx pg4i)
     */
    entry = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
    if ((len = g_utf8_strlen(entry, -1)) > 0)
    {
        end = entry + g_utf8_strlen(entry, -1);
        for (j = entry; j < end; ++j)
        {
            if (!gpredict_legal_char(*j))
            {
                gdk_display_beep(gdk_display_get_default());
                pos = gtk_editable_get_position(GTK_EDITABLE(widget));
                gtk_editable_delete_text(GTK_EDITABLE(widget), pos, pos + 1);
            }
        }
    }

    /* step 2: if name seems all right, enable OK button */
    text = gtk_entry_get_text(GTK_ENTRY(widget));

    if (g_utf8_strlen(text, -1) > 0)
    {
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_OK, TRUE);
    }
    else
    {
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_OK, FALSE);
    }
}

static void aztype_changed_cb(GtkComboBox * box, gpointer data)
{
    gint            type = gtk_combo_box_get_active(box);

    (void)data;

    switch (type)
    {
    case ROT_AZ_TYPE_360:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(minaz), 0.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxaz), 360.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(azstoppos), 0.0);
        break;

    case ROT_AZ_TYPE_180:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(minaz), -180.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxaz), +180.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(azstoppos), -180.0);
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Invalid AZ rotator type."), __FILE__, __func__);
        break;
    }
}

static GtkWidget *create_editor_widgets(rotor_conf_t * conf)
{
    GtkWidget      *table;
    GtkWidget      *label;

    table = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);
    gtk_grid_set_row_spacing(GTK_GRID(table), 5);

    /* Config name */
    label = gtk_label_new(_("Name"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    name = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(name), 25);
    gtk_widget_set_tooltip_text(name,
                                _("Enter a short name for this configuration, "
                                  " e.g. ROTOR-1.\n"
                                  "Allowed characters: 0..9, a..z, A..Z, - and _"));
    gtk_grid_attach(GTK_GRID(table), name, 1, 0, 3, 1);

    /* attach changed signal so that we can enable OK button when
       a proper name has been entered
     */
    g_signal_connect(name, "changed", G_CALLBACK(name_changed), NULL);

    /* Host */
    label = gtk_label_new(_("Host"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);

    host = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(host), 50);
    gtk_entry_set_text(GTK_ENTRY(host), "localhost");
    gtk_widget_set_tooltip_text(host,
                                _("Enter the host where rotctld is running. "
                                  "You can use both host name and IP address, "
                                  "e.g. 192.168.1.100\n\n"
                                  "If gpredict and rotctld are running on the "
                                  "same computer, use localhost"));
    gtk_grid_attach(GTK_GRID(table), host, 1, 1, 3, 1); 

    /* port */
    label = gtk_label_new(_("Port"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 2, 1, 1);

    port = gtk_spin_button_new_with_range(1024, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 4533);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(port), 0);
    gtk_widget_set_tooltip_text(port,
                                _("Enter the port number where rotctld is "
                                  "listening. Default is 4533."));
    gtk_grid_attach(GTK_GRID(table), port, 1, 2, 1, 1); 

    gtk_grid_attach(GTK_GRID(table),
                    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                    0, 3, 4, 1);

    /* Az-type */
    label = gtk_label_new(_("Az type"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 4, 1, 1);

    aztype = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(aztype),
                                   "0\302\260 \342\206\222 180\302\260 \342\206\222 360\302\260");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(aztype),
                                   "-180\302\260 \342\206\222 0\302\260 \342\206\222 +180\302\260");
    gtk_combo_box_set_active(GTK_COMBO_BOX(aztype), 0);
    gtk_widget_set_tooltip_text(aztype,
                                _("Select your azimuth range here. Note that "
                                  "gpredict assumes that 0\302\260 is at North "
                                  "and + direction is clockwise for both types"));
    gtk_grid_attach(GTK_GRID(table), aztype, 1, 4, 2, 1);
    g_signal_connect(G_OBJECT(aztype), "changed",
                     G_CALLBACK(aztype_changed_cb), NULL);

    /* Az and El limits */
    label = gtk_label_new(_(" Min Az"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 5, 1, 1);
    minaz = gtk_spin_button_new_with_range(-200, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minaz), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(minaz), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(minaz), FALSE);
    gtk_grid_attach(GTK_GRID(table), minaz, 1, 5, 1, 1);

    label = gtk_label_new(_(" Max Az"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 5, 1, 1);
    maxaz = gtk_spin_button_new_with_range(0, 450, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxaz), 360);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(maxaz), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(maxaz), FALSE);
    gtk_grid_attach(GTK_GRID(table), maxaz, 3, 5, 1, 1);

    label = gtk_label_new(_(" Min El"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 6, 1, 1);
    minel = gtk_spin_button_new_with_range(-10, 180, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minel), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(minel), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(minel), FALSE);
    gtk_grid_attach(GTK_GRID(table), minel, 1, 6, 1, 1);

    label = gtk_label_new(_(" Max El"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 2, 6, 1, 1);
    maxel = gtk_spin_button_new_with_range(-10, 180, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxel), 90);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(maxel), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(maxel), FALSE);
    gtk_grid_attach(GTK_GRID(table), maxel, 3, 6, 1, 1);

    label = gtk_label_new(_(" Azimuth end stop position"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 1, 7, 2, 1);
    azstoppos = gtk_spin_button_new_with_range(-180, 360, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(azstoppos), 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(azstoppos), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(azstoppos), FALSE);
    gtk_widget_set_tooltip_text(azstoppos,
                                _("Set the position of the azimuth end stop "
                                  "here, where 0\302\260 is at North, "
                                  "-180\302\260 is south, etc. "
                                  "The default for a 0\302\260 \342\206\222 "
                                  "180\302\260 \342\206\222 360\302\260 rotor "
                                  "is 0\302\260, and the default for a "
                                  "-180\302\260 \342\206\222 0\302\260 "
                                  "\342\206\222 +180\302\260 rotor is -180\302\260."));
    gtk_grid_attach(GTK_GRID(table), azstoppos, 3, 7, 1, 1);

    if (conf->name != NULL)
        update_widgets(conf);

    gtk_widget_show_all(table);

    return table;
}

/* Called when the user clicks the OK button */
static gboolean apply_changes(rotor_conf_t * conf)
{
    /* name */
    if (conf->name)
        g_free(conf->name);

    conf->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name)));

    /* host */
    if (conf->host)
        g_free(conf->host);

    conf->host = g_strdup(gtk_entry_get_text(GTK_ENTRY(host)));

    /* port */
    conf->port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(port));

    /* az type */
    conf->aztype = gtk_combo_box_get_active(GTK_COMBO_BOX(aztype));

    /* az and el ranges */
    conf->minaz = gtk_spin_button_get_value(GTK_SPIN_BUTTON(minaz));
    conf->maxaz = gtk_spin_button_get_value(GTK_SPIN_BUTTON(maxaz));
    conf->minel = gtk_spin_button_get_value(GTK_SPIN_BUTTON(minel));
    conf->maxel = gtk_spin_button_get_value(GTK_SPIN_BUTTON(maxel));

    /* az stop position */
    conf->azstoppos = gtk_spin_button_get_value(GTK_SPIN_BUTTON(azstoppos));

    return TRUE;
}

/**
 * Add or edit a rotor configuration.
 *
 * @param conf Pointer to a rotator configuration.
 *
 * If conf->name is not NULL the widgets will be populated with the data.
 */
void sat_pref_rot_editor_run(rotor_conf_t * conf)
{
    gint            response;
    gboolean        finished = FALSE;

    /* crate dialog and add contents */
    dialog = gtk_dialog_new_with_buttons(_("Edit rotator configuration"),
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Clear", GTK_RESPONSE_REJECT,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Ok", GTK_RESPONSE_OK,
                                         NULL);

    /* disable OK button to begin with */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                      GTK_RESPONSE_OK, FALSE);

    gtk_container_add(GTK_CONTAINER
                      (gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                      create_editor_widgets(conf));

    /* this hacky-thing is to keep the dialog running in case the
       CLEAR button is plressed. OK and CANCEL will exit the loop
     */
    while (!finished)
    {
        response = gtk_dialog_run(GTK_DIALOG(dialog));

        switch (response)
        {
        case GTK_RESPONSE_OK:
            if (apply_changes(conf))
                finished = TRUE;
            else
                finished = FALSE;
            break;

        case GTK_RESPONSE_REJECT:
            /* CLEAR */
            clear_widgets();
            break;

        default:
            /* Everything else is considered CANCEL */
            finished = TRUE;
            break;
        }
    }

    gtk_widget_destroy(dialog);
}
