/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

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
/** \brief Utilities to ensure compatibility across multiple platforms.
 */

#include <glib.h>
#include "compat.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif



/** \brief Get data directory.
 *
 * On linux it corresponds to the PACKAGE_DATA_DIR macro defined in build-config.h
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar *
get_data_dir   ()
{
        gchar *dir = NULL;


#ifdef G_OS_UNIX
        dir = g_strconcat (PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S, "data", NULL);
#else
#  ifdef G_OS_WIN32
        gchar *buff = g_win32_get_package_installation_directory (NULL, NULL);
        dir = g_strconcat (buff, G_DIR_SEPARATOR_S,
                           "share", G_DIR_SEPARATOR_S,
                           "gpredict", G_DIR_SEPARATOR_S,
                           "data", NULL);
        g_free (buff);
#  endif
#endif

        return dir;
}


/** \brief Get absolute file name of a data file.
 *
 * This function returns the absolute file name of a data file. It is intended to
 * be a one-line filename constructor.
 * The returned gchar * should be freed when no longer needed.
 */
gchar *
data_file_name (const gchar *data)
{
        gchar *filename = NULL;
        gchar *buff;

        buff = get_data_dir ();
        filename = g_strconcat (buff, G_DIR_SEPARATOR_S, data, NULL);
        g_free (buff);

        return filename;
}



/** \brief Get maps directory.
 *
 * On linux it corresponds to the PACKAGE_DATA_DIR/pixmaps/maps
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar *
get_maps_dir   ()
{
        gchar *dir = NULL;


#ifdef G_OS_UNIX
        dir = g_strconcat (PACKAGE_PIXMAPS_DIR, G_DIR_SEPARATOR_S,
                           "maps", NULL);
#else
#  ifdef G_OS_WIN32
        gchar *buff = g_win32_get_package_installation_directory (NULL, NULL);
        dir = g_strconcat (buff, G_DIR_SEPARATOR_S,
                           "share", G_DIR_SEPARATOR_S,
			   /* FIXME */
                           "gpredict", G_DIR_SEPARATOR_S,
                           "pixmaps", G_DIR_SEPARATOR_S,
                           "maps", NULL);
        g_free (buff);
#  endif
#endif

        return dir;
}


/** \brief Get absolute file name of a map file.
 *
 * This function returns the absolute file name of a map file. It is intended to
 * be a one-line filename constructor.
 * The returned gchar * should be freed when no longer needed.
 */
gchar *
map_file_name (const gchar *map)
{
        gchar *filename = NULL;
        gchar *buff;

        buff = get_maps_dir ();
        filename = g_strconcat (buff, G_DIR_SEPARATOR_S, map, NULL);
        g_free (buff);

        return filename;
}



/** \brief Get icon directory.
 *
 * On linux it corresponds to the PACKAGE_DATA_DIR/pixmaps/icons
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar *
get_icon_dir   ()
{
        gchar *dir = NULL;


#ifdef G_OS_UNIX
        dir = g_strconcat (PACKAGE_PIXMAPS_DIR, G_DIR_SEPARATOR_S,
                           "icons", NULL);
#else
#  ifdef G_OS_WIN32
        gchar *buff = g_win32_get_package_installation_directory (NULL, NULL);
        dir = g_strconcat (buff, G_DIR_SEPARATOR_S,
                           "share", G_DIR_SEPARATOR_S,
                           "gpredict", G_DIR_SEPARATOR_S,
                           "pixmaps", G_DIR_SEPARATOR_S,
                           "icons", NULL);
        g_free (buff);
#  endif
#endif

        return dir;
}


/** \brief Get absolute file name of an icon file.
 *
 * This function returns the absolute file name of an icon file. It is intended to
 * be a one-line filename constructor.
 * The returned gchar * should be freed when no longer needed.
 */
gchar *
icon_file_name (const gchar *icon)
{
        gchar *filename = NULL;
        gchar *buff;

        buff = get_icon_dir ();
        filename = g_strconcat (buff, G_DIR_SEPARATOR_S, icon, NULL);
        g_free (buff);

        return filename;
}


/** \brief Get user configuration directory.
 *
 * On linux it corresponds to $HOME/.gpredict2
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */gchar *get_conf_dir   (void)
{
    gchar *dir;
    
    dir = g_strconcat (g_get_home_dir(), G_DIR_SEPARATOR_S, ".gpredict2", NULL);
    return dir;
}
