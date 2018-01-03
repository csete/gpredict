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
#include "sat-pref-debug.h"


#define SEC_PER_DAY         86400
#define SEC_PER_WEEK        604800
#define SEC_PER_MONTH       18144000

static GtkWidget *level;
static GtkWidget *age;

static gboolean dirty = FALSE;
static gboolean reset = FALSE;


/* Select proper log file age in combo box. */
static void select_age(void)
{
    gint            num = sat_cfg_get_int(SAT_CFG_INT_LOG_CLEAN_AGE);

    switch (num)
    {
    case SEC_PER_DAY:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 1);
        break;

    case SEC_PER_WEEK:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 2);
        break;

    case SEC_PER_MONTH:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 3);
        break;

    default:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 0);
    }
}

/* User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_debug_cancel()
{
}

/* User pressed OK. Any changes should be stored in config. */
void sat_pref_debug_ok()
{
    gint            num = gtk_combo_box_get_active(GTK_COMBO_BOX(age));

    if (dirty)
    {
        /* store new values */
        sat_cfg_set_int(SAT_CFG_INT_LOG_LEVEL,
                        gtk_combo_box_get_active(GTK_COMBO_BOX(level)));

        switch (num)
        {
        case 1:
            sat_cfg_set_int(SAT_CFG_INT_LOG_CLEAN_AGE, SEC_PER_DAY);
            break;

        case 2:
            sat_cfg_set_int(SAT_CFG_INT_LOG_CLEAN_AGE, SEC_PER_WEEK);
            break;

        case 3:
            sat_cfg_set_int(SAT_CFG_INT_LOG_CLEAN_AGE, SEC_PER_MONTH);
            break;

        default:
            sat_cfg_set_int(SAT_CFG_INT_LOG_CLEAN_AGE, 0);
        }
    }

    else if (reset)
    {
        /* reset values */
        sat_cfg_reset_int(SAT_CFG_INT_LOG_LEVEL);
        sat_cfg_reset_int(SAT_CFG_INT_LOG_CLEAN_AGE);
    }

    dirty = FALSE;
    reset = FALSE;
}

static void state_change_cb(GtkWidget * widget, gpointer data)
{
    (void)widget;
    (void)data;

    dirty = TRUE;
}

static void reset_cb(GtkWidget * button, gpointer data)
{
    gint            num = sat_cfg_get_int_def(SAT_CFG_INT_LOG_CLEAN_AGE);

    (void)button;
    (void)data;

    switch (num)
    {
    case SEC_PER_DAY:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 1);
        break;

    case SEC_PER_WEEK:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 2);
        break;

    case SEC_PER_MONTH:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 3);
        break;

    default:
        gtk_combo_box_set_active(GTK_COMBO_BOX(age), 0);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(level),
                             sat_cfg_get_int_def(SAT_CFG_INT_LOG_LEVEL));

    reset = TRUE;
    dirty = FALSE;
}

GtkWidget      *sat_pref_debug_create()
{
    GtkWidget      *vbox;       /* vbox containing the list part and the details part */
    GtkWidget      *hbox;
    GtkWidget      *rbut;
    GtkWidget      *label;
    GtkWidget      *butbox;
    gchar          *msg;
    gchar          *confdir;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_box_set_spacing(GTK_BOX(vbox), 10);

    /* debug level */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);

    label = gtk_label_new(_("Debug level:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    level = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level),
                                   _("Level 0: None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level),
                                   _("Level 1: Error"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level),
                                   _("Level 2: Warning"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level),
                                   _("Level 3: Info"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level),
                                   _("Level 4: Debug"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(level),
                             sat_cfg_get_int(SAT_CFG_INT_LOG_LEVEL));
    g_signal_connect(G_OBJECT(level), "realize",
                     G_CALLBACK(gpredict_set_combo_tooltips),
                     _("Select the debug level. The higher the level, "
                       "the more messages will be logged."));
    g_signal_connect(G_OBJECT(level), "changed", G_CALLBACK(state_change_cb),
                     NULL);

    gtk_box_pack_start(GTK_BOX(hbox), level, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* clean frequency */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);

    label = gtk_label_new(_("Delete log files older than:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    age = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(age),
                                   _("Always delete"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(age), _("1 day"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(age), _("1 week"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(age), _("1 month"));
    select_age();
    g_signal_connect(G_OBJECT(age), "realize",
                     G_CALLBACK(gpredict_set_combo_tooltips),
                     _("Select how often gpredict should delete "
                       "old log files."));
    g_signal_connect(G_OBJECT(age), "changed", G_CALLBACK(state_change_cb),
                     NULL);
    gtk_box_pack_start(GTK_BOX(hbox), age, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* separator */
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    /* info label */
    confdir = get_user_conf_dir();
    msg =
        g_strdup_printf(_
                        ("Gpredict stores all run-time messages in the %s%slogs%s\n"
                         "directory. The current log file is called gpredict.log and the file is\n"
                         "always kept until the next execution, so that you can examine it in case\n"
                         "of a failure. If old log files are kept, they are called gpredict-XYZ.log\n"
                         "where XYZ is a unique timestamp."), confdir,
                        G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S);
    label = gtk_label_new(msg);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    g_free(msg);

    /* reset button */
    rbut = gtk_button_new_with_label(_("Reset"));
    gtk_widget_set_tooltip_text(rbut,
                                _("Reset settings to the default values."));
    g_signal_connect(G_OBJECT(rbut), "clicked", G_CALLBACK(reset_cb), NULL);
    butbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end(GTK_BOX(butbox), rbut, FALSE, TRUE, 10);

    gtk_box_pack_end(GTK_BOX(vbox), butbox, FALSE, TRUE, 0);

    return vbox;
}
