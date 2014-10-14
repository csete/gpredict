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
/** \brief High level entry point for saving satellite passes.
 *  \ingroup predict
 *
 * This module contains the main entry points into saving satellite passes.
 * The function can be called directly from the button click callback handlers.
 *
 */
#include <gtk/gtk.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-pass-dialogs.h"
#include "predict-tools.h"
#include "gtk-sat-data.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "pass-to-txt.h"
#include "save-pass.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif



static void file_changed (GtkWidget *widget, gpointer data);
static void    save_pass_exec (GtkWidget *parent,
                                pass_t *pass, qth_t *qth,
                                const gchar *savedir, const gchar *savefile,
                                gint format, gint contents);
static void save_passes_exec (GtkWidget *parent,
                              GSList *passes, qth_t *qth,
                              const gchar *savedir, const gchar *savefile,
                              gint format, gint contents);
static void save_to_file (GtkWidget *parent, const gchar *fname, const gchar *data);


enum pass_content_e {
    PASS_CONTENT_ALL = 0,
    PASS_CONTENT_TABLE,
    PASS_CONTENT_DATA,
};

enum passes_content_e {
    PASSES_CONTENT_FULL = 0,
    PASSES_CONTENT_SUM,
};


/** \brief Save a satellite pass.
 *  \param toplevel Pointer to the parent dialogue window
 *
 * This function is called from the button click handler of the satellite pass
 * dialogue when the user presses the Save button.
 *
 * The function opens the Save Pass dialogue asking the user for where to save
 * the pass and in which format. When the user has made the required choices,
 * the function uses the lower level functions to save the pass information
 * to a file.
 *
 * \note All the relevant data are attached to the parent dialogue window.
 */
void save_pass   (GtkWidget *parent)
{
    GtkWidget *dialog;
    GtkWidget *table;
    GtkWidget *dirchooser;
    GtkWidget *filchooser;
    GtkWidget *fmtchooser;
    GtkWidget *contents;
    GtkWidget *label;
    gint       response;
    pass_t    *pass;
    //gchar     *sat;
    qth_t     *qth;
    gchar     *savedir = NULL;
    gchar     *savefile;
    gint       format;
    gint       cont;


    /* get data attached to parent */
    //sat = (gchar *) g_object_get_data (G_OBJECT (parent), "sat");
    qth = (qth_t *) g_object_get_data (G_OBJECT (parent), "qth");
    pass = (pass_t *) g_object_get_data (G_OBJECT (parent), "pass");


    /* create the dialog */
    dialog = gtk_dialog_new_with_buttons (_("Save Pass Details"), GTK_WINDOW (parent),
                                          GTK_DIALOG_MODAL |
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_REJECT,
                                          GTK_STOCK_SAVE,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);


    /* create the table */
    table = gtk_table_new (4, 2, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (table), 10);
    gtk_container_set_border_width (GTK_CONTAINER (table), 10);

    /* directory chooser */
    label = gtk_label_new (_("Save in folder:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    dirchooser = gtk_file_chooser_button_new (_("Select a folder"),
                                              GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    savedir = sat_cfg_get_str (SAT_CFG_STR_PRED_SAVE_DIR);
    if (savedir) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dirchooser), savedir);
        g_free (savedir);
    }
    else {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dirchooser),
                                             g_get_home_dir ());
    }
    gtk_table_attach_defaults (GTK_TABLE (table), dirchooser, 1, 2, 0, 1);

    /* file name */
    label = gtk_label_new (_("Save using file name:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

    filchooser = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (filchooser), 100);
    g_signal_connect (filchooser, "changed", G_CALLBACK (file_changed), dialog);
    gtk_table_attach_defaults (GTK_TABLE (table), filchooser, 1, 2, 1, 2);

    /* use satellite name + orbit num as default; replace invalid characters
       with dash */
    savefile = g_strdup_printf ("%s-%d", pass->satname, pass->orbit);
    savefile = g_strdelimit (savefile, " ", '-');
    savefile = g_strdelimit (savefile, "!?/\()*&%$#@[]{}=+<>,.|:;", '_');
    gtk_entry_set_text (GTK_ENTRY (filchooser), savefile);
    g_free (savefile);


    /* file format */
    label = gtk_label_new (_("Save as:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

    fmtchooser = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (fmtchooser), _("Plain text (*.txt)"));
    /*     gtk_combo_box_append_text (GTK_COMBO_BOX (fmtchooser), _("Hypertext (*.html)")); */
    /*     gtk_combo_box_append_text (GTK_COMBO_BOX (fmtchooser), _("Docbook (*.xml)")); */
    gtk_combo_box_set_active (GTK_COMBO_BOX (fmtchooser),
                              sat_cfg_get_int (SAT_CFG_INT_PRED_SAVE_FORMAT));
    gtk_table_attach_defaults (GTK_TABLE (table), fmtchooser, 1, 2, 2, 3);

    gtk_widget_set_sensitive (fmtchooser, FALSE);

    /* file contents */
    label = gtk_label_new (_("File contents:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);

    contents = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (contents), _("Info+header+data"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (contents), _("Header + data"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (contents), _("Data only"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (contents),
                              sat_cfg_get_int (SAT_CFG_INT_PRED_SAVE_CONTENTS));
    gtk_table_attach_defaults (GTK_TABLE (table), contents, 1, 2, 3, 4);

    gtk_widget_show_all (table);
    gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), table);


    /* run the dialog */
    response = gtk_dialog_run (GTK_DIALOG (dialog));

    switch (response) {

        /* user clicked the save button */
    case GTK_RESPONSE_ACCEPT:

        /* get file and directory */
        savedir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dirchooser));
        savefile = g_strdup (gtk_entry_get_text (GTK_ENTRY (filchooser)));
        format = gtk_combo_box_get_active (GTK_COMBO_BOX (fmtchooser));
        cont = gtk_combo_box_get_active (GTK_COMBO_BOX (contents));

        /* call saver */
        save_pass_exec (dialog, pass, qth, savedir, savefile, format, cont);

        /* store new settings */
        sat_cfg_set_str (SAT_CFG_STR_PRED_SAVE_DIR, savedir);
        sat_cfg_set_int (SAT_CFG_INT_PRED_SAVE_FORMAT, format);
        sat_cfg_set_int (SAT_CFG_INT_PRED_SAVE_CONTENTS, cont);

        /* clean up */
        g_free (savedir);
        g_free (savefile);
        break;

        /* cancel */
    default:
        break;

    }

    gtk_widget_destroy (dialog);
}



/** \brief Save a satellite passes.
 *  \param toplevel Pointer to the parent dialogue window
 *
 * This function is called from the button click handler of the satellite passes
 * dialogue when the user presses the Save button.
 *
 * The function opens the Save Pass dialogue asking the user for where to save
 * the data and in which format. When the user has made the required choices,
 * the function uses the lower level functions to save the pass information
 * to a file.
 *
 * \note All the relevant data are attached to the parent dialogue window.
 */
void save_passes (GtkWidget *parent)
{
    GtkWidget *dialog;
    GtkWidget *table;
    GtkWidget *dirchooser;
    GtkWidget *filchooser;
    GtkWidget *fmtchooser;
    GtkWidget *contents;
    GtkWidget *label;
    gint       response;
    GSList    *passes;
    gchar     *sat;
    qth_t     *qth;
    gchar     *savedir = NULL;
    gchar     *savefile;
    gint       format;
    gint       cont;


    /* get data attached to parent */
    sat = (gchar *) g_object_get_data (G_OBJECT (parent), "sat");
    qth = (qth_t *) g_object_get_data (G_OBJECT (parent), "qth");
    passes = (GSList *) g_object_get_data (G_OBJECT (parent), "passes");


    /* create the dialog */
    dialog = gtk_dialog_new_with_buttons (_("Save Passes"), GTK_WINDOW (parent),
                                          GTK_DIALOG_MODAL |
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_REJECT,
                                          GTK_STOCK_SAVE,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);


    /* create the table */
    table = gtk_table_new (4, 2, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (table), 10);
    gtk_container_set_border_width (GTK_CONTAINER (table), 10);

    /* directory chooser */
    label = gtk_label_new (_("Save in folder:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    dirchooser = gtk_file_chooser_button_new (_("Select a folder"),
                                              GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    savedir = sat_cfg_get_str (SAT_CFG_STR_PRED_SAVE_DIR);
    if (savedir) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dirchooser), savedir);
        g_free (savedir);
    }
    else {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dirchooser),
                                             g_get_home_dir ());
    }
    gtk_table_attach_defaults (GTK_TABLE (table), dirchooser, 1, 2, 0, 1);

    /* file name */
    label = gtk_label_new (_("Save using file name:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

    filchooser = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (filchooser), 100);
    g_signal_connect (filchooser, "changed", G_CALLBACK (file_changed), dialog);
    gtk_table_attach_defaults (GTK_TABLE (table), filchooser, 1, 2, 1, 2);

    /* use satellite name + orbit num as default; replace invalid characters
       with dash */
    savefile = g_strdup_printf ("%s-passes", sat);
    savefile = g_strdelimit (savefile, " ", '-');
    savefile = g_strdelimit (savefile, "!?/\()*&%$#@[]{}=+<>,.|:;", '_');
    gtk_entry_set_text (GTK_ENTRY (filchooser), savefile);
    g_free (savefile);


    /* file format */
    label = gtk_label_new (_("Save as:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

    fmtchooser = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (fmtchooser), _("Plain text (*.txt)"));
    /*     gtk_combo_box_append_text (GTK_COMBO_BOX (fmtchooser), _("Hypertext (*.html)")); */
    /*     gtk_combo_box_append_text (GTK_COMBO_BOX (fmtchooser), _("Docbook (*.xml)")); */
    gtk_combo_box_set_active (GTK_COMBO_BOX (fmtchooser),
                              sat_cfg_get_int (SAT_CFG_INT_PRED_SAVE_FORMAT));
    gtk_table_attach_defaults (GTK_TABLE (table), fmtchooser, 1, 2, 2, 3);

    gtk_widget_set_sensitive (fmtchooser, FALSE);

    /* file contents */
    label = gtk_label_new (_("File contents:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);

    contents = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (contents), _("Complete report"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (contents), _("Summary"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (contents), 0);
    gtk_table_attach_defaults (GTK_TABLE (table), contents, 1, 2, 3, 4);

    gtk_widget_show_all (table);
    gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), table);


    /* run the dialog */
    response = gtk_dialog_run (GTK_DIALOG (dialog));

    switch (response) {

        /* user clicked the save button */
    case GTK_RESPONSE_ACCEPT:

        /* get file and directory */
        savedir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dirchooser));
        savefile = g_strdup (gtk_entry_get_text (GTK_ENTRY (filchooser)));
        format = gtk_combo_box_get_active (GTK_COMBO_BOX (fmtchooser));
        cont = gtk_combo_box_get_active (GTK_COMBO_BOX (contents));

        /* call saver */
        save_passes_exec (dialog, passes, qth, savedir, savefile, format, cont);

        /* store new settings */
        sat_cfg_set_str (SAT_CFG_STR_PRED_SAVE_DIR, savedir);
        sat_cfg_set_int (SAT_CFG_INT_PRED_SAVE_FORMAT, format);

        /* clean up */
        g_free (savedir);
        g_free (savefile);
        break;

        /* cancel */
    default:
        break;

    }

    gtk_widget_destroy (dialog);

}




/** \brief Manage file name changes.
 *  \param widget The GtkEntry that received the signal
 *  \param data User data (not used).
 *
 * This function is called when the contents of the file name entry changes.
 * It validates all characters and invalid chars are deleted.
 * The function sets the state of the Save button according to the validity
 * of the current file name.
 */
static void file_changed          (GtkWidget *widget, gpointer data)
{
    gchar       *entry, *end, *j;
    gint         len, pos;
    const gchar *text;
    GtkWidget   *dialog = GTK_WIDGET (data);


    /* ensure that only valid characters are entered
       (stolen from xlog, tnx pg4i)
    */
    entry = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
    if ((len = g_utf8_strlen (entry, -1)) > 0) {
        end = entry + g_utf8_strlen (entry, -1);
        for (j = entry; j < end; ++j) {
            switch (*j)    {
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
                                           GTK_RESPONSE_ACCEPT,
                                           TRUE);
    }
    else {
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           FALSE);
    }
}    

/** \brief Save data to file.
 *  \param parent Parent window (needed for error dialogs).
 *  \param pass The pass data to save.
 *  \param qth The observer data
 *  \param savedir The directory where data should be saved.
 *  \param savefile The file where data should be saved.
 *  \param format The file format
 *  \param contents The contents defining whether to save headers and such.
 *
 * This is the function that does the actual saving to a data file once all
 * required information has been gathered (i.e. file name, format, contents).
 * The function does some last minute checking while saving and provides
 * error messages if anything fails during the process.
 *
 * \note The formatting is done by external functions according to the selected
 *       file format.
 */
static void save_passes_exec (GtkWidget *parent,
                              GSList *passes, qth_t *qth,
                              const gchar *savedir, const gchar *savefile,
                              gint format, gint contents)
{
    gchar      *fname;
    gchar      *pgheader;
    gchar      *tblheader;
    gchar      *tblcontents;
    gchar      *buff = NULL;
    gchar      *data = NULL;
    pass_t     *pass;
    gint        fields;
    guint       i,n;


    switch (format) {

    case SAVE_FORMAT_TXT:

        /* prepare full file name */
        fname = g_strconcat (savedir, G_DIR_SEPARATOR_S, savefile, ".txt", NULL);

        /* get visible columns for summary */
        fields = sat_cfg_get_int (SAT_CFG_INT_PRED_MULTI_COL);

        /* create file contents */
        pgheader = passes_to_txt_pgheader (passes, qth, fields);
        tblheader = passes_to_txt_tblheader (passes, qth, fields);
        tblcontents = passes_to_txt_tblcontents (passes, qth, fields);

        data = g_strconcat (pgheader, tblheader, tblcontents, NULL);

        g_free (pgheader);
        g_free (tblheader);
        g_free (tblcontents);

        if (contents == PASSES_CONTENT_FULL) {
            fields = sat_cfg_get_int (SAT_CFG_INT_PRED_SINGLE_COL);
            n = g_slist_length (passes);

            for (i = 0; i < n; i++) {

                pass = PASS (g_slist_nth_data (passes, i));

                tblheader = pass_to_txt_tblheader (pass, qth, fields);
                tblcontents = pass_to_txt_tblcontents (pass, qth, fields);
                buff = g_strdup_printf ("%s\n Orbit %d\n%s%s",
                                        data, pass->orbit,
                                        tblheader, tblcontents);
                g_free (data);
                data = g_strdup (buff);
                g_free (buff);

            }
        }

        /* save data */
        save_to_file (parent, fname, data);
        g_free (data);
        g_free (fname);

        break;

    default:
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Invalid file format: %d"),
                     __func__, format);
        break;
    }

}




/** \brief Save data to file.
 *  \param parent Parent window (needed for error dialogs).
 *  \param passes The pass data to save.
 *  \param qth The observer data
 *  \param savedir The directory where data should be saved.
 *  \param savefile The file where data should be saved.
 *  \param format The file format
 *  \param contents The contents defining whether to save headers and such.
 *
 * This is the function that does the actual saving to a data file once all
 * required information has been gathered (i.e. file name, format, contents).
 * The function does some last minute checking while saving and provides
 * error messages if anything fails during the process.
 *
 * \note The formatting is done by external functions according to the selected
 *       file format.
 */
static void save_pass_exec (GtkWidget *parent,
                            pass_t *pass, qth_t *qth,
                            const gchar *savedir, const gchar *savefile,
                            gint format, gint contents)
{
    gchar      *fname;
    gchar      *pgheader;
    gchar      *tblheader;
    gchar      *tblcontents;
    gchar      *buff = NULL;
    gchar      *data = NULL;
    gint        fields;


    switch (format) {

    case SAVE_FORMAT_TXT:

        /* prepare full file name */
        fname = g_strconcat (savedir, G_DIR_SEPARATOR_S, savefile, ".txt", NULL);

        /* get visible columns */
        fields = sat_cfg_get_int (SAT_CFG_INT_PRED_SINGLE_COL);

        /* create file contents */
        pgheader = pass_to_txt_pgheader (pass, qth, fields);
        tblheader = pass_to_txt_tblheader (pass, qth, fields);
        tblcontents = pass_to_txt_tblcontents (pass, qth, fields);

        /* Add page header if selected */
        if (contents == PASS_CONTENT_ALL) {
            data = g_strdup (pgheader);
        }

        /* Add table header if selected */
        if ((contents == PASS_CONTENT_ALL) || (contents == PASS_CONTENT_TABLE)) {

            if (data != NULL) {
                buff = g_strdup (data);
                g_free (data);
                data = g_strconcat (buff, tblheader, NULL);
                g_free (buff);
            }
            else {
                data = g_strdup (tblheader);
            }

        }

        /* Add data */
        if (data != NULL) {
            buff = g_strdup (data);
            g_free (data);
            data = g_strconcat (buff, tblcontents, NULL);
            g_free (buff);
        }
        else {
            data = g_strdup (tblcontents);
        }
    
        /* save data */
        save_to_file (parent, fname, data);

        /* clean up memory */
        g_free (fname);
        g_free (data);
        g_free (pgheader);
        g_free (tblheader);
        g_free (tblcontents);

        break;

    default:
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Invalid file format: %d"),
                     __func__, format);
        break;
    }

}




static void save_to_file (GtkWidget *parent, const gchar *fname, const gchar *data)
{
    GIOChannel *chan;
    GError     *err = NULL;
    GtkWidget  *dialog;
    gsize       count;


    /* create file */
    chan = g_io_channel_new_file (fname, "w", &err);
    if (err != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Could not create file %s (%s)"),
                     __func__, fname, err->message);

        /* error dialog */
        dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         _("Could not create file %s\n\n%s"),
                                         fname, err->message);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        /* clean up and return */
        g_clear_error (&err);

        return;
    }

    /* save contents to file */
    g_io_channel_write_chars (chan, data, -1, &count, &err);
    if (err != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: An error occurred while saving data to %s (%s)"),
                     __func__, fname, err->message);

        dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         _("An error occurred while saving data to %s\n\n%s"),
                                         fname, err->message);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_clear_error (&err);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: Written %d characters to %s"),
                     __func__, count, fname);
    }

    /* close file, we don't care about errors here */
    g_io_channel_shutdown (chan, TRUE, NULL);
    g_io_channel_unref (chan);

}

