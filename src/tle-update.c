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
#include "tle-lookup.h"
#include "tle-update.h"


/* Flag indicating whether TLE update is in progress.
   This should avoid multiple attempts to update TLE,
   e.g. user starts update from menubar while automatic
   update is in progress
*/
static gboolean tle_in_progress = FALSE;


/* private function prototypes */
#if 0
static void    copy_tle (tle_t *source, tle_t *target);
static gint    compare_epoch (tle_t *tle1, tle_t *tle2);
static tle_t  *gchar_to_tle (tle_type_t type, const gchar *lines);
static gchar  *tle_to_gchar (tle_type_t type, tle_t *tle);
#endif

static size_t  my_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream);
static gint    read_fresh_tle (const gchar *dir, const gchar *fnam, GHashTable *data);

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
void
tle_update_from_files (const gchar *dir, const gchar *filter,
                        gboolean silent, GtkWidget *progress,
                        GtkWidget *label1, GtkWidget *label2)
{
    GHashTable  *data;        /* hash table with fresh TLE data */
    GDir        *cache_dir;   /* directory to scan fresh TLE */
    GDir        *loc_dir;     /* directory for gpredict TLE files */
    GError      *err = NULL;
    gchar       *text;
    gchar       *ldname;
    const gchar *fnam;
    guint        num = 0;
    guint        updated,updated_tmp;
    guint        skipped,skipped_tmp;
    guint        nodata,nodata_tmp;
    guint        newsats = 0;
    guint        total,total_tmp;
    gdouble      fraction = 0.0;
    gdouble      start = 0.0;



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

        /* read fresh tle files into memory */
        while ((fnam = g_dir_read_name (cache_dir)) != NULL) {

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
                g_usleep (G_USEC_PER_SEC / 10);
            }

            /* now, do read the fresh data */
            num = read_fresh_tle (dir, fnam, data);
            
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

        /* store number of available sats */
        num = tle_lookup_count ();


        /* now it is time to  read gpredict-tle into memory, too */
        ldname = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
                                ".gpredict2", G_DIR_SEPARATOR_S,
                                "tle", NULL);


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

            /* update TLE files one by one */
            while ((fnam = g_dir_read_name (loc_dir)) != NULL) {

                /* skip 'cache' directory */
                if (g_ascii_strcasecmp (fnam, "cache")) {

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
                    total   += total_tmp;

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
                        g_usleep (G_USEC_PER_SEC / 10);
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

}


/** \brief Check if satellite is new, if so, add it to local database */
static void check_and_add_sat (gpointer key, gpointer value, gpointer user_data)
{
    gchar     *ofn;
    FILE      *fp;
    new_tle_t *ntle = (new_tle_t *) value;
    guint     *num = user_data;
    
    /* check if sat is new */
    if (ntle->isnew) {
        
        /* Open $HOME/.gpredict2/tle/new.tle */
        ofn = g_strconcat (g_get_home_dir(), G_DIR_SEPARATOR_S,
                           ".gpredict2", G_DIR_SEPARATOR_S,
                           "tle", G_DIR_SEPARATOR_S,
                           "new.tle", NULL);
        fp = g_fopen (ofn, "a");
        g_free (ofn);
        
        /* store tle data to file */
        if (fp) {
            fputs (ntle->satname, fp);
            fputs (ntle->line1, fp);
            fputs (ntle->line2, fp);
        
            /* close file */
            fclose (fp);
               
            /* update data */
            *num += 1;
        }
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
void 
tle_update_from_network (gboolean   silent,
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
    if (tle_in_progress)
        return;


    tle_in_progress = TRUE;

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

            /* create local file */
            locfile = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
                                    ".gpredict2", G_DIR_SEPARATOR_S,
                                    "tle", G_DIR_SEPARATOR_S,
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

            g_free (curfile);
            g_free (locfile);
            fclose (outfile);
        }

        curl_easy_cleanup (curl);

        /* continue update if we have fetched at least one file */
        if (success > 0) {
            /* call update_from_files */
            cache = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
                                    ".gpredict2", G_DIR_SEPARATOR_S,
                                    "tle", G_DIR_SEPARATOR_S,
                                    "cache", NULL);
            tle_update_from_files (cache, NULL, silent, progress, label1, label2);
            g_free (cache);
        }
                                
    }

    /* clear cache and memory */
    g_free (server);
    g_strfreev (files);
    g_free (files_tmp);
    if (proxy != NULL)
        g_free (proxy);

    /* open cache */
    cache = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S,
                            ".gpredict2", G_DIR_SEPARATOR_S,
                            "tle", G_DIR_SEPARATOR_S,
                            "cache", NULL);
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
    tle_in_progress = FALSE;

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
static size_t
my_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    /*** FIXME: TBC whether this works in wintendo */
    return fwrite (ptr, size, nmemb, stream);
}



/** \brief Read fresh TLE data into hash table.
 *  \param dir The directory to read from.
 *  \param fnam The name of the file to read from.
 *  \param fresh_data Hash table where the data should be stored.
 *  \return The number of satellites successfully read.
 */
static gint
read_fresh_tle (const gchar *dir, const gchar *fnam, GHashTable *data)
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


    path = g_strconcat (dir, G_DIR_SEPARATOR_S, fnam, NULL);

    fp = g_fopen (path, "r");

    if (fp != NULL) {

        /* read 3 lines at a time */
        while (fgets (tle_str[0], 80, fp)) {
            /* read second and third lines */
            b = fgets (tle_str[1], 80, fp);
            b = fgets (tle_str[2], 80, fp);

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
                /* DATA OK, phew... */
    /* 				sat_log_log (SAT_LOG_LEVEL_DEBUG, */
    /* 						     _("%s: Good data for %d"), */
    /* 						     __FUNCTION__, */
    /* 						     catnr); */

                /* add data to hash table */
                key = g_try_new0 (guint, 1);
                *key = catnr;


                if (g_hash_table_lookup (data, key) == NULL) {

                    /* create new_tle structure */
                    ntle = g_try_new (new_tle_t, 1);
                    ntle->epoch = tle.epoch;
                    ntle->satname = g_strdup (tle_str[0]);
                    ntle->line1   = g_strdup (tle_str[1]);
                    ntle->line2   = g_strdup (tle_str[2]);
                    ntle->isnew   = TRUE; /* flag will be reset when using data */
                    

                    g_hash_table_insert (data, key, ntle);
                    retcode++;
                }
                else {
                    g_free (key);
                }
            }

        }

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
static void
update_tle_in_file (const gchar *ldname,
                    const gchar *fname,
                    GHashTable  *data,
                    guint       *sat_upd,
                    guint       *sat_ski,
                    guint       *sat_nod,
                    guint       *sat_tot)
{
    gchar     *path;
    gchar     *tmp_path;
    guint      updated = 0;  /* number of updated sats */
    guint      nodata  = 0;  /* no sats for which no fresh data available */
    guint      skipped = 0;  /* no. sats where fresh data is older */
    guint      total   = 0;  /* total no. of sats in gpredict tle file */
    gchar      tle_str[3][80];
    gchar      catstr[6];
    gchar     *b;
    FILE      *fp;
    FILE      *tmp;
    guint      catnr,i;
    guint     *key = NULL;
    tle_t      tle;
    new_tle_t *ntle;
    gint       retcode = 0;


    /* open input file (file containing old tle) */
    path = g_strconcat (ldname, G_DIR_SEPARATOR_S, fname, NULL);
    fp = g_fopen (path, "r");

    if (fp != NULL) {

        /* open output temp file */
        tmp_path = g_strconcat (ldname, G_DIR_SEPARATOR_S, "TLEUPDATE.tmp", NULL);
        tmp = g_fopen (tmp_path, "w");

        if (tmp != NULL) {

            /* read 3 lines at a time */
            while (fgets (tle_str[0], 80, fp)) {
                /* read second and third lines */
                b = fgets (tle_str[1], 80, fp);
                b = fgets (tle_str[2], 80, fp);

                /* stats */
                total++;

                /* copy catnum and convert to integer */
                for (i = 2; i < 7; i++) {
                    catstr[i-2] = tle_str[1][i];
                }
                catstr[5] = '\0';
                catnr = (guint) g_ascii_strtod (catstr, NULL);


                if (Get_Next_Tle_Set (tle_str, &tle) != 1) {
                    /* TLE data not good */
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                    _("%s:%s: Original data for %d seems to be bad"),
                                    __FILE__, __FUNCTION__, catnr);
                }
            
                /* get data from hash table */
                key = g_try_new0 (guint, 1);
                *key = catnr;
                ntle = (new_tle_t *) g_hash_table_lookup (data, key);
                g_free (key);

                if (ntle == NULL) {

                    /* no new data found for this sat => obsolete */
                    nodata++;
                    
                    /* check if obsolete sats should be deleted */
                    /**** FIXME: This is dangereous, so we omit it */

                }
                else {

                    /* new data is available for this sat */
                    if (tle.epoch < ntle->epoch) {
                        /* fresh data is newer than original
                            => update
                        */
                        updated++;

                        /* write new data to file */
                        fputs (ntle->satname, tmp);
                        fputs (ntle->line1, tmp);
                        fputs (ntle->line2, tmp);

                    }
                    else {
                        /* fresh data is older than original
                            => don't update
                        */
                        skipped++;

                        /* write current data back to tle file */
                        fputs (tle_str[0], tmp);
                        fputs (tle_str[1], tmp);
                        fputs (tle_str[2], tmp);
                    }
                    
                    /* clear isnew flag */
                    ntle->isnew = FALSE;

                }

            }

            fclose (tmp);
            fclose (fp);

            /* if there is at least one updated sat */
            if (updated > 0) {

                /* remove old tle file fp */
                retcode = g_remove (path);

                if (retcode != 0) {
                    /* could not remove file */
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                    _("%s: Could not remove %s (file open?)"),
                                    __FUNCTION__, path);
                }

                /* rename temp file tmp to old tle file */
                retcode = g_rename (tmp_path, path);

                if (retcode != 0) {
                    /* could not rename file */
                    sat_log_log (SAT_LOG_LEVEL_ERROR,
                                    _("%s: Could not rename %s to %s"),
                                    __FUNCTION__, tmp_path, path);
                }

            }
            else {
                /* we have to remove the temp file */
                g_remove (tmp_path);
            }

        }
        else {
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                            _("%s: Failed to open temp file %s"),
                            __FUNCTION__, tmp_path);

            /* close input TLE file */
            fclose (fp);
        }

        /* close input TLE file */
        //fclose (fp);

        /* free path to tmp file */
        g_free (tmp_path);

        /* print stats */
        sat_log_log (SAT_LOG_LEVEL_MSG,
                        _("%s: Update statistics for %s (U/O/N/T): %d/%d/%d/%d"),
                        __FUNCTION__, fname,
                        updated, skipped, nodata, total);

    }

    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                        _("%s: Failed to open %s"),
                        __FUNCTION__, path);
    }

    g_free (path);

    /* update out parameters */
    *sat_upd = updated;
    *sat_ski = skipped;
    *sat_nod = nodata;
    *sat_tot = total;

}



#if 0
/** \brief Copy a TLE structure.
 *  \param source The TLE structure to copy from.
 *  \param target The TLE structure to copy to.
 *
 */
static void
copy_tle (tle_t *source, tle_t *target)
{

	target->epoch = source->epoch;
	target->epoch_year = source->epoch_year;
	target->epoch_day = source->epoch_day;
	target->epoch_fod = source->epoch_fod;
	target->xndt2o = source->xndt2o;
	target->xndd6o = source->xndd6o;
	target->bstar = source->bstar;
	target->xincl = source->xincl;
	target->xnodeo = source->xnodeo;
	target->eo = source->eo;
	target->omegao = source->omegao;
	target->xmo = source->xmo;
	target->xno = source->xno;
	/*target->catnr = source->catnr;*/
	target->elset = source->elset;
	target->revnum = source->revnum;
	target->status = source->status;

	/*
	target->xincl1 = source->xincl1;
	target->xnodeo1 = source->xnodeo1;
	target->omegao1 = source->omegao1;
	*/
}


/** \brief Compare EPOCH of two element sets
 *  \param tle1 The first element set.
 *  \param tle2 The second element set.
 *  \return 0 if the epochs are equal, -1 if tle1->epoch < tle2->epoch, and
 *          +1 if tle1->epoch > tle2->epoch.
 */
static gint
compare_epoch (tle_t *tle1, tle_t *tle2)
{
	gint retval = 0;


	if (tle1->epoch > tle2->epoch)
		retval = 1;
	else if (tle1->epoch < tle2->epoch)
		retval = -1;
	else
		retval = 0;


	return retval;
}


static tle_t *
gchar_to_tle (tle_type_t type, const gchar *lines)
{
	return NULL;
}


static gchar *
tle_to_gchar (tle_type_t type, tle_t *tle)
{
    return NULL;
}
#endif





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
