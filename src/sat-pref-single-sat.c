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

#include "config-keys.h"
#include "gtk-single-sat.h"
#include "mod-cfg-get-param.h"
#include "sat-cfg.h"
#include "sat-pref-single-sat.h"


#define Y0          1           /* First row where checkboxes are placed */
#define COLUMNS     3           /* Number of columns in the table */

static GtkWidget *check[SINGLE_SAT_FIELD_NUMBER];
static guint    startflags;
static guint    flags;
static gboolean dirty = FALSE;
static gboolean reset = FALSE;

extern const gchar *SINGLE_SAT_FIELD_TITLE[];
extern const gchar *SINGLE_SAT_FIELD_HINT[];


/* User pressed cancel. Any changes to config must be cancelled. */
void sat_pref_single_sat_cancel(GKeyFile * cfg)
{
    (void)cfg;

    dirty = FALSE;
    reset = FALSE;
}

/* User pressed OK. Any changes should be stored in config. */
void sat_pref_single_sat_ok(GKeyFile * cfg)
{
    if (dirty)
    {
        if (cfg != NULL)
        {
            g_key_file_set_integer(cfg,
                                   MOD_CFG_SINGLE_SAT_SECTION,
                                   MOD_CFG_SINGLE_SAT_FIELDS, flags);
        }
        else
        {
            sat_cfg_set_int(SAT_CFG_INT_SINGLE_SAT_FIELDS, flags);
        }
    }
    else if (reset)
    {
        if (cfg != NULL)
        {
            /* local mode; remove key */
            g_key_file_remove_key(cfg,
                                  MOD_CFG_SINGLE_SAT_SECTION,
                                  MOD_CFG_SINGLE_SAT_FIELDS, NULL);
        }
        else
        {
            /* global mode; reset value */
            sat_cfg_reset_int(SAT_CFG_INT_SINGLE_SAT_FIELDS);
        }
    }

    dirty = FALSE;
    reset = FALSE;
}

static void toggle_cb(GtkToggleButton * toggle, gpointer data)
{
    if (gtk_toggle_button_get_active(toggle))
        flags |= (1 << GPOINTER_TO_UINT(data));
    else
        flags &= ~(1 << GPOINTER_TO_UINT(data));

    /* clear dirty flag if we are back where we started */
    dirty = (flags != startflags);
}

static void reset_cb(GtkWidget * button, gpointer cfg)
{
    guint           i;

    (void)button;

    if (cfg == NULL)
    {
        /* global mode, get defaults */
        flags = sat_cfg_get_int_def(SAT_CFG_INT_SINGLE_SAT_FIELDS);
    }
    else
    {
        /* local mode, get global value */
        flags = sat_cfg_get_int(SAT_CFG_INT_SINGLE_SAT_FIELDS);
    }

    for (i = 0; i < SINGLE_SAT_FIELD_NUMBER; i++)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check[i]),
                                     flags & (1 << i));
    }

    /* reset flags */
    dirty = FALSE;
    reset = TRUE;
}

static void create_reset_button(GKeyFile * cfg, GtkBox * vbox)
{
    GtkWidget      *button;
    GtkWidget      *butbox;

    button = gtk_button_new_with_label(_("Reset"));
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(reset_cb), cfg);

    if (cfg == NULL)
    {
        gtk_widget_set_tooltip_text(button,
                                    _
                                    ("Reset settings to the default values."));
    }
    else
    {
        gtk_widget_set_tooltip_text(button,
                                    _
                                    ("Reset module settings to the global values."));
    }

    butbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end(GTK_BOX(butbox), button, FALSE, TRUE, 10);

    gtk_box_pack_end(vbox, butbox, FALSE, TRUE, 0);
}

GtkWidget      *sat_pref_single_sat_create(GKeyFile * cfg)
{
    GtkWidget      *table;
    GtkWidget      *vbox;
    GtkWidget      *label;
    guint           i;

    table = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);

    /* create header */
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Visible Fields:</b>"));
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 2, 1);

    /* get visible column flags */
    if (cfg != NULL)
    {
        flags = mod_cfg_get_int(cfg,
                                MOD_CFG_SINGLE_SAT_SECTION,
                                MOD_CFG_SINGLE_SAT_FIELDS,
                                SAT_CFG_INT_SINGLE_SAT_FIELDS);
    }
    else
    {
        flags = sat_cfg_get_int(SAT_CFG_INT_SINGLE_SAT_FIELDS);
    }

    for (i = 0; i < SINGLE_SAT_FIELD_NUMBER; i++)
    {
        check[i] =
            gtk_check_button_new_with_label(_(SINGLE_SAT_FIELD_TITLE[i]));
        gtk_widget_set_tooltip_text(check[i], _(SINGLE_SAT_FIELD_HINT[i]));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check[i]),
                                     flags & (1 << i));
        g_signal_connect(check[i], "toggled",
                         G_CALLBACK(toggle_cb), GUINT_TO_POINTER(i));

        gtk_grid_attach(GTK_GRID(table), check[i],
                        i % COLUMNS, Y0 + i / COLUMNS, 1, 1);
    }

    /* create vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

    /* create RESET button */
    create_reset_button(cfg, GTK_BOX(vbox));

    startflags = flags;
    dirty = FALSE;
    reset = FALSE;

    return vbox;
}
