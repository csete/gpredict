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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "compat.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "mod-cfg-get-param.h"
#include "sat-log.h"
#include "gtk-sat-module.h"
#include "sat-pref-layout.h"



static gboolean dirty = FALSE;
static gboolean reset = FALSE;

/* check boxes for window positioning */
static GtkWidget *mwin,*mod,*state;

/* Text entry for layout string */
static GtkWidget *gridstr;
static gulong gridstr_sigid;

/* layout selector combo */
static GtkWidget *selector;

/* layout thumbnail */
static GtkWidget *thumb;


/* private functions */
static void create_layout_selector (GKeyFile *cfg, GtkTable *table);
static void layout_selected_cb     (GtkComboBox *combo, gpointer data);
static void create_window_placement (GtkBox *vbox);
static void create_reset_button    (GKeyFile *cfg, GtkBox *vbox);
static void reset_cb               (GtkWidget *button, gpointer cfg);
static void window_pos_toggle_cb   (GtkWidget *toggle, gpointer data);
static void layout_code_changed    (GtkWidget *widget, gpointer data);
static gchar *thumb_file_from_sel  (guint sel);


/** \brief the number of predefined layouts (+1 for custom). */
#define PREDEF_NUM 10

/** \brief Predefined layouts. */
gchar *predef_layout[PREDEF_NUM][3] = {
    { "1;0;2;0;1;2;0;1;1;2;3;1;2;1;2", N_("World map, polar and single sat"), "gpredict-layout-00.png" },
    { "1;0;2;0;1", N_("World map"), "gpredict-layout-01.png" },
    { "0;0;2;0;1", N_("Table"), "gpredict-layout-02.png" },
    { "1;0;2;0;2;0;0;2;2;3", N_("World map and table"), "gpredict-layout-03.png" },
    { "2;0;1;0;1;3;1;2;0;1", N_("Polar and single sat"), "gpredict-layout-04.png" },
    { "2;0;1;0;1;4;1;2;0;1", N_("Polar and upcoming passes"), "gpredict-layout-05.png" },
    { "1;0;3;0;4;0;0;3;4;6;2;0;1;6;8;3;1;2;6;8;4;2;3;6;8", N_("All views (narrow)"), "gpredict-layout-06.png" },
    { "1;0;3;0;3;0;0;3;3;4;2;3;4;0;2;4;3;4;2;3;3;3;4;3;4", N_("All views (wide)"), "gpredict-layout-07.png"},
    { "1;0;3;0;3;0;0;3;3;4;2;3;4;0;2;3;3;4;2;4", N_("Map, table, polar and single sat (wide)"), "gpredict-layout-08.png"},
    { "", N_("Custom"), "gpredict-layout-99.png" }
};


/** \brief Create and initialise widgets for the layout view preferences tab.
 *
 * The widgets must be preloaded with values from config. If a config value
 * is NULL, sensible default values, eg. those from defaults.h should
 * be laoded.
 */
GtkWidget *sat_pref_layout_create (GKeyFile *cfg)
{
    GtkWidget *table;
    GtkWidget *vbox;



    /* create the table */
    table = gtk_table_new (8, 5, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 10);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);

    /* layout selector */
    create_layout_selector (cfg, GTK_TABLE (table));

    /* separator */
    gtk_table_attach (GTK_TABLE (table),
                      gtk_hseparator_new (),
                      0, 5, 3, 4,
                      GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);


    /* create vertical box */
    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 20);
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);

    /* window placement */
    if (cfg == NULL) {
        create_window_placement (GTK_BOX (vbox));
    }

    /* create RESET button */
    create_reset_button (cfg, GTK_BOX (vbox));

    /* reset flags */
    dirty = FALSE;
    reset = FALSE;

    return vbox;;
}


/** \brief User pressed cancel. Any changes to config must be cancelled.
 */
void sat_pref_layout_cancel (GKeyFile *cfg)
{
    gchar *str;

    str = sat_cfg_get_str (SAT_CFG_STR_MODULE_GRID);
    gtk_entry_set_text (GTK_ENTRY (gridstr), str);
    g_free (str);

    dirty = FALSE;
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void sat_pref_layout_ok (GKeyFile *cfg)
{
    if (dirty) {
        /* we have new settings */
        if (cfg != NULL) {
            g_key_file_set_string (cfg,
                                   MOD_CFG_GLOBAL_SECTION,
                                   MOD_CFG_GRID,
                                   gtk_entry_get_text (GTK_ENTRY (gridstr)));
        }
        else {
            sat_cfg_set_str (SAT_CFG_STR_MODULE_GRID,
                             gtk_entry_get_text (GTK_ENTRY (gridstr)));
            sat_cfg_set_bool (SAT_CFG_BOOL_MAIN_WIN_POS,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (mwin)));
            sat_cfg_set_bool (SAT_CFG_BOOL_MOD_WIN_POS,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (mod)));
            sat_cfg_set_bool (SAT_CFG_BOOL_MOD_STATE,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (state)));
        }
    }

    else if (reset) {
        /* we have to reset the values to global or default settings */
        if (cfg == NULL) {

            /* layout */
            sat_cfg_reset_str (SAT_CFG_STR_MODULE_GRID);

            /* window placement */
            sat_cfg_reset_bool (SAT_CFG_BOOL_MAIN_WIN_POS);
            sat_cfg_reset_bool (SAT_CFG_BOOL_MOD_WIN_POS);
            sat_cfg_reset_bool (SAT_CFG_BOOL_MOD_STATE);
        }
        else {
            g_key_file_remove_key ((GKeyFile *)(cfg),
                                   MOD_CFG_GLOBAL_SECTION,
                                   MOD_CFG_GRID,
                                   NULL);
        }

    }
    dirty = FALSE;
    reset = FALSE;
}


/** \brief Create layout selector. */
static void create_layout_selector (GKeyFile *cfg, GtkTable *table)
{
    GtkWidget *label;
    gchar     *buffer;
    gchar     *thumbfile;
    guint      i,sel=PREDEF_NUM-1;


    /* get the current settings */
    if (cfg != NULL) {
        buffer = mod_cfg_get_str (cfg,
                                  MOD_CFG_GLOBAL_SECTION,
                                  MOD_CFG_GRID,
                                  SAT_CFG_STR_MODULE_GRID);
    }
    else {
        buffer = sat_cfg_get_str (SAT_CFG_STR_MODULE_GRID);
    }

    /* create header */
    label = gtk_label_new (_("Select layout:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);

    /* layout selector */
    selector = gtk_combo_box_new_text ();
    gtk_table_attach (GTK_TABLE (table), selector, 1, 3, 0, 1,
                      GTK_FILL, GTK_SHRINK, 0, 0);

    for (i = 0; i < PREDEF_NUM; i++) {
        /* append default layout string to combo box */
        gtk_combo_box_append_text (GTK_COMBO_BOX (selector), predef_layout[i][1]);

        /* check if this layout corresponds to the settings */
        if (!g_ascii_strcasecmp (buffer, predef_layout[i][0])) {
            sel = i;
        }
    }

    /* select settings */
    gtk_combo_box_set_active (GTK_COMBO_BOX (selector), sel);

    /* connect signal handler */
    g_signal_connect (selector, "changed", G_CALLBACK (layout_selected_cb), NULL);


    /* layout preview thumbnail */
    thumbfile = thumb_file_from_sel (sel);
    thumb = gtk_image_new_from_file (thumbfile);
    g_free (thumbfile);
    gtk_table_attach (GTK_TABLE (table), thumb, 1, 3, 1, 2,
                      GTK_EXPAND, GTK_EXPAND, 0, 5);


    /* layout string */
    label = gtk_label_new (_("Layout code:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                      GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);


    gridstr = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (gridstr), buffer);
    g_free (buffer);

    gtk_widget_set_tooltip_text (gridstr,
                                 _("This entry holds the layout code for the module.\n"\
                                   "Consult the user manual for how to create custom layouts "\
                                   "using layout codes."));

    /* disable if it is a predefined layout */
    if (sel < PREDEF_NUM-1) {
        gtk_widget_set_sensitive (gridstr, FALSE);
    }

    /* connect changed signal handler */
    gridstr_sigid = g_signal_connect (gridstr, "changed", G_CALLBACK (layout_code_changed), NULL);

    gtk_table_attach_defaults (GTK_TABLE (table), gridstr, 1, 4, 2, 3);


}


/** \brief Get thumbnail icon filename from selection ID.
  * \param sel The ID of the predefined layout or PREDEF_NUM-1 for custom.
  * \return A newly allocated string containing the full path of the icon.
  *
  * This function generates an icon file name from the ID of a predefined
  * layout. PREDEF_NUM-1 corresponds to the last entry in predef_layout[][],
  * which is the custom layout. The returned string should be freed when no
  * longer needed.
  *
  * The function checks that sel is within valid range (0...PREDEF_NUM-1). If
  * sel is outside the range, the custom layout icon is returned.
  */
static gchar *thumb_file_from_sel (guint sel)
{
    gchar *fname;

    if (sel < PREDEF_NUM) {
        fname = icon_file_name (predef_layout[sel][2]);
    }
    else {
        fname = icon_file_name (predef_layout[PREDEF_NUM-1][2]);
    }

    return fname;
}


/** \brief Callback to manage layout selection via combo box.
 *
 * This function is called when the user selects a new layout using the
 * layout selector combo box. The function updates the thumbnail and the
 * layout code text entry.
 */
static void layout_selected_cb (GtkComboBox *combo, gpointer data)
{
    gint   idx;
    gchar *icon;


    idx = gtk_combo_box_get_active (combo);
    if (idx < PREDEF_NUM) {
        dirty = TRUE;

        /* update icon */
        icon = thumb_file_from_sel (idx);
        gtk_image_set_from_file (GTK_IMAGE (thumb), icon);
        g_free (icon);

        /* update layout code, unless Custom is selected */
        if (idx < PREDEF_NUM-1) {
            g_signal_handler_block (gridstr, gridstr_sigid);
            gtk_entry_set_text (GTK_ENTRY (gridstr), predef_layout[idx][0]);
            g_signal_handler_unblock (gridstr, gridstr_sigid);
            gtk_widget_set_sensitive (gridstr, FALSE);
        }
        else {
            gtk_widget_set_sensitive (gridstr, TRUE);
        }
    }
}




/** \brief Create window placement widgets.
 *  \param vbox The GtkVBox into which the widgets should be packed.
 *
 */
static void create_window_placement (GtkBox *vbox)
{
    GtkWidget *label;
    GtkTooltips *tips;


    /* create header */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (label),
                          _("<b>Window Placements:</b>"));
    gtk_box_pack_start (vbox, label, FALSE, FALSE, 0);

    /* main window setting */
    mwin = gtk_check_button_new_with_label (_("Restore position of main window"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mwin),
                                  sat_cfg_get_bool (SAT_CFG_BOOL_MAIN_WIN_POS));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, mwin,
                          _("If you check this button, gpredict will try to "\
                            "place the main window at the position it was "\
                            "during the last session.\n"\
                            "Note that window managers can ignore this request."),
                          NULL);
    g_signal_connect (G_OBJECT (mwin), "toggled",
                      G_CALLBACK (window_pos_toggle_cb), NULL);
    gtk_box_pack_start (vbox, mwin, FALSE, FALSE, 0);

    /* module window setting */
    mod = gtk_check_button_new_with_label (_("Restore position of module windows"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mod),
                                  sat_cfg_get_bool (SAT_CFG_BOOL_MOD_WIN_POS));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, mod,
                          _("If you check this button, gpredict will try to "\
                            "place the module windows at the position they were "\
                            "the last time.\n"\
                            "Note that window managers can ignore this request."),
                          NULL);
    g_signal_connect (G_OBJECT (mod), "toggled",
                      G_CALLBACK (window_pos_toggle_cb), NULL);
    gtk_box_pack_start (vbox, mod, FALSE, FALSE, 0);

    /* module state */
    state = gtk_check_button_new_with_label (_("Restore the state of modules when reopened (docked or window)"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (state),
                                  sat_cfg_get_bool (SAT_CFG_BOOL_MOD_STATE));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, state,
                          _("If you check this button, gpredict will restore "\
                            "the states of the modules from the last time they were used."),
                          NULL);
    g_signal_connect (G_OBJECT (state), "toggled",
                      G_CALLBACK (window_pos_toggle_cb), NULL);
    gtk_box_pack_start (vbox, state, FALSE, FALSE, 0);
}



/** \brief Create RESET button.
 *  \param cfg Config data or NULL in global mode.
 *  \param vbox The container.
 *
 * This function creates and sets up the RESET button.
 */
static void create_reset_button (GKeyFile *cfg, GtkBox *vbox)
{
    GtkWidget   *button;
    GtkWidget   *butbox;
    GtkTooltips *tips;


    button = gtk_button_new_with_label (_("Reset"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (reset_cb), cfg);

    tips = gtk_tooltips_new ();
    if (cfg == NULL) {
        gtk_tooltips_set_tip (tips, button,
                              _("Reset settings to the default values."),
                              NULL);
    }
    else {
        gtk_tooltips_set_tip (tips, button,
                              _("Reset module settings to the global values."),
                              NULL);
    }

    butbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (butbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end (GTK_BOX (butbox), button, FALSE, TRUE, 10);

    gtk_box_pack_end (vbox, butbox, FALSE, TRUE, 0);

}


/** \brief Reset settings.
 *  \param button The RESET button.
 *  \param cfg Pointer to the module config or NULL in global mode.
 *
 * This function is called when the user clicks on the RESET button. In global mode
 * (when cfg = NULL) the function will reset the settings to the efault values, while
 * in "local" mode (when cfg != NULL) the function will reset the module settings to
 * the global settings. This is done by removing the corresponding key from the GKeyFile.
 */
static void reset_cb (GtkWidget *button, gpointer cfg)
{
    guint i,sel=PREDEF_NUM-1;
    gchar *buffer;


    /* views */
    if (cfg == NULL) {
        /* global mode, get defaults */
        buffer = sat_cfg_get_str_def (SAT_CFG_STR_MODULE_GRID);
        gtk_entry_set_text (GTK_ENTRY (gridstr), buffer);
    }
    else {
        /* local mode, get global value */
        buffer = sat_cfg_get_str (SAT_CFG_STR_MODULE_GRID);
        gtk_entry_set_text (GTK_ENTRY (gridstr), buffer);
    }

    /* findcombo box setting */
    for (i = 0; i < PREDEF_NUM; i++) {
        /* check if this layout corresponds to the settings */
        if (!g_ascii_strcasecmp (buffer, predef_layout[i][0])) {
            sel = i;
        }
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX(selector),sel);
    g_free (buffer);


    /* window placement settings */
    if (cfg == NULL) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mwin),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_MAIN_WIN_POS));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mod),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_MOD_WIN_POS));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (state),
                                      sat_cfg_get_bool_def (SAT_CFG_BOOL_MOD_STATE));
    }


    /* reset flags */
    reset = TRUE;
    dirty = FALSE;
}




/** \brief Toggle window positioning settings. */
static void window_pos_toggle_cb   (GtkWidget *toggle, gpointer data)
{
    dirty = TRUE;
}

/** \brief Manage layout code changes.
 *
 * This function is called when the contents of the lyout code changes.
 * The purpose of this function is to check whether entered character is valid
 * and to make the configuration "dirty".
 */
static void layout_code_changed (GtkWidget *widget, gpointer data)
{
    gchar       *entry, *end, *j;
    gint         len, pos;


    /* step 1: ensure that only valid characters are entered
           (stolen from xlog, tnx pg4i)
        */
    entry = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
    if ((len = g_utf8_strlen (entry, -1)) > 0) {
        end = entry + g_utf8_strlen (entry, -1);
        for (j = entry; j < end; ++j) {
            switch (*j) {
            case '0' ... '9':
            case ';':
                dirty = TRUE;
                /* ensure combo box is set to custom */
                if (gtk_combo_box_get_active (GTK_COMBO_BOX(selector)) != PREDEF_NUM-1) {
                    gtk_combo_box_set_active (GTK_COMBO_BOX(selector), PREDEF_NUM-1);
                }
                break;
            default:
                gdk_beep ();
                pos = gtk_editable_get_position (GTK_EDITABLE (widget));
                gtk_editable_delete_text (GTK_EDITABLE (widget),
                                          pos, pos+1);
                break;
            }
        }
    }

}
