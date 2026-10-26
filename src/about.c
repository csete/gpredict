/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2019  Alexandru Csete, OZ9AEC.

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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "about.h"
#include "compat.h"
#include "gpredict-help.h"
#include "sat-log.h"

/* Credits shown if the AUTHORS file can not be read */
static const gchar *fallback_authors[] = {
    "Alexandru Csete, OZ9AEC, and contributors",
    NULL
};

/* Last year of the copyright period shown in the about dialog */
#define COPYRIGHT_YEAR "2026"

extern GtkWidget *app;

/**
 * Read the list of authors from the AUTHORS file.
 *
 * Each line of the file becomes one line in the credits, with the "- "
 * list markers removed. Leading and trailing empty lines are dropped.
 *
 * @return NULL-terminated string array to be freed with g_strfreev(), or
 *         NULL if the file could not be read.
 */
static gchar **read_authors(void)
{
    GError *error = NULL;
    GPtrArray *lines;
    gchar **raw;
    gchar *fname;
    gchar *contents;
    gchar *line;
    guint i;

    fname = gpredict_help_txt_file_name("AUTHORS");
    if (!g_file_get_contents(fname, &contents, NULL, &error))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Failed to load %s (%s)"),
                    __func__, fname, error->message);
        g_clear_error(&error);
        g_free(fname);
        return NULL;
    }
    g_free(fname);

    raw = g_strsplit(contents, "\n", -1);
    g_free(contents);

    lines = g_ptr_array_new_with_free_func(g_free);
    for (i = 0; raw[i] != NULL; i++)
    {
        line = g_strstrip(raw[i]);
        if (g_str_has_prefix(line, "- "))
            line += 2;

        /* skip leading empty lines */
        if (lines->len == 0 && line[0] == '\0')
            continue;

        g_ptr_array_add(lines, g_strdup(line));
    }
    g_strfreev(raw);

    /* drop trailing empty lines */
    while (lines->len > 0 &&
           ((gchar *) g_ptr_array_index(lines, lines->len - 1))[0] == '\0')
        g_ptr_array_remove_index(lines, lines->len - 1);

    g_ptr_array_add(lines, NULL);

    return (gchar **) g_ptr_array_free(lines, FALSE);
}

void about_dialog_create()
{
    GtkWidget *dialog;
    GdkPixbuf *logo;
    gchar *iconfile;
    gchar *logofile;
    gchar *copyright;
    gchar **authors;

    dialog = gtk_about_dialog_new();

    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app));
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), _("Gpredict"));
    gtk_about_dialog_set_comments(
        GTK_ABOUT_DIALOG(dialog),
        _("real time satellite tracking and orbit prediction program"));
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
    copyright = g_strdup_printf(
        _("Copyright (C) 2001-%s Alexandru Csete OZ9AEC and contributors"),
        COPYRIGHT_YEAR);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), copyright);
    g_free(copyright);
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
                                 "https://oz9aec.dk/gpredict");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog),
                                      GTK_LICENSE_GPL_2_0);

    logofile = logo_file_name("gpredict_icon_color.svg");
    logo = gdk_pixbuf_new_from_file_at_size(logofile, 128, 128, NULL);
    g_free(logofile);
    if (logo)
    {
        gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), logo);
        g_object_unref(logo);
    }

    iconfile = icon_file_name("gpredict-icon.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(dialog), iconfile, NULL);
    g_free(iconfile);

    authors = read_authors();
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog),
                                 authors ? (const gchar **) authors :
                                 fallback_authors);
    g_strfreev(authors);
    gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog),
                                            _("translator-credits"));

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
