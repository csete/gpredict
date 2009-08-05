/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2008  Alexandru Csete, OZ9AEC.

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
/*

+------------------------------------+
|           +---------------------+  |
|  Name     | My Kool Module      |  |
|           +---------------------+  |
|           +-----------+---+ +---+  |
|  Location | OZ9AEC    | v | | + |  |
|           +-----------+---+ +---+  |
| ---------------------------------- |
|             Satellites             |
| +--------------------------+-----+ |
| | Name                     | Sel | |
| +--------------------------+-----+ |      
| | > Amateur                |     | |
| | v Weather                |     | |
| |     NOAA 14              |  x  | |
| |     NOAA 15              |  x  | |
| |     NOAA 17              |  x  | |
| | > Military               |     | |
| +--------------------------+-----+ |
| ---------------------------------- |
| +----------------------+ +------+  |
| |  (not implemented)   | | Find |  |
| +----------------------+ +------+  |
|                                    |
|      Advanced Settings             |
+------------------------------------+
*/
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sat-log.h"
#include "config-keys.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "gpredict-utils.h"
#include "compat.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "sat-pref-modules.h"
#include "gtk-sat-tree.h"
#include "qth-editor.h"
#include "mod-cfg.h"


extern GtkWidget *app;



/* private widgets */
static GtkWidget *namew;  /* GtkEntry widget for module name */
static GtkWidget *locw;   /* GtkComboBox for location selection */
static GtkWidget *tree;   /* GtkSatTree for selecting satellites */


/* private functions prototype */
static GtkWidget *mod_cfg_editor_create (const gchar *modname,
										 GKeyFile *cfgdata,
										 GtkWidget *toplevel);
static void       mod_cfg_apply         (GKeyFile *cfgdata);
static void       name_changed          (GtkWidget *widget, gpointer data);
static GtkWidget *create_loc_selector   (GKeyFile *cfgdata);

static void       add_qth_cb (GtkWidget *button, gpointer data);

static void       edit_advanced_settings (GtkDialog *parent, GKeyFile *cfgdata);


/** \brief Create a new module.
 *
 * This function creates a new module. The name of the module is
 * returned  and it should be freed when no longer needed.
 */
gchar *
mod_cfg_new    ()
{
	GtkWidget        *dialog;
	GKeyFile         *cfgdata;
	gchar            *name = NULL;
	gint              response;
	mod_cfg_status_t  status;
	gboolean          finished = FALSE;
	gsize             num = 0;
	guint            *sats;

	/* create cfg data */
	cfgdata = g_key_file_new ();

	dialog = mod_cfg_editor_create (NULL, cfgdata, app);
	gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 400);

	while (!finished) {

		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {

			/* user pressed OK */
		case GTK_RESPONSE_OK:
			
			/* check that user has selected at least one satellite */
			sats = gtk_sat_tree_get_selected (GTK_SAT_TREE (tree), &num);
			
			if (num > 0) {
				/* we have at least one sat selected */
				gchar *filename;
				gboolean save = TRUE;

				/* check if there is already a module with this name */
				filename = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
										".gpredict2", G_DIR_SEPARATOR_S,
										"modules", G_DIR_SEPARATOR_S,
										gtk_entry_get_text (GTK_ENTRY (namew)),
										".mod", NULL);

				if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
					GtkWidget *warn;

					sat_log_log (SAT_LOG_LEVEL_DEBUG,
								 _("%s: Already have module %s. Ask user to confirm..."),
								 __FUNCTION__, gtk_entry_get_text (GTK_ENTRY (namew)));

					warn = gtk_message_dialog_new (//NULL,
												   GTK_WINDOW (app),
												   GTK_DIALOG_MODAL |
												   GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_QUESTION,
												   GTK_BUTTONS_YES_NO,
												   _("There is already a module called %s.\n"\
													 "Do you want to overwrite this module?"),
												   gtk_entry_get_text (GTK_ENTRY (namew)));
					switch (gtk_dialog_run (GTK_DIALOG (warn))) {

					case GTK_RESPONSE_YES:
						save = TRUE;
						break;

					default:
						save = FALSE;
						break;
					}
					gtk_widget_destroy (warn);
				}

				g_free (filename);

				if (save) {
					name = g_strdup (gtk_entry_get_text (GTK_ENTRY (namew)));
					mod_cfg_apply (cfgdata);
					status = mod_cfg_save (name, cfgdata);
			
					if (status != MOD_CFG_OK) {
						/* send an error message */
						sat_log_log (SAT_LOG_LEVEL_ERROR,
									 _("%s: Error while saving module data (%d)."),
									 __FUNCTION__, status);
					}

					finished = TRUE;
				}

				g_free (sats);
			}
			else {
				sat_log_log (SAT_LOG_LEVEL_DEBUG,
							 _("%s: User tried to create module with no sats."),
							 __FUNCTION__);

				/* tell user to behave nicely */
				GtkWidget *errdialog;

				errdialog = gtk_message_dialog_new (NULL,
													//GTK_WINDOW (app),
													GTK_DIALOG_MODAL |
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_OK,
													_("Please select at least one satellite "\
													  "from the list."));
				gtk_dialog_run (GTK_DIALOG (errdialog));
				gtk_widget_destroy (errdialog);
			}
			
			break;
			
			/* bring up properties editor */
		case GTK_RESPONSE_HELP:
			edit_advanced_settings (GTK_DIALOG (dialog), cfgdata);
			finished = FALSE;
			break;

			/* everything else is regarded CANCEL */
		default:
			finished = TRUE;
			break;
		}
	}

	/* clean up */
	g_key_file_free (cfgdata);
	gtk_widget_destroy (dialog);

	return name;
}


/** \brief Edit configuration for an existing module.
 *  \param modname The name of the module to edit.
 *  \param cfgdata Configuration data for the module.
 *  \param toplevel Pointer to the topleve window.
 *
 * This function allows the user to edit the configuration
 * of the module specified by modname. The changes are stored
 * in the cfgdata configuration structure but are not saved
 * to disc; that has to be done separately using the
 * mod_cfg_save function. 
 *
 */
mod_cfg_status_t
mod_cfg_edit   (gchar *modname, GKeyFile *cfgdata, GtkWidget *toplevel)
{
	GtkWidget        *dialog;
	gint              response;
	gboolean          finished = FALSE;
	gsize             num = 0;
	guint            *sats;
	mod_cfg_status_t  status = MOD_CFG_CANCEL;


	dialog = mod_cfg_editor_create (modname, cfgdata, toplevel);
	gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 400);

	while (!finished) {

		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {

			/* user pressed OK */
		case GTK_RESPONSE_OK:

			/* check that user has selected at least one satellite */
			sats = gtk_sat_tree_get_selected (GTK_SAT_TREE (tree), &num);
			
			if (num > 0) {

				/* we have at least one sat selected */
				mod_cfg_apply (cfgdata);
				finished = TRUE;
				g_free (sats);
				status = MOD_CFG_OK;
			}
			else {
				sat_log_log (SAT_LOG_LEVEL_DEBUG,
							 _("%s: User tried to create module with no sats."),
							 __FUNCTION__);

				/* tell user not to do that */
				GtkWidget *errdialog;

				errdialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
													GTK_DIALOG_MODAL |
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_OK,
													_("Please select at least one satellite "\
													  "from the list."));
				gtk_dialog_run (GTK_DIALOG (errdialog));
				gtk_widget_destroy (errdialog);
			}

			break;
			
			/* bring up properties editor */
		case GTK_RESPONSE_HELP:
			edit_advanced_settings (GTK_DIALOG (dialog), cfgdata);
			finished = FALSE;
			break;

			/* everything else is regarded CANCEL */
		default:
			finished = TRUE;
			break;
		}
	}

	/* clean up */
	gtk_widget_destroy (dialog);

	return status;
}


/** \brief Save module configuration to disk.
 *  \param modname The name of the module.
 *  \param cfgdata The configuration data to save.
 *
 * This function saves the module configuration data from cfgdata
 * into a module configuration file called modname.mod placed in
 * the /home/username/.gpredict2/modules/ directory.
 */
mod_cfg_status_t
mod_cfg_save   (gchar *modname, GKeyFile *cfgdata)
{
	GError     *error = NULL;  /* Error handler */
	gchar      *datastream;    /* cfgdata string */
	GIOChannel *cfgfile;       /* .mod file */
	gchar      *filename;      /* file name */
	gsize       length;        /* length of the data stream */
	gsize       written;       /* number of bytes actually written */
	gboolean    err;


	if (!modname) {
		sat_log_log (SAT_LOG_LEVEL_BUG,
					 _("%s: Attempt to save data to empty file name."),
					 __FUNCTION__);

		return MOD_CFG_ERROR;
	}
	if (!cfgdata) {
		sat_log_log (SAT_LOG_LEVEL_BUG,
					 _("%s: Attempt to save NULL data."),
					 __FUNCTION__);

		return MOD_CFG_ERROR;
	}

	/* ok, go on and convert the data */
	datastream = g_key_file_to_data (cfgdata, &length, &error);

	if (error != NULL) {
		sat_log_log (SAT_LOG_LEVEL_ERROR,
					 _("%s: Could not create config data (%s)."),
					 __FUNCTION__, error->message);

		g_clear_error (&error);

		return MOD_CFG_ERROR;
	}

	/* create file and write data stream */
	filename = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
							".gpredict2", G_DIR_SEPARATOR_S,
							"modules", G_DIR_SEPARATOR_S,
							modname, ".mod", NULL);

	cfgfile = g_io_channel_new_file (filename, "w", &error);

	if (error != NULL) {
		sat_log_log (SAT_LOG_LEVEL_ERROR,
					 _("%s: Could not create config file (%s)."),
					 __FUNCTION__, error->message);

		g_clear_error (&error);

		err = 1;
	}
	else {
		g_io_channel_write_chars (cfgfile,
								  datastream,
								  length,
								  &written,
								  &error);

		g_io_channel_shutdown (cfgfile, TRUE, NULL);
		g_io_channel_unref (cfgfile);

		if (error != NULL) {
			sat_log_log (SAT_LOG_LEVEL_ERROR,
						 _("%s: Error writing config data (%s)."),
						 __FUNCTION__, error->message);
			
			g_clear_error (&error);
			
			err = 1;
		}
		else if (length != written) {
			sat_log_log (SAT_LOG_LEVEL_WARN,
						 _("%s: Wrote only %d out of %d chars."),
						 __FUNCTION__, written, length);
			
			err = 1;
		}
		else {
			sat_log_log (SAT_LOG_LEVEL_MSG,
						 _("%s: Configuration saved for module %s."),
						 __FUNCTION__, modname);
			
			err = 0;
		}
	}

	g_free (datastream);
	g_free (filename);

	if (err)
		return MOD_CFG_ERROR;

	return MOD_CFG_OK;
}


/** \brief Delete module configuration file.
 *  \param modname The name of the module.
 *  \param needcfm Flag indicating whether the user should confirm th eoperation.
 *
 * This function deletes the module configuration file
 * /home/username/.gpredict2/modules/modname.mod from the disk. If needcfm is
 * TRUE the user will be asked for confirmation first. The function returns
 * MOD_CFG_CANCEL if the user has cancelled the operation.
 */
mod_cfg_status_t
mod_cfg_delete (gchar *modname, gboolean needcfm)
{
	return MOD_CFG_CANCEL;
}




static GtkWidget *
mod_cfg_editor_create (const gchar *modname, GKeyFile *cfgdata, GtkWidget *toplevel)
{
	GtkWidget   *dialog;
	GtkWidget   *add;
	GtkWidget   *table;
	GtkWidget   *label;
	GtkTooltips *tooltips;
	gchar       *icon;      /* windo icon file name */



	gboolean   new = (modname != NULL) ? FALSE : TRUE;

	if (new) {
		dialog = gtk_dialog_new_with_buttons (_("Create New Module"),
											  GTK_WINDOW (toplevel),
											  GTK_DIALOG_MODAL |
											  GTK_DIALOG_DESTROY_WITH_PARENT,
											  GTK_STOCK_PROPERTIES,
											  GTK_RESPONSE_HELP,
											  GTK_STOCK_CANCEL,
											  GTK_RESPONSE_CANCEL,
											  GTK_STOCK_OK,
											  GTK_RESPONSE_OK,
											  NULL);
	}
	else {
		dialog = gtk_dialog_new_with_buttons (_("Edit Module"),
											  GTK_WINDOW (toplevel),
											  GTK_DIALOG_MODAL |
											  GTK_DIALOG_DESTROY_WITH_PARENT,
											  GTK_STOCK_PROPERTIES,
											  GTK_RESPONSE_HELP,
											  GTK_STOCK_CANCEL,
											  GTK_RESPONSE_CANCEL,
											  GTK_STOCK_OK,
											  GTK_RESPONSE_OK,
											  NULL);
	}

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

	/* window icon */
	icon = icon_file_name ("gpredict-sat-pref.png");
	if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
		gtk_window_set_icon_from_file (GTK_WINDOW (dialog),
									   icon,
									   NULL);
	}
	g_free (icon);

	/* module name */
	namew = gtk_entry_new ();

	if (!new) {
		/* set module name and make entry insensitive */
		gtk_entry_set_text (GTK_ENTRY (namew), modname);
		gtk_widget_set_sensitive (namew, FALSE);
	}
	else {
		/* connect changed signal to text-checker */
		gtk_entry_set_max_length (GTK_ENTRY (namew), 25);
		tooltips = gtk_tooltips_new ();
		gtk_tooltips_set_tip (tooltips, namew,
							  _("Enter a short name for this module.\n"\
								"Allowed characters: 0..9, a..z, A..Z, - and _"),
							  _("The name will be used to identify the module "\
								"and it is also used a file name for saving the data."\
								"Max length is 25 characters."));

		/* attach changed signal so that we can enable OK button when
		   a proper name has been entered
		   oh, btw. disable OK button to begin with....
		*/
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
										   GTK_RESPONSE_OK,
										   FALSE);
		g_signal_connect (namew, "changed", G_CALLBACK (name_changed), dialog);
		
	}

	/* ground station selector */
	locw = create_loc_selector (cfgdata);

	table = gtk_table_new (2, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);

	label = gtk_label_new (_("Module Name"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), namew, 1, 3, 0, 1);
	label = gtk_label_new (_("Ground Station"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), locw, 1, 3, 1, 2);

	/* add button */
	add = gpredict_hstock_button (GTK_STOCK_ADD, NULL, _("Add new ground station"));
	g_signal_connect (add, "clicked", G_CALLBACK (add_qth_cb), dialog);
	gtk_table_attach_defaults (GTK_TABLE (table), add, 4, 5, 1, 2);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
						gtk_hseparator_new (), FALSE, FALSE, 5);

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Select Satellites:</b>"));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, FALSE, FALSE, 5);

	/* satellite selector */
	tree = gtk_sat_tree_new (0);
	if (!new) {
		/* select satellites */
		gint   *sats = NULL;
		gsize   length;
		GError *error = NULL;
		guint   i;

		sats = g_key_file_get_integer_list (cfgdata,
											MOD_CFG_GLOBAL_SECTION,
											MOD_CFG_SATS_KEY,
											&length,
											&error);
			
		if (error != NULL) {
			sat_log_log (SAT_LOG_LEVEL_ERROR,
						 _("%s: Failed to get list of satellites (%s)"),
						 __FUNCTION__, error->message);

			g_clear_error (&error);

			/* GLib API says nothing about the contents in case of error */
			if (sats) {
				g_free (sats);
			}

		}
		else {
			for (i = 0; i < length; i++) {
				gtk_sat_tree_select (GTK_SAT_TREE (tree), sats[i]);
			}
			g_free (sats);
		}

	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tree, TRUE, TRUE, 0);

	gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);

	return dialog;
}


/** \brief Manage name changes.
 *
 * This function is called when the contents of the name entry changes.
 * The primary purpose of this function is to check whether the char length
 * of the name is greater than zero, if yes enable the OK button of the dialog.
 */
static void
name_changed          (GtkWidget *widget, gpointer data)
{
	const gchar *text;
	gchar       *entry, *end, *j;
	gint         len, pos;
	GtkWidget   *dialog = GTK_WIDGET (data);


	/* step 1: ensure that only valid characters are entered
	   (stolen from xlog, tnx pg4i)
	*/
	entry = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
	if ((len = g_utf8_strlen (entry, -1)) > 0)
		{
			end = entry + g_utf8_strlen (entry, -1);
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
							gdk_beep ();
							pos = gtk_editable_get_position (GTK_EDITABLE (widget));
							gtk_editable_delete_text (GTK_EDITABLE (widget),
													  pos, pos+1);
							break;
						}
				}
		}


	/* step 2: if name seems all right, enable OK button */
	text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (g_utf8_strlen (text, -1) > 0) {
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
										   GTK_RESPONSE_OK,
										   TRUE);
	}
	else {
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
										   GTK_RESPONSE_OK,
										   FALSE);
	}
}


/** \brief Create location selector combo box.
 *
 * This function creates the location selector combo box and initialises it
 * with the names of the existing locations.
 */
static GtkWidget *
create_loc_selector   (GKeyFile *cfgdata)
{
	GtkWidget    *combo;
	GDir         *dir = NULL;
	GError       *error = NULL;
	gchar        *dirname;
	const gchar  *filename;
	gchar        *defqth = NULL;
	gchar       **buffv;
	gint         idx = -1;
	gint         count = 0;


	combo = gtk_combo_box_new_text ();

	/* get qth file name from cfgdata; if cfg data has no QTH
	   set defqth = "** DEFAULT **"
	*/

	if (g_key_file_has_key (cfgdata, MOD_CFG_GLOBAL_SECTION, MOD_CFG_QTH_FILE_KEY, NULL)) {
		defqth = g_key_file_get_string (cfgdata,
										MOD_CFG_GLOBAL_SECTION,
										MOD_CFG_QTH_FILE_KEY,
										&error);
	}
	else {
		sat_log_log (SAT_LOG_LEVEL_MSG,
					 _("%s: Module has no QTH; use default."),
					 __FUNCTION__);

		defqth = g_strdup (_("** DEFAULT **"));
	}


	/* scan for .qth files in the user config directory and
	   add the contents of each .qth file to the list store
	*/
	dirname = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
						   ".gpredict2", NULL);
	dir = g_dir_open (dirname, 0, &error);

	if (dir) {
		while ((filename = g_dir_read_name (dir))) {

			if (g_strrstr (filename, ".qth")) {

				buffv = g_strsplit (filename, ".qth", 0);
				gtk_combo_box_append_text (GTK_COMBO_BOX (combo), buffv[0]);
				g_strfreev (buffv);

				/* is this the QTH for this module? */
				if (!g_ascii_strcasecmp (defqth, filename)) {
					idx = count;
				}

				count++;
			}

		}

	}
	else {
		sat_log_log (SAT_LOG_LEVEL_ERROR,
					 _("%s:%d: Failed to open user cfg dir %s (%s)"),
					 __FILE__, __LINE__, dirname, error->message);
		g_clear_error (&error);
	}

	/* finally, add "** DEFAULT **" string; secting this will
	   clear the MOD_CFG_QTH_FILE_KEY module configuration
	   key ensuring that the module will use the default QTH
	*/
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("** DEFAULT **"));


	/* select the qth of this module;
	   if idx == -1 we should select the "** DEFAULT **"
	   string corresponding to index = count
	*/
	if (idx == -1) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), count);
	}
	else {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), idx);
	}

	g_free (defqth);
	g_free (dirname);
	g_dir_close (dir);


	return combo;
}




/** \brief Apply changes.
 *
 * This function is responsible for actually storing the changes
 * in the cfgdata structure. The changes are not applied
 * automatically, since there needs to be a possibility to
 * CANCEL any action.
 */
static void
mod_cfg_apply         (GKeyFile *cfgdata)
{
	guint *sats;
	gsize num;
	guint i;
	gchar *satstr = NULL;
	gchar *buff;


	/* store location */
	buff = gtk_combo_box_get_active_text (GTK_COMBO_BOX (locw));

	/* is buff == "** DEFAULT **" clear the configuration key
	   otherwise store the filename
	*/
	if (!g_ascii_strcasecmp (buff, _("** DEFAULT **"))) {
		g_key_file_remove_key (cfgdata,
							   MOD_CFG_GLOBAL_SECTION,
							   MOD_CFG_QTH_FILE_KEY,
							   NULL);
	}
	else {
		satstr = g_strconcat (buff, ".qth", NULL);
		g_key_file_set_string (cfgdata,
							   MOD_CFG_GLOBAL_SECTION,
							   MOD_CFG_QTH_FILE_KEY,
							   satstr);
		g_free (satstr);
	}

	g_free (buff);

	/* store satellites */
	sats = gtk_sat_tree_get_selected (GTK_SAT_TREE (tree), &num);

	for (i = 0; i < num; i++) {

		if (i) {
			buff = g_strdup_printf ("%s;%d", satstr, sats[i]);
			g_free (satstr);
		}
		else {
			buff = g_strdup_printf ("%d", sats[i]);
		}
		satstr = g_strdup (buff);
		g_free (buff);
	}

	g_key_file_set_string (cfgdata,
						   MOD_CFG_GLOBAL_SECTION,
						   MOD_CFG_SATS_KEY,
						   satstr);
	g_free (satstr);
	g_free (sats);

	sat_log_log (SAT_LOG_LEVEL_MSG,
				 _("%s: Applied changes to %s."),
				 __FUNCTION__, gtk_entry_get_text (GTK_ENTRY (namew)));
}




/** \brief Edit advanced settings.
 *  \param parent The parent mod-cfg dialog.
 *  \param cfgdata The GKeyFile structure containing module config.
 *
 * This function creates a dialog window containing a notebook with the
 * relevant sat-pref config modules, i.e. those which are relevant for
 * configuring GtkSatModules.
 */
static void
edit_advanced_settings (GtkDialog *parent, GKeyFile *cfgdata)
{
	GtkWidget *dialog;
	GtkWidget *contents;
	gchar     *icon;      /* window icon file name */

	dialog = gtk_dialog_new_with_buttons (_("Module Properties"),
										  GTK_WINDOW (parent),
										  GTK_DIALOG_MODAL |
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CANCEL,
										  GTK_RESPONSE_REJECT,
										  GTK_STOCK_OK,
										  GTK_RESPONSE_ACCEPT,
										  NULL);

	/* window icon */
	icon = icon_file_name ("gpredict-sat-pref.png");
	if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
		gtk_window_set_icon_from_file (GTK_WINDOW (dialog),
									   icon,
									   NULL);
	}


	contents = sat_pref_modules_create (cfgdata);
	gtk_widget_show_all (contents);

	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), contents);

	/* execute dialog */
	switch (gtk_dialog_run (GTK_DIALOG (dialog))) {

	case GTK_RESPONSE_ACCEPT:
		sat_pref_modules_ok (cfgdata);
		break;

	default:
		sat_pref_modules_cancel (cfgdata);
		break;
	}

	gtk_widget_destroy (dialog);

}



/** \brief Add QTH ("+" button).
 *  \param button The button that has been clicked.
 *  \param data Pointer to the dialog window.
 *
 */
static void
add_qth_cb (GtkWidget *button, gpointer data)
{
	GtkWindow       *parent = GTK_WINDOW (data);
	GtkResponseType  response;
	qth_t            qth;

	qth.name = NULL;
	qth.loc = NULL;
	qth.desc = NULL;
	qth.wx = NULL;
	qth.qra = NULL;
	qth.data = NULL;

	response = qth_editor_run (&qth, parent);

	if (response == GTK_RESPONSE_OK) {
		gtk_combo_box_prepend_text (GTK_COMBO_BOX (locw), qth.name);
		gtk_combo_box_set_active (GTK_COMBO_BOX (locw), 0);

		/* clean up */
		g_free (qth.name);
		if (qth.loc != NULL)
			g_free (qth.loc);
		if (qth.desc != NULL)
			g_free (qth.desc);
		if (qth.wx != NULL)
			g_free (qth.wx);
		if (qth.qra != NULL)
			g_free (qth.qra);
		/* 		if (qth.data != NULL) */
		/* 			g_key_file_free (data); */
	}
}
