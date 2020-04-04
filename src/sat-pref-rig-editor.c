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
#include "radio-conf.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-rig-editor.h"


extern GtkWidget *window;       /* dialog window defined in sat-pref.c */
static GtkWidget *dialog;       /* dialog window */
static GtkWidget *name;         /* config name */
static GtkWidget *host;         /* host */
static GtkWidget *port;         /* port number */
static GtkWidget *type;         /* rig type */
static GtkWidget *ptt;          /* PTT */
static GtkWidget *vfo;          /* VFO Up/Down selector */
static GtkWidget *lo;           /* local oscillator of downconverter */
static GtkWidget *loup;         /* local oscillator of upconverter */
static GtkWidget *sigaos;       /* AOS signalling */
static GtkWidget *siglos;       /* LOS signalling */


static void clear_widgets()
{
    gtk_entry_set_text(GTK_ENTRY(name), "");
    gtk_entry_set_text(GTK_ENTRY(host), "localhost");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 4532);     /* hamlib default? */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lo), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(loup), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(type), RIG_TYPE_RX);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ptt), PTT_TYPE_NONE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptt), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sigaos), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(siglos), FALSE);
}

static void update_widgets(radio_conf_t * conf)
{
    /* configuration name */
    gtk_entry_set_text(GTK_ENTRY(name), conf->name);

    /* host name */
    if (conf->host)
        gtk_entry_set_text(GTK_ENTRY(host), conf->host);

    /* port */
    if (conf->port > 1023)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), conf->port);
    else
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 4532); /* hamlib default? */

    /* radio type */
    gtk_combo_box_set_active(GTK_COMBO_BOX(type), conf->type);

    /* ptt */
    gtk_combo_box_set_active(GTK_COMBO_BOX(ptt), conf->ptt);

    /* vfo up/down */
    if (conf->type == RIG_TYPE_DUPLEX)
    {
        if (conf->vfoUp == VFO_MAIN)
            gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 1);
        else if (conf->vfoUp == VFO_SUB)
            gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 2);
        else if (conf->vfoUp == VFO_A)
            gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 3);
        else
            gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 4);
    }

    /* lo down in MHz */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lo), conf->lo / 1000000.0);

    /* lo up in MHz */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(loup), conf->loup / 1000000.0);

    /* AOS / LOS signalling */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sigaos), conf->signal_aos);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(siglos), conf->signal_los);
}

/*
 * Manage VFO changed signals.
 * Widget is the GtkComboBox that received the signal.
 *  
 * This function is called when the user selects a new VFO up/down combination.
 */
static void vfo_changed(GtkWidget * widget, gpointer data)
{
    (void)data;

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == 0 &&
        gtk_combo_box_get_active(GTK_COMBO_BOX(type)) == RIG_TYPE_DUPLEX)
    {
        /* not good, we need to have proper VFO combi for this type */
        gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 1);
    }
}

/*
 * Manage ptt type changed signals.
 * Widget is the GtkComboBox that received the signal.
 *  
 * This function is called when the user selects a new ptt type.
 */
static void ptt_changed(GtkWidget * widget, gpointer data)
{
    (void)data;

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == PTT_TYPE_NONE &&
        gtk_combo_box_get_active(GTK_COMBO_BOX(type)) == RIG_TYPE_TRX)
    {
        /* not good, we need to have PTT for this type */
        gtk_combo_box_set_active(GTK_COMBO_BOX(widget), PTT_TYPE_CAT);
    }
}

/*
 * Manage name changes.
 *
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

/*
 * Manage rig type changed signals.
 * widget os the GtkComboBox that received the signal.
 *  
 * This function is called when the user selects a new radio type.
 */
static void type_changed(GtkWidget * widget, gpointer data)
{
    (void)data;

    /* PTT consistency */
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == RIG_TYPE_TRX)
    {
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(ptt)) == PTT_TYPE_NONE)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(ptt), PTT_TYPE_CAT);
        }
    }

    if ((gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) ==
         RIG_TYPE_TOGGLE_AUTO) ||
        (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) ==
         RIG_TYPE_TOGGLE_MAN))
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(ptt), PTT_TYPE_CAT);
    }


    /* VFO consistency */
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == RIG_TYPE_DUPLEX &&
        gtk_combo_box_get_active(GTK_COMBO_BOX(vfo)) == 0)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 1);
    }
}

static GtkWidget *create_editor_widgets(radio_conf_t * conf)
{
    GtkWidget      *table;
    GtkWidget      *label;

    table = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_grid_set_column_homogeneous(GTK_GRID(table), FALSE);
    gtk_grid_set_row_homogeneous(GTK_GRID(table), FALSE);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);
    gtk_grid_set_row_spacing(GTK_GRID(table), 5);

    /* Config name */
    label = gtk_label_new(_("Name"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    name = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(name), 25);
    g_signal_connect(name, "changed", G_CALLBACK(name_changed), NULL);
    gtk_widget_set_tooltip_text(name,
                                _("Enter a short name for this configuration, "
                                  "e.g. IC910-1.\n"
                                  "Allowed characters: "
                                  "0..9, a..z, A..Z, - and _"));
    gtk_grid_attach(GTK_GRID(table), name, 1, 0, 3, 1);

    /* Host */
    label = gtk_label_new(_("Host"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);

    host = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(host), 50);
    gtk_entry_set_text(GTK_ENTRY(host), "localhost");
    gtk_widget_set_tooltip_text(host,
                                _("Enter the host where rigctld is running. "
                                  "You can use both host name and IP address, "
                                  "e.g. 192.168.1.100\n\n"
                                  "If gpredict and rigctld are running on the "
                                  "same computer use localhost"));
    gtk_grid_attach(GTK_GRID(table), host, 1, 1, 3, 1);

    /* port */
    label = gtk_label_new(_("Port"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 2, 1, 1);

    port = gtk_spin_button_new_with_range(1024, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 4532);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(port), 0);
    gtk_widget_set_tooltip_text(port,
                                _("Enter the port number where rigctld is "
                                  "listening"));
    gtk_grid_attach(GTK_GRID(table), port, 1, 2, 1, 1);

    /* radio type */
    label = gtk_label_new(_("Radio type"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    //gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
    gtk_grid_attach(GTK_GRID(table), label, 0, 3, 1, 1);

    type = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type), _("RX only"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type), _("TX only"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type), _("Half-duplex"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type), _("Full-duplex"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type),
                                   _("FT817/857/897 (auto)"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type),
                                   _("FT817/857/897 (manual)"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(type), RIG_TYPE_RX);
    g_signal_connect(type, "changed", G_CALLBACK(type_changed), NULL);
    gtk_widget_set_tooltip_markup(type,
                                  _("<b>RX only:</b>  The radio shall only be "
                                    "used as receiver. If <i>Monitor PTT "
                                    "status</i> is checked the doppler tuning "
                                    "will be suspended while PTT is ON "
                                    "(manual TX). If not, the controller will "
                                    "always perform doppler tuning and "
                                    "you cannot use the same RIG for uplink.\n\n"
                                    "<b>TX only:</b>  The radio shall only be "
                                    "used for uplink. If <i>Monitor PTT status</i>"
                                    " is checked the doppler tuning will be "
                                    "suspended while PTT is OFF (manual RX).\n\n"
                                    "<b>Half-duplex:</b>  The radio should be "
                                    "used for both up- and downlink but not at "
                                    "the same time. This option requires "
                                    "that the PTT status is monitored (otherwise "
                                    "gpredict cannot know whether to tune the "
                                    "RX or the TX).\n\n"
                                    "<b>Full-duplex:</b>  The radio is a full-duplex"
                                    " radio, such as the IC910H. Gpredict will "
                                    "be continuously tuning both uplink and "
                                    "downlink simultaneously and not care about "
                                    "PTT setting.\n\n"
                                    "<b>FT817/857/897 (auto):</b> "
                                    "This is a special mode that can be used with "
                                    "YAESU FT-817, 857 and 897 radios. These radios"
                                    " do not allow computer control while in TX mode."
                                    " Therefore, TX Doppler correction is applied "
                                    "while the radio is in RX mode by toggling "
                                    "between VFO A/B.\n\n"
                                    "<b>FT817/857/897 (manual):</b> "
                                    "This is similar to the previous mode except"
                                    " that switching to TX is done by pressing the"
                                    " SPACE key on the keyboard. Gpredict will "
                                    "then update the TX Doppler before actually"
                                    " switching to TX."));
    gtk_grid_attach(GTK_GRID(table), type, 1, 3, 2, 1);

    /* ptt */
    label = gtk_label_new(_("PTT status"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 4, 1, 1);

    ptt = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ptt), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ptt), _("Read PTT"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ptt), _("Read DCD"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(ptt), 0);
    g_signal_connect(ptt, "changed", G_CALLBACK(ptt_changed), NULL);
    gtk_widget_set_tooltip_markup(ptt,
                                  _("Select PTT type.\n\n"
                                    "<b>None:</b>\nDon't read PTT status from this radio.\n\n"
                                    "<b>Read PTT:</b>\nRead PTT status using get_ptt CAT command. "
                                    "You have to check that your radio and hamlib supports this.\n\n"
                                    "<b>Read DCD:</b>\nRead PTT status using get_dcd command. "
                                    "This can be used if your radio does not support the read_ptt "
                                    "CAT command and you have a special interface that can "
                                    "read squelch status and send it via CTS."));
    gtk_grid_attach(GTK_GRID(table), ptt, 1, 4, 2, 1);

    /* VFO Up/Down */
    label = gtk_label_new(_("VFO Up/Down"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 5, 1, 1);

    vfo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vfo), _("Not applicable"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vfo),
                                   _("MAIN \342\206\221 / SUB \342\206\223"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vfo),
                                   _("SUB \342\206\221 / MAIN \342\206\223"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vfo),
                                   _("A \342\206\221 / B \342\206\223"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vfo),
                                   _("B \342\206\221 / A \342\206\223"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 0);
    g_signal_connect(vfo, "changed", G_CALLBACK(vfo_changed), NULL);
    gtk_widget_set_tooltip_markup(vfo,
                                  _
                                  ("Select which VFO to use for uplink and downlink. "
                                   "This setting is used for full-duplex radios only, "
                                   "such as the IC-910H, FT-847 and the TS-2000.\n\n"
                                   "<b>IC-910H:</b> MAIN\342\206\221 / SUB\342\206\223\n"
                                   "<b>FT-847:</b> SUB\342\206\221 / MAIN\342\206\223\n"
                                   "<b>TS-2000:</b> B\342\206\221 / A\342\206\223"));
    gtk_grid_attach(GTK_GRID(table), vfo, 1, 5, 2, 1);

    /* Downconverter LO frequency */
    label = gtk_label_new(_("LO Down"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 6, 1, 1);

    lo = gtk_spin_button_new_with_range(-10000, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lo), 0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(lo), 0);
    gtk_widget_set_tooltip_text(lo,
                                _
                                ("Enter the frequency of the local oscillator "
                                 " of the downconverter, if any."));
    gtk_grid_attach(GTK_GRID(table), lo, 1, 6, 2, 1);

    label = gtk_label_new(_("MHz"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 3, 6, 1, 1);

    /* Upconverter LO frequency */
    label = gtk_label_new(_("LO Up"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 7, 1, 1);

    loup = gtk_spin_button_new_with_range(-10000, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(loup), 0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(loup), 0);
    gtk_widget_set_tooltip_text(loup,
                                _
                                ("Enter the frequency of the local oscillator "
                                 "of the upconverter, if any."));
    gtk_grid_attach(GTK_GRID(table), loup, 1, 7, 2, 1);

    label = gtk_label_new(_("MHz"));
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 3, 7, 1, 1);

    /* AOS / LOS signalling */
    label = gtk_label_new(_("Signalling"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 0, 8, 1, 1);

    sigaos = gtk_check_button_new_with_label(_("AOS"));
    gtk_grid_attach(GTK_GRID(table), sigaos, 1, 8, 1, 1);
    gtk_widget_set_tooltip_text(sigaos,
                                _("Enable AOS signalling for this radio."));

    siglos = gtk_check_button_new_with_label(_("LOS"));
    gtk_grid_attach(GTK_GRID(table), siglos, 2, 8, 1, 1);
    gtk_widget_set_tooltip_text(siglos,
                                _("Enable LOS signalling for this radio."));

    if (conf->name != NULL)
        update_widgets(conf);

    gtk_widget_show_all(table);

    return table;
}

/* Apply changes. Returns TRUE if things are ok, FALSE otherwise */
static gboolean apply_changes(radio_conf_t * conf)
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

    /* lo down freq */
    conf->lo = 1000000.0 * gtk_spin_button_get_value(GTK_SPIN_BUTTON(lo));

    /* lo up freq */
    conf->loup = 1000000.0 * gtk_spin_button_get_value(GTK_SPIN_BUTTON(loup));

    /* rig type */
    conf->type = gtk_combo_box_get_active(GTK_COMBO_BOX(type));

    /* ptt */
    conf->ptt = gtk_combo_box_get_active(GTK_COMBO_BOX(ptt));

    /* vfo up/down */
    if (conf->type == RIG_TYPE_DUPLEX)
    {
        switch (gtk_combo_box_get_active(GTK_COMBO_BOX(vfo)))
        {

        case 1:
            conf->vfoUp = VFO_MAIN;
            conf->vfoDown = VFO_SUB;
            break;

        case 2:
            conf->vfoUp = VFO_SUB;
            conf->vfoDown = VFO_MAIN;
            break;

        case 3:
            conf->vfoUp = VFO_A;
            conf->vfoDown = VFO_B;
            break;

        case 4:
            conf->vfoUp = VFO_B;
            conf->vfoDown = VFO_A;
            break;

        default:
            conf->vfoUp = VFO_MAIN;
            conf->vfoDown = VFO_SUB;
            break;
        }
    }

    /* AOS / LOS signalling */
    conf->signal_aos = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sigaos));
    conf->signal_los = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(siglos));

    return TRUE;
}

/* Add or edit a radio configuration */
void sat_pref_rig_editor_run(radio_conf_t * conf)
{
    gint            response;
    gboolean        finished = FALSE;

    /* crate dialog and add contents */
    dialog = gtk_dialog_new_with_buttons(_("Edit radio configuration"),
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

    /* keep the dialog running when CLEAR button is plressed
     * OK and CANCEL will exit the loop
     */
    while (!finished)
    {
        response = gtk_dialog_run(GTK_DIALOG(dialog));

        switch (response)
        {
            /* OK */
        case GTK_RESPONSE_OK:
            if (apply_changes(conf))
                finished = TRUE;
            else
                finished = FALSE;
            break;

            /* CLEAR */
        case GTK_RESPONSE_REJECT:
            clear_widgets();
            break;

            /* Everything else is considered CANCEL */
        default:
            finished = TRUE;
            break;
        }
    }

    gtk_widget_destroy(dialog);
}
