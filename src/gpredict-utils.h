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
#ifndef GPREDICT_UTILS_H
#define GPREDICT_UTILS_H 1

#include <gtk/gtk.h>

#define M_TO_FT(x) (3.2808399*x)
#define FT_TO_M(x) (x/3.2808399)
#define KM_TO_MI(x) (x/1.609344)
#define MI_TO_KM(x) (1.609344*x)


GtkWidget *gpredict_hpixmap_button (const gchar *file,
                                    const gchar *text,
                                    const gchar *tooltip);

GtkWidget *gpredict_vpixmap_button (const gchar *file,
                                    const gchar *text,
                                    const gchar *tooltip);

GtkWidget *gpredict_hstock_button  (const gchar *stock_id,
                                    const gchar *text,
                                    const gchar *tooltip);

GtkWidget *gpredict_mini_mod_button (const gchar *pixmapfile,
                                              const gchar *tooltip);

void gpredict_set_combo_tooltips (GtkWidget *combo, gpointer text);

gint       gpredict_file_copy      (const gchar *in,
                                    const gchar *out);


void   rgb2gdk   (guint rgb, GdkColor *color);
void   rgba2gdk  (guint rgba, GdkColor *color, guint16 *alpha);
void   gdk2rgb   (const GdkColor *color, guint *rgb);
void   gdk2rgba  (const GdkColor *color, guint16 alpha, guint *rgba);
gchar *rgba2html (guint rgba);

#endif
