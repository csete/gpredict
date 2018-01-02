/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
#include <build-config.h>
#endif
#include "compat.h"
#include "gpredict-help.h"
#include "sat-cfg.h"
#include "sat-log.h"


extern GtkWidget *app;

/**
 * Show a text file in the gpredict system directory
 * @param filename The basic file name
 *
 * This function is intended to display files like NEWS, COPYING, etc.
 * Note that on windows these files have .txt suffix, while on Unix they
 * do not.
 */
void gpredict_help_show_txt(const gchar * filename)
{
    GtkWidget      *dialog;
    GtkWidget      *swin;
    GtkWidget      *view;
    GtkTextBuffer  *txtbuf;
    GIOChannel     *chan;
    GError         *error = NULL;
    gchar          *fname;
    gchar          *buff;
    gsize           length;

    /* get system data directory */
#ifdef G_OS_UNIX
    fname = g_strconcat(PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S, filename, NULL);
#endif
#ifdef G_OS_WIN32
    buff = g_win32_get_package_installation_directory_of_module(NULL);
    fname = g_strconcat(buff, G_DIR_SEPARATOR_S, "doc",
                        G_DIR_SEPARATOR_S, filename, ".txt", NULL);
    g_free(buff);
#endif

    /* load file into buffer */
    chan = g_io_channel_new_file(fname, "r", &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to load %s (%s)"),
                    __func__, fname, error->message);
        g_free(fname);
        g_clear_error(&error);

        return;
    }

    g_io_channel_read_to_end(chan, &buff, &length, &error);
    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error reading %s (%s)"),
                    __func__, fname, error->message);

        g_free(buff);
        g_clear_error(&error);
        g_io_channel_shutdown(chan, TRUE, NULL);
        g_io_channel_unref(chan);

        return;
    }

    g_free(fname);

    /* create text view and text buffer widgets */
    view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 15);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 15);
    if (GTK_MINOR_VERSION >= 16)
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), FALSE);

    txtbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

    gtk_text_buffer_set_text(txtbuf, buff, -1);
    g_free(buff);

    /* scrolled window */
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin),
                                        GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swin), view);

    /* create and show dialogue with textbuffer */
    dialog = gtk_dialog_new_with_buttons(_("Gpredict Info"),
                                         GTK_WINDOW(app), 0,
                                         "_Close", GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_set_size_request(dialog, -1, 450);
    buff = icon_file_name("gpredict-icon.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(dialog), buff, NULL);
    g_free(buff);

    g_signal_connect_swapped(dialog, "response",
                             G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_box_pack_start(GTK_BOX
                       (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), swin,
                       TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);
}
