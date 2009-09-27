/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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



static gtk_sat_mod_layout_t layout = 0;
static gboolean dirty = FALSE;
static gboolean reset = FALSE;


/* combo boxes for view seelctors */
static GtkWidget *combo1,*combo2,*combo3;

/* radio buttons for layout selector */
static GtkWidget *but1,*but2,*but3,*but4;

/* check boxes for window positioning */
static GtkWidget *mwin,*mod,*state;


static void create_layout_selector (GKeyFile *cfg, GtkTable *table);
static void layout_selected_cb     (GtkToggleButton *but, gpointer data);
static void create_view_selectors  (GKeyFile *cfg, GtkTable *table);
static void create_window_placement (GtkBox *vbox);
static void create_reset_button    (GKeyFile *cfg, GtkBox *vbox);
static GtkWidget *create_combo     (void);
static void combo_changed_cb       (GtkComboBox *widget, gpointer data);
static void reset_cb               (GtkWidget *button, gpointer cfg);
static void window_pos_toggle_cb   (GtkWidget *toggle, gpointer data);


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
					  0, 5, 2, 3,
					  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);

	/* view selectors */
	create_view_selectors (cfg, GTK_TABLE (table));

	/* separator */
	gtk_table_attach (GTK_TABLE (table),
					  gtk_hseparator_new (),
					  0, 5, 7, 8,
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
void
sat_pref_layout_cancel (GKeyFile *cfg)
{
	layout = sat_cfg_get_int (SAT_CFG_INT_MODULE_LAYOUT);
	dirty = FALSE;
}


/** \brief User pressed OK. Any changes should be stored in config.
 */
void
sat_pref_layout_ok     (GKeyFile *cfg)
{
	if (dirty) {
		/* we have new settings */
		if (cfg != NULL) {
			g_key_file_set_integer (cfg,
									MOD_CFG_GLOBAL_SECTION,
									MOD_CFG_LAYOUT,
									layout);
			g_key_file_set_integer (cfg,
									MOD_CFG_GLOBAL_SECTION,
									MOD_CFG_VIEW_1,
									gtk_combo_box_get_active (GTK_COMBO_BOX (combo1)));
			g_key_file_set_integer (cfg,
									MOD_CFG_GLOBAL_SECTION,
									MOD_CFG_VIEW_2,
									gtk_combo_box_get_active (GTK_COMBO_BOX (combo2)));
			g_key_file_set_integer (cfg,
									MOD_CFG_GLOBAL_SECTION,
									MOD_CFG_VIEW_3,
									gtk_combo_box_get_active (GTK_COMBO_BOX (combo3)));

		}
		else {
			sat_cfg_set_int (SAT_CFG_INT_MODULE_LAYOUT, layout);
			sat_cfg_set_int (SAT_CFG_INT_MODULE_VIEW_1,
							 gtk_combo_box_get_active (GTK_COMBO_BOX (combo1)));
			sat_cfg_set_int (SAT_CFG_INT_MODULE_VIEW_2,
							 gtk_combo_box_get_active (GTK_COMBO_BOX (combo2)));
			sat_cfg_set_int (SAT_CFG_INT_MODULE_VIEW_3,
							 gtk_combo_box_get_active (GTK_COMBO_BOX (combo3)));
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

			/* views */
			sat_cfg_reset_int (SAT_CFG_INT_MODULE_VIEW_1);
			sat_cfg_reset_int (SAT_CFG_INT_MODULE_VIEW_2);
			sat_cfg_reset_int (SAT_CFG_INT_MODULE_VIEW_3);

			/* layout */
			sat_cfg_reset_int (SAT_CFG_INT_MODULE_LAYOUT);

			/* window placement */
			sat_cfg_reset_bool (SAT_CFG_BOOL_MAIN_WIN_POS);
			sat_cfg_reset_bool (SAT_CFG_BOOL_MOD_WIN_POS);
            sat_cfg_reset_bool (SAT_CFG_BOOL_MOD_STATE);
        }
		else {

			/* layout views */
			g_key_file_remove_key ((GKeyFile *)(cfg),
								   MOD_CFG_GLOBAL_SECTION,
								   MOD_CFG_VIEW_1,
								   NULL);
			g_key_file_remove_key ((GKeyFile *)(cfg),
								   MOD_CFG_GLOBAL_SECTION,
								   MOD_CFG_VIEW_2,
								   NULL);
			g_key_file_remove_key ((GKeyFile *)(cfg),
								   MOD_CFG_GLOBAL_SECTION,
								   MOD_CFG_VIEW_3,
								   NULL);
			
			/* layout */
			g_key_file_remove_key ((GKeyFile *)(cfg),
								   MOD_CFG_GLOBAL_SECTION,
								   MOD_CFG_LAYOUT,
								   NULL);
		}

	}
	dirty = FALSE;
	reset = FALSE;
}


/** \brief Create layout selector. */
static void
create_layout_selector (GKeyFile *cfg, GtkTable *table)
{
	GtkWidget *label;
	GtkWidget *image;
	gchar     *fname;

	/* create header */
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), 
						  _("<b>Default Layout:</b>"));

	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
					  GTK_FILL, GTK_SHRINK, 0, 0);

	/* layout 1 */
	but1 = gtk_radio_button_new (NULL);
	gtk_table_attach (table, but1, 0, 1, 1, 2,
					  GTK_SHRINK, GTK_SHRINK, 0, 0);
	fname = icon_file_name ("gpredict-layout-1.png");
	image = gtk_image_new_from_file (fname);
	g_free (fname);
	gtk_container_add (GTK_CONTAINER (but1), image);

	/* layout 2 */
	but2 = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (but1));
	gtk_table_attach (table, but2, 1, 2, 1, 2,
					  GTK_SHRINK, GTK_SHRINK, 0, 0);
	fname = icon_file_name ("gpredict-layout-2.png");
	image = gtk_image_new_from_file (fname);
	g_free (fname);
	gtk_container_add (GTK_CONTAINER (but2), image);

	/* layout 3 */
	but3 = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (but1));
	gtk_table_attach (table, but3, 2, 3, 1, 2,
					  GTK_SHRINK, GTK_SHRINK, 0, 0);
	fname = icon_file_name ("gpredict-layout-3.png");
	image = gtk_image_new_from_file (fname);
	g_free (fname);
	gtk_container_add (GTK_CONTAINER (but3), image);

	/* layout 4 */
	but4 = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (but1));
	gtk_table_attach (table, but4, 3, 4, 1, 2,
					  GTK_SHRINK, GTK_SHRINK, 0, 0);
	fname = icon_file_name ("gpredict-layout-4.png");
	image = gtk_image_new_from_file (fname);
	g_free (fname);
	gtk_container_add (GTK_CONTAINER (but4), image);

	/* select current layout */
	if (cfg != NULL) {
		layout = mod_cfg_get_int (cfg,
								  MOD_CFG_GLOBAL_SECTION,
								  MOD_CFG_LAYOUT,
								  SAT_CFG_INT_MODULE_LAYOUT);
	}
	else {
		layout = sat_cfg_get_int (SAT_CFG_INT_MODULE_LAYOUT);
	}

	switch (layout) {

	case GTK_SAT_MOD_LAYOUT_1:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but1), TRUE);
		break;

	case GTK_SAT_MOD_LAYOUT_2:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but2), TRUE);
		break;

	case GTK_SAT_MOD_LAYOUT_3:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but3), TRUE);
		break;

	case GTK_SAT_MOD_LAYOUT_4:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but4), TRUE);
		break;

	default:
		sat_log_log (SAT_LOG_LEVEL_BUG,
					 _("%s: Invalid module layout (%d)"),
					 __FUNCTION__, layout);
		break;
	}

	/* connect radio button signals */
	g_signal_connect (but1, "toggled",
					  G_CALLBACK (layout_selected_cb),
					  GINT_TO_POINTER (GTK_SAT_MOD_LAYOUT_1));
	g_signal_connect (but2, "toggled",
					  G_CALLBACK (layout_selected_cb),
					  GINT_TO_POINTER (GTK_SAT_MOD_LAYOUT_2));
	g_signal_connect (but3, "toggled",
					  G_CALLBACK (layout_selected_cb),
					  GINT_TO_POINTER (GTK_SAT_MOD_LAYOUT_3));
	g_signal_connect (but4, "toggled",
					  G_CALLBACK (layout_selected_cb),
					  GINT_TO_POINTER (GTK_SAT_MOD_LAYOUT_4));
}

/** \brief Callback to manage radio button clicks.
 *
 * This function is called when the user makes a new layout selection,
 * i.e. a radio button is clicked. The function is called for both the
 * newly selected radio button and the de-selected radio button. It must
 * therefore check whether the button is currently active or not
 */
static void
layout_selected_cb (GtkToggleButton *but, gpointer data)
{
	
	/* update layout if this button is selected */
	if (gtk_toggle_button_get_active (but)) {
		layout = GPOINTER_TO_INT (data);
		dirty = TRUE;

		/* enable/disable combos */
		switch (layout) {
		      
		case GTK_SAT_MOD_LAYOUT_1:
			gtk_widget_set_sensitive (combo2, FALSE);
			gtk_widget_set_sensitive (combo3, FALSE);
			break;

		case GTK_SAT_MOD_LAYOUT_2:
			gtk_widget_set_sensitive (combo2, TRUE);
			gtk_widget_set_sensitive (combo3, FALSE);
			break;

		default:
			gtk_widget_set_sensitive (combo2, TRUE);
			gtk_widget_set_sensitive (combo3, TRUE);
			break;
		}
			
	}
}



/** \brief Create view selectors.
 *  \param cfg Config data or NULL in global mode.
 *  \param table The container.
 *
 * This function creates and sets up the view selector combos.
 */
static void create_view_selectors (GKeyFile *cfg, GtkTable *table)
{
	GtkWidget *label;
	gint idx;

	/* create header */
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), 
						  _("<b>Views:</b>"));
	gtk_table_attach (table, label, 0, 2, 3, 4,
					  GTK_FILL, GTK_SHRINK, 0, 0);

	/* labels */
	label = gtk_label_new (_("View 1:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (table, label, 0, 1, 4, 5,
					  GTK_FILL, GTK_SHRINK, 0, 0);

	label = gtk_label_new (_("View 2:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (table, label, 0, 1, 5, 6,
					  GTK_FILL, GTK_SHRINK, 0, 0);

	label = gtk_label_new (_("View 3:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (table, label, 0, 1, 6, 7,
					  GTK_FILL, GTK_SHRINK, 0, 0);

	/* combo boxes */
	combo1 = create_combo ();
	if (cfg != NULL) {
		idx = mod_cfg_get_int (cfg,
							   MOD_CFG_GLOBAL_SECTION,
							   MOD_CFG_VIEW_1,
							   SAT_CFG_INT_MODULE_VIEW_1);
	}
	else {
		idx = sat_cfg_get_int (SAT_CFG_INT_MODULE_VIEW_1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo1), idx);
	gtk_table_attach (table, combo1, 1, 2, 4, 5,
					  GTK_FILL, GTK_SHRINK, 0, 0);
	g_signal_connect (combo1, "changed", G_CALLBACK (combo_changed_cb), NULL);

	combo2 = create_combo ();
	if (cfg != NULL) {
		idx = mod_cfg_get_int (cfg,
							   MOD_CFG_GLOBAL_SECTION,
							   MOD_CFG_VIEW_2,
							   SAT_CFG_INT_MODULE_VIEW_2);
	}
	else {
		idx = sat_cfg_get_int (SAT_CFG_INT_MODULE_VIEW_2);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo2), idx);
	gtk_table_attach (table, combo2, 1, 2, 5, 6,
					  GTK_FILL, GTK_SHRINK, 0, 0);
	g_signal_connect (combo2, "changed", G_CALLBACK (combo_changed_cb), NULL);

	combo3 = create_combo ();
	if (cfg != NULL) {
		idx = mod_cfg_get_int (cfg,
							   MOD_CFG_GLOBAL_SECTION,
							   MOD_CFG_VIEW_3,
							   SAT_CFG_INT_MODULE_VIEW_3);
	}
	else {
		idx = sat_cfg_get_int (SAT_CFG_INT_MODULE_VIEW_3);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo3), idx);
	gtk_table_attach (table, combo3, 1, 2, 6, 7,
					  GTK_FILL, GTK_SHRINK, 0, 0);
	g_signal_connect (combo3, "changed", G_CALLBACK (combo_changed_cb), NULL);

	/* enable/disable combos */
	switch (layout) {
		
	case GTK_SAT_MOD_LAYOUT_1:
		gtk_widget_set_sensitive (combo2, FALSE);
		gtk_widget_set_sensitive (combo3, FALSE);
		break;

	case GTK_SAT_MOD_LAYOUT_2:
		gtk_widget_set_sensitive (combo2, TRUE);
		gtk_widget_set_sensitive (combo3, FALSE);
		break;

	default:
		gtk_widget_set_sensitive (combo2, TRUE);
		gtk_widget_set_sensitive (combo3, TRUE);
		break;
	}
}


/** \brief Convenience function for creatring combo boxes
 *
 * note: texts must correspond to the order of gtk_sat_mod_view_t
 */
static GtkWidget *
create_combo (void)
{
	GtkWidget *combo;

	combo = gtk_combo_box_new_text ();
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("List View"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Map View"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Polar View"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Single Sat"));


	return combo;
}

static void
combo_changed_cb (GtkComboBox *widget, gpointer data)
{
	dirty = TRUE;
}



/** \brief Create window placement widgets.
 *  \param vbox The GtkVBox into which the widgets should be packed.
 *
 */
static void
create_window_placement (GtkBox *vbox)
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
static void
create_reset_button (GKeyFile *cfg, GtkBox *vbox)
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
static void
reset_cb               (GtkWidget *button, gpointer cfg)
{
	gint idx;


	/* views */
	if (cfg == NULL) {
		/* global mode, get defaults */
		idx = sat_cfg_get_int_def (SAT_CFG_INT_MODULE_VIEW_1);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo1), idx);
		idx = sat_cfg_get_int_def (SAT_CFG_INT_MODULE_VIEW_2);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo2), idx);
		idx = sat_cfg_get_int_def (SAT_CFG_INT_MODULE_VIEW_3);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo3), idx);
	}
	else {
		/* local mode, get global value */
		idx = sat_cfg_get_int (SAT_CFG_INT_MODULE_VIEW_1);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo1), idx);
		idx = sat_cfg_get_int (SAT_CFG_INT_MODULE_VIEW_2);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo2), idx);
		idx = sat_cfg_get_int (SAT_CFG_INT_MODULE_VIEW_3);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo3), idx);
	}

	/* read new layout */
	if (cfg == NULL) {
		layout = sat_cfg_get_int_def (SAT_CFG_INT_MODULE_LAYOUT);
	}
	else {
		layout = sat_cfg_get_int (SAT_CFG_INT_MODULE_LAYOUT);
	}

	switch (layout) {

	case GTK_SAT_MOD_LAYOUT_1:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but1), TRUE);
		break;

	case GTK_SAT_MOD_LAYOUT_2:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but2), TRUE);
		break;

	case GTK_SAT_MOD_LAYOUT_3:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but3), TRUE);
		break;

	case GTK_SAT_MOD_LAYOUT_4:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (but4), TRUE);
		break;

	default:
		sat_log_log (SAT_LOG_LEVEL_BUG,
					 _("%s: Invalid module layout (%d)"),
					 __FUNCTION__, layout);
		break;
	}


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
static void
window_pos_toggle_cb   (GtkWidget *toggle, gpointer data)
{
	dirty = TRUE;
}

