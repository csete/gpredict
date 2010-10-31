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
#include "gtk-sat-module.h"
#include "sat-pref-layout-utils.h"


/** \brief Check the correctness of a layout string.
  * \param layout The layout string as stored in the config files
  *        "type1;left1;right1;top1;bottom1;type2; ... "
  * \return LAYOUT_GOOD if the layout is valid, LAYOUT_BAD if the layout is invalid.
  *
  * A layout is invalid if it refers to a non-existent view type or if there is something wrong
  * with the coordinates (e.g. left > right).
  */
gboolean  sat_pref_layout_check (const gchar *layout)
{
    gchar   **buffv;
    guint     length, nviews;
    gint     *grid;
    guint     i;
    gboolean  error = FALSE;


    /* split layout string into an array of strings */
    buffv = g_strsplit (layout, ";", 0);
    length = g_strv_length (buffv);

    /* check correct number of entries */
    if ((length == 0) || (length % 5 != 0)) {
        error |= TRUE;
    }

    nviews = length / 5;
    grid = g_try_new0 (gint, length);

    /* convert string to array of integers */
    for (i = 0; i < length; i++) {
        grid[i] = (gint) g_ascii_strtoll (buffv[i], NULL, 0);
    }

    /* check each view record */
    for (i = 0; i < nviews; i++) {

        if ((grid[5*i] >= GTK_SAT_MOD_VIEW_NUM) ||   /* view type within range */
            (grid[5*i+1] >= grid[5*i+2]) ||          /* left < right */
            (grid[5*i+3] >= grid[5*i+4])) {          /* top < bottom */

            error |= TRUE;
        }

    }

    g_free (grid);
    g_strfreev (buffv);

    return (error ? LAYOUT_BAD : LAYOUT_GOOD);
}


/** \brief Generate a preview of the specified layout.
  * \param layout The layout string as stored in the config files
  *        "type1;left1;right1;top1;bottom1;type2; ... "
  * \return The preview of the layout or a label if invalid
  */
GtkWidget *sat_pref_layout_preview (const gchar *layout)
{
    GtkWidget *preview;

    if (sat_pref_layout_check (layout) == LAYOUT_BAD) {
        preview = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (preview), _("<b>Invalid</b>"));

        return preview;
    }

    preview = gtk_table_new (2, 2, TRUE);

    return preview;
}
