/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2015  Alexandru Csete, OZ9AEC.

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

/** \brief Edit radio configuration */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "radio-conf.h"
#include "sat-pref-rig-editor.h"


extern GtkWidget *window;       /* dialog window defined in sat-pref.c */

/* private widgets */
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

static GtkWidget *create_editor_widgets(radio_conf_t * conf);
static void     update_widgets(radio_conf_t * conf);
static void     clear_widgets(void);
static gboolean apply_changes(radio_conf_t * conf);
static void     name_changed(GtkWidget * widget, gpointer data);
static void     type_changed(GtkWidget * widget, gpointer data);
static void     ptt_changed(GtkWidget * widget, gpointer data);
static void     vfo_changed(GtkWidget * widget, gpointer data);


/**
 * \brief Add or edit a radio configuration.
 * \param conf Pointer to a radio configuration.
 *
 * Of conf->name is not NULL the widgets will be populated with the data.
 */
void sat_pref_rig_editor_run(radio_conf_t * conf)
{
    gint            response;
    gboolean        finished = FALSE;


    /* crate dialog and add contents */
    dialog = gtk_dialog_new_with_buttons(_("Edit radio configuration"),
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CLEAR,
                                         GTK_RESPONSE_REJECT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

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
        /* OK */
        case GTK_RESPONSE_OK:
            if (apply_changes(conf))
            {
                finished = TRUE;
            }
            else
            {
                finished = FALSE;
            }
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


/** \brief Create and initialise widgets */
static GtkWidget *create_editor_widgets(radio_conf_t * conf)
{
    GtkWidget      *table;
    GtkWidget      *label;

    table = gtk_table_new(9, 4, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);

    /* Config name */
    label = gtk_label_new(_("Name"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

    name = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(name), 25);
    gtk_widget_set_tooltip_text(name,
                                _("Enter a short name for this configuration, "
                                  "e.g. IC910-1.\n"
                                  "Allowed characters: "
                                  "0..9, a..z, A..Z, - and _"));
    gtk_table_attach_defaults(GTK_TABLE(table), name, 1, 4, 0, 1);

    /* attach changed signal so that we can enable OK button when
       a proper name has been entered
     */
    g_signal_connect(name, "changed", G_CALLBACK(name_changed), NULL);

    /* Host */
    label = gtk_label_new(_("Host"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    host = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(host), 50);
    gtk_entry_set_text(GTK_ENTRY(host), "localhost");
    gtk_widget_set_tooltip_text(host,
                                _("Enter the host where rigctld is running. "
                                  "You can use both host name and IP address, "
                                  "e.g. 192.168.1.100\n\n"
                                  "If gpredict and rigctld are running on the "
                                  "same computer use localhost"));
    gtk_table_attach_defaults(GTK_TABLE(table), host, 1, 4, 1, 2);

    /* port */
    label = gtk_label_new(_("Port"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

    port = gtk_spin_button_new_with_range(1024, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 4532);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(port), 0);
    gtk_widget_set_tooltip_text(port,
                                _("Enter the port number where rigctld is "
                                  "listening"));
    gtk_table_attach_defaults(GTK_TABLE(table), port, 1, 3, 2, 3);

    /* radio type */
    label = gtk_label_new(_("Radio type"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);

    type = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(type), _("RX only"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(type), _("TX only"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(type), _("Simplex TRX"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(type), _("Duplex TRX"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(type), _("FT817/857/897 (auto)"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(type),
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
                                    "<b>Simplex TRX:</b>  The radio should be "
                                    "used for both up- and downlink but in "
                                    "simplex mode only. This option requires "
                                    "that the PTT status is monitored (otherwise "
                                    "gpredict cannot know whether to tune the "
                                    "RX or the TX).\n\n"
                                    "<b>Duplex:</b>  The radio is a full duplex "
                                    "radio, such as the IC910H. Gpredict will "
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
    gtk_table_attach_defaults(GTK_TABLE(table), type, 1, 3, 3, 4);

    /* ptt */
    label = gtk_label_new(_("PTT status"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 4, 5);

    ptt = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(ptt), _("None"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(ptt), _("Read PTT"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(ptt), _("Read DCD"));
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
    gtk_table_attach_defaults(GTK_TABLE(table), ptt, 1, 3, 4, 5);

    /* VFO Up/Down */
    label = gtk_label_new(_("VFO Up/Down"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 5, 6);

    vfo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(vfo), _("Not applicable"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(vfo),
                              _("MAIN \342\206\221 / SUB \342\206\223"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(vfo),
                              _("SUB \342\206\221 / MAIN \342\206\223"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(vfo),
                              _("A \342\206\221 / B \342\206\223"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(vfo),
                              _("B \342\206\221 / A \342\206\223"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 0);
    g_signal_connect(vfo, "changed", G_CALLBACK(vfo_changed), NULL);
    gtk_widget_set_tooltip_markup(vfo,
                                  _("Select which VFO to use for uplink and downlink. "
                                    "This setting is used for full-duplex radios only, "
                                    "such as the IC-910H, FT-847 and the TS-2000.\n\n"
                                    "<b>IC-910H:</b> MAIN\342\206\221 / SUB\342\206\223\n"
                                    "<b>FT-847:</b> SUB\342\206\221 / MAIN\342\206\223\n"
                                    "<b>TS-2000:</b> B\342\206\221 / A\342\206\223"));
    gtk_table_attach_defaults(GTK_TABLE(table), vfo, 1, 3, 5, 6);

    /* Downconverter LO frequency */
    label = gtk_label_new(_("LO Down"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 6, 7);

    lo = gtk_spin_button_new_with_range(-10000, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(lo), 0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(lo), 0);
    gtk_widget_set_tooltip_text(lo,
                                _("Enter the frequency of the local oscillator "
                                  " of the downconverter, if any."));
    gtk_table_attach_defaults(GTK_TABLE(table), lo, 1, 3, 6, 7);

    label = gtk_label_new(_("MHz"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 6, 7);

    /* Upconverter LO frequency */
    label = gtk_label_new(_("LO Up"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 7, 8);

    loup = gtk_spin_button_new_with_range(-10000, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(loup), 0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(loup), 0);
    gtk_widget_set_tooltip_text(loup,
                                _("Enter the frequency of the local oscillator "
                                  "of the upconverter, if any."));
    gtk_table_attach_defaults(GTK_TABLE(table), loup, 1, 3, 7, 8);

    label = gtk_label_new(_("MHz"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 7, 8);

    /* AOS / LOS signalling */
    label = gtk_label_new(_("Signalling"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 8, 9);

    sigaos = gtk_check_button_new_with_label(_("AOS"));
    gtk_table_attach_defaults(GTK_TABLE(table), sigaos, 1, 2, 8, 9);
    gtk_widget_set_tooltip_text(sigaos,
                                _("Enable AOS signalling for this radio."));

    siglos = gtk_check_button_new_with_label(_("LOS"));
    gtk_table_attach_defaults(GTK_TABLE(table), siglos, 2, 3, 8, 9);
    gtk_widget_set_tooltip_text(siglos,
                                _("Enable LOS signalling for this radio."));


    if (conf->name != NULL)
        update_widgets(conf);

    gtk_widget_show_all(table);

    return table;
}


/** \brief Update widgets from the currently selected row in the treeview */
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

/**
 * \brief Clear the contents of all widgets.
 *
 * This function is usually called when the user clicks on the CLEAR button
 *
 */
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

/**
 * \brief Apply changes.
 * \return TRUE if things are ok, FALSE otherwise.
 *
 * This function is usually called when the user clicks the OK button.
 */
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

/**
 * \brief Manage name changes.
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

    (void)data;                 /* avoid unused parameter compiler warning */

    /* step 1: ensure that only valid characters are entered
       (stolen from xlog, tnx pg4i)
     */
    entry = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
    if ((len = g_utf8_strlen(entry, -1)) > 0)
    {
        end = entry + g_utf8_strlen(entry, -1);
        for (j = entry; j < end; ++j)
        {
            switch (*j)
            {
            case '0' ... '9':
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '-':
            case '_':
                break;
            default:
                gdk_beep();
                pos = gtk_editable_get_position(GTK_EDITABLE(widget));
                gtk_editable_delete_text(GTK_EDITABLE(widget), pos, pos + 1);
                break;
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

/**
 * \brief Manage rig type changed signals.
 * \param widget The GtkComboBox that received the signal.
 * \param data User data (always NULL).
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
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == RIG_TYPE_DUPLEX)
    {
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(vfo)) == 0)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(vfo), 1);
        }
    }

}

/**
 * \brief Manage ptt type changed signals.
 * \param widget The GtkComboBox that received the signal.
 * \param data User data (always NULL).
 *  
 * This function is called when the user selects a new ptt type.
 */
static void ptt_changed(GtkWidget * widget, gpointer data)
{
    (void)data;

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == PTT_TYPE_NONE)
    {
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(type)) == RIG_TYPE_TRX)
        {
            /* not good, we need to have PTT for this type */
            gtk_combo_box_set_active(GTK_COMBO_BOX(widget), PTT_TYPE_CAT);
        }
    }
}

/**
 * \brief Manage VFO changed signals.
 * \param widget The GtkComboBox that received the signal.
 * \param data User data (always NULL).
 *  
 * This function is called when the user selects a new VFO up/down combination.
 */
static void vfo_changed(GtkWidget * widget, gpointer data)
{
    (void)data;

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == 0)
    {
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(type)) == RIG_TYPE_DUPLEX)
        {
            /* not good, we need to have proper VFO combi for this type */
            gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 1);
        }
    }
}
