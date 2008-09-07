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
/** \defgroup tle_lookup TLE Link Manager
 *  \ingroup utils
 *
 * The TLE link manager maintains a mapping between satellite catalog numbers
 * and the .tle files found in the user's data directory, usually
 * $HOME/.gpredict/tle/
 */
/** \defgroup tle_lookup_if Interface
 *  \ingroup tle_lookup
 */
/** \defgroup tle_lookup_priv Private
 *  \ingroup tle_lookup
 */
#include <glib.h>
#include <glib/gi18n.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "sat-log.h"
#include "tle-lookup.h"


/** \brief The has tanle containing the links.
 *  \ingroup tle_lookup_priv
 */
GHashTable *linktable = NULL;

/** \brief Key used when accessing the hash table.
 *  \ingroup tle_lookup_priv
 */
guint      *key = NULL;


/* private function prototypes */
static guint tle_lookup_scan_file (const gchar *name);



/** \brief Create and initialise hash table.
 *  \param tledir Directory to scan (optional).
 *  \return Error code indicating whether the process has been successful 
 *          (TLE_LOOKUP_INIT_OK) or not.
 *  \ingroup tle_lookup_if
 *
 * The function creates and initialises the lookup hash table. It then scans
 * the specified directory for tle files and adds each sattelite (catnum,file)
 * to the hash table. If the specified directory is NULL, $HOME/.gpredict/tle/
 * is used.
 *
 * \note The error TLE_LOOKUP_NO_MEM is fatal and the main program should exit
 *       immediately in such case.
 */
gint
tle_lookup_init  (const gchar *tledir)
{
	GDir  *dir;
	gchar *dirname;
	const gchar *name;
	gchar *msg;
	guint  num;

	/* create hash table  */
	linktable = g_hash_table_new_full (g_int_hash,
									   g_int_equal,
									   g_free,
									   g_free);

	if (!linktable) {
		return TLE_LOOKUP_NO_MEM;
	}

	/* open directory */
	if (tledir != NULL) {
		dirname = g_strdup (tledir);
	}
	else {
		dirname = g_strdup_printf ("%s%s.gpredict2%stle",
								   g_get_home_dir (),
								   G_DIR_SEPARATOR_S,
								   G_DIR_SEPARATOR_S);
	}

	/* debug message */
	sat_log_log (SAT_LOG_LEVEL_DEBUG,
				 _("%s: Directory is: %s"),
				 __FUNCTION__, dirname);

	dir = g_dir_open (dirname, 0, NULL);

	/* no tle files */
	if (!dir) {
		sat_log_log (SAT_LOG_LEVEL_ERROR,
					 _("%s: No .tle files found in %s."),
					 __FUNCTION__, dirname);
		
		return TLE_LOOKUP_EMPTY_DIR;
	}

	/* Scan data directory for .tle files.
	   For each file scan through the file and
	   add entry to the hash table.
	*/
	while ((name = g_dir_read_name (dir))) {

		if (g_strrstr (name, ".tle")) {

			msg = g_strconcat (dirname, G_DIR_SEPARATOR_S,
							   name, NULL);
			num = tle_lookup_scan_file (msg);
			g_free (msg);

			sat_log_log (SAT_LOG_LEVEL_MSG,
						 _("%s: Read %d sats from %s "),
						 __FUNCTION__, num, name);
		}

	}

	g_dir_close (dir);
	g_free (dirname);

	return TLE_LOOKUP_INIT_OK;
}


/** \brief Close link table and free resources.
 *  \ingroup tle_lookup_if
 *
 * This function destroys the link table freeing any memory that has been
 * allocated to it.
 */
void
tle_lookup_close ()
{

	g_hash_table_destroy (linktable);
	sat_log_log (SAT_LOG_LEVEL_DEBUG,
				 _("%s: Hash table destroyed, resources freed."),
				 __FUNCTION__);
}


/** \brief Get number of satellites in link table */
guint
tle_lookup_count ()
{
	if (!linktable) {

		return 0;
	}

	return g_hash_table_size (linktable);
}


/** \brief Look up satellite
 *  \return copy of the file name in which the satellite can be found or
 *          NULL if the satellite can not be found.
 *
 * \note The returned string should be freed when no onger needed, unless
 *       it is already NULL :-P
 */
gchar *
tle_lookup       (gint catnum)
{
	gchar *filename = NULL;
	guint *key;

	if (!linktable) {
		sat_log_log (SAT_LOG_LEVEL_BUG,
					 _("%s: Link table is NULL!"),
					 __FUNCTION__);

		return NULL;
	}

	key = g_new0 (guint, 1);
	*key = (guint) catnum;

	/* GUINT_TO_POINTER(catnum) does not work */
	filename = (gchar *) g_hash_table_lookup (linktable, key);

	g_free (key);

	if (filename) {
		return g_strdup (filename);
	}
	else {
		return NULL;
	}
}


/** \brief Scan TLE file for satellites.
 *  \param name The TLE file to scan.
 *  \return The number of satellites found.
 *  \ingroup tle_lookup_priv
 *
 * This function scans the specified TLE file and adds each satellite
 * to the hash table. It returns the number of satellites found in the
 * file.
 *
 * FIXME: USE GError
 */
static guint
tle_lookup_scan_file (const gchar *name)
{
	guint i = 0;
	guint j;
	GIOChannel *tlefile;
	gchar *line;
	gsize  length;
	gchar  catstr[6];

	/* open IO channel and read 3 lines at a time */
	tlefile = g_io_channel_new_file (name, "r", NULL);

	if (tlefile) {

		/*** FIXME: add error handling */

		while (g_io_channel_read_line (tlefile, &line, &length, NULL, NULL) != G_IO_STATUS_EOF) {

			/* free line */
			g_free (line);

			/* extract catnum from second line; index 2..6 */
			g_io_channel_read_line (tlefile, &line, &length,
									NULL, NULL);
			
			for (j = 2; j < 7; j++) {
				catstr[j-2] = line[j];
			}
			catstr[5] = '\0';

			key = g_try_malloc (sizeof (guint));
			if (key) {
				*key = (guint) g_ascii_strtod (catstr, NULL);
			}
			else {
				sat_log_log (SAT_LOG_LEVEL_ERROR,
							 _("%s: Could not allocate memory for KEY"),
							 __FUNCTION__);

				/* FIXME: What to do now? We should try to bail out! */
			}

			/* add (catnum,name) to hash table;
			   warn if key already exists...
			*/
			if (g_hash_table_lookup (linktable, key)) {
				sat_log_log (SAT_LOG_LEVEL_WARN,
							 _("%s: Catnum %d found more than once."),
							 __FUNCTION__, *key);
			}
			
			g_hash_table_insert (linktable, key, g_path_get_basename (name));

			/* free line */
			g_free (line);

			/* read the third line */
			g_io_channel_read_line (tlefile, &line, &length, NULL, NULL);

			/* free line */
			g_free (line);

			i++;
		}

		/* close IO chanel; don't care about status */
		g_io_channel_shutdown (tlefile, TRUE, NULL);
		g_io_channel_unref (tlefile);
	}

	return i;
}
