/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.
    Copyright (C)  2009 Charles Suprin AA1VS.

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
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include <curl/curl.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "sat-cfg.h"
#include "compat.h"
#include "tle-update.h"


/* Flag indicating whether TLE update is in progress.
   This should avoid multiple attempts to update TLE,
   e.g. user starts update from menubar while automatic
   update is in progress
*/
/* static gboolean tle_in_progress = FALSE; */
/* Replace flag with lock */
/* http://library.gnome.org/devel/glib/unstable/glib-Threads.html */
static GStaticMutex tle_in_progress = G_STATIC_MUTEX_INIT ;
static GStaticMutex tle_file_in_progress = G_STATIC_MUTEX_INIT ;

/* private function prototypes */
static size_t  my_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream);
static gint    read_fresh_tle (const gchar *dir, const gchar *fnam, GHashTable *data);
static gboolean is_tle_file (const gchar *dir, const gchar *fnam);


static void    update_tle_in_file (const gchar *ldname,
                                   const gchar *fname,
                                   GHashTable  *data,
                                   guint       *sat_upd,
                                   guint       *sat_ski,
                                   guint       *sat_nod,
                                   guint       *sat_tot);

static guint add_new_sats (GHashTable *data);



/** \bief Free a new_tle_t structure. */
static void free_new_tle (gpointer data)
{
    new_tle_t *tle;

    tle = (new_tle_t *) data;

    g_free (tle->satname);
    g_free (tle->line1);
    g_free (tle->line2);
    g_free (tle->srcfile);
    g_free (tle);
}





/** \brief Update TLE files from local files.
 *  \param dir Directory where files are located.
 *  \param filter File filter, e.g. *.txt (not used at the moment!)
 *  \param silent TRUE if function should execute without graphical status indicator.
 *  \param label1 Activity label (can be NULL)
 *  \param label2 Statistics label (can be NULL)
 *  \param progress Pointer to progress indicator.
 *  \param init_prgs Initial value of progress indicator, e.g 0.5 if we are updating
 *                   from network.
 *
 * This function is used to update the TLE data from local files.
 *
 * Functional description: TBD
 *
 */
void tle_update_from_files (const gchar *dir, const gchar *filter,
                            gboolean silent, GtkWidget *progress,
                            GtkWidget *label1, GtkWidget *label2)
{
    GHashTable  *data;        /* hash table with fresh TLE data */
    GDir        *cache_dir;   /* directory to scan fresh TLE */
    GDir        *loc_dir;     /* directory for gpredict TLE files */
    GError      *err = NULL;
    gchar       *text;
    gchar       *ldname;
    gchar       *userconfdir;
    const gchar *fnam;
    guint        num = 0;
    guint        updated,updated_tmp;
    guint        skipped,skipped_tmp;
    guint        nodata,nodata_tmp;
    guint        newsats = 0;
    guint        total,total_tmp;
    gdouble      fraction = 0.0;
    gdouble      start = 0.0;

    if (g_static_mutex_trylock(&tle_file_in_progress)==FALSE) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: A TLE update process is already running. Aborting."),
                     __FUNCTION__);

        return;
    }

    /* create hash table */
    data = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, free_new_tle);


    /* open directory and read files one by one */
    cache_dir = g_dir_open (dir, 0, &err);

    if (err != NULL) {

        /* send an error message */
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error opening directory %s (%s)"),
                     __FUNCTION__, dir, err->message);

        /* insert error message into the status string, too */
        if (!silent && (label1 != NULL)) {
            text = g_strdup_printf (_("<b>ERROR</b> opening directory %s\n%s"),
                                    dir, err->message);

            gtk_label_set_markup (GTK_LABEL (label1), text);
            g_free (text);

        }

        g_clear_error (&err);
        err = NULL;
    }
    else {

        /* scan directory for tle files */
        while ((fnam = g_dir_read_name (cache_dir)) != NULL) {

            /* check that we got a TLE file */
            if (is_tle_file(dir, fnam)) {
                
                /* status message */
                if (!silent && (label1 != NULL)) {
                    text = g_strdup_printf (_("Reading data from %s"), fnam);
                    gtk_label_set_text (GTK_LABEL (label1), text);
                    g_free (text);
    
    
                    /* Force the drawing queue to be processed otherwise there will
                        not be any visual feedback, ie. frozen GUI
                        - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                    */
                    while (g_main_context_iteration (NULL, FALSE));
    
                    /* give user a chance to follow progress */
                    g_usleep (G_USEC_PER_SEC / 100);
                }
    
                /* now, do read the fresh data */
                num = read_fresh_tle (dir, fnam, data);
            }
            
            if (num < 1) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: No valid TLE data found in %s"),
                             __FUNCTION__, fnam);
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: Read %d sats from %s into memory"),
                             __FUNCTION__, num, fnam);
            }
        }

        /* close directory since we don't need it anymore */
        g_dir_close (cache_dir);

        /* now we load each .sat file and update if we have new data */
        userconfdir = get_user_conf_dir ();
        ldname = g_strconcat (userconfdir, G_DIR_SEPARATOR_S, "satdata", NULL);
        g_free (userconfdir);

        /* open directory and read files one by one */
        loc_dir = g_dir_open (ldname, 0, &err);

        if (err != NULL) {

            /* send an error message */
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Error opening directory %s (%s)"),
                         __FUNCTION__, dir, err->message);

            /* insert error message into the status string, too */
            if (!silent && (label1 != NULL)) {
                text = g_strdup_printf (_("<b>ERROR</b> opening directory %s\n%s"),
                                        dir, err->message);

                gtk_label_set_markup (GTK_LABEL (label1), text);
                g_free (text);
            }

            g_clear_error (&err);
            err = NULL;
        }
        else {
            /* clear statistics */
            updated = 0;
            skipped = 0;
            nodata = 0;
            total = 0;

            /* get initial value of progress indicator */
            if (progress != NULL)
                start = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (progress));

            /* This is insane but I don't know how else to count the number of sats */
            num = 0;
            while ((fnam = g_dir_read_name (loc_dir)) != NULL) {
                /* only consider .sat files */
                if (g_str_has_suffix (fnam, ".sat")) {
                    num++;
                }
            }

            g_dir_rewind (loc_dir);

            /* update TLE files one by one */
            while ((fnam = g_dir_read_name (loc_dir)) != NULL) {
                /* only consider .sat files */
                if (g_str_has_suffix (fnam, ".sat")) {

                    /* clear stat bufs */
                    updated_tmp = 0;
                    skipped_tmp = 0;
                    nodata_tmp = 0;
                    total_tmp = 0;
                    

                    /* update TLE data in this file */
                    update_tle_in_file (ldname, fnam, data, 
                                        &updated_tmp,
                                        &skipped_tmp,
                                        &nodata_tmp,
                                        &total_tmp);

                    /* update statistics */
                    updated += updated_tmp;
                    skipped += skipped_tmp;
                    nodata  += nodata_tmp;
                    total   = updated+skipped+nodata;

                    if (!silent) {

                        if (label1 != NULL) {
                            gtk_label_set_text (GTK_LABEL (label1),
                                                _("Updating data..."));
                        }

                        if (label2 != NULL) {
                            text = g_strdup_printf (_("Satellites updated:\t %d\n"\
                                                      "Satellites skipped:\t %d\n"\
                                                      "Missing Satellites:\t %d\n"),
                                                    updated, skipped, nodata);
                            gtk_label_set_text (GTK_LABEL (label2), text);
                            g_free (text);
                        }

                        if (progress != NULL) {

                            /* calculate and saturate fraction
                                (number of sats in TLE files is greater
                                than the number of sats in link table)
                            */
                            fraction = start + (1.0-start) * ((gdouble) total) / ((gdouble) num);
                            if (fraction >= 0.95)
                                fraction = 0.98;
                            
                            gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
                                                           fraction);

                        }

                        /* Force the drawing queue to be processed otherwise there will
                            not be any visual feedback, ie. frozen GUI
                            - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                        */
                        while (g_main_context_iteration (NULL, FALSE));

                        /* give user a chance to follow progress */
                        g_usleep (G_USEC_PER_SEC / 1000);
                    }
                }
            }

            /* close directory handle */
            g_dir_close (loc_dir);
            
            /* see if we have any new sats that need to be added */
            if (sat_cfg_get_bool (SAT_CFG_BOOL_TLE_ADD_NEW)) {
                
                newsats = add_new_sats (data);
                
                if (!silent && (label2 != NULL)) {
                    text = g_strdup_printf (_("Satellites updated:\t %d\n"\
                                              "Satellites skipped:\t %d\n"\
                                              "Missing Satellites:\t %d\n"\
                                              "New Satellites:\t\t %d"),
                                            updated, skipped, nodata, newsats);
                    gtk_label_set_text (GTK_LABEL (label2), text);
                    g_free (text);

                }
                
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: Added %d new satellites to local database"),
                             __FUNCTION__, newsats);
            }

            /* store time of update if we have updated something */
            if ((updated > 0) || (newsats > 0)) {
                GTimeVal tval;
                
                g_get_current_time (&tval);
                sat_cfg_set_int (SAT_CFG_INT_TLE_LAST_UPDATE, tval.tv_sec);
            }

        }

        g_free (ldname);

        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("%s: TLE elements updated."),
                     __FUNCTION__);
    }

    /* destroy hash tables */
    g_hash_table_destroy (data);

    g_static_mutex_unlock(&tle_file_in_progress);
}



/** \brief Check if satellite is new, if so, add it to local database */
static void check_and_add_sat (gpointer key, gpointer value, gpointer user_data)
{
    new_tle_t  *ntle = (new_tle_t *) value;
    guint      *num = user_data;
    GKeyFile   *satdata;
    GIOChannel *satfile;
    gchar      *cfgstr, *cfgfile;
    gsize       length, written;
    GError     *err = NULL;
    

    /* check if sat is new */
    if (ntle->isnew) {

        /* create config data */
        satdata = g_key_file_new ();

        /* store data */
        g_key_file_set_string (satdata, "Satellite", "VERSION", "1.1");
        g_key_file_set_string (satdata, "Satellite", "NAME", ntle->satname);
        g_key_file_set_string (satdata, "Satellite", "NICKNAME", ntle->satname);
        g_key_file_set_string (satdata, "Satellite", "TLE1", ntle->line1);
        g_key_file_set_string (satdata, "Satellite", "TLE2", ntle->line2);

        /* convert data to text */
        cfgstr = g_key_file_to_data (satdata, &length, NULL);

        /* create an I/O channel and store data */
        cfgfile = sat_file_name_from_catnum (ntle->catnum);
        satfile = g_io_channel_new_file (cfgfile, "w", &err);

        if (err != NULL) {
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Could not create satellite file (%s)."),
                         __FUNCTION__, err->message);
            g_clear_error (&err);
        }
        else {
            g_io_channel_write_chars (satfile, cfgstr, length, &written, &err);
            g_io_channel_shutdown (satfile, TRUE, NULL);
            g_io_channel_unref (satfile);

            if (err != NULL) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: Error writing satellite data for %d (%s)."),
                             __FUNCTION__, ntle->catnum, err->message);
                g_clear_error (&err);
            }
            else if (length != written) {
                sat_log_log (SAT_LOG_LEVEL_WARN,
                             _("%s: Wrote only %d out of %d chars for satellite data %d."),
                             __FUNCTION__, written, length, ntle->catnum);
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: Data for new sat %d successfully added."),
                             __FUNCTION__, ntle->catnum);
                *num += 1;
            }
        }

        /* clean up memory */
        g_free (cfgstr);
        g_free (cfgfile);
        g_key_file_free (satdata);


        /**** FIXME: NEED TO CREATE COPY of cache */
        /* finally, new satellite must be added to proper category */
        gchar *catfile;
        gchar **buff;

        buff = g_strsplit (ntle->srcfile, ".", 0);
        cfgfile = g_strconcat (buff[0], ".cat", NULL);
        catfile = sat_file_name (cfgfile);

        /* g_io_channel */
        satfile = g_io_channel_new_file (catfile, "a", &err);

        if (err != NULL) {
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Could not open category file file %s (%s)."),
                         __FUNCTION__, cfgfile, err->message);
            g_clear_error (&err);
        }
        else {
            cfgstr = g_strdup_printf ("%d\n", ntle->catnum);
            g_io_channel_write_chars (satfile, cfgstr, -1, NULL, &err);
            g_io_channel_shutdown (satfile, TRUE, NULL);
            g_io_channel_unref (satfile);
            g_free (cfgstr);

            if (err != NULL) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: Error adding %d to %s (%s)."),
                             __FUNCTION__, ntle->catnum, cfgfile, err->message);
                g_clear_error (&err);
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: Added satellite %d to %s."),
                             __FUNCTION__, ntle->catnum, cfgfile);
            }
        }

        g_free (catfile);
        g_free (cfgfile);
        g_strfreev (buff);
    }

}


/** \brief Add new satellites to local database */
static guint add_new_sats (GHashTable *data)
{
    guint num = 0;

    g_hash_table_foreach (data, check_and_add_sat, &num);

    return num;
}



/** \brief Update TLE files from network.
 *  \param silent TRUE if function should execute without graphical status indicator.
 *  \param progress Pointer to a GtkProgressBar progress indicator (can be NULL)
 *  \param label1 GtkLabel for activity string.
 *  \param label2 GtkLabel for statistics string.
 */
void tle_update_from_network (gboolean   silent,
                              GtkWidget *progress,
                              GtkWidget *label1,
                              GtkWidget *label2)
{
    gchar       *server;
    gchar       *proxy = NULL;
    gchar       *files_tmp;
    gchar      **files;
    guint        numfiles,i;
    gchar       *curfile;
    gchar       *locfile;
    gchar       *userconfdir;
    CURL        *curl;
    CURLcode     res;
    gboolean     error = FALSE;
    gdouble      fraction,start=0;
    FILE        *outfile;
    GDir        *dir;
    gchar       *cache;
    const gchar *fname;
    gchar       *text;
    GError      *err = NULL;
    guint        success = 0; /* no. of successfull downloads */ 

    /* bail out if we are already in an update process */
    /*if (tle_in_progress)*/
    if (g_static_mutex_trylock(&tle_in_progress)==FALSE) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: A TLE update process is already running. Aborting."),
                     __FUNCTION__);

        return;
    }


    /*tle_in_progress = TRUE;*/

    /* get server, proxy, and list of files */
    server = sat_cfg_get_str (SAT_CFG_STR_TLE_SERVER);
    proxy  = sat_cfg_get_str (SAT_CFG_STR_TLE_PROXY);
    files_tmp = sat_cfg_get_str (SAT_CFG_STR_TLE_FILES);
    files = g_strsplit (files_tmp, ";", 0);
    numfiles = g_strv_length (files);

    if (numfiles < 1) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: No files to fetch from network."),
                     __FUNCTION__);

        /* set activity string, so user knows why nothing happens */
        if (!silent && (label1 != NULL)) {
            gtk_label_set_text (GTK_LABEL (label1),
                                _("No files to fetch from network"));
        }
    }
    else {

        /* initialise progress bar */
        if (!silent && (progress != NULL))
            start = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (progress));

        /* initialise curl */
        curl = curl_easy_init();
        if (proxy != NULL)
            curl_easy_setopt (curl, CURLOPT_PROXY, proxy);

        curl_easy_setopt (curl, CURLOPT_USERAGENT, "gpredict/curl");
        curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 10);

        /* get files */
        for (i = 0; i < numfiles; i++) {

            /* set URL */
            curfile = g_strconcat (server, files[i], NULL);
            curl_easy_setopt (curl, CURLOPT_URL, curfile);

            /* set activity message */
            if (!silent && (label1 != NULL)) {

                text = g_strdup_printf (_("Fetching %s"), files[i]);
                gtk_label_set_text (GTK_LABEL (label1), text);
                g_free (text);

                /* Force the drawing queue to be processed otherwise there will
                    not be any visual feedback, ie. frozen GUI
                    - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                */
                while (g_main_context_iteration (NULL, FALSE));
            }

            /* create local cache file */
            userconfdir = get_user_conf_dir ();
            locfile = g_strconcat (userconfdir, G_DIR_SEPARATOR_S,
                                   "satdata", G_DIR_SEPARATOR_S,
                                   "cache", G_DIR_SEPARATOR_S,
                                   files[i], NULL);
            outfile = g_fopen (locfile, "wb");
            curl_easy_setopt (curl, CURLOPT_WRITEDATA, outfile);
            curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, my_write_func);

            /* get file */
            res = curl_easy_perform (curl);

            if (res != CURLE_OK) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: Error fetching %s (%s)"),
                             __FUNCTION__, curfile, curl_easy_strerror (res));
                error = TRUE;
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: Successfully fetched %s"),
                             __FUNCTION__, curfile);
                success++;
            }

            /* update progress indicator */
            if (!silent && (progress != NULL)) {

                /* complete download corresponds to 50% */
                fraction = start + (0.5-start) * i / (2.0 * numfiles);
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), fraction);

                /* Force the drawing queue to be processed otherwise there will
                    not be any visual feedback, ie. frozen GUI
                    - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                */
                while (g_main_context_iteration (NULL, FALSE));

            }

            g_free (userconfdir);
            g_free (curfile);
            g_free (locfile);
            fclose (outfile);
        }

        curl_easy_cleanup (curl);

        /* continue update if we have fetched at least one file */
        if (success > 0) {
            sat_log_log (SAT_LOG_LEVEL_MSG,
                         _("%s: Fetched %d files from network; updating..."),
                         __FUNCTION__, success);
            /* call update_from_files */
            cache = sat_file_name ("cache");
            tle_update_from_files (cache, NULL, silent, progress, label1, label2);
            g_free (cache);

        }
        else {
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Could not fetch any new TLE files from network; aborting..."),
                         __FUNCTION__);
        }

    }

    /* clear cache and memory */
    g_free (server);
    g_strfreev (files);
    g_free (files_tmp);
    if (proxy != NULL)
        g_free (proxy);

    /* open cache */
    cache = sat_file_name ("cache");
    dir = g_dir_open (cache, 0, &err);

    if (err != NULL) {
        /* send an error message */
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error opening %s (%s)"),
                     __FUNCTION__, dir, err->message);
        g_clear_error (&err);
    }
    else {
        /* delete files in cache one by one */
        while ((fname = g_dir_read_name (dir)) != NULL) {

            locfile = g_strconcat (cache, G_DIR_SEPARATOR_S,
                                   fname, NULL);

            g_remove (locfile);
            g_free (locfile);
        }
        /* close cache */
        g_dir_close (dir);
    }

    g_free (cache);

    /* clear busy flag */
    /* tle_in_progress = FALSE; */
    g_static_mutex_unlock(&tle_in_progress);

}


/** \brief Write TLE data block to file.
 *  \param ptr Pointer to the data block to be written.
 *  \param size Size of data block.
 *  \param nmemb Size multiplier?
 *  \param stream Pointer to the file handle.
 *  \return The number of bytes actually written.
 *
 * This function writes the received data to the file pointed to by stream.
 * It is used as write callback by to curl exec function.
 */
static size_t my_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    /*** FIXME: TBC whether this works in wintendo */
    return fwrite (ptr, size, nmemb, stream);
}


/** \brief Check whether file is TLE file.
 *  \param dir The directory.
 *  \param fnam The file name.
 * 
 * This function checks whether the file with path dir/fnam is a potential
 * TLE file. Checks performed:
 *   - It is a real file
 *   - suffix is .txt or .tle
 */
static gboolean is_tle_file (const gchar *dir, const gchar *fnam)
{
    gchar    *path;
    gchar    *fname_lower;
    gboolean  fileIsOk = FALSE;
    
    path = g_strconcat (dir, G_DIR_SEPARATOR_S, fnam, NULL);
    fname_lower=g_ascii_strdown(fnam,-1);
    
    if (g_file_test (path, G_FILE_TEST_IS_REGULAR) && 
        (g_str_has_suffix(fname_lower, ".tle") || g_str_has_suffix(fname_lower, ".txt")))
    {
        fileIsOk = TRUE;      
    }
    g_free (fname_lower);
    g_free (path);
    
    return fileIsOk;
}


/** \brief Read fresh TLE data into hash table.
 *  \param dir The directory to read from.
 *  \param fnam The name of the file to read from.
 *  \param fresh_data Hash table where the data should be stored.
 *  \return The number of satellites successfully read.
 * 
 * This function will read fresh TLE data from local files into memory.
 * If there is a saetllite category (.cat file) with the same name as the
 * input file it will also update the satellites in that category.
 */
static gint read_fresh_tle (const gchar *dir, const gchar *fnam, GHashTable *data)
{
    new_tle_t *ntle;
    tle_t      tle;
    gchar     *path;
    gchar      tle_str[3][80];
    gchar      catstr[6];
    gchar     *b;
    FILE      *fp;
    gint       retcode = 0;
    guint      catnr,i;
    guint     *key = NULL;

    /* category sync related */
    gchar     *catname, *catpath, *buff, **buffv;
    FILE      *catfile;
    gchar      category[80];
    gboolean   catsync = FALSE; /* whether .cat file should be synced */



    path = g_strconcat (dir, G_DIR_SEPARATOR_S, fnam, NULL);

    fp = g_fopen (path, "r");

    if (fp != NULL) {

        /* Prepare .cat file for sync while we read data */
        buffv = g_strsplit (fnam, ".", 0);
        catname = g_strconcat (buffv[0], ".cat", NULL);
        g_strfreev (buffv);
        catpath = sat_file_name (catname);
        g_free (catname);

        /* read category name for catfile */
        catfile = g_fopen (catpath, "r");
        if (catfile!=NULL) {
            b = fgets (category, 80, catfile);
            fclose (catfile);
            catsync = TRUE;
        }
        else {
            /* There is no category with this name (could be update from custom file) */
            sat_log_log (SAT_LOG_LEVEL_MSG,
                         _("%s:%s: There is no category called %s"),
                         __FILE__, __FUNCTION__, fnam);
        }

        /* reopen a new catfile and write category name */
        if (catsync) {
            catfile = g_fopen (catpath, "w");
            if (catfile != NULL) {
                fputs (category, catfile);
            }
            else {
                catsync = FALSE;
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s:%s: Could not reopen .cat file while reading TLE from %s"),
                             __FILE__, __FUNCTION__, fnam);
            }
            
            /* .cat file now contains the category name;
               satellite catnums will be added during update in the while loop */
        }
        

        /* read 3 lines at a time */
        while (fgets (tle_str[0], 80, fp)) {
            /* read second and third lines */
            b = fgets (tle_str[1], 80, fp);
            b = fgets (tle_str[2], 80, fp);

            tle_str[1][69] = '\0';
            tle_str[2][69] = '\0';

            /* copy catnum and convert to integer */
            for (i = 2; i < 7; i++) {
                catstr[i-2] = tle_str[1][i];
            }
            catstr[5] = '\0';
            catnr = (guint) g_ascii_strtod (catstr, NULL);


            if (Get_Next_Tle_Set (tle_str, &tle) != 1) {
                /* TLE data not good */
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s:%s: Invalid data for %d"),
                             __FILE__, __FUNCTION__, catnr);
            }
            else {

                if (catsync) {
                    /* store catalog number in catfile */
                    buff = g_strdup_printf ("%d\n", catnr);
                    fputs (buff, catfile);
                    g_free (buff);
                }

                /* add data to hash table */
                key = g_try_new0 (guint, 1);
                *key = catnr;

                /* check if satellite already in hash table */
                if (g_hash_table_lookup (data, key) == NULL) {

                    /* create new_tle structure */
                    ntle = g_try_new (new_tle_t, 1);
                    ntle->catnum = catnr;
                    ntle->epoch = tle.epoch;
                    ntle->satname = g_strdup (g_strchomp(tle_str[0]));
                    ntle->line1   = g_strdup (tle_str[1]);
                    ntle->line2   = g_strdup (tle_str[2]);
                    ntle->srcfile = g_strdup (fnam);
                    ntle->isnew   = TRUE; /* flag will be reset when using data */


                    g_hash_table_insert (data, key, ntle);
                    retcode++;
                }
                else {
                    g_free (key);
                }
            }

        }

        if (catsync) {
            /* close category file */
            fclose (catfile);
        }

        g_free (catpath);

        /* close input TLE file */
        fclose (fp);

    }

    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Failed to open %s"),
                     __FILE__, __FUNCTION__, path);
    }

    g_free (path);


    return retcode;
}



/** \brief Update TLE data in a file.
 *  \param ldname Directory name for gpredict tle files.
 *  \param fname The name of the TLE file.
 *  \param data The hash table containing the fresh data.
 *  \param sat_upd OUT: number of sats updated.
 *  \param sat_ski OUT: number of sats skipped.
 *  \param sat_nod OUT: number of sats for which no data found
 *  \param sat_tot OUT: total number of sats
 *
 * For each satellite in the TLE file ldname/fnam, this function
 * checks whether there is any newer data available in the hash table.
 * If yes, the function writes the fresh data to temp_file, if no, the
 * old data is copied to temp_file.
 * When all sats have been copied ldname/fnam is deleted and temp_file
 * is renamed to ldname/fnam.
 */
static void update_tle_in_file (const gchar *ldname,
                                const gchar *fname,
                                GHashTable  *data,
                                guint       *sat_upd,
                                guint       *sat_ski,
                                guint       *sat_nod,
                                guint       *sat_tot)
{
    gchar     *path;
    guint      updated = 0;  /* number of updated sats */
    guint      nodata  = 0;  /* no sats for which no fresh data available */
    guint      skipped = 0;  /* no. sats where fresh data is older */
    guint      total   = 0;  /* total no. of sats in gpredict tle file */
    gchar    **catstr;
    guint      catnr;
    guint     *key = NULL;
    tle_t      tle;
    new_tle_t *ntle;
    GError    *error = NULL;
    GKeyFile  *satdata;
    gchar     *tlestr1, *tlestr2, *rawtle;
    gchar     *cfgstr;
    GIOChannel *cfgfile;
    gsize      length, written;


    /* open input file (file containing old tle) */
    path = g_strconcat (ldname, G_DIR_SEPARATOR_S, fname, NULL);
    satdata = g_key_file_new ();
    if (!g_key_file_load_from_file (satdata, path, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Error loading %s (%s)"),
                     __FUNCTION__, path, error->message);
        g_clear_error (&error);

        skipped++;

    }

    else {

        /* get catalog number for this satellite */
        catstr = g_strsplit (fname, ".sat", 0);
        catnr = (guint) g_ascii_strtod (catstr[0], NULL);

        /* see if we have new data for this satellite */
        key = g_try_new0 (guint, 1);
        *key = catnr;
        ntle = (new_tle_t *) g_hash_table_lookup (data, key);
        g_free (key);

        if (ntle == NULL) {
            /* no new data found for this sat => obsolete */
            nodata++;

            /* check if obsolete sats should be deleted */
            /**** FIXME: This is dangereous, so we omit it */
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: No new TLE data found for %d. Satellite might be obsolete."),
                         __FUNCTION__, catnr);
        }
        else {
            /* This satellite is not new */
            ntle->isnew = FALSE;

            /* get TLE data */
            tlestr1 = g_key_file_get_string (satdata, "Satellite", "TLE1", NULL);
            tlestr2 = g_key_file_get_string (satdata, "Satellite", "TLE2", NULL);
            rawtle = g_strconcat (tlestr1, tlestr2, NULL);

            if (!Good_Elements (rawtle)) {
                sat_log_log (SAT_LOG_LEVEL_WARN,
                             _("%s: Current TLE data for %d appears to be bad"),
                             __FUNCTION__, catnr);
            }
            Convert_Satellite_Data (rawtle, &tle);

            g_free (tlestr1);
            g_free (tlestr2);
            g_free (rawtle);

            if (tle.epoch < ntle->epoch) {
                /* new data is newer than what we already have */
                /* store new data */
                g_key_file_set_string (satdata, "Satellite", "TLE1", ntle->line1);
                g_key_file_set_string (satdata, "Satellite", "TLE2", ntle->line2);

                /* convert configuration data struct to charachter string */
                cfgstr = g_key_file_to_data (satdata, &length, NULL); /* this function never reports error */

                /* create and open a file for writing */
                cfgfile = g_io_channel_new_file (path, "w", &error);

                if (error != NULL) {
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                 _("%s: Could not create satellite data file (%s)."),
                                 __FUNCTION__, error->message);
                    g_clear_error (&error);
                    skipped++;
                }
                else {
                    g_io_channel_write_chars (cfgfile, cfgstr, length, &written, &error);

                    g_io_channel_shutdown (cfgfile, TRUE, NULL);
                    g_io_channel_unref (cfgfile);

                    if (error != NULL) {
                        sat_log_log (SAT_LOG_LEVEL_ERROR,
                                     _("%s: Error writing satellite data (%s)."),
                                     __FUNCTION__, error->message);
                        g_clear_error (&error);
                        skipped++;
                    }
                    else if (length != written) {
                        sat_log_log (SAT_LOG_LEVEL_WARN,
                                     _("%s: Wrote only %d out of %d chars for satellite data."),
                                     __FUNCTION__, written, length);
                        skipped++;
                    }
                    else {
                        sat_log_log (SAT_LOG_LEVEL_MSG,
                                     _("%s: Satellite data written for %d."),
                                     __FUNCTION__, catnr);
                        updated++;
                    }
                }
                g_free (cfgstr);
            }
            else {
                skipped++;
            }
        }
        g_strfreev (catstr);
    }
    g_key_file_free (satdata);
    g_free (path);

    /* update out parameters */
    *sat_upd = updated;
    *sat_ski = skipped;
    *sat_nod = nodata;
    *sat_tot = total;

}





const gchar *freq_to_str[TLE_AUTO_UPDATE_NUM] = {
    N_("Never"),
    N_("Monthly"),
    N_("Weekly"),
    N_("Daily")
};

const gchar *
        tle_update_freq_to_str (tle_auto_upd_freq_t freq)
{
    if ((freq < TLE_AUTO_UPDATE_NEVER) ||
        (freq >= TLE_AUTO_UPDATE_NUM)) {

        freq = TLE_AUTO_UPDATE_NEVER;

    }

    return _(freq_to_str[freq]);
}
