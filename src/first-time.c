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
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include "compat.h"
#include "sat-log.h"
#include "sat-cfg.h"
#include "gpredict-utils.h"
#include "first-time.h"

/* private function prototypes */
static void     first_time_check_step_01(guint * error);
static void     first_time_check_step_02(guint * error);
static void     first_time_check_step_03(guint * error);
static void     first_time_check_step_04(guint * error);
static void     first_time_check_step_05(guint * error);
static void     first_time_check_step_06(guint * error);
static void     first_time_check_step_07(guint * error);
static void     first_time_check_step_08(guint * error);

/**
 * Perform first time checks.
 *
 * This function is called by the main function very early during program
 * startup. It's purpose is to check the user configuration to see whether
 * this is the first time gpredict is executed. If it is, a new default
 * configuration is set up so that the user has some sort of setup to get
 * started with.
 *
 * Check logic:
 *
 * 0. USER_CONF_DIR already exists because sat_log_init() initializes it.
 *
 * 1. Check for USER_CONF_DIR/gpredict.cfg - if not found, check if there is a
 *    gpredict.cfg in the old configuration directory and copy it to the new location.
 * 2. Check for the existence of at least one .qth file in USER_CONF_DIR
 *    If no such file found, check if there are any in the pre-1.1 configuration.
 *    If still none, copy PACKAGE_DATA_DIR/data/sample.qth to this
 *    directory.
 * 3. Check for the existence of USER_CONF_DIR/modules directory and create
 *    it if it does not exist. Moreover, if this is a new installation, check
 *    for .mod files in pre-1.1 directory (use get_old_conf_dir()). If no .mod
 *    files are available copy PACKAGE_DATA_DIR/data/Amateur.mod to
 *    USER_CONF_DIR/modules/
 * 4. Check for the existence of USER_CONF_DIR/satdata directory and create it if
 *    it does not exist.
 * 5. Check if there are any .sat files in USER_CONF_DIR/satdata/ - if not extract
 *    PACKAGE_DATA_DIR/data/satdata/satellites.dat to .sat files.
 *    Do the same with .cat files.
 * 6. Check for the existence of USER_CONF_DIR/satdata/cache directory. This
 *    directory is used to store temporary TLE files when updating from
 *    network.
 * 7. Check for the existence of USER_CONF_DIR/hwconf directory. This
 *    directory contains radio and rotator configurations (.rig and .rot files).
 *    If the directory is newly created, check if we have any existing configuration
 *    in the pre-1.1 configuration directory (use get_old_conf_dir()).
 * 8. Check for the existence of USER_CONF_DIR/trsp directory. This
 *    directory contains transponder data for satellites.
 *
 * Send both error, warning and verbose debug messages to sat-log during this
 * process.
 *
 * The function returns 0 if everything seems to be ready or 1 if an error occurred
 * during on of the steps above. In case of error, the only safe thing is to exit
 * immediately.
 *
 * FIXME: Should only have one parameterized function for checking directories.
 */
guint first_time_check_run()
{
    guint           error = 0;

    first_time_check_step_01(&error);
    first_time_check_step_02(&error);
    first_time_check_step_03(&error);
    first_time_check_step_04(&error);
    first_time_check_step_05(&error);
    first_time_check_step_06(&error);
    first_time_check_step_07(&error);
    first_time_check_step_08(&error);

    return error;
}

/**
 * Execute step 1 of the first time checks.
 *
 * 1. Check for USER_CONF_DIR/gpredict.cfg - if not found, check if there is a
 *    gpredict.cfg in the old configuration directory and copy it to the new location.
 *
 */
static void first_time_check_step_01(guint * error)
{
    gchar          *newdir, *olddir;
    gchar          *source, *target;

    /* FIXME */
    (void)error;                /* avoid unused parameter compiler warning */

    newdir = get_user_conf_dir();
    target = g_strconcat(newdir, G_DIR_SEPARATOR_S, "gpredict.cfg", NULL);
    g_free(newdir);

    if (g_file_test(target, G_FILE_TEST_EXISTS))
    {
        /* already have config file => return */
        g_free(target);

        return;
    }

    /* check if we have old configuration */
    olddir = get_old_conf_dir();
    source = g_strconcat(olddir, G_DIR_SEPARATOR_S, "gpredict.cfg", NULL);
    g_free(olddir);

    if (g_file_test(source, G_FILE_TEST_EXISTS))
    {
        /* copy old config file to new location */
        gpredict_file_copy(source, target);

    }

    g_free(source);
    g_free(target);
}

/**
 * Execute step 2 of the first time checks.
 *
 * 2. Check for the existence of at least one .qth file in USER_CONF_DIR
 *    If no such file found, check if there are any in the pre-1.1 configuration.
 *    If still none, copy PACKAGE_DATA_DIR/data/sample.qth to this
 *    directory.
 *
 */
static void first_time_check_step_02(guint * error)
{
    GDir           *dir;
    gchar          *dirname;
    gchar          *filename;
    const gchar    *datafile;
    gchar          *target;
    gboolean        foundqth = FALSE;

    dirname = get_user_conf_dir();
    dir = g_dir_open(dirname, 0, NULL);

    /* directory does not exist, something went wrong in step 1 */
    if (!dir)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not open %s."), __func__, dirname);

        /* no reason to continue */
        *error |= FTC_ERROR_STEP_02;
    }
    else
    {
        /* read files, if any; count number of .qth files */
        while ((datafile = g_dir_read_name(dir)))
        {

            /* note: filename is not a newly allocated gchar *,
               so we must not free it
             */

            if (g_str_has_suffix(datafile, ".qth"))
                foundqth = TRUE;
        }

        g_dir_close(dir);

        if (foundqth)
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s: Found at least one .qth file."), __func__);
        else
        {
            /* try to see if there are any .qth file in pre-1.1 configuration */
            gchar          *olddir = get_old_conf_dir();

            dir = g_dir_open(olddir, 0, NULL);

            if (dir)
            {
                /* read files, if any; count number of .qth files */
                while ((datafile = g_dir_read_name(dir)))
                {

                    /* note: filename is not a newly allocated gchar *,
                       so we must not free it
                     */
                    if (g_str_has_suffix(datafile, ".qth"))
                    {

                        gchar          *source =
                            g_strconcat(olddir, G_DIR_SEPARATOR_S, datafile,
                                        NULL);

                        /* copy .qth file to USER_CONF_DIR */
                        target =
                            g_strconcat(dirname, G_DIR_SEPARATOR_S, datafile,
                                        NULL);
                        if (!gpredict_file_copy(source, target))
                        {
                            /* success */
                            foundqth = TRUE;
                        }
                        g_free(target);
                        g_free(source);
                    }

                }
                g_dir_close(dir);
            }
            else if (!foundqth)
            {
                /* try to copy sample.qth */
                filename = data_file_name("sample.qth");
                target =
                    g_strconcat(dirname, G_DIR_SEPARATOR_S, "sample.qth",
                                NULL);

                if (gpredict_file_copy(filename, target))
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Failed to copy sample.qth"), __func__);

                    *error |= FTC_ERROR_STEP_02;
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                _("%s: Copied sample.qth to %s/"),
                                __func__, dirname);
                }
                g_free(target);
                g_free(filename);
            }
            g_free(olddir);
        }
    }
    g_free(dirname);
}

/**
 * Execute step 3 of the first time checks.
 *
 * 3. Check for the existence of USER_CONF_DIR/modules directory and create
 *    it if it does not exist. Moreover, if this is a new installation, check
 *    for .mod files in pre-1.1 directory (use get_old_conf_dir()). If no .mod
 *    files are available copy PACKAGE_DATA_DIR/data/Amateur.mod to
 *    USER_CONF_DIR/modules/
 *
 */
static void first_time_check_step_03(guint * error)
{
    GDir           *dir;
    gchar          *confdir, *olddir, *buff;
    int             status;
    gchar          *target;
    gchar          *filename;
    const gchar    *datafile;
    gboolean        foundmod = FALSE;


    confdir = get_modules_dir();

    if (g_file_test(confdir, G_FILE_TEST_IS_DIR))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Check successful."), __func__);
    }
    else
    {
        /* try to create directory */
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Check failed. Creating %s"), __func__, confdir);

        status = g_mkdir_with_parents(confdir, 0755);

        if (status)
        {
            /* set error flag */
            *error |= FTC_ERROR_STEP_03;

            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Failed to create %s"), __func__, confdir);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s: Created %s."), __func__, confdir);

            /* try to see if there are any .mod file in pre-1.1 configuration */
            buff = get_old_conf_dir();
            olddir = g_strconcat(buff, G_DIR_SEPARATOR_S, "modules", NULL);
            dir = g_dir_open(olddir, 0, NULL);

            g_free(buff);

            if (dir)
            {
                /* read files, if any; count number of .qth files */
                while ((datafile = g_dir_read_name(dir)))
                {

                    /* note: filename is not a newly allocated gchar *,
                       so we must not free it
                     */
                    if (g_str_has_suffix(datafile, ".mod"))
                    {
                        gchar          *source =
                            g_strconcat(olddir, G_DIR_SEPARATOR_S, datafile,
                                        NULL);

                        /* copy .qth file to USER_CONF_DIR */
                        target =
                            g_strconcat(confdir, G_DIR_SEPARATOR_S, datafile,
                                        NULL);
                        if (!gpredict_file_copy(source, target))
                        {
                            /* success */
                            foundmod = TRUE;
                        }
                        g_free(target);
                        g_free(source);
                    }

                }
                g_dir_close(dir);
            }
            else if (!foundmod)
            {
                /* copy Amateur.mod to this directory */
                filename = data_file_name("Amateur.mod");
                target =
                    g_strconcat(confdir, G_DIR_SEPARATOR_S, "Amateur.mod",
                                NULL);

                if (gpredict_file_copy(filename, target))
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Failed to copy Amateur.mod"), __func__);

                    *error |= FTC_ERROR_STEP_02;
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                _("%s: Copied amateur.mod to %s/"),
                                __func__, dir);
                }
                g_free(target);
                g_free(filename);
            }
            g_free(olddir);
        }
    }
    g_free(confdir);
}

/**
 * Execute step 4 of the first time checks.
 *
 * 4. Check for the existence of USER_CONF_DIR/satdata directory and create it if
 *    it does not exist.
 *
 */
static void first_time_check_step_04(guint * error)
{
    gchar          *dir;
    int             status;

    dir = get_satdata_dir();

    if (g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Check successful."), __func__);
    }
    else
    {
        /* try to create directory */
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Check failed. Creating %s"), __func__, dir);

        status = g_mkdir_with_parents(dir, 0755);

        if (status)
        {
            /* set error flag */
            *error |= FTC_ERROR_STEP_04;

            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Failed to create %s"),
                        __func__, dir);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s: Created %s."), __func__, dir);
        }
    }
    g_free(dir);
}

/* create .sat files from a satellites.dat file */
static void create_sat_files(guint * error)
{
    gchar          *satfilename, *targetfilename;
    gchar          *datadir;
    gchar         **satellites;
    GKeyFile       *satfile, *target;
    gsize           num;
    GError         *err = NULL;
    guint           i;
    guint           newsats = 0;
    gchar          *name, *nickname, *website, *tle1, *tle2, *cfgver;

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("Copying satellite data to user config"));

    /* open datellites.dat and load into memory */
    datadir = get_data_dir();
    satfilename = g_strconcat(datadir, G_DIR_SEPARATOR_S, "satdata",
                              G_DIR_SEPARATOR_S, "satellites.dat", NULL);

    satfile = g_key_file_new();
    if (!g_key_file_load_from_file
        (satfile, satfilename, G_KEY_FILE_KEEP_COMMENTS, &err))
    {
        /* an error occurred */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to load data from %s (%s)"),
                    __func__, satfilename, err->message);

        g_clear_error(&err);
        *error |= FTC_ERROR_STEP_05;
    }
    else
    {
        satellites = g_key_file_get_groups(satfile, &num);
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Found %d satellites in %s"),
                    __func__, num, satfilename);

        for (i = 0; i < num; i++)
        {
            /* first, check if this satellite already has a .sat file */
            targetfilename = sat_file_name_from_catnum_s(satellites[i]);
            if (g_file_test(targetfilename, G_FILE_TEST_EXISTS))
            {
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s: %s.sat already exists. Skipped."),
                            __func__, satellites[i]);
            }
            else
            {
                /* read data for this satellite */
                cfgver =
                    g_key_file_get_string(satfile, satellites[i], "VERSION",
                                          NULL);
                name =
                    g_key_file_get_string(satfile, satellites[i], "NAME",
                                          NULL);
                nickname =
                    g_key_file_get_string(satfile, satellites[i], "NICKNAME",
                                          NULL);
                website =
                    g_key_file_get_string(satfile, satellites[i], "WEBSITE",
                                          NULL);
                tle1 =
                    g_key_file_get_string(satfile, satellites[i], "TLE1",
                                          NULL);
                tle2 =
                    g_key_file_get_string(satfile, satellites[i], "TLE2",
                                          NULL);

                /* create output .sat file */
                target = g_key_file_new();
                g_key_file_set_string(target, "Satellite", "VERSION", cfgver);
                g_key_file_set_string(target, "Satellite", "NAME", name);
                g_key_file_set_string(target, "Satellite", "NICKNAME",
                                      nickname);
                if (website != NULL)
                {
                    g_key_file_set_string(target, "Satellite", "WEBSITE",
                                          website);
                    g_free(website);
                }
                g_key_file_set_string(target, "Satellite", "TLE1", tle1);
                g_key_file_set_string(target, "Satellite", "TLE2", tle2);

                if (gpredict_save_key_file(target, targetfilename))
                {
                    *error |= FTC_ERROR_STEP_05;
                }
                else
                {
                    newsats++;
                }

                g_key_file_free(target);

                g_free(cfgver);
                g_free(name);
                g_free(nickname);
                g_free(tle1);
                g_free(tle2);
            }
            g_free(targetfilename);
        }
        g_strfreev(satellites);
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Written %d new satellite to user config"),
                    __func__, newsats);
    }
    g_key_file_free(satfile);
    g_free(satfilename);
    g_free(datadir);
}

/* create .cat files in user conf directory */
static void create_cat_files(guint * error)
{
    gchar          *datadir;
    GError         *err = NULL;
    GDir           *srcdir;
    gchar          *srcdirname;
    const gchar    *filename;

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("Copying satellite categories to user config"));

    /* .cat files: if .cat file does not exist, copy it, otherwise skip */
    datadir = get_data_dir();
    srcdirname = g_strconcat(datadir, G_DIR_SEPARATOR_S, "satdata", NULL);
    srcdir = g_dir_open(srcdirname, 0, &err);

    /* directory does not exist, something went wrong in step 4 */
    if (!srcdir)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not open %s (%s)."),
                    __func__, srcdirname, err->message);

        /* no reason to continue */
        g_clear_error(&err);
        *error |= FTC_ERROR_STEP_05;
    }
    else
    {
        /* get each .cat file and check if they already exist in user conf */
        /* read files one by one, if any; count number of .tle files */
        while ((filename = g_dir_read_name(srcdir)))
        {
            /* note: filename is not a newly allocated gchar *,
               so we must not free it
             */
            if (g_str_has_suffix(filename, ".cat"))
            {

                /* check whether .cat file exists in user conf */
                gchar          *catfilename = sat_file_name(filename);

                if (!g_file_test(catfilename, G_FILE_TEST_EXISTS))
                {
                    /* copy file to target dir */
                    gchar          *source =
                        g_strconcat(srcdirname, G_DIR_SEPARATOR_S, filename,
                                    NULL);
                    if (gpredict_file_copy(source, catfilename))
                    {
                        sat_log_log(SAT_LOG_LEVEL_ERROR,
                                    _("%s: Failed to copy %s"),
                                    __func__, filename);
                    }
                    else
                    {
                        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                    _("%s: Successfully copied %s"),
                                    __func__, filename);
                    }
                    g_free(source);
                }
                g_free(catfilename);
            }
        }
        g_dir_close(srcdir);
    }
    g_free(srcdirname);
    g_free(datadir);
}

/**
 * Execute step 5 of the first time checks.
 *
 * 5. Check if there are any .sat files in USER_CONF_DIR/satdata/ - if not extract
 *    PACKAGE_DATA_DIR/data/satdata/satellites.dat to .sat files.
 *    Do the same with .cat files.
 *
 */
static void first_time_check_step_05(guint * error)
{
    gchar          *datadir_str;
    GDir           *datadir;
    const gchar    *filename;
    gboolean        have_sat = FALSE;
    gboolean        have_cat = FALSE;

    /* check if there already is a .sat and .cat in ~/.config/... */
    datadir_str = get_satdata_dir();
    datadir = g_dir_open(datadir_str, 0, NULL);
    while ((filename = g_dir_read_name(datadir)))
    {
        /* note: filename is not newly allocated */
        if (g_str_has_suffix(filename, ".sat"))
            have_sat = TRUE;
        if (g_str_has_suffix(filename, ".cat"))
            have_cat = TRUE;
    }
    g_free(datadir_str);
    g_dir_close(datadir);

    if (!have_sat)
        create_sat_files(error);

    if (!have_cat)
        create_cat_files(error);
}

/**
 * Execute step 6 of the first time checks.
 *
 * 6. Check for the existence of USER_CONF_DIR/satdata/cache directory. This
 *    directory is used to store temporary TLE files when updating from
 *    network.
 *
 */
static void first_time_check_step_06(guint * error)
{
    gchar          *buff, *dir;
    int             status;

    buff = get_satdata_dir();
    dir = g_strconcat(buff, G_DIR_SEPARATOR_S, "cache", NULL);
    g_free(buff);

    if (g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Check successful."), __func__);
    }
    else
    {
        /* try to create directory */
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Check failed. Creating %s"), __func__, dir);

        status = g_mkdir_with_parents(dir, 0755);

        if (status)
        {
            /* set error flag */
            *error |= FTC_ERROR_STEP_06;

            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Failed to create %s"),
                        __func__, dir);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s: Created %s."), __func__, dir);
        }
    }
    g_free(dir);
}

/**
 * Execute step 7 of the first time checks.
 *
 * 7. Check for the existence of USER_CONF_DIR/hwconf directory. This
 *    directory contains radio and rotator configurations (.rig and .rot files).
 *    If the directory is newly created, check if we have any existing configuration
 *    in the pre-1.1 configuration directory (use get_old_conf_dir()).
 *
 */
static void first_time_check_step_07(guint * error)
{
    GDir           *dir;
    gchar          *confdir, *olddir, *buff;
    int             status;
    gchar          *target;
    const gchar    *datafile;

    confdir = get_hwconf_dir();

    if (g_file_test(confdir, G_FILE_TEST_IS_DIR))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Check successful."), __func__);
    }
    else
    {
        /* try to create directory */
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Check failed. Creating %s"), __func__, confdir);

        status = g_mkdir_with_parents(confdir, 0755);

        if (status)
        {
            /* set error flag */
            *error |= FTC_ERROR_STEP_03;
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Failed to create %s"), __func__, confdir);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s: Created %s."), __func__, confdir);

            /* try to see if there are any .rig or .rot file in pre-1.1 configuration */
            buff = get_old_conf_dir();
            olddir = g_strconcat(buff, G_DIR_SEPARATOR_S, "hwconf", NULL);

            dir = g_dir_open(olddir, 0, NULL);

            g_free(buff);

            if (dir)
            {
                /* read files, if any; count number of .qth files */
                while ((datafile = g_dir_read_name(dir)))
                {
                    gchar          *source =
                        g_strconcat(olddir, G_DIR_SEPARATOR_S, datafile, NULL);

                    /* note: filename is not a newly allocated gchar *,
                       so we must not free it
                     */

                    /* copy file to USER_CONF_DIR */
                    target = g_strconcat(confdir, G_DIR_SEPARATOR_S,
                                    datafile, NULL);
                    gpredict_file_copy(source, target);
                    g_free(target);
                    g_free(source);
                }
                g_dir_close(dir);
            }
            g_free(olddir);
        }
    }
    g_free(confdir);

}

/**
 * Execute step 8 of the first time checks.
 *
 * 8. Check for the existence of USER_CONF_DIR/trsp directory. This
 *    directory contains transponder data for satellites.
 *
 */
static void first_time_check_step_08(guint * error)
{
    gchar          *dir;
    int             status;

    dir = get_trsp_dir();

    if (g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Check successful."), __func__);
    }
    else
    {
        /* try to create directory */
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Check failed. Creating %s"), __func__, dir);

        status = g_mkdir_with_parents(dir, 0755);

        if (status)
        {
            /* set error flag */
            *error |= FTC_ERROR_STEP_08;
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Failed to create %s"),
                        __func__, dir);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _("%s: Created %s."), __func__, dir);
        }
    }

    g_free(dir);
}
