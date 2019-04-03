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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib.h>
#include <stdlib.h>
#include "compat.h"

/**
 * Get data directory.
 *
 * On linux it corresponds to the PACKAGE_DATA_DIR macro defined in build-config.h
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar          *get_data_dir()
{
    gchar          *dir = NULL;

#ifdef G_OS_UNIX
    char* data_dir = getenv("GPREDICT_DATA_DIR");
    dir = g_strconcat(data_dir ? data_dir : PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S, "data", NULL);
#else
#ifdef G_OS_WIN32
    gchar          *buff =
        g_win32_get_package_installation_directory_of_module(NULL);

    dir = g_strconcat(buff, G_DIR_SEPARATOR_S,
                      "share", G_DIR_SEPARATOR_S, "gpredict",
                      G_DIR_SEPARATOR_S, "data", NULL);
    g_free(buff);
#endif
#endif

    return dir;
}

/**
 * Get absolute file name of a data file.
 *
 * This function returns the absolute file name of a data file. It is intended to
 * be a one-line filename constructor.
 * The returned gchar * should be freed when no longer needed.
 */
gchar          *data_file_name(const gchar * data)
{
    gchar          *filename = NULL;
    gchar          *buff;

    buff = get_data_dir();
    filename = g_strconcat(buff, G_DIR_SEPARATOR_S, data, NULL);
    g_free(buff);

    return filename;
}

/**
 * Get maps directory.
 *
 * On linux it corresponds to the PACKAGE_DATA_DIR/pixmaps/maps
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar          *get_maps_dir()
{
    gchar          *dir = NULL;

#ifdef G_OS_UNIX
    dir = g_strconcat(PACKAGE_PIXMAPS_DIR, G_DIR_SEPARATOR_S, "maps", NULL);
#else
#ifdef G_OS_WIN32
    gchar          *buff =
        g_win32_get_package_installation_directory_of_module(NULL);

    dir = g_strconcat(buff, G_DIR_SEPARATOR_S, "share", G_DIR_SEPARATOR_S,
                      /* FIXME */
                      "gpredict", G_DIR_SEPARATOR_S, "pixmaps",
                      G_DIR_SEPARATOR_S, "maps", NULL);
    g_free(buff);
#endif
#endif

    return dir;
}

/**
 * Get absolute file name of a map file.
 *
 * This function returns the absolute file name of a map file. It is intended to
 * be a one-line filename constructor.
 * The returned gchar * should be freed when no longer needed.
 */
gchar          *map_file_name(const gchar * map)
{
    gchar          *filename = NULL;
    gchar          *buff;

    buff = get_maps_dir();
    filename = g_strconcat(buff, G_DIR_SEPARATOR_S, map, NULL);
    g_free(buff);

    return filename;
}

/**
 * Get logo directory.
 *
 * On linux it corresponds to the PACKAGE_DATA_DIR/pixmaps/logos
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar          *get_logo_dir()
{
    gchar          *dir = NULL;

#ifdef G_OS_UNIX
    dir = g_strconcat(PACKAGE_PIXMAPS_DIR, G_DIR_SEPARATOR_S, "logos", NULL);
#else
#ifdef G_OS_WIN32
    gchar          *buff =
        g_win32_get_package_installation_directory_of_module(NULL);

    dir = g_strconcat(buff, G_DIR_SEPARATOR_S,
                      "share", G_DIR_SEPARATOR_S,
                      "gpredict", G_DIR_SEPARATOR_S, "pixmaps",
                      G_DIR_SEPARATOR_S, "logos", NULL);
    g_free(buff);
#endif
#endif

    return dir;
}

/**
 * Get icon directory.
 *
 * On linux it corresponds to the PACKAGE_DATA_DIR/pixmaps/icons
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar          *get_icon_dir()
{
    gchar          *dir = NULL;

#ifdef G_OS_UNIX
    dir = g_strconcat(PACKAGE_PIXMAPS_DIR, G_DIR_SEPARATOR_S, "icons", NULL);
#else
#ifdef G_OS_WIN32
    gchar          *buff =
        g_win32_get_package_installation_directory_of_module(NULL);

    dir = g_strconcat(buff, G_DIR_SEPARATOR_S,
                      "share", G_DIR_SEPARATOR_S,
                      "gpredict", G_DIR_SEPARATOR_S, "pixmaps",
                      G_DIR_SEPARATOR_S, "icons", NULL);
    g_free(buff);
#endif
#endif

    return dir;
}

/**
 * Get absolute file name of a logo file.
 *
 * This function returns the absolute file name of a logo file. It is intended
 * to be a one-line filename constructor.
 * The returned gchar * should be freed when no longer needed.
 */
gchar          *logo_file_name(const gchar * logo)
{
    gchar          *filename = NULL;
    gchar          *buff;

    buff = get_logo_dir();
    filename = g_strconcat(buff, G_DIR_SEPARATOR_S, logo, NULL);
    g_free(buff);

    return filename;
}

/**
 * Get absolute file name of an icon file.
 *
 * This function returns the absolute file name of an icon file. It is intended to
 * be a one-line filename constructor.
 * The returned gchar * should be freed when no longer needed.
 */
gchar          *icon_file_name(const gchar * icon)
{
    gchar          *filename = NULL;
    gchar          *buff;

    buff = get_icon_dir();
    filename = g_strconcat(buff, G_DIR_SEPARATOR_S, icon, NULL);
    g_free(buff);

    return filename;
}

/**
 * Get the old user configuration directory.
 *
 * On linux it corresponds to $HOME/.gpredict2
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar          *get_old_conf_dir(void)
{
    gchar          *dir;

    dir = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".gpredict2", NULL);
    return dir;
}

/**
 * Get user configuration directory.
 *
 * Linux: $HOME/.config/Gpredict
 * Windows: C:\Users\username\Gpredict
 * Mac OS X: /home/username/Library/Application Support/Gpredict
 *
 * The function returns a newly allocated gchar * which must be free when
 * it is no longer needed.
 */
gchar          *get_user_conf_dir(void)
{
    gchar          *dir = NULL;

#ifdef G_OS_UNIX
    dir =
        g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S, "Gpredict",
                    NULL);
#endif
#ifdef G_OS_WIN32
    // FIXME: does this work?
    dir = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, "Gpredict", NULL);
#endif
/* see gtk-osx.sourceforge.net -> Integration */
#ifdef MAC_INTEGRATION
    dir = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S,
                      "Library", G_DIR_SEPARATOR_S,
                      "Application Support", G_DIR_SEPARATOR_S, "Gpredict",
                      NULL);
#endif

    return dir;
}

/** Get USER_CONF_DIR/modules */
gchar          *get_modules_dir(void)
{
    gchar          *confdir;
    gchar          *dir;

    confdir = get_user_conf_dir();
    dir = g_strconcat(confdir, G_DIR_SEPARATOR_S, "modules", NULL);
    g_free(confdir);

    return dir;
}

/** Get USER_CONF_DIR/satdata */
gchar          *get_satdata_dir(void)
{
    gchar          *confdir;
    gchar          *dir;

    confdir = get_user_conf_dir();
    dir = g_strconcat(confdir, G_DIR_SEPARATOR_S, "satdata", NULL);
    g_free(confdir);

    return dir;
}

/** Get USER_CONF_DIR/trsp */
gchar          *get_trsp_dir(void)
{
    gchar          *confdir;
    gchar          *dir;

    confdir = get_user_conf_dir();
    dir = g_strconcat(confdir, G_DIR_SEPARATOR_S, "trsp", NULL);
    g_free(confdir);

    return dir;
}

/** Get USER_CONF_DIR/hwconf */
gchar          *get_hwconf_dir(void)
{
    gchar          *confdir;
    gchar          *dir;

    confdir = get_user_conf_dir();
    dir = g_strconcat(confdir, G_DIR_SEPARATOR_S, "hwconf", NULL);
    g_free(confdir);

    return dir;
}

/** Get full path of a .sat or .cat file */
gchar          *sat_file_name(const gchar * satfile)
{
    gchar          *filename = NULL;
    gchar          *buff;

    buff = get_satdata_dir();
    filename = g_strconcat(buff, G_DIR_SEPARATOR_S, satfile, NULL);
    g_free(buff);

    return filename;
}

/** Build satellite file path from catnum (integer) */
gchar          *sat_file_name_from_catnum(guint catnum)
{
    gchar          *filename;
    gchar          *buff;
    gchar          *dir;

    buff = g_strdup_printf("%d.sat", catnum);
    dir = get_satdata_dir();

    filename = g_strconcat(dir, G_DIR_SEPARATOR_S, buff, NULL);

    g_free(buff);
    g_free(dir);

    return filename;
}

/** Build satellite file path from catnum (string) */
gchar          *sat_file_name_from_catnum_s(gchar * catnum)
{
    gchar          *filename;
    gchar          *buff;
    gchar          *dir;

    buff = g_strdup_printf("%s.sat", catnum);
    dir = get_satdata_dir();

    filename = g_strconcat(dir, G_DIR_SEPARATOR_S, buff, NULL);

    g_free(buff);
    g_free(dir);

    return filename;
}

/** Get full path of a .trsp file */
gchar          *trsp_file_name(const gchar * trspfile)
{
    gchar          *filename = NULL;
    gchar          *buff;

    buff = get_trsp_dir();
    filename = g_strconcat(buff, G_DIR_SEPARATOR_S, trspfile, NULL);
    g_free(buff);

    return filename;
}

/** Get full path of a .rig or .rot file */
gchar          *hw_file_name(const gchar * hwfile)
{
    gchar          *filename = NULL;
    gchar          *buff;

    buff = get_hwconf_dir();
    filename = g_strconcat(buff, G_DIR_SEPARATOR_S, hwfile, NULL);
    g_free(buff);

    return filename;
}
