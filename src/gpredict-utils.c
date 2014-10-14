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
/** \brief Various utility functions.
 *
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "compat.h"
#include "sat-log.h"
#include "gpredict-utils.h"
#include "strnatcmp.h"




static void set_combo_tooltip (GtkWidget *combo, gpointer text);


/** \brief Create a horizontal pixmap button.
 *
 * The text will be placed to the right of the image.
 * file is only the icon name, not the full path.
 */
GtkWidget *
gpredict_hpixmap_button (const gchar *file, const gchar *text, const gchar *tooltip)
{
     GtkWidget *button;
     GtkWidget *image;
     GtkWidget *box;
     gchar     *path;

     path = icon_file_name (file);
     image = gtk_image_new_from_file (path);
     g_free (path);
     box = gtk_hbox_new (FALSE, 0);
     gtk_box_pack_start (GTK_BOX (box), image, TRUE, TRUE, 0);
     if (text != NULL)
          gtk_box_pack_start (GTK_BOX (box), gtk_label_new (text), TRUE, TRUE, 0);

     button = gtk_button_new ();
     gtk_widget_set_tooltip_text (button, tooltip);
     gtk_container_add (GTK_CONTAINER (button), box);

     return button;
}




/** \brief Create a vertical pixmap button.
 *
 * The text will be placed under the image.
 * file is only the icon name, not the full path.
 */
GtkWidget *
gpredict_vpixmap_button (const gchar *file, const gchar *text, const gchar *tooltip)
{
     GtkWidget *button;
     GtkWidget *image;
     GtkWidget *box;
     gchar     *path;

     path = icon_file_name (file);
     image = gtk_image_new_from_file (path);
     g_free (path);
     box = gtk_vbox_new (FALSE, 0);
     gtk_box_pack_start (GTK_BOX (box), image, TRUE, TRUE, 0);
     if (text != NULL)
          gtk_box_pack_start (GTK_BOX (box), gtk_label_new (text), TRUE, TRUE, 0);

     button = gtk_button_new ();
     gtk_widget_set_tooltip_text (button, tooltip);
     gtk_container_add (GTK_CONTAINER (button), box);

     return button;
}


/** \brief Create a horizontal pixmap button using stock pixmap.
 *
 * The text will be placed to the right of the image.
 * The icon size will be GTK_ICON_SIZE_BUTTON.
 */
GtkWidget *
gpredict_hstock_button (const gchar *stock_id, const gchar *text, const gchar *tooltip)
{
     GtkWidget *button;
     GtkWidget *image;
     GtkWidget *box;

     image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
     box = gtk_hbox_new (FALSE, 0);
     gtk_box_pack_start (GTK_BOX (box), image, TRUE, TRUE, 0);
     if (text != NULL)
          gtk_box_pack_start (GTK_BOX (box), gtk_label_new (text), TRUE, TRUE, 0);

     button = gtk_button_new ();
     gtk_widget_set_tooltip_text (button, tooltip);
     gtk_container_add (GTK_CONTAINER (button), box);

     return button;
}


/** \brief Create and set tooltips for GtkComboBox.
 *  \param combo The GtkComboBox widget.
 *  \param text  Pointer to the desired tooltip text.
 *
 * This function creates and sets the tooltips for the specified widget.
 * The interface is implemented such that this function can be connected
 * directly to the \a realized signal of the GtkComboBox.
 *
 * Actually, this function only loops over all the children of the GtkComboBox
 * and calls the set_combo_tooltip internal function.
 *
 * \note This works only if the funcion is actually used as callback for the
 *       \a realize signal og the GtkComboBox.
 *
 * \note This great trick has been pointed out by Matthias Clasen, he has done the
 *       the same for the filter combo in the new GtkFileChooser
 *       ref: gtkfilechooserdefault.c:3151 in Gtk+ 2.5.5
 */
void
gpredict_set_combo_tooltips (GtkWidget *combo, gpointer text)
{

     /* for each child in the container call the internal
        function which actually creates the tooltips.
     */
     gtk_container_forall (GTK_CONTAINER (combo),
                                set_combo_tooltip,
                                text);

}


/** \brief Create and set tooltips for GtkComboBox.
 *  \param text  Pointer to the desired tooltip text.
 *
 * This function creates and sets the tooltips for the specified widget.
 * This function is called by the \a grig_set_combo_tooltips function which
 * is must be used as callback for the "realize" signal of the GtkComboBox.
 */
static void
set_combo_tooltip (GtkWidget *combo, gpointer text)
{

     /* if current child is a button we have BINGO! */
     if (GTK_IS_BUTTON (combo)) {
         gtk_widget_set_tooltip_text (combo, text);
     }

}




/** \brief Copy file.
 */
gint
gpredict_file_copy (const gchar *in, const gchar *out)
{
     gchar    *contents;
     gboolean  status=0;
     GError   *err=NULL;
     gsize     ulen;
     gssize    slen;


     g_return_val_if_fail (in != NULL, 1);
     g_return_val_if_fail (out != NULL, 1);


     /* read source file */
     if (!g_file_get_contents (in, &contents, &ulen, &err)) {

          sat_log_log (SAT_LOG_LEVEL_ERROR, "%s: %s", __func__, err->message);
          g_clear_error (&err);

          status = 1;
     }
     else {

          /* write contents to new file */
          slen = (gssize) ulen;

          if (!g_file_set_contents (out, contents, slen, &err)) {

               sat_log_log (SAT_LOG_LEVEL_ERROR, "%s: %s", __func__, err->message);
               g_clear_error (&err);

               status = 1;

          }

          g_free (contents);
     }


     return status;
}



/** \brief Create a miniature pixmap button with no relief.
 *
 *  Pixmapfile is only the icon name, not the full path.
 */
GtkWidget *
gpredict_mini_mod_button (const gchar *pixmapfile, const gchar *tooltip)
{
     GtkWidget *button;
     GtkWidget *image;
     gchar     *path;

     path = icon_file_name (pixmapfile);
     image = gtk_image_new_from_file (path);
     g_free (path);

     button = gtk_button_new ();
     gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
     gtk_widget_set_tooltip_text (button, tooltip);
     gtk_container_add (GTK_CONTAINER (button), image);

     return button;
}


/** \brief Convert a BCD colour to a GdkColor structure.
 *  \param rgb The source colour in 0xRRGGBB form.
 *  \param color Pointer to an existing GdkColor structure.
 */
void
rgb2gdk  (guint rgb, GdkColor *color)
{
     guint16 r,g,b;
     guint tmp;

     /* sanity checks */
     if (color == NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
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
     color->red   = 257 * r;
     color->green = 257 * g;
     color->blue  = 257 * b;
}


/** \brief Convert a BCD colour to a GdkColor structure.
 *  \param rgba The source colour in 0xRRGGBBAA form.
 *  \param color Pointer to an existing GdkColor structure.
 *  \param alpha Pointer to where the alpha channel value should be stored
 */
void
rgba2gdk (guint rgba, GdkColor *color, guint16 *alpha)
{
     guint16 r,g,b;
     guint tmp;


     /* sanity checks */
     if (color == NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s:%s: %s called with color = NULL"),
                          __FILE__, __LINE__, __func__);
          return;
     }
     if (alpha == NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
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
     color->red   = 257 * r;
     color->green = 257 * g;
     color->blue  = 257 * b;
}


/** \brief Convert GdkColor to BCD colour.
 *  \param color The GdkColor structure.
 *  \param rgb Pointer to where the 0xRRGGBB encoded colour should be stored.
 */
void
gdk2rgb  (const GdkColor *color, guint *rgb)
{
     guint r,g,b;
     guint16 tmp;


     /* sanity checks */
     if (color == NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s:%s: %s called with color = NULL"),
                          __FILE__, __LINE__, __func__);
          return;
     }
     if (rgb == NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
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


/** \brief Convert GdkColor and alpha channel to BCD colour.
 *  \param color The GdkColor structure.
 *  \param alpha The value of the alpha channel.
 *  \param rgb Pointer to where the 0xRRGGBBAA encoded colour should be stored.
 */
void
gdk2rgba (const GdkColor *color, guint16 alpha, guint *rgba)
{
     guint r,g,b,a;
     guint16 tmp;


     /* sanity checks */
     if (color == NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s:%s: %s called with color = NULL"),
                          __FILE__, __LINE__, __func__);
          return;
     }
     if (rgba == NULL) {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
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


/** \brief Convert GdkColor to RRGGBB hex format (for Pango Markup).
 *  \param color The GdkColor structure.
 *  \return A newly allocated character string.
 */
gchar *
rgba2html (guint rgba)
{
     gchar *col;
     guint8 r,g,b;
     guint16 tmp;


     tmp = rgba & 0xFF000000;
     r = (guint8) (tmp >> 24);

     /* green */
     tmp = rgba & 0x00FF0000;
     g = (guint8) (tmp >> 16);

     /* blue */
     tmp = rgba & 0x0000FF00;
     b = (guint8) (tmp >> 8);

     col = g_strdup_printf ("%X%X%X",r,g,b);

     return col;
}

/** \brief String comparison function
 *  \param s1 first string
 *  \param s2 second string
 */

int gpredict_strcmp (const char *s1, const char *s2) {
#if 0

    gchar *a,*b;
    int retval;
    a=g_ascii_strup(s1,-1);
    b=g_ascii_strup(s2,-1);
    
    retval=strverscmp(a,b);
    g_free(a);
    g_free(b);
    return retval;
#else
    return strnatcasecmp(s1,s2);
#endif 
}


/** \brief substring finding function
 *  \param s1 the larger string
 *  \param s2 the substring that we are searching for.
 *  \return pointer to the substring location
 *  
 *  this is a substitute for strcasestr which is a gnu c extension and not available everywhere.
 */


char * gpredict_strcasestr(const char *s1, const char *s2)
{
	size_t s1_len = strlen(s1);
	size_t s2_len = strlen(s2);
	while (s1_len >= s2_len) {
		if (strncasecmp(s1, s2, s2_len) == 0)
		    return (char *) s1;

		s1++;
		s1_len--;
	}

	return NULL;
}

/** \brief Save a GKeyFile structure to a file
 *  \param cfgdata is a pointer to the GKeyFile.
 *  \param filename is a pointer the filename string.
 *  \return 1 on error and zero on success.
 *  This might one day be in glib but for now it is not a standard function.
 *  Variants of this were throughout the code and it is now consilidated here.
 */

gboolean gpredict_save_key_file (GKeyFile *cfgdata, const char *filename) {
    gchar      *datastream;    /* cfgdata string */    
    gsize       length;        /* length of the data stream */
    gsize       written;       /* number of bytes actually written */
    gboolean    err = 0;       /* the error value */
    GIOChannel *cfgfile;       /* file */
    GError     *error = NULL;  /* Error handler */
        
    /* ok, go on and convert the data */
    datastream = g_key_file_to_data (cfgdata, &length, &error);
    
    if (error != NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Could not create config data (%s)."),
                     __func__, error->message);
        
        g_clear_error (&error);
        
        err = 1;
    } else {
        cfgfile = g_io_channel_new_file (filename, "w", &error);
        
        if (error != NULL) {
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Could not create config file (%s)."),
                         __func__, error->message);
            
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
                             __func__, error->message);
                
                g_clear_error (&error);
                
                err = 1;
            }
            else if (length != written) {
                sat_log_log (SAT_LOG_LEVEL_WARN,
                             _("%s: Wrote only %d out of %d chars."),
                             __func__, written, length);
                
                err = 1;
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_INFO,
                             _("%s: Configuration saved for %s."),
                             __func__, filename);
                
                err = 0;
            }
        }
    }
    g_free (datastream);

    return err;
    
}
