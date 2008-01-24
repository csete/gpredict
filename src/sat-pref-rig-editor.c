/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>

  Comments, questions and bugreports should be submitted via
  http://sourceforge.net/projects/groundstation/
  More details can be found at the project home page:

  http://groundstation.sourceforge.net/
 
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

/** \brief Edit radio configuration.
 *
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "radio-conf.h"
#include "sat-pref-rig-editor.h"

#ifdef HAVE_HAMLIB
#  include <hamlib/rig.h>


extern GtkWidget *window; /* dialog window defined in sat-pref.c */



/* private widgets */
static GtkWidget *dialog;   /* dialog window */
static GtkWidget *name;     /* Configuration name */
static GtkWidget *model;    /* radio model, e.g. TS-2000 */
static GtkWidget *civ;      /* Icom CI-V address */
static GtkWidget *type;     /* radio type */
static GtkWidget *port;     /* port selector */
static GtkWidget *speed;    /* serial speed selector */
static GtkWidget *dtr,*rts; /* DTR and RTS line states */


static GtkWidget    *create_editor_widgets (radio_conf_t *conf);
static void          update_widgets        (radio_conf_t *conf);
static void          clear_widgets         (void);
static gboolean      apply_changes         (radio_conf_t *conf);
static void          name_changed          (GtkWidget *widget, gpointer data);
static GtkTreeModel *create_rig_model      (void);
static gint          rig_list_add          (const struct rig_caps *, void *);
static gint          rig_list_compare_mfg  (gconstpointer, gconstpointer);
static gint          rig_list_compare_mod  (gconstpointer, gconstpointer);
static void          is_rig_model          (GtkCellLayout   *cell_layout,
                                            GtkCellRenderer *cell,
                                            GtkTreeModel    *tree_model,
                                            GtkTreeIter     *iter,
                                            gpointer         data);
static void          select_rig            (guint rigid);


/** \brief Add or edit a radio configuration.
 * \param conf Pointer to a radio configuration.
 *
 * Of conf->name is not NULL the widgets will be populated with the data.
 */
void
sat_pref_rig_editor_run (radio_conf_t *conf)
{
	gint       response;
	gboolean   finished = FALSE;


	/* crate dialog and add contents */
	dialog = gtk_dialog_new_with_buttons (_("Edit radio configuration"),
										  GTK_WINDOW (window),
										  GTK_DIALOG_MODAL |
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CLEAR,
										  GTK_RESPONSE_REJECT,
										  GTK_STOCK_CANCEL,
										  GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OK,
										  GTK_RESPONSE_OK,
										  NULL);

	/* disable OK button to begin with */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
									   GTK_RESPONSE_OK,
									   FALSE);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
					   create_editor_widgets (conf));

	/* this hacky-thing is to keep the dialog running in case the
	   CLEAR button is plressed. OK and CANCEL will exit the loop
	*/
	while (!finished) {

		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {

			/* OK */
		case GTK_RESPONSE_OK:
			if (apply_changes (conf)) {
				finished = TRUE;
			}
			else {
				finished = FALSE;
			}
			break;

			/* CLEAR */
		case GTK_RESPONSE_REJECT:
			clear_widgets ();
			break;

			/* Everything else is considered CANCEL */
		default:
			finished = TRUE;
			break;
		}
	}

	gtk_widget_destroy (dialog);
}


/** \brief Create and initialise widgets */
static GtkWidget *
create_editor_widgets (radio_conf_t *conf)
{
	GtkWidget    *table;
	GtkWidget    *label;
    GtkTreeModel *riglist;
    GtkCellRenderer *renderer;
    gchar           *buff;
    guint            i;


	table = gtk_table_new (5, 5, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);

	/* Config name */
	label = gtk_label_new (_("Name"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 4, 0, 1);
	
	name = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (name), 25);
    gtk_widget_set_tooltip_text (name,
                                 _("Enter a short name for this configuration, e.g. IC910-1.\n"\
                                   "Allowed charachters: 0..9, a..z, A..Z, - and _"));
	gtk_table_attach_defaults (GTK_TABLE (table), name, 1, 4, 0, 1);

	/* attach changed signal so that we can enable OK button when
	   a proper name has been entered
	*/
	g_signal_connect (name, "changed", G_CALLBACK (name_changed), NULL);

	/* Model */
	label = gtk_label_new (_("Model"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	
    riglist = create_rig_model ();
	model = gtk_combo_box_new_with_model (riglist);
    g_object_unref (riglist);
    gtk_table_attach_defaults (GTK_TABLE (table), model, 1, 2, 1, 2);
    
    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (model), renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (model), renderer,
                                    "text", 0,
                                    NULL);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (model),
                                        renderer,
                                        is_rig_model,
                                        NULL, NULL);
    gtk_widget_set_tooltip_text (model, _("Click to select a radio."));
    select_rig (1);
    
    /* ICOM CI-V adress */
    label = gtk_label_new (_("ICOM CI-V"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 1, 2);
    
    civ = gtk_combo_box_new_text ();
    gtk_widget_set_tooltip_text (civ,
                                 _("Select ICOM CI-V address of the radio."));
    
    /* works, but pretty lame... */
    gtk_combo_box_append_text (GTK_COMBO_BOX (civ), _("Default"));
    for (i = 1; i < 0xF0; i++) {
        if (i < 0x10)
            buff = g_strdup_printf ("0x0%X", i);
        else
            buff = g_strdup_printf ("0x%X", i);
        gtk_combo_box_append_text (GTK_COMBO_BOX (civ), buff);
        g_free (buff);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (civ), 0);
    gtk_table_attach_defaults (GTK_TABLE (table), civ, 4, 5, 1, 2);


	/* Type */
	label = gtk_label_new (_("Type"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
    type = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (type), _("Receiver"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (type), _("Transmitter"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (type), _("RX + TX"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (type), _("Full Duplex"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (type), 0);
    gtk_widget_set_tooltip_text (type,
                               _("Select radio type. Consult the user manual, if unsure"));
    gtk_table_attach_defaults (GTK_TABLE (table), type, 1, 2, 2, 3);

	/* Port */
	label = gtk_label_new (_("Port"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
    
    port = gtk_combo_box_entry_new_text ();
    if (conf->port != NULL) {
        gtk_combo_box_append_text (GTK_COMBO_BOX (port), conf->port);
    }
    gtk_combo_box_append_text (GTK_COMBO_BOX (port), "/dev/ttyS0");
    gtk_combo_box_append_text (GTK_COMBO_BOX (port), "/dev/ttyS1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (port), "/dev/ttyS2");
    gtk_combo_box_append_text (GTK_COMBO_BOX (port), "/dev/ttyUSB0");
    gtk_combo_box_append_text (GTK_COMBO_BOX (port), "/dev/ttyUSB1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (port), "/dev/ttyUSB2");
    gtk_combo_box_set_active (GTK_COMBO_BOX (port), 0);
    gtk_widget_set_tooltip_text (port, _("Select or enter communication port"));
    gtk_table_attach_defaults (GTK_TABLE (table), port, 1, 2, 3, 4);

    /* DTR State */
    label = gtk_label_new (_("DTR Line"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 3, 4);
    
    dtr = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (dtr), _("Undefined"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dtr), _("OFF"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dtr), _("ON"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dtr), _("PTT"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dtr), _("CW"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (dtr), 0);
    gtk_widget_set_tooltip_text (dtr, _("Select status and use of DTR line"));
    gtk_table_attach_defaults (GTK_TABLE (table), dtr, 4, 5, 3, 4);
    
   
	/* Speed */
	label = gtk_label_new (_("Rate"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
    speed = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "300");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "1200");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "2400");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "4800");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "9600");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "19200");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "38400");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "57600");
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed), "115200");
    gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 4);
    gtk_widget_set_tooltip_text (speed, _("Select serial port speed"));
    gtk_table_attach_defaults (GTK_TABLE (table), speed, 1, 2, 4, 5);

    /* RTS State */
    label = gtk_label_new (_("RTS Line"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 4, 5);
    
    rts = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (rts), _("Undefined"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (rts), _("OFF"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (rts), _("ON"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (rts), _("PTT"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (rts), _("CW"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (rts), 0);
    gtk_widget_set_tooltip_text (rts, _("Select status and use of RTS line"));
    gtk_table_attach_defaults (GTK_TABLE (table), rts, 4, 5, 4, 5);

    /* separator between port/speed and DTR/RTS */
    gtk_table_attach (GTK_TABLE (table), gtk_vseparator_new(), 2, 3, 1, 5,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);
    
	if (conf->name != NULL)
		update_widgets (conf);

	gtk_widget_show_all (table);

	return table;
}


/** \brief Update widgets from the currently selected row in the treeview
 */
static void
update_widgets (radio_conf_t *conf)
{
    
    /* configuration name */
    gtk_entry_set_text (GTK_ENTRY (name), conf->name);
    
    /* model */
    select_rig (conf->id);
    
    /* type */
    gtk_combo_box_set_active (GTK_COMBO_BOX (type), conf->type);
    
    /* port */
    gtk_combo_box_prepend_text (GTK_COMBO_BOX (port), conf->port);
    gtk_combo_box_set_active (GTK_COMBO_BOX (port), 0);

    /*serial speed */        
    switch (conf->speed) {
        case 300:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 0);
            break;
        case 1200:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 1);
            break;
        case 2400:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 2);
            break;
        case 4800:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 3);
            break;
        case 9600:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 4);
            break;
        case 19200:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 5);
            break;
        case 38400:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 6);
            break;
        case 57600:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 7);
            break;
        case 115200:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 8);
            break;
        default:
            gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 4);
            break;
    }

    /* CI-V */
    gtk_combo_box_set_active (GTK_COMBO_BOX (civ), conf->civ);
    
    /* DTR and RTS lines */
    gtk_combo_box_set_active (GTK_COMBO_BOX (dtr), conf->dtr);
    gtk_combo_box_set_active (GTK_COMBO_BOX (rts), conf->rts);
}



/** \brief Clear the contents of all widgets.
 *
 * This function is usually called when the user clicks on the CLEAR button
 *
 */
static void
clear_widgets () 
{
    gtk_entry_set_text (GTK_ENTRY (name), "");
    select_rig (1);
    gtk_combo_box_set_active (GTK_COMBO_BOX (type), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (port), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (speed), 4);
    gtk_combo_box_set_active (GTK_COMBO_BOX (civ), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (dtr), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (rts), 0);
}


/** \brief Apply changes.
 *  \return TRUE if things are ok, FALSE otherwise.
 *
 * This function is usually called when the user clicks the OK button.
 */
static gboolean
apply_changes         (radio_conf_t *conf)
{
    GtkTreeIter   iter1,iter2;
    GtkTreeModel *riglist;
    gchar        *b1,*b2;
    guint         id;
    
    
    /* name */
    if (conf->name)
        g_free (conf->name);

    conf->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (name)));
    
    /* model */
    if (conf->model)
        g_free (conf->model);

    /* iter1 is needed to construct full model name */
    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (model), &iter2);
    riglist = gtk_combo_box_get_model (GTK_COMBO_BOX (model));
    gtk_tree_model_iter_parent (riglist, &iter1, &iter2);
    
    /* build model string */
    gtk_tree_model_get (riglist, &iter1, 0, &b1, -1);
    gtk_tree_model_get (riglist, &iter2, 0, &b2, -1);
    conf->model = g_strconcat (b1, " ", b2, NULL);
    g_free (b1);
    g_free (b2);
    
    /* ID */
    gtk_tree_model_get (riglist, &iter2, 1, &id, -1);
    conf->id = id;

    /* radio type */
    conf->type = gtk_combo_box_get_active (GTK_COMBO_BOX (type));
    
    /* port / device */
    if (conf->port)
        g_free (conf->port);
    
    conf->port = gtk_combo_box_get_active_text (GTK_COMBO_BOX (port));

    /* CI-V */
    conf->civ = gtk_combo_box_get_active (GTK_COMBO_BOX (civ));
    
    /* serial speed */
    switch (gtk_combo_box_get_active (GTK_COMBO_BOX (speed))) {
        case 0:
            conf->speed = 300;
            break;
        case 1:
            conf->speed = 1200;
            break;
        case 2:
            conf->speed = 2400;
            break;
        case 3:
            conf->speed = 4800;
            break;
        case 4:
            conf->speed = 9600;
            break;
        case 5:
            conf->speed = 19200;
            break;
        case 6:
            conf->speed = 38400;
            break;
        case 7:
            conf->speed = 57600;
            break;
        case 8:
            conf->speed = 115200;
            break;
        default:
            conf->speed = 9600;
            break;
    }
    
    /* DTR and RTS */
    conf->dtr = gtk_combo_box_get_active (GTK_COMBO_BOX (dtr));
    conf->rts = gtk_combo_box_get_active (GTK_COMBO_BOX (rts));
    
	return TRUE;
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
	gchar *entry, *end, *j;
	gint len, pos;


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


/** \brief Radio info to be used when building the rig model */
typedef struct {
    gint    id;       /*!< Model ID. */
    gchar  *mfg;      /*!< Manufacurer name (eg. KENWOOD). */
    gchar  *model;    /*!< Radio model (eg. TS-440). */
} rig_info_t;


/** \brief Build tree model containing radios.
 * \return A tree model where the radios are ordered according to
 *         manufacturer.
 * 
 */
static GtkTreeModel *create_rig_model ()
{
    GArray      *array;
    rig_info_t  *info;
    GtkTreeIter  iter1; /* iter used for manufacturer */
    GtkTreeIter  iter2; /* iter used for model */
    GtkTreeStore *store;
    gchar        *buff;
    gint          status;
    gint          i;

    
    /* create araay containing rigs */
    array = g_array_new (FALSE, FALSE, sizeof (rig_info_t));
    rig_load_all_backends();
 
    /* fill list using rig_list_foreach */
    status = rig_list_foreach (rig_list_add, (void *) array);
    
    /* sort the array, first by model then by mfg */
    g_array_sort (array, rig_list_compare_mod);
    g_array_sort (array, rig_list_compare_mfg);

    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                 _("%s:%d: Read %d distinct radios into array."),
                   __FILE__, __LINE__, array->len);
    
    /* create a tree store with two cols (name and ID) */
    store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    
    /* add array contents to treestore */
    for (i = 0; i < array->len; i++) {
        
        /* get rig info struct */
        info = &g_array_index (array, rig_info_t, i);
        
        if (gtk_tree_store_iter_is_valid (store, &iter1)) {
            /* iter1 is valid, i.e. we already have a manufacturer */
            gtk_tree_model_get (GTK_TREE_MODEL (store), &iter1,
                                0, &buff,
                                -1);
            if (g_ascii_strcasecmp (buff, info->mfg)) {
                /* mfg different, add new mfg */
                gtk_tree_store_append (store, &iter1, NULL);
                gtk_tree_store_set (store, &iter1, 0, info->mfg, -1);
            }
            /* else: mfg are identical; nothing to do */
        }
        else {
            /* iter1 is not valid, so add the first manufacturer */
            gtk_tree_store_append (store, &iter1, NULL);
            gtk_tree_store_set (store, &iter1, 0, info->mfg, -1);
        }
        
        /* iter1 points to the parent mfg; insert this rig */
        gtk_tree_store_append (store, &iter2, &iter1);
        gtk_tree_store_set (store, &iter2,
                            0, info->model,
                            1, info->id,
                            -1);
        
        /* done with this model */
        g_free (info->mfg);
        g_free (info->model);
    }
    
    g_array_free (array,TRUE);

    return GTK_TREE_MODEL (store);
}


/** \brief Add new entry to list of radios.
 *  \param caps Structure with the capablities of thecurrent radio.
 *  \param array Pointer to the GArray into which the new entry should be 
 *               stored.
 *  \return Always 1 to keep rig_list_foreach running.
 *
 * This function is called by the rig_list_foreach hamlib function for each
 * supported radio. It copies the relevant data into a grig_rig_info_t
 * structure and adds the new entry to the GArray containing the list of
 * supported radios.
 *
 * \sa rig_list_compare
 */
static gint
rig_list_add (const struct rig_caps *caps, void *array)
{
    rig_info_t *info;

    /* create new entry */
    info = g_malloc (sizeof (rig_info_t));

    /* fill values */
    info->id      = caps->rig_model;
    info->mfg     = g_strdup (caps->mfg_name);
    info->model   = g_strdup (caps->model_name);

    /* append new element to array */
    array = (void *) g_array_append_vals ((GArray *) array, info, 1);

    /* keep on running */
    return 1;
}



/** \brief Compare two rig info entries.
 *  \param a Pointer to the first entry.
 *  \param b Pointer to the second entry.
 *  \return Negative value if a < b; zero if a = b; positive value if a > b.
 *
 * This function is used to compare two rig entries in the list of radios
 * when the list is sorted. It compares the manufacturer of the two radios.
 *
 * \sa rig_list_add
 */
static gint
rig_list_compare_mfg  (gconstpointer a, gconstpointer b)
{
    gchar *ida, *idb;

    ida = ((rig_info_t *) a)->mfg;
    idb = ((rig_info_t *) b)->mfg;

    if (g_ascii_strcasecmp(ida,idb) < 0) {
        return -1;
    }
    else if (g_ascii_strcasecmp(ida,idb) > 0) {
        return 1;
    }
    else {
        return 0;
    }

}



/** \brief Compare two rig info entries.
 *  \param a Pointer to the first entry.
 *  \param b Pointer to the second entry.
 *  \return Negative value if a < b; zero if a = b; positive value if a > b.
 *
 * This function is used to compare two rig entries in the list of radios
 * when the list is sorted. It compares the model of the two radios.
 *
 * \sa rig_list_add
 */
static gint
rig_list_compare_mod  (gconstpointer a, gconstpointer b)
{
    gchar *ida, *idb;

    ida = ((rig_info_t *) a)->model;
    idb = ((rig_info_t *) b)->model;

    if (g_ascii_strcasecmp(ida,idb) < 0) {
        return -1;
    }
    else if (g_ascii_strcasecmp(ida,idb) > 0) {
        return 1;
    }
    else {
        return 0;
    }

}


/** \brief Set cell sensitivity.
 * 
 * This function is used to disable the sensitive of manifacturer entries
 * as children. Otherwise, the manufacturer would appear as the first entry
 * in a submenu.
 * */
static void
is_rig_model (GtkCellLayout   *cell_layout,
              GtkCellRenderer *cell,
              GtkTreeModel    *tree_model,
              GtkTreeIter     *iter,
              gpointer         data)
{
    gboolean sensitive;

    sensitive = !gtk_tree_model_iter_has_child (tree_model, iter);

    g_object_set (cell, "sensitive", sensitive, NULL);
}


/** \brief Select a radio in the combo box.
 * \param rigid The hamlib id of the radio.
 * 
 * This function selects the specified radio in the combobox. This is done
 * by looping over all items in the tree model until a match is reached
 * (or there are no more items left).
 */
static void
select_rig            (guint rigid)
{
    GtkTreeIter   iter1,iter2;
    GtkTreeModel *riglist;
    guint         i,j,n,m;
    guint         thisrig = 0;
    
    
    /* get the tree model */
    riglist = gtk_combo_box_get_model (GTK_COMBO_BOX (model));
    
    /* get the number of toplevel nodes */
    n = gtk_tree_model_iter_n_children (riglist, NULL);
    for (i = 0; i < n; i++) {
        
        /* get the i'th toplevel node */
        if (gtk_tree_model_iter_nth_child (riglist, &iter1, NULL, i)) {
            
            /* get the number of children */
            m = gtk_tree_model_iter_n_children (riglist, &iter1);
            for (j = 0; j < m; j++) {
                
                /* get the j'th child */
                if (gtk_tree_model_iter_nth_child (riglist, &iter2, &iter1, j)) {
                    
                    /* get ID of this model */
                    gtk_tree_model_get (riglist, &iter2, 1, &thisrig, -1);
                    
                    if (thisrig == rigid) {
                        /* select this rig and terminate loop */
                        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (model), &iter2);
                        j = m;
                        i = n;
                    }
                }
                else {
                    sat_log_log (SAT_LOG_LEVEL_BUG,
                                 _("%s:%s: NULL child node at index %d:%d"),
                                   __FILE__, __FUNCTION__, i, j);
                    
                }
            }
            
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_BUG,
                         _("%s:%s: NULL toplevel node at index %d"),
                           __FILE__, __FUNCTION__, i);
        }
    }
    
    
}

#endif
