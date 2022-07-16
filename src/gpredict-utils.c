/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2019  Alexandru Csete, OZ9AEC
 
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
#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <ctype.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gpredict-utils.h"
#include "sat-log.h"
#include "strnatcmp.h"



/**
 * Create a horizontal pixmap button.
 *
 * The text will be placed to the right of the image.
 * file is only the icon name, not the full path.
 */
GtkWidget      *gpredict_hpixmap_button(const gchar * file, const gchar * text,
                                        const gchar * tooltip)
{
    GtkWidget      *button;
    GtkWidget      *image;
    GtkWidget      *box;
    gchar          *path;

    path = icon_file_name(file);
    image = gtk_image_new_from_file(path);
    g_free(path);
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_box_pack_start(GTK_BOX(box), image, TRUE, TRUE, 0);
    if (text != NULL)
        gtk_box_pack_start(GTK_BOX(box), gtk_label_new(text), TRUE, TRUE, 0);

    button = gtk_button_new();
    gtk_widget_set_tooltip_text(button, tooltip);
    gtk_container_add(GTK_CONTAINER(button), box);

    return button;
}

/**
 * Create a vertical pixmap button.
 *
 * The text will be placed under the image.
 * file is only the icon name, not the full path.
 */
GtkWidget      *gpredict_vpixmap_button(const gchar * file, const gchar * text,
                                        const gchar * tooltip)
{
    GtkWidget      *button;
    GtkWidget      *image;
    GtkWidget      *box;
    gchar          *path;

    path = icon_file_name(file);
    image = gtk_image_new_from_file(path);
    g_free(path);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_box_pack_start(GTK_BOX(box), image, TRUE, TRUE, 0);
    if (text != NULL)
        gtk_box_pack_start(GTK_BOX(box), gtk_label_new(text), TRUE, TRUE, 0);

    button = gtk_button_new();
    gtk_widget_set_tooltip_text(button, tooltip);
    gtk_container_add(GTK_CONTAINER(button), box);

    return button;
}

/**
 * Create a horizontal pixmap button using stock pixmap.
 *
 * The text will be placed to the right of the image.
 * The icon size will be GTK_ICON_SIZE_BUTTON.
 */
GtkWidget      *gpredict_hstock_button(const gchar * stock_id,
                                       const gchar * text,
                                       const gchar * tooltip)
{
    GtkWidget      *button;
    GtkWidget      *image;
    GtkWidget      *box;

    image = gtk_image_new_from_icon_name(stock_id, GTK_ICON_SIZE_BUTTON);
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_box_pack_start(GTK_BOX(box), image, TRUE, TRUE, 0);
    if (text != NULL)
        gtk_box_pack_start(GTK_BOX(box), gtk_label_new(text), TRUE, TRUE, 0);

    button = gtk_button_new();
    gtk_widget_set_tooltip_text(button, tooltip);
    gtk_container_add(GTK_CONTAINER(button), box);

    return button;
}

/**
 * Create and set tooltips for GtkComboBox.
 *
 * @param text  Pointer to the desired tooltip text.
 *
 * This function creates and sets the tooltips for the specified widget.
 * This function is called by the \a grig_set_combo_tooltips function which
 * is must be used as callback for the "realize" signal of the GtkComboBox.
 */
static void set_combo_tooltip(GtkWidget * combo, gpointer text)
{
    /* if current child is a button we have BINGO! */
    if (GTK_IS_BUTTON(combo))
        gtk_widget_set_tooltip_text(combo, text);
}

/**
 * Create and set tooltips for GtkComboBox.
 *
 * @param combo The GtkComboBox widget.
 * @param text  Pointer to the desired tooltip text.
 *
 * This function creates and sets the tooltips for the specified widget.
 * The interface is implemented such that this function can be connected
 * directly to the @a realized signal of the GtkComboBox.
 *
 * Actually, this function only loops over all the children of the GtkComboBox
 * and calls the set_combo_tooltip internal function.
 *
 * @note This works only if the function is actually used as callback for the
 *       @a realize signal og the GtkComboBox.
 *
 * @note This great trick has been pointed out by Matthias Clasen, he has done the
 *       the same for the filter combo in the new GtkFileChooser
 *       ref: gtkfilechooserdefault.c:3151 in Gtk+ 2.5.5
 */
void gpredict_set_combo_tooltips(GtkWidget * combo, gpointer text)
{
    /* for each child in the container call the internal
       function which actually creates the tooltips.
     */
    gtk_container_forall(GTK_CONTAINER(combo), set_combo_tooltip, text);

}

gint gpredict_file_copy(const gchar * in, const gchar * out)
{
    gchar          *contents;
    gboolean        status = 0;
    GError         *err = NULL;
    gsize           ulen;
    gssize          slen;

    g_return_val_if_fail(in != NULL, 1);
    g_return_val_if_fail(out != NULL, 1);

    /* read source file */
    if (!g_file_get_contents(in, &contents, &ulen, &err))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, "%s: %s", __func__, err->message);
        g_clear_error(&err);

        status = 1;
    }
    else
    {
        /* write contents to new file */
        slen = (gssize) ulen;

        if (!g_file_set_contents(out, contents, slen, &err))
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR, "%s: %s", __func__, err->message);
            g_clear_error(&err);

            status = 1;
        }

        g_free(contents);
    }

    return status;
}

/**
 * Create a miniature pixmap button with no relief.
 *
 * Pixmapfile is only the icon name, not the full path.
 */
GtkWidget      *gpredict_mini_mod_button(const gchar * pixmapfile,
                                         const gchar * tooltip)
{
    GtkWidget      *button;
    GtkWidget      *image;
    gchar          *path;

    path = icon_file_name(pixmapfile);
    image = gtk_image_new_from_file(path);
    g_free(path);

    button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(button, tooltip);
    gtk_container_add(GTK_CONTAINER(button), image);

    return button;
}

/**
 * Convert a BCD colour to a GdkColor structure.
 * 
 * @param rgb The source colour in 0xRRGGBB form.
 * @param color Pointer to an existing GdkColor structure.
 */
void rgb2gdk(guint rgb, GdkColor * color)
{
    guint16         r, g, b;
    guint           tmp;

    /* sanity checks */
    if (color == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: %s called with color = NULL"),
                    __FILE__, __LINE__, __func__);
        return;
    }

    /* red */
    tmp = rgb & 0xFF0000;
    r = (guint16) (tmp >> 16);

    /* green */
    tmp = rgb & 0x00FF00;
    g = (guint16) (tmp >> 8);

    /* blue */
    tmp = rgb & 0x0000FF;
    b = (guint16) tmp;

    /* store colours */
    color->red = 257 * r;
    color->green = 257 * g;
    color->blue = 257 * b;
}

/**
 * Convert a BCD colour to a GdkColor structure.
 *
 * @param rgba The source colour in 0xRRGGBBAA form.
 * @param color Pointer to an existing GdkColor structure.
 * @param alpha Pointer to where the alpha channel value should be stored
 */
void rgba2gdk(guint rgba, GdkColor * color, guint16 * alpha)
{
    guint16         r, g, b;
    guint           tmp;

    /* sanity checks */
    if (color == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: %s called with color = NULL"),
                    __FILE__, __LINE__, __func__);
        return;
    }
    if (alpha == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: %s called with alpha = NULL"),
                    __FILE__, __LINE__, __func__);
        return;
    }

    /* red */
    tmp = rgba & 0xFF000000;
    r = (guint16) (tmp >> 24);

    /* green */
    tmp = rgba & 0x00FF0000;
    g = (guint16) (tmp >> 16);

    /* blue */
    tmp = rgba & 0x0000FF00;
    b = (guint16) (tmp >> 8);

    /* alpha channel */
    *alpha = (guint16) (257 * (rgba & 0x000000FF));

    /* store colours */
    color->red = 257 * r;
    color->green = 257 * g;
    color->blue = 257 * b;
}

/**
 * Convert GdkColor to BCD colour.
 *
 * @param color The GdkColor structure.
 * @param rgb Pointer to where the 0xRRGGBB encoded colour should be stored.
 */
void gdk2rgb(const GdkColor * color, guint * rgb)
{
    guint           r, g, b;
    guint16         tmp;

    /* sanity checks */
    if (color == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: %s called with color = NULL"),
                    __FILE__, __LINE__, __func__);
        return;
    }
    if (rgb == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: %s called with rgb = NULL"),
                    __FILE__, __LINE__, __func__);
        return;
    }

    /* red */
    tmp = color->red / 257;
    r = (guint) (tmp << 16);

    /* green */
    tmp = color->green / 257;
    g = (guint) (tmp << 8);

    /* blue */
    tmp = color->blue / 257;
    b = (guint) tmp;

    *rgb = (r | g | b);
}

/**
 * Convert GdkColor and alpha channel to BCD colour.
 *
 * @param color The GdkColor structure.
 * @param alpha The value of the alpha channel.
 * @param rgb Pointer to where the 0xRRGGBBAA encoded colour should be stored.
 */
void gdk2rgba(const GdkColor * color, guint16 alpha, guint * rgba)
{
    guint           r, g, b, a;
    guint16         tmp;

    /* sanity checks */
    if (color == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: %s called with color = NULL"),
                    __FILE__, __LINE__, __func__);
        return;
    }
    if (rgba == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: %s called with rgba = NULL"),
                    __FILE__, __LINE__, __func__);
        return;
    }

    /* red */
    tmp = color->red / 257;
    r = (guint) (tmp << 24);

    /* green */
    tmp = color->green / 257;
    g = (guint) (tmp << 16);

    /* blue */
    tmp = color->blue / 257;
    b = (guint) (tmp << 8);

    /* alpha */
    tmp = alpha / 257;
    a = (guint) tmp;

    *rgba = (r | g | b | a);
}

/**
 * Convert GdkColor to RRGGBB hex format (for Pango Markup).
 *
 * @param color The GdkColor structure.
 * @return A newly allocated character string.
 */
gchar          *rgba2html(guint rgba)
{
    gchar          *col;
    guint8          r, g, b;
    guint           tmp;

    tmp = rgba & 0xFF000000;
    r = (guint8) (tmp >> 24);

    /* green */
    tmp = rgba & 0x00FF0000;
    g = (guint8) (tmp >> 16);

    /* blue */
    tmp = rgba & 0x0000FF00;
    b = (guint8) (tmp >> 8);

    col = g_strdup_printf("%X%X%X", r, g, b);

    return col;
}

/**
 * String comparison function
 * @param s1 first string
 * @param s2 second string
 */
int gpredict_strcmp(const char *s1, const char *s2)
{
#if 0
    gchar          *a, *b;
    int             retval;

    a = g_ascii_strup(s1, -1);
    b = g_ascii_strup(s2, -1);

    retval = strverscmp(a, b);
    g_free(a);
    g_free(b);
    return retval;
#else
    return strnatcasecmp(s1, s2);
#endif
}

/**
 * Substring finding function
 *
 * @param s1 the larger string
 * @param s2 the substring that we are searching for.
 * @return pointer to the substring location
 *  
 *  this is a substitute for strcasestr which is a gnu c extension and not available everywhere.
 */
char           *gpredict_strcasestr(const char *s1, const char *s2)
{
    size_t          s1_len = strlen(s1);
    size_t          s2_len = strlen(s2);

    while (s1_len >= s2_len)
    {
        if (strncasecmp(s1, s2, s2_len) == 0)
            return (char *)s1;

        s1++;
        s1_len--;
    }

    return NULL;
}

/**
 * Save a GKeyFile structure to a file
 *
 * @param cfgdata is a pointer to the GKeyFile.
 * @param filename is a pointer the filename string.
 * @return 1 on error and zero on success.
 *
 */
gboolean gpredict_save_key_file(GKeyFile * cfgdata, const char *filename)
{
    GError         *error = NULL; 

    if (!g_key_file_save_to_file(cfgdata, filename, &error))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error writing config data (%s)."),
                    __func__,
                    error != NULL ? error->message : "unknown error");
        g_clear_error(&error);
        return 1;
    }

    return 0;
}


/**
 * Check if \c ch is an alpha-num; in range \c "[0-9a-zA-F]".
 * Or \c "ch == '-'" or \c "ch == '_'".
 *
 * @param ch the character code to check.
 * @return TRUE if okay.
 */
gboolean gpredict_legal_char(int ch)
{
    if (g_ascii_isalnum(ch) || ch == '-' || ch == '_')
        return (TRUE);
    return (FALSE);
}


/* Convert a 0xRRGGBBAA encoded config integer to a GdkRGBA structure */
void rgba_from_cfg(guint cfg_rgba, GdkRGBA * gdk_rgba)
{
    if (gdk_rgba == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s called with gdk_rgba = NULL"), __func__);
        return;
    }

    gdk_rgba->red = ((cfg_rgba >> 24) & 0xFF) / 255.0;
    gdk_rgba->green = ((cfg_rgba >> 16) & 0xFF) / 255.0;
    gdk_rgba->blue = ((cfg_rgba >> 8) & 0xFF) / 255.0;
    gdk_rgba->alpha = (cfg_rgba & 0xFF) / 255.0;
}

/* convert GdkRGBA struct to 0xRRGGBBAA formatted config integer */
guint rgba_to_cfg(const GdkRGBA * gdk_rgba)
{
    guint           cfg_int = 0;

    if (gdk_rgba == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s called with gdk_rgba = NULL"), __func__);
        return 0;
    }

    cfg_int = ((guint) (gdk_rgba->red * 255.0)) << 24;
    cfg_int += ((guint) (gdk_rgba->green * 255.0)) << 16;
    cfg_int += ((guint) (gdk_rgba->blue * 255.0)) << 8;
    cfg_int += (guint) (gdk_rgba->alpha * 255.0);

    return cfg_int;
}

/* Convert a 0xRRGGBB encoded config integer to a GdkRGBA structure (no alpha channel) */
void rgb_from_cfg(guint cfg_rgb, GdkRGBA * gdk_rgba)
{
    if (gdk_rgba == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s called with gdk_rgba = NULL"), __func__);
        return;
    }

    gdk_rgba->red = ((cfg_rgb >> 16) & 0xFF) / 255.0;
    gdk_rgba->green = ((cfg_rgb >> 8) & 0xFF) / 255.0;
    gdk_rgba->blue = (cfg_rgb & 0xFF) / 255.0;
    gdk_rgba->alpha = 1.0;
}

/* convert GdkRGBA struct to 0xRRGGBB formatted config integer, ignoring the alpha channel */
guint rgb_to_cfg(const GdkRGBA * gdk_rgba)
{
    guint           cfg_int = 0;

    if (gdk_rgba == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s called with gdk_rgba = NULL"), __func__);
        return 0;
    }

    cfg_int = ((guint) (gdk_rgba->red * 255.0)) << 16;
    cfg_int += ((guint) (gdk_rgba->green * 255.0)) << 8;
    cfg_int += (guint) (gdk_rgba->blue * 255.0);

    return cfg_int;
}
