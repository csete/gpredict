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
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <sys/stat.h>

#include "compat.h"
#include "map-selector.h"
#include "sat-log.h"


/**
 * Convert map filesize into a human readable format
 *
 * @param size Size in bytes.
 * @param buffer Destination buffer or NULL to dynamically allocate a buffer.
 * @param buflen Length of @buffer in bytes.
 * @return A string containing a human readable description of size.
 *
 * The caller is responsible to free the returned string using g_free()
 * if you pass NULL for buffer. Else the returned string will be a
 * pointer to buffer.
 *
 * Credits: Thunar (XFCE file manager).
 *
 **/
static gchar   *get_map_humanize_size(gint64 size, gchar * buffer,
                                      gsize buflen)
{
    /* allocate buffer if necessary */
    if (buffer == NULL)
    {
        buffer = g_new(gchar, 32);
        buflen = 32;
    }

    if (size > 1024l * 1024l * 1024l)
        g_snprintf(buffer, buflen, "%0.1f GB",
                   size / (1024.f * 1024.f * 1024.f));
    else if (size > 1024l * 1024l)
        g_snprintf(buffer, buflen, "%0.1f MB", size / (1024.f * 1024.f));
    else if (size > 1024)
        g_snprintf(buffer, buflen, "%0.1f kB", size / 1024.f);
    else
        g_snprintf(buffer, buflen, "%lu B", (gulong) size);

    return buffer;
}

static GtkWidget *create_preview_widget(const gchar * selection)
{
    GtkWidget      *vbox;
    GtkWidget      *img;
    GtkWidget      *label;
    GdkPixbuf      *obuf, *sbuf;
    gchar          *buff;
    gint            w, h;
    gint64          size;
    struct stat     sb;
    gchar          *bf = NULL;


    label = gtk_label_new(NULL);
    img = gtk_image_new();

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    g_object_set_data(G_OBJECT(vbox), "image", img);
    g_object_set_data(G_OBJECT(vbox), "label", label);
    gtk_box_pack_start(GTK_BOX(vbox), img, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_widget_show_all(vbox);

    /* load current map into preview widget */
    obuf = gdk_pixbuf_new_from_file(selection, NULL);

    if (G_LIKELY(obuf != NULL))
    {

        /* store data */
        w = gdk_pixbuf_get_width(obuf);
        h = gdk_pixbuf_get_height(obuf);

        /* try to stat the file */
        if (G_UNLIKELY(g_stat(selection, &sb) < 0))
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%d: Could not stat %s"),
                        __FILE__, __LINE__, selection);

            size = 0;
        }
        else
        {
            size = sb.st_size;
        }

        bf = get_map_humanize_size(size, bf, 0);

        buff = g_strdup_printf("%dx%d pixels\n%s", w, h, bf);
        gtk_label_set_text(GTK_LABEL(label), buff);
        g_free(buff);
        g_free(bf);

        /* scale the pixbuf */
        sbuf = gdk_pixbuf_scale_simple(obuf, 160, 80, GDK_INTERP_HYPER);
        g_object_unref(obuf);

        /* update the GtkImage from the pixbuf */
        gtk_image_set_from_pixbuf(GTK_IMAGE(img), sbuf);
        g_object_unref(sbuf);
    }
    else
    {
        gtk_image_set_from_icon_name(GTK_IMAGE(img), "image-missing",
                                     GTK_ICON_SIZE_LARGE_TOOLBAR);
    }

    return vbox;
}

static void update_preview_widget(GtkFileChooser * chooser, gpointer data)
{
    GtkWidget      *box = GTK_WIDGET(data);
    GtkImage       *img = GTK_IMAGE(g_object_get_data(G_OBJECT(box), "image"));
    GtkLabel       *lab = GTK_LABEL(g_object_get_data(G_OBJECT(box), "label"));
    gchar          *sel = NULL;
    GdkPixbuf      *obuf, *sbuf;
    gint            w, h;
    gint64          size;
    struct stat     sb;
    gchar          *buff = NULL;


    sel = gtk_file_chooser_get_preview_filename(chooser);

    if (sel != NULL)
    {
        obuf = gdk_pixbuf_new_from_file(sel, NULL);

        /* try to stat the file */
        if (g_stat(sel, &sb) < 0)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%d: Could not stat %s"),
                        __FILE__, __LINE__, sel);

            size = 0;
        }
        else
        {
            size = sb.st_size;
        }

        g_free(sel);

        if (obuf != NULL)
        {

            /* store data */
            w = gdk_pixbuf_get_width(obuf);
            h = gdk_pixbuf_get_height(obuf);

            buff = get_map_humanize_size(size, buff, 0);

            sel = g_strdup_printf("%dx%d pixels\n%s", w, h, buff);
            gtk_label_set_text(lab, sel);
            g_free(sel);
            g_free(buff);

            /* scale the pixbuf */
            sbuf = gdk_pixbuf_scale_simple(obuf, 160, 80, GDK_INTERP_HYPER);
            g_object_unref(obuf);

            /* update the GtkImage from the pixbuf */
            gtk_image_clear(img);
            gtk_image_set_from_pixbuf(img, sbuf);
            g_object_unref(sbuf);

            /* report success */
            gtk_file_chooser_set_preview_widget_active(chooser, TRUE);
        }
        else
        {
            gtk_image_clear(img);
            gtk_image_set_from_icon_name(GTK_IMAGE(img), "image-missing",
                                         GTK_ICON_SIZE_LARGE_TOOLBAR);
            /* report failure */
            gtk_file_chooser_set_preview_widget_active(chooser, FALSE);
        }
    }
}

/**
 * Run map selector.
 *
 * @param curmap The name of the current map.
 * @return A newly allocated string with filename of the selected map
 *         or NULL if action was cancelled.
 *
 * This function creates and executes a file chooser dialog
 * and selects the currently selected map curmap. The file chooser dialogue
 * has a custom map preview widget showing the scaled down copty of
 * the selected map as well as the size of the map.
 */
gchar          *select_map(const gchar * curmap)
{
    GtkWidget      *chooser;
    GtkFileFilter  *filter;
    gchar          *selection;
    gchar          *mapsdir;
    gchar          *ret = NULL;
    GtkWidget      *preview;


    /* we are going to need this as a shortcut folder
       and to find out whether selected map is stock
       or user specific
     */
    mapsdir = get_maps_dir();

    /* create a new file chooser dialogue in "open file" mode */
    chooser = gtk_file_chooser_dialog_new(_("Select Map"),
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                          "_Open", GTK_RESPONSE_ACCEPT,
                                          NULL);

    /* single selection only */
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), FALSE);

    /* add shortcut */
    gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(chooser), mapsdir,
                                         NULL);

    /* create filter */
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Image files"));
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    /* select current map file */
    if (g_path_is_absolute(curmap))
    {
        /* map is user specific, ie. in USER_CONF_DIR/maps/ */
        selection = g_strdup(curmap);
    }
    else
    {
        /* build complete path */
        selection = map_file_name(curmap);
    }
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(chooser), selection);

    /* preview widget */
    preview = create_preview_widget(selection);
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(chooser), preview);
    gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(chooser),
                                               TRUE);
    /* sig connect */
    g_signal_connect(G_OBJECT(chooser), "update-preview",
                     G_CALLBACK(update_preview_widget), preview);

    /* we don't need this anymore */
    g_free(selection);

    /* run the dialogue */
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
    {
        char           *filename;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));

        /* find out whether selected file is a
           stock map or user map */
        if (g_str_has_prefix(filename, mapsdir))
        {
            /* selected map is a stock map */

            /* get file name without dir component */
            ret = g_path_get_basename(filename);

        }
        else
        {
            ret = g_strdup(filename);
        }


        /* clean up */
        g_free(filename);
    }

    gtk_widget_destroy(chooser);

    g_free(mapsdir);

    return ret;
}
