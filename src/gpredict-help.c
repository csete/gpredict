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
#include "sat-log.h"
#include "sat-cfg.h"
#include "sat-pref-help.h"
#include "gpredict-help.h"


extern sat_help_t sat_help[]; /* in sat-pref-help.c */
extern GtkWidget *app;


static gint config_help (void);


/** \brief Launch help system.
 *
 */
void
gpredict_help_launch (gpredict_help_type_t type)
{
     browser_type_t idx;
     gint           resp;

     (void) type; /* avoid unused parameter compiler warning */


     idx = sat_cfg_get_int (SAT_CFG_INT_WEB_BROWSER_TYPE);

     /* some sanity check before accessing the arrays ;-) */
     if ((idx <= BROWSER_TYPE_NONE) || (idx >= BROWSER_TYPE_NUM)) {
          idx = BROWSER_TYPE_NONE;
     }

     if (idx == BROWSER_TYPE_NONE) {
          sat_log_log (SAT_LOG_LEVEL_INFO,
                    _("%s: Help browser is not set up yet."),
                    __func__);

          resp = config_help ();

          if (resp == GTK_RESPONSE_CANCEL) {
               sat_log_log (SAT_LOG_LEVEL_INFO,
                         _("%s: Configure help browser cancelled."),
                         __func__);

               return;
          }

          /* else try again */
          idx = sat_cfg_get_int (SAT_CFG_INT_WEB_BROWSER_TYPE);          
     }

     if ((idx <= BROWSER_TYPE_NONE) || (idx >= BROWSER_TYPE_NUM)) {
          return;
     }

     /* launch help browser */
     sat_log_log (SAT_LOG_LEVEL_DEBUG,
               _("%s: Launching help browser %s."),
               __func__, sat_help[idx].type);

     g_print ("FIXME: FINSH IMPELMTATION\n");
}



/** \brief Configure help system.
 *  \return GTK_RESPONSE_OK if the help browser has been set up or
 *          GTK_RESPONSE_CANCEL if the user has cancelled the action.
 *
 * This function is called if the user wants to see the online
 * help but has not yet configured a help browser. The function
 * will create a dialog containing the same cfg widget as in sat-pref-help
 * and allow the user to configure the html browser.
 */
static gint
config_help (void)
{
     GtkWidget *dialog;
     GtkWidget *label;
     GtkBox *vbox;
     gint resp;


     dialog = gtk_dialog_new_with_buttons (_("Configure Help Browser"),
                               GTK_WINDOW (app),
                               GTK_DIALOG_MODAL |
                               GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL,
                               GTK_STOCK_OK,
                               GTK_RESPONSE_OK,
                               NULL);

     label = gtk_label_new (_("Please select a HTML browser to be used to view the help."));
     vbox = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog)));
     gtk_box_pack_start (vbox, label, FALSE, TRUE, 0);
     gtk_box_pack_start (vbox, sat_pref_help_create (), TRUE, FALSE, 0);

     gtk_widget_show_all (GTK_WIDGET(vbox));

     resp = gtk_dialog_run (GTK_DIALOG (dialog));

     switch (resp) {

          /* save browser settings */
     case GTK_RESPONSE_OK:
          sat_pref_help_ok ();
          break;
          
     default:
          sat_pref_help_cancel ();
          break;
     }

     gtk_widget_destroy (dialog);

     return resp;
}



/** \brief Show a text file in the gpredict system directory
 *  \param filename The basic file name
 *
 * This function is intended to display files like NEWS, COPYING, etc.
 * Note that on windows these files have .txt suffix, while on Unix they
 * do not.
 *
 */
void
gpredict_help_show_txt (const gchar *filename)
{
     GtkWidget     *dialog;
     GtkWidget     *swin;
     GtkWidget     *view;
     GtkTextBuffer *txtbuf;
     GIOChannel    *chan;
     GError        *error = NULL;
     gchar         *fname;
     gchar         *buff;
     gsize          length;


     /* get system data directory */
#ifdef G_OS_UNIX
     fname = g_strconcat (PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S, filename, NULL);
#else
#  ifdef G_OS_WIN32
        buff = g_win32_get_package_installation_directory_of_module (NULL);
        fname = g_strconcat (buff, G_DIR_SEPARATOR_S,
                                    "doc", G_DIR_SEPARATOR_S,
                                    filename, ".txt", NULL);
        g_free (buff);
#  endif
#endif

     /* load file into buffer */
     chan = g_io_channel_new_file (fname, "r", &error);
     if (error != NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s: Failed to load %s (%s)"),
                          __func__, fname, error->message);
          g_free (fname);
          g_clear_error (&error);

          return;
     }

     g_io_channel_read_to_end (chan, &buff, &length, &error);
     if (error != NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s: Error reading %s (%s)"),
                          __func__, fname, error->message);

          g_free (buff);
          g_clear_error (&error);
          g_io_channel_shutdown (chan, TRUE, NULL);
          g_io_channel_unref (chan);

          return;
     }

     g_free (fname);
          
     /* create text view and text buffer widgets */
     view = gtk_text_view_new ();
     gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
     gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
     txtbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

     gtk_text_buffer_set_text (txtbuf, buff, -1);
     g_free (buff);

     /* scrolled window */
     swin = gtk_scrolled_window_new (NULL, NULL);
     gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin),
                                                   GTK_SHADOW_ETCHED_IN);
     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                             GTK_POLICY_NEVER,
                                             GTK_POLICY_AUTOMATIC);
     gtk_container_add (GTK_CONTAINER (swin), view);

     /* create and show dialogue with textbuffer */
     dialog = gtk_dialog_new_with_buttons (_("Gpredict Info"),
                                                    NULL,
                                                    0,
                                                    GTK_STOCK_CLOSE,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
     gtk_widget_set_size_request (dialog, -1, 450);
     buff = icon_file_name ("gpredict-icon.png");
     gtk_window_set_icon_from_file (GTK_WINDOW (dialog), buff, NULL);
     g_free (buff);

     g_signal_connect_swapped (dialog, "response", 
                                     G_CALLBACK (gtk_widget_destroy), dialog);

     gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), swin);
     gtk_widget_show_all (dialog);

}
