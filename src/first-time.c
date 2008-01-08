/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>

    Comments, questions and bugreports should be submitted via
    http://sourceforge.net/projects/groundstation/
    More details can be found at the project home page:

            http://groundstation.sourceforge.net/
 
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


/**** FIXME: clean up / rewrite: check_dir, checkfile and driver */


/* private function prototypes */
static void first_time_check_step_01 (guint *error);
static void first_time_check_step_02 (guint *error);
static void first_time_check_step_03 (guint *error);
static void first_time_check_step_04 (guint *error);
static void first_time_check_step_05 (guint *error);
static void first_time_check_step_06 (guint *error);
static void first_time_check_step_07 (guint *error);



/** \brief Perform first time checks.
 *
 * This function is called by the main function very early during program
 * startup. It's purpose is to check the user configuration to see weather
 * this is the first time gpredict is executed. If it is, a new default
 * configuration is set up so that the user has some sort of setup to get
 * started with.
 *
 * Check logic:
 *
 * 1. Check for $HOME/.gpredict2/ and create dir if it does not exist
 * 2. Check for the existence of at least one .qth file in $HOME/.gpredict2/
 *    If no such file found, copy PACKAGE_DATA_DIR/data/sample.qth to this
 *    directory.
 * 3. Check for the existence of $HOME/.gpredict2/modules directory and create
 *    it if it does not exist. Copy PACKAGE_DATA_DIR/data/Amateur.mod to the
 *    newly created directory.
 * 4. Check for the existence of $HOME/.gpredict2/tle directory and create it if
 *    it does not exist.
 * 5. Check for the existence of at least one .tle file in the above mentioned
 *    directory. If no such file found, copy PACKAGE_DATA_DIR/data/xxx.tle to this
 *    directory.
 * 6. Check for the existence of $HOME/.gpredict2/tle/cache directory. This
 *    directory is used to store temporary TLE files when updating from
 *    network.
 * 7. Check for the existence of $HOME/.gpredict2/hwconf directory. This
 *    directory contains radio and rotator configurations (.rig and .rot files).
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

	return error;
}



/** \brief Execute step 1 of the first time checks.
 *
 * 1. Check for $HOME/.gpredict2/ and create dir if it does not exist
 *
 */
static void
first_time_check_step_01 (guint *error)
{
	gchar *dir;
	int    status;

	dir = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
					   ".gpredict2", NULL);

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

		status = g_mkdir (dir, 0755);

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
		}
	}

	g_free (dir);
}



/** \brief Execute step 2 of the first time checks.
 *
 * 2. Check for the existence of at least one .qth file in $HOME/.gpredict2/
 *    If no such file found, copy PACKAGE_DATA_DIR/data/sample.qth to this
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


	dirname = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
						   ".gpredict2", NULL);

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
			/* try to copy sample.qth */
			filename = data_file_name ("sample.qth");
			target = g_strconcat (dirname, G_DIR_SEPARATOR_S,
								  "sample.qth", NULL);

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

	g_free (dirname);
}



/** \brief Execute step 3 of the first time checks.
 *
 * 3. Check for the existence of $HOME/.gpredict2/modules directory and create
 *    it if it does not exist.
 *
 */
static void
first_time_check_step_03 (guint *error)
{
	gchar  *dir;
	int     status;
	gchar  *target;
	gchar  *filename;


	dir = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
					   ".gpredict2", G_DIR_SEPARATOR_S,
					   "modules", NULL);

	if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
		sat_log_log (SAT_LOG_LEVEL_DEBUG,
					 _("%s: Check successful."),
					 __FUNCTION__);
	}
	else {
		/* try to create directory */
		sat_log_log (SAT_LOG_LEVEL_DEBUG,
					 _("%s: Check failed. Creating %s"),
					 __FUNCTION__,
					 dir);

		status = g_mkdir (dir, 0755);

		if (status) {
			/* set error flag */
			*error |= FTC_ERROR_STEP_03;

			sat_log_log (SAT_LOG_LEVEL_ERROR, 
						 _("%s: Failed to create %s"),
						 __FUNCTION__, dir);
		}
		else {
			sat_log_log (SAT_LOG_LEVEL_DEBUG,
						 _("%s: Created %s."),
						 __FUNCTION__, dir);

			/* copy Amateur.mod to this directory */
			filename = data_file_name ("Amateur.mod");
			target = g_strconcat (dir, G_DIR_SEPARATOR_S,
								  "Amateur.mod", NULL);

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

	g_free (dir);
}



/** \brief Execute step 4 of the first time checks.
 *
 * 4. Check for the existence of $HOME/.gpredict2/tle directory and create it if
 *    it does not exist.
 *
 */
static void
first_time_check_step_04 (guint *error)
{
	gchar *dir;
	int    status;

	dir = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
					   ".gpredict2", G_DIR_SEPARATOR_S,
					   "tle", NULL);

	if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
		sat_log_log (SAT_LOG_LEVEL_DEBUG, 
					 _("%s: Check successful."), __FUNCTION__);
	}
	else {
		/* try to create directory */
		sat_log_log (SAT_LOG_LEVEL_DEBUG, 
					 _("%s: Check failed. Creating %s"),
					 __FUNCTION__, dir);

		status = g_mkdir (dir, 0755);

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
 * 5. Check for the existence of at least one .tle file in the above mentioned
 *    directory. If no such file found, copy PACKAGE_DATA_DIR/data/xxx.tle to this
 *    directory.
 *
 */
static void
first_time_check_step_05 (guint *error)
{
	GDir     *dir;
	gchar    *dirname;
	gchar    *datadir;
	const gchar *filename;
	gchar    *target;
	gchar    *tlefile;
	gboolean  foundtle = FALSE;


	dirname = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
						   ".gpredict2", G_DIR_SEPARATOR_S,
						   "tle", NULL);

	dir = g_dir_open (dirname, 0, NULL);

	/* directory does not exist, something went wrong in step 1 */
	if (!dir) {
		sat_log_log (SAT_LOG_LEVEL_ERROR, 
					 _("%s: Could not open %s."),
					 __FUNCTION__, dirname);
		
		/* no reason to continue */
		*error |= FTC_ERROR_STEP_05;
	}
	else {
		/* read files, if any; count number of .tle files */
		while ((filename = g_dir_read_name (dir))) {

			/* note: filename is not a newly allocated gchar *,
			   so we must not free it
			*/

			if (g_strrstr (filename, ".tle")) {
				foundtle = TRUE;
			}

		}

		g_dir_close (dir);

		if (foundtle) {
			sat_log_log (SAT_LOG_LEVEL_DEBUG, 
						 _("%s: Found at least one .tle file."),
						 __FUNCTION__);
		}
		else {

			/* try to copy each tle file from instalation dir */
			datadir = get_data_dir ();
			dir = g_dir_open (datadir, 0, NULL);

			/* 			g_print ("====> %s\n", datadir); */

			while ((filename = g_dir_read_name (dir))) {

				/* note: filename is not a newly allocated gchar *,
				   so we must not free it
				*/

				if (g_strrstr (filename, ".tle")) {

					tlefile = data_file_name (filename);

					target = g_strconcat (dirname,
										  G_DIR_SEPARATOR_S,
										  filename,
										  NULL);

					if (gpredict_file_copy (tlefile, target)) {
						sat_log_log (SAT_LOG_LEVEL_ERROR, 
									 _("%s: Failed to copy %s"),
									 __FUNCTION__, filename);

						*error |= FTC_ERROR_STEP_05;
					}
					else {
						sat_log_log (SAT_LOG_LEVEL_DEBUG, 
									 _("%s: Successfully copied %s"),
									 __FUNCTION__, filename);
					}
					g_free (tlefile);
					g_free (target);
				}

			}
			g_free (datadir);
			g_dir_close (dir);
		}

	}
	g_free (dirname);

}


/** \brief Execute step 6 of the first time checks.
 *
 * 6. Check for the existence of $HOME/.gpredict2/tle/cache directory. This
 *    directory is used to store temporary TLE files when updating from
 *    network.
 *
 */
static void
first_time_check_step_06 (guint *error)
{
	gchar *dir;
	int    status;

	dir = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
					   ".gpredict2", G_DIR_SEPARATOR_S,
					   "tle", G_DIR_SEPARATOR_S,
					   "cache", NULL);

	if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
		sat_log_log (SAT_LOG_LEVEL_DEBUG, 
					 _("%s: Check successful."), __FUNCTION__);
	}
	else {
		/* try to create directory */
		sat_log_log (SAT_LOG_LEVEL_DEBUG, 
					 _("%s: Check failed. Creating %s"),
					 __FUNCTION__, dir);

		status = g_mkdir (dir, 0755);

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
 * 7. Check for the existence of $HOME/.gpredict2/hwconf directory. This
 *    directory contains radio and rotator configurations (.rig and .rot files).
 *
 */
static void first_time_check_step_07 (guint *error)
{
    gchar *cfg;
    gchar *dir;
    int    status;

    cfg = get_conf_dir ();
    dir = g_strconcat (cfg, G_DIR_SEPARATOR_S, "hwconf", NULL);
    g_free (cfg);

    if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                     _("%s: Check successful."), __FUNCTION__);
    }
    else {
        /* try to create directory */
        sat_log_log (SAT_LOG_LEVEL_DEBUG, 
                     _("%s: Check failed. Creating %s"),
                       __FUNCTION__, dir);

        status = g_mkdir (dir, 0755);

        if (status) {
            /* set error flag */
            *error |= FTC_ERROR_STEP_07;

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

