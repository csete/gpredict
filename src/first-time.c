/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2008  Alexandru Csete, OZ9AEC.

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
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "compat.h"
#include "sat-log.h"
#include "sat-cfg.h"
#include "gpredict-utils.h"
#include "first-time.h"



/* private function prototypes */
static void first_time_check_step_01 (guint *error);
static void first_time_check_step_02 (guint *error);
static void first_time_check_step_03 (guint *error);
static void first_time_check_step_04 (guint *error);
static void first_time_check_step_05 (guint *error);
static void first_time_check_step_06 (guint *error);
static void first_time_check_step_07 (guint *error);
static void first_time_check_step_08 (guint *error);
static void first_time_check_step_09 (guint *error);



/** \brief Perform first time checks.
 *
 * This function is called by the main function very early during program
 * startup. It's purpose is to check the user configuration to see whether
 * this is the first time gpredict is executed. If it is, a new default
 * configuration is set up so that the user has some sort of setup to get
 * started with.
 *
 * Check logic:
 *
 * 1. Check for USER_CONF_DIR (use get_user_conf_dir()) and create dir if it
 *    does not exist. If we create the directory, check if there is a
 *    gpredict.cfg in the old configuration directory.
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
 * 5. Copy PACKAGE_DATA_DIR/data/satdata/xxx.sat to USER_CONF_DIR/satdata/ if it
 *    does not already exist. Do the same with .cat files.
 * 6. Check for the existence of USER_CONF_DIR/satdata/cache directory. This
 *    directory is used to store temporary TLE files when updating from
 *    network.
 * 7. Check for the existence of USER_CONF_DIR/hwconf directory. This
 *    directory contains radio and rotator configurations (.rig and .rot files).
 *    If the directory is newly created, check if we have any existing configuration
 *    in the pre-1.1 configuration directory (use get_old_conf_dir()).
 * 8. Check for the existence of USER_CONF_DIR/trsp directory. This
 *    directory contains transponder data for satellites.
 * 9. Check the .trsp files in USER_CONF_DIR/trsp/ and compare to the ones
 *    available in PACKAGE_DATA_DIR/data/trsp/xxx.trsp, and update if necessary.
 *
 * Send both error, warning and verbose debug messages to sat-log during this
 * process.
 *
 * The function returns 0 if everything seems to be ready or 1 if an error occured
 * during on of the steps above. In case of error, the only safe thing is to exit
 * imediately.
 *
 * FIXME: Should only have one parameterized function for checking directories.
 */
guint
first_time_check_run ()
{
    guint error = 0;

    first_time_check_step_01 (&error);
    first_time_check_step_02 (&error);
    first_time_check_step_03 (&error);
    first_time_check_step_04 (&error);
    first_time_check_step_05 (&error);
    first_time_check_step_06 (&error);
    first_time_check_step_07 (&error);
    first_time_check_step_08 (&error);
    first_time_check_step_09 (&error);

    return error;
}



/** \brief Execute step 1 of the first time checks.
 *
 * 1. Check for USER_CONF_DIR (use get_user_conf_dir()) and create dir if it
 *    does not exist. If we create the directory, check if there is a
 *    gpredict.cfg in the old configuration directory.
 *
 */
static void
first_time_check_step_01 (guint *error)
{
    gchar *dir;
    gchar *cfgfile;
    int    status;

    dir = get_user_conf_dir ();

    if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                        _("%s: Check successful."), __FUNCTION__);
    }
    else {
        /* try to create directory */
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                        _("%s: Check failed. Creating %s"),
                        __FUNCTION__,
                        dir);

        status = g_mkdir_with_parents (dir, 0755);

        if (status) {
            /* set error flag */
            *error |= FTC_ERROR_STEP_01;

            sat_log_log (SAT_LOG_LEVEL_ERROR,
                            _("%s: Failed to create %s"),
                            __FUNCTION__, dir );
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_DEBUG,
                            _("%s: Created %s."),
                            __FUNCTION__, dir);

            /* now check if there is a gpredict.cfg in the pre-1.1 config
               folder. */
            cfgfile = g_strconcat (dir, G_DIR_SEPARATOR_S, "gpredict.cfg", NULL);
            if (g_file_test (cfgfile, G_FILE_TEST_EXISTS)) {
                /* copy cfg file to USER_CONF_DIR */
                gchar *target = g_strconcat (dir, G_DIR_SEPARATOR_S, "gpredict.cfg", NULL);
                gpredict_file_copy (cfgfile, target); /* FIXME: Error ignored */
                g_free (target);
            }
            g_free (cfgfile);
        }
    }

    g_free (dir);
}



/** \brief Execute step 2 of the first time checks.
 *
 * 2. Check for the existence of at least one .qth file in USER_CONF_DIR
 *    If no such file found, check if there are any in the pre-1.1 configuration.
 *    If still none, copy PACKAGE_DATA_DIR/data/sample.qth to this
 *    directory.
 *
 */
static void
first_time_check_step_02 (guint *error)
{
    GDir        *dir;
    gchar       *dirname;
    gchar       *filename;
    const gchar *datafile;
    gchar       *target;
    gboolean     foundqth = FALSE;


    dirname = get_user_conf_dir ();

    dir = g_dir_open (dirname, 0, NULL);

    /* directory does not exist, something went wrong in step 1 */
    if (!dir) {
        sat_log_log (SAT_LOG_LEVEL_ERROR, 
                        _("%s: Could not open %s."),
                        __FUNCTION__, dirname);
        
        /* no reason to continue */
        *error |= FTC_ERROR_STEP_02;
    }
    else {
        /* read files, if any; count number of .qth files */
        while ((datafile = g_dir_read_name (dir))) {

            /* note: filename is not a newly allocated gchar *,
                so we must not free it
            */

            if (g_strrstr (datafile, ".qth")) {
                foundqth = TRUE;
            }

        }

        g_dir_close (dir);

        if (foundqth) {
            sat_log_log (SAT_LOG_LEVEL_DEBUG,
                            _("%s: Found at least one .qth file."),
                            __FUNCTION__);
        }
        else {
            /* try to see if there are any .qth file in pre-1.1 configuration */
            gchar *olddir = get_old_conf_dir ();

            dir = g_dir_open (olddir, 0, NULL);

            if (dir) {
                /* read files, if any; count number of .qth files */
                while ((datafile = g_dir_read_name (dir))) {

                    /* note: filename is not a newly allocated gchar *,
                       so we must not free it
                    */
                    if (g_strrstr (datafile, ".qth")) {

                        /* copy .qth file to USER_CONF_DIR */
                        target = g_strconcat (dirname, G_DIR_SEPARATOR_S, datafile, NULL);
                        if (!gpredict_file_copy (datafile, target)) {
                            /* success */
                            foundqth = TRUE;
                        }
                        g_free (target);
                    }

                }

                g_dir_close (dir);
            }
            else if (!foundqth) {
                /* try to copy sample.qth */
                filename = data_file_name ("sample.qth");
                target = g_strconcat (dirname, G_DIR_SEPARATOR_S, "sample.qth", NULL);

                if (gpredict_file_copy (filename, target)) {
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                _("%s: Failed to copy sample.qth"),
                                __FUNCTION__);

                    *error |= FTC_ERROR_STEP_02;
                }
                else {
                    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                                _("%s: Copied sample.qth to %s/"),
                                __FUNCTION__, dirname);
                }

                g_free (target);
                g_free (filename);
            }

        }
    }

    g_free (dirname);
}



/** \brief Execute step 3 of the first time checks.
 *
 * 3. Check for the existence of USER_CONF_DIR/modules directory and create
 *    it if it does not exist. Moreover, if this is a new installation, check
 *    for .mod files in pre-1.1 directory (use get_old_conf_dir()). If no .mod
 *    files are available copy PACKAGE_DATA_DIR/data/Amateur.mod to
 *    USER_CONF_DIR/modules/
 *
 */
static void
first_time_check_step_03 (guint *error)
{
    GDir   *dir;
    gchar  *confdir,*olddir,*buff;
    int     status;
    gchar  *target;
    gchar  *filename;
    const gchar *datafile;
    gboolean foundmod = FALSE;


    confdir = get_modules_dir ();

    if (g_file_test (confdir, G_FILE_TEST_IS_DIR)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                        _("%s: Check successful."),
                        __FUNCTION__);
    }
    else {
        /* try to create directory */
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                        _("%s: Check failed. Creating %s"),
                        __FUNCTION__,
                        confdir);

        status = g_mkdir_with_parents (confdir, 0755);

        if (status) {
            /* set error flag */
            *error |= FTC_ERROR_STEP_03;

            sat_log_log (SAT_LOG_LEVEL_ERROR, 
                            _("%s: Failed to create %s"),
                            __FUNCTION__, confdir);
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_DEBUG,
                            _("%s: Created %s."),
                            __FUNCTION__, confdir);


            /* try to see if there are any .mod file in pre-1.1 configuration */
            buff = get_old_conf_dir ();
            olddir = g_strconcat (buff, G_DIR_SEPARATOR_S, "modules", NULL);

            dir = g_dir_open (olddir, 0, NULL);

            g_free (buff);
            g_free (olddir);

            if (dir) {
                /* read files, if any; count number of .qth files */
                while ((datafile = g_dir_read_name (dir))) {

                    /* note: filename is not a newly allocated gchar *,
                       so we must not free it
                    */
                    if (g_strrstr (datafile, ".mod")) {

                        /* copy .qth file to USER_CONF_DIR */
                        target = g_strconcat (confdir, G_DIR_SEPARATOR_S, datafile, NULL);
                        if (!gpredict_file_copy (datafile, target)) {
                            /* success */
                            foundmod = TRUE;
                        }
                        g_free (target);
                    }

                }

                g_dir_close (dir);
            }
            else if (!foundmod) {
                /* copy Amateur.mod to this directory */
                filename = data_file_name ("Amateur.mod");
                target = g_strconcat (confdir, G_DIR_SEPARATOR_S, "Amateur.mod", NULL);

                if (gpredict_file_copy (filename, target)) {
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                _("%s: Failed to copy Amateur.mod"),
                                __FUNCTION__);

                    *error |= FTC_ERROR_STEP_02;
                }
                else {
                    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                                _("%s: Copied amateur.mod to %s/"),
                                __FUNCTION__, dir);
                }

                g_free (target);
                g_free (filename);
            }
        }
    }

    g_free (confdir);
}



/** \brief Execute step 4 of the first time checks.
 *
 * 4. Check for the existence of USER_CONF_DIR/satdata directory and create it if
 *    it does not exist.
 *
 */
static void
first_time_check_step_04 (guint *error)
{
    gchar *dir;
    int    status;

    dir = get_satdata_dir ();

    if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                        _("%s: Check successful."), __FUNCTION__);
    }
    else {
        /* try to create directory */
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                        _("%s: Check failed. Creating %s"),
                        __FUNCTION__, dir);

        status = g_mkdir_with_parents (dir, 0755);

        if (status) {
            /* set error flag */
            *error |= FTC_ERROR_STEP_04;

            sat_log_log (SAT_LOG_LEVEL_ERROR, _("%s: Failed to create %s"),
                            __FUNCTION__, dir);
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                            _("%s: Created %s."),
                            __FUNCTION__, dir);
        }
    }

    g_free (dir);
}



/** \brief Execute step 5 of the first time checks.
 *
 * 5. Copy PACKAGE_DATA_DIR/data/satdata/xxx.sat to USER_CONF_DIR/satdata/ if it
 *    does not already exist. Do the same with .cat files.
 *
 */
static void
first_time_check_step_05 (guint *error)
{
    GDir *srcdir;
    gchar *buff,*srcdirname,*targetdirname;
    const gchar *filename;
    guint i = 0;


    /* source directory */
    buff = get_data_dir ();
    srcdirname = g_strconcat (buff, G_DIR_SEPARATOR_S, "satdata", NULL);
    g_free (buff);

    /* target directory */
    targetdirname = get_satdata_dir ();

    srcdir = g_dir_open (srcdirname, 0, NULL);

    /* directory does not exist, something went wrong in step 4 */
    if (!srcdir) {
        sat_log_log (SAT_LOG_LEVEL_ERROR, 
                        _("%s: Could not open %s."),
                        __FUNCTION__, srcdirname);
        
        /* no reason to continue */
        *error |= FTC_ERROR_STEP_05;
    }
    else {
        /* read files one by one, if any; count number of .tle files */
        while ((filename = g_dir_read_name (srcdir))) {
            /* note: filename is not a newly allocated gchar *,
                so we must not free it
            */

            /* check whether .sat or .cat file exisits in user conf */
            gchar *target = sat_file_name (filename);

            if (!g_file_test (target, G_FILE_TEST_EXISTS)) {
                /* copy file to target dir */
                gchar *source = g_strconcat (srcdirname, G_DIR_SEPARATOR_S, filename, NULL);
                if (gpredict_file_copy (source, target)) {
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                 _("%s: Failed to copy %s"),
                                 __FUNCTION__, filename);
                }
                else {
                    sat_log_log (SAT_LOG_LEVEL_DEBUG,
                                 _("%s: Successfully copied %s"),
                                 __FUNCTION__, filename);
                }
                g_free (source);
                i++;
            }
            g_free (target);

        }
        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("%s: Copied %d files to %s"),
                     __FUNCTION__, i, targetdirname);

        g_dir_close (srcdir);
    }

    g_free (srcdirname);
    g_free (targetdirname);
}


/** \brief Execute step 6 of the first time checks.
 *
 * 6. Check for the existence of USER_CONF_DIR/satdata/cache directory. This
 *    directory is used to store temporary TLE files when updating from
 *    network.
 *
 */
static void
first_time_check_step_06 (guint *error)
{
    gchar *buff,*dir;
    int    status;


    buff = get_satdata_dir ();
    dir = g_strconcat (buff, G_DIR_SEPARATOR_S, "cache", NULL);
    g_free (buff);

    if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                        _("%s: Check successful."), __FUNCTION__);
    }
    else {
        /* try to create directory */
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                        _("%s: Check failed. Creating %s"),
                        __FUNCTION__, dir);

        status = g_mkdir_with_parents (dir, 0755);

        if (status) {
            /* set error flag */
            *error |= FTC_ERROR_STEP_06;

            sat_log_log (SAT_LOG_LEVEL_ERROR, _("%s: Failed to create %s"),
                            __FUNCTION__, dir);
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                            _("%s: Created %s."),
                            __FUNCTION__, dir);
        }
    }

    g_free (dir);
}


/** \brief Execute step 7 of the first time checks.
 *
 * 7. Check for the existence of USER_CONF_DIR/hwconf directory. This
 *    directory contains radio and rotator configurations (.rig and .rot files).
 *    If the directory is newly created, check if we have any existing configuration
 *    in the pre-1.1 configuration directory (use get_old_conf_dir()).
 *
 */
static void first_time_check_step_07 (guint *error)
{
    GDir   *dir;
    gchar  *confdir,*olddir,*buff;
    int     status;
    gchar  *target;

    const gchar *datafile;



    confdir = get_hwconf_dir ();

    if (g_file_test (confdir, G_FILE_TEST_IS_DIR)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                        _("%s: Check successful."),
                        __FUNCTION__);
    }
    else {
        /* try to create directory */
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                        _("%s: Check failed. Creating %s"),
                        __FUNCTION__,
                        confdir);

        status = g_mkdir_with_parents (confdir, 0755);

        if (status) {
            /* set error flag */
            *error |= FTC_ERROR_STEP_03;

            sat_log_log (SAT_LOG_LEVEL_ERROR,
                            _("%s: Failed to create %s"),
                            __FUNCTION__, confdir);
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_DEBUG,
                            _("%s: Created %s."),
                            __FUNCTION__, confdir);


            /* try to see if there are any .rig or .rot file in pre-1.1 configuration */
            buff = get_old_conf_dir ();
            olddir = g_strconcat (buff, G_DIR_SEPARATOR_S, "hwconf", NULL);

            dir = g_dir_open (olddir, 0, NULL);

            g_free (buff);
            g_free (olddir);

            if (dir) {
                /* read files, if any; count number of .qth files */
                while ((datafile = g_dir_read_name (dir))) {

                    /* note: filename is not a newly allocated gchar *,
                       so we must not free it
                    */

                    /* copy file to USER_CONF_DIR */
                    target = g_strconcat (confdir, G_DIR_SEPARATOR_S, datafile, NULL);
                    gpredict_file_copy (datafile, target);

                    g_free (target);
                }

            }
        }
    }

    g_free (confdir);

}

/** \brief Execute step 8 of the first time checks.
 *
 * 8. Check for the existence of USER_CONF_DIR/trsp directory. This
 *    directory contains transponder data for satellites.
 *
 */
static void first_time_check_step_08 (guint *error)
{
    gchar *dir;
    int    status;

    dir = get_trsp_dir ();

    if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                     _("%s: Check successful."), __FUNCTION__);
    }
    else {
        /* try to create directory */
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                     _("%s: Check failed. Creating %s"),
                       __FUNCTION__, dir);

        status = g_mkdir_with_parents (dir, 0755);

        if (status) {
            /* set error flag */
            *error |= FTC_ERROR_STEP_08;

            sat_log_log (SAT_LOG_LEVEL_ERROR, _("%s: Failed to create %s"),
                         __FUNCTION__, dir);
        }
        else {
            sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                         _("%s: Created %s."),
                           __FUNCTION__, dir);
        }
    }

    g_free (dir);
}

/** \brief Execute step 9 of the first time checks.
 *
 * 9. Check the .trsp files in USER_CONF_DIR/trsp/ and compare to the ones
 *    available in PACKAGE_DATA_DIR/data/trsp/xxx.trsp, and update if necessary.
 *
 */
static void first_time_check_step_09 (guint *error)
{
    GDir     *targetdir,*dir;
    gchar    *targetdirname;
    gchar    *datadirname;
    const gchar *filename;
    gchar    *srcfile,*destfile;
    gchar    *buff;


    /* open data directory */
    targetdirname = get_trsp_dir ();

    targetdir = g_dir_open (targetdirname, 0, NULL);

    /* directory does not exist, something went wrong in step 8 */
    if (!targetdir) {
        sat_log_log (SAT_LOG_LEVEL_ERROR, 
                        _("%s: Could not open %s."),
                        __FUNCTION__, targetdirname);
        
        /* no reason to continue */
        *error |= FTC_ERROR_STEP_09;
    }
    else {
        /* no need to keep this dir open */
        g_dir_close (targetdir);
        
        /* open data dir */
        buff = get_data_dir ();
        datadirname = g_strconcat (buff, G_DIR_SEPARATOR_S, "trsp", NULL);
        g_free (buff);
        dir = g_dir_open (datadirname, 0, NULL);
        
        /* for each .trsp file found in data dir */
        while ((filename = g_dir_read_name (dir))) {
            if (g_strrstr (filename, ".trsp")) {
                /* check if .trsp file already in user dir */
                destfile = g_strconcat (targetdirname, G_DIR_SEPARATOR_S, filename, NULL);
                
                /* check if .trsp file already in user dir */
                if (!g_file_test (destfile, G_FILE_TEST_EXISTS)) {
                    sat_log_log (SAT_LOG_LEVEL_MSG,
                                 _("%s: %s does not appear to be in user conf dir; adding."),
                                 __FUNCTION__, filename);

                                 /* copy new .trsp file to user dir */
                    srcfile = g_strconcat (datadirname, G_DIR_SEPARATOR_S, filename,NULL);
                    if (gpredict_file_copy (srcfile, destfile)) {
                        sat_log_log (SAT_LOG_LEVEL_ERROR, 
                                     _("%s: Failed to copy %s"),
                                     __FUNCTION__, filename);

                        *error |= FTC_ERROR_STEP_09;
                    }
                    g_free (srcfile);
                }
                else {
                    sat_log_log (SAT_LOG_LEVEL_MSG,
                                 _("%s: %s already in user conf dir."),
                                 __FUNCTION__, filename);
                }
                
                g_free (destfile);
            
            }
        }
        g_dir_close (dir);
        g_free (datadirname);
    }
    g_free (targetdirname);
}
