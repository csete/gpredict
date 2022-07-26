/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.
    Copyright (C)  2009 Charles Suprin AA1VS.

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
#include <gtk/gtk.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#ifdef WIN32
#include "win32-fetch.h"
#else
#include <curl/curl.h>
#endif

#include "compat.h"
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "tle-update.h"


/* private function prototypes */
#ifndef WIN32
static size_t   my_write_func(void *ptr, size_t size, size_t nmemb,
                              FILE * stream);
#endif
static gint     read_fresh_tle(const gchar * dir, const gchar * fnam,
                               GHashTable * data);
static gboolean is_tle_file(const gchar * dir, const gchar * fnam);


static void     update_tle_in_file(const gchar * ldname,
                                   const gchar * fname,
                                   GHashTable * data,
                                   guint * sat_upd,
                                   guint * sat_ski,
                                   guint * sat_nod, guint * sat_tot);

static guint    add_new_sats(GHashTable * data);
static gboolean is_computer_generated_name(gchar * satname);


/** Free a new_tle_t structure. */
static void free_new_tle(gpointer data)
{
    new_tle_t      *tle;

    tle = (new_tle_t *) data;

    g_free(tle->satname);
    g_free(tle->line1);
    g_free(tle->line2);
    g_free(tle->srcfile);
    g_free(tle);
}


/**
 * Update TLE files from local files.
 *
 * @param dir Directory where files are located.
 * @param filter File filter, e.g. *.txt (not used at the moment!)
 * @param silent TRUE if function should execute without graphical status indicator.
 * @param label1 Activity label (can be NULL)
 * @param label2 Statistics label (can be NULL)
 * @param progress Pointer to progress indicator.
 * @param init_prgs Initial value of progress indicator, e.g 0.5 if we are updating
 *                  from network.
 *
 * This function is used to update the TLE data from local files.
 */
void tle_update_from_files(const gchar * dir, const gchar * filter,
                           gboolean silent, GtkWidget * progress,
                           GtkWidget * label1, GtkWidget * label2)
{
    static GMutex   tle_file_in_progress;

    GHashTable     *data;       /* hash table with fresh TLE data */
    GDir           *cache_dir;  /* directory to scan fresh TLE */
    GDir           *loc_dir;    /* directory for gpredict TLE files */
    GError         *err = NULL;
    gchar          *text;
    gchar          *ldname;
    gchar          *userconfdir;
    const gchar    *fnam;
    guint           num = 0;
    guint           updated, updated_tmp;
    guint           skipped, skipped_tmp;
    guint           nodata, nodata_tmp;
    guint           newsats = 0;
    guint           total, total_tmp;
    gdouble         fraction = 0.0;
    gdouble         start = 0.0;

    (void)filter;

    if (g_mutex_trylock(&tle_file_in_progress) == FALSE)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _
                    ("%s: A TLE update process is already running. Aborting."),
                    __func__);
        return;
    }

    /* create hash table */
    data = g_hash_table_new_full(g_int_hash, g_int_equal, g_free,
                                 free_new_tle);

    /* open directory and read files one by one */
    cache_dir = g_dir_open(dir, 0, &err);

    if (err != NULL)
    {
        /* send an error message */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error opening directory %s (%s)"),
                    __func__, dir, err->message);

        /* insert error message into the status string, too */
        if (!silent && (label1 != NULL))
        {
            text = g_strdup_printf(_("<b>ERROR</b> opening directory %s\n%s"),
                                   dir, err->message);

            gtk_label_set_markup(GTK_LABEL(label1), text);
            g_free(text);
        }

        g_clear_error(&err);
        err = NULL;
    }
    else
    {
        /* scan directory for tle files */
        while ((fnam = g_dir_read_name(cache_dir)) != NULL)
        {
            /* check that we got a TLE file */
            if (is_tle_file(dir, fnam))
            {
                /* status message */
                if (!silent && (label1 != NULL))
                {
                    text = g_strdup_printf(_("Reading data from %s"), fnam);
                    gtk_label_set_text(GTK_LABEL(label1), text);
                    g_free(text);

                    /* Force the drawing queue to be processed otherwise there will
                       not be any visual feedback, ie. frozen GUI
                       - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                     */
                    while (g_main_context_iteration(NULL, FALSE));

                    /* give user a chance to follow progress */
                    g_usleep(G_USEC_PER_SEC / 100);
                }

                /* now, do read the fresh data */
                num = read_fresh_tle(dir, fnam, data);
            }
            else
            {
                num = 0;
            }

            if (num < 1)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: No valid TLE data found in %s"),
                            __func__, fnam);
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_INFO,
                            _("%s: Read %d sats from %s into memory"),
                            __func__, num, fnam);
            }
        }

        /* close directory since we don't need it anymore */
        g_dir_close(cache_dir);

        /* now we load each .sat file and update if we have new data */
        userconfdir = get_user_conf_dir();
        ldname = g_strconcat(userconfdir, G_DIR_SEPARATOR_S, "satdata", NULL);
        g_free(userconfdir);

        /* open directory and read files one by one */
        loc_dir = g_dir_open(ldname, 0, &err);

        if (err != NULL)
        {
            /* send an error message */
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error opening directory %s (%s)"),
                        __func__, dir, err->message);

            /* insert error message into the status string, too */
            if (!silent && (label1 != NULL))
            {
                text =
                    g_strdup_printf(_("<b>ERROR</b> opening directory %s\n%s"),
                                    dir, err->message);

                gtk_label_set_markup(GTK_LABEL(label1), text);
                g_free(text);
            }

            g_clear_error(&err);
            err = NULL;
        }
        else
        {
            /* clear statistics */
            updated = 0;
            skipped = 0;
            nodata = 0;
            total = 0;

            /* get initial value of progress indicator */
            if (progress != NULL)
                start =
                    gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progress));

            /* This is insane but I don't know how else to count the number of sats */
            num = 0;
            while ((fnam = g_dir_read_name(loc_dir)) != NULL)
            {
                /* only consider .sat files */
                if (g_str_has_suffix(fnam, ".sat"))
                {
                    num++;
                }
            }

            g_dir_rewind(loc_dir);

            /* update TLE files one by one */
            while ((fnam = g_dir_read_name(loc_dir)) != NULL)
            {
                /* only consider .sat files */
                if (g_str_has_suffix(fnam, ".sat"))
                {
                    /* clear stat bufs */
                    updated_tmp = 0;
                    skipped_tmp = 0;
                    nodata_tmp = 0;
                    total_tmp = 0;

                    /* update TLE data in this file */
                    update_tle_in_file(ldname, fnam, data,
                                       &updated_tmp,
                                       &skipped_tmp, &nodata_tmp, &total_tmp);

                    /* update statistics */
                    updated += updated_tmp;
                    skipped += skipped_tmp;
                    nodata += nodata_tmp;
                    total = updated + skipped + nodata;

                    if (!silent)
                    {
                        if (label1 != NULL)
                        {
                            gtk_label_set_text(GTK_LABEL(label1),
                                               _("Updating data..."));
                        }

                        if (label2 != NULL)
                        {
                            text =
                                g_strdup_printf(_
                                                ("Satellites updated:\t %d\n"
                                                 "Satellites skipped:\t %d\n"
                                                 "Missing Satellites:\t %d\n"),
                                                updated, skipped, nodata);
                            gtk_label_set_text(GTK_LABEL(label2), text);
                            g_free(text);
                        }

                        if (progress != NULL)
                        {
                            /* two different calculations for completeness depending on whether 
                               we are adding new satellites or not. */
                            if (sat_cfg_get_bool(SAT_CFG_BOOL_TLE_ADD_NEW))
                            {
                                /* In this case we are possibly processing more than num satellites
                                   How many more? We do not know yet.  Worst case is g_hash_table_size more.

                                   As we update skipped and updated we can reduce the denominator count
                                   as those are in both pools (files and hash table). When we have processed 
                                   all the files, updated and skipped are completely correct and the progress 
                                   is correct. It may be correct sooner if the missed satellites are the 
                                   last files to process.

                                   Until then, if we eliminate the ones that are updated and skipped from being 
                                   double counted, our progress will shown will always be less or equal to our 
                                   true progress since the denominator will be larger than is correct.

                                   Advantages to this are that the progress bar does not stall close to 
                                   finished when there are a large number of new satellites.
                                 */
                                fraction =
                                    start + (1.0 -
                                             start) * ((gdouble) total) /
                                    ((gdouble) num + g_hash_table_size(data) -
                                     updated - skipped);
                            }
                            else
                            {
                                /* here we only process satellites we have have files for so divide by num */
                                fraction =
                                    start + (1.0 -
                                             start) * ((gdouble) total) /
                                    ((gdouble) num);
                            }
                            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
                                                          (progress),
                                                          fraction);

                        }

                        /* update the gui only every so often to speed up the process */
                        /* 47 was selected empirically to balance the update looking smooth but not take too much time. */
                        /* it also tumbles all digits in the numbers so that there is no obvious pattern. */
                        /* on a developer machine this improved an update from 5 minutes to under 20 seconds. */
                        if (total % 47 == 0)
                        {
                            /* Force the drawing queue to be processed otherwise there will
                               not be any visual feedback, ie. frozen GUI
                               - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                             */
                            while (g_main_context_iteration(NULL, FALSE));

                            /* give user a chance to follow progress */
                            g_usleep(G_USEC_PER_SEC / 1000);
                        }
                    }
                }
            }

            /* force gui update */
            while (g_main_context_iteration(NULL, FALSE));

            /* close directory handle */
            g_dir_close(loc_dir);

            /* see if we have any new sats that need to be added */
            if (sat_cfg_get_bool(SAT_CFG_BOOL_TLE_ADD_NEW))
            {
                newsats = add_new_sats(data);

                if (!silent && (label2 != NULL))
                {
                    text = g_strdup_printf(_("Satellites updated:\t %d\n"
                                             "Satellites skipped:\t %d\n"
                                             "Missing Satellites:\t %d\n"
                                             "New Satellites:\t\t %d"),
                                           updated, skipped, nodata, newsats);
                    gtk_label_set_text(GTK_LABEL(label2), text);
                    g_free(text);
                }

                sat_log_log(SAT_LOG_LEVEL_INFO,
                            _("%s: Added %d new satellites to local database"),
                            __func__, newsats);
            }

            /* store time of update if we have updated something */
            if ((updated > 0) || (newsats > 0))
            {
		gint64          now;

		now = g_get_real_time() / G_USEC_PER_SEC;
                sat_cfg_set_int(SAT_CFG_INT_TLE_LAST_UPDATE, now);
            }
        }

        g_free(ldname);
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: TLE elements updated."), __func__);
    }

    /* destroy hash tables */
    g_hash_table_destroy(data);
    g_mutex_unlock(&tle_file_in_progress);
}


/** Check if satellite is new, if so, add it to local database */
static void check_and_add_sat(gpointer key, gpointer value, gpointer user_data)
{
    new_tle_t      *ntle = (new_tle_t *) value;
    guint          *num = user_data;
    GKeyFile       *satdata;
    gchar          *cfgfile;

    (void)key;

    /* check if sat is new */
    if (!ntle->isnew)
        return;

    /* create config data */
    satdata = g_key_file_new();

    /* store data */
    g_key_file_set_string(satdata, "Satellite", "VERSION", "1.1");
    g_key_file_set_string(satdata, "Satellite", "NAME", ntle->satname);
    g_key_file_set_string(satdata, "Satellite", "NICKNAME", ntle->satname);
    g_key_file_set_string(satdata, "Satellite", "TLE1", ntle->line1);
    g_key_file_set_string(satdata, "Satellite", "TLE2", ntle->line2);
    g_key_file_set_integer(satdata, "Satellite", "STATUS", ntle->status);

    /* create an I/O channel and store data */
    cfgfile = sat_file_name_from_catnum(ntle->catnum);
    if (!gpredict_save_key_file(satdata, cfgfile))
        *num += 1;

    /* clean up memory */
    g_free(cfgfile);
    g_key_file_free(satdata);
}

/** Add new satellites to local database */
static guint add_new_sats(GHashTable * data)
{
    guint           num = 0;

    g_hash_table_foreach(data, check_and_add_sat, &num);

    return num;
}

/**
 * Update TLE files from network.
 *
 * @param silent TRUE if function should execute without graphical status indicator.
 * @param progress Pointer to a GtkProgressBar progress indicator (can be NULL)
 * @param label1 GtkLabel for activity string.
 * @param label2 GtkLabel for statistics string.
 */
void tle_update_from_network(gboolean silent,
                             GtkWidget * progress,
                             GtkWidget * label1, GtkWidget * label2)
{
    static GMutex   tle_in_progress;

    gchar          *proxy = NULL;
    gchar          *files_tmp;
    gchar         **files;
    guint           numfiles, i;
    gchar          *curfile;
    gchar          *locfile;
    gchar          *userconfdir;
#ifdef WIN32
    int             res;
#else
    CURL           *curl;
    CURLcode        res;
#endif
    gdouble         fraction, start = 0;
    FILE           *outfile;
    GDir           *dir;
    gchar          *cache;
    const gchar    *fname;
    gchar          *text;
    GError         *err = NULL;
    guint           success = 0;        /* no. of successful downloads */

    /* bail out if we are already in an update process */
    if (g_mutex_trylock(&tle_in_progress) == FALSE)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: TLE update is already running. Aborting."),
                    __func__);

        return;
    }

    /* get server, proxy, and list of files */
    proxy = sat_cfg_get_str(SAT_CFG_STR_TLE_PROXY);

    /* avoid empty proxy string (bug in <2.2) */
    if (proxy != NULL && strlen(proxy) == 0)
    {
        sat_cfg_reset_str(SAT_CFG_STR_TLE_PROXY);
        g_free(proxy);
        proxy = NULL;
    }

    files_tmp = sat_cfg_get_str(SAT_CFG_STR_TLE_URLS);
    files = g_strsplit(files_tmp, ";", 0);
    numfiles = g_strv_length(files);

    if (numfiles < 1)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: No files to fetch from network."), __func__);

        /* set activity string, so user knows why nothing happens */
        if (!silent && (label1 != NULL))
        {
            gtk_label_set_text(GTK_LABEL(label1),
                               _("No files to fetch from network"));
        }
    }
    else
    {
        /* initialise progress bar */
        if (!silent && (progress != NULL))
            start = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progress));

#ifndef WIN32
        /* initialise curl */
        curl = curl_easy_init();
        if (proxy != NULL)
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "gpredict/curl");
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
#endif

        /* get files */
        for (i = 0; i < numfiles; i++)
        {
            /* set URL */
            curfile = g_strdup(files[i]);
#ifndef WIN32
            curl_easy_setopt(curl, CURLOPT_URL, curfile);
#endif

            /* set activity message */
            if (!silent && (label1 != NULL))
            {

                text = g_strdup_printf(_("Fetching %s"), files[i]);
                gtk_label_set_text(GTK_LABEL(label1), text);
                g_free(text);

                /* Force the drawing queue to be processed otherwise there will
                   not be any visual feedback, ie. frozen GUI
                   - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                 */
                while (g_main_context_iteration(NULL, FALSE));
            }

            /* create local cache file ~/.config/Gpredict/satdata/cache/file-%d.tle */
            userconfdir = get_user_conf_dir();
            locfile = g_strdup_printf("%s%ssatdata%scache%sfile-%d.tle",
                                      userconfdir, G_DIR_SEPARATOR_S,
                                      G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S, i);
            outfile = g_fopen(locfile, "wb");
            if (outfile != NULL)
            {
#ifdef WIN32
                res = win32_fetch(curfile, outfile, proxy, "gpredict/win32");
                if (res != 0)
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Error fetching %s (%x)"),
                                __func__, curfile, res);
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_INFO,
                                _("%s: Successfully fetched %s"),
                                __func__, curfile);
                    success++;
                }
#else
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                /* get file */
                res = curl_easy_perform(curl);

                if (res != CURLE_OK)
                {
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Error fetching %s (%s)"),
                                __func__, curfile, curl_easy_strerror(res));
                }
                else
                {
                    sat_log_log(SAT_LOG_LEVEL_INFO,
                                _("%s: Successfully fetched %s"),
                                __func__, curfile);
                    success++;
                }
#endif
                fclose(outfile);
            }
            else
            {
                sat_log_log(SAT_LOG_LEVEL_INFO,
                            _("%s: Failed to open %s preventing update"),
                            __func__, locfile);
            }
            /* update progress indicator */
            if (!silent && (progress != NULL))
            {

                /* complete download corresponds to 50% */
                fraction = start + (0.5 - start) * i / (1.0 * numfiles);
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress),
                                              fraction);

                /* Force the drawing queue to be processed otherwise there will
                   not be any visual feedback, ie. frozen GUI
                   - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                 */
                while (g_main_context_iteration(NULL, FALSE));
            }

            g_free(userconfdir);
            g_free(curfile);
            g_free(locfile);
        }

#ifndef WIN32
        curl_easy_cleanup(curl);
#endif

        /* continue update if we have fetched at least one file */
        if (success > 0)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Fetched %d files from network; updating..."),
                        __func__, success);
            /* call update_from_files */
            cache = sat_file_name("cache");
            tle_update_from_files(cache, NULL, silent, progress, label1,
                                  label2);
            g_free(cache);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _
                        ("%s: Could not fetch TLE files from network. Aborting."),
                        __func__);
        }

    }

    /* clear cache and memory */
    g_strfreev(files);
    g_free(files_tmp);
    if (proxy != NULL)
        g_free(proxy);

    /* open cache */
    cache = sat_file_name("cache");
    dir = g_dir_open(cache, 0, &err);

    if (err != NULL)
    {
        /* send an error message */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Error opening %s (%s)"),
                    __func__, dir, err->message);
        g_clear_error(&err);
    }
    else
    {
        /* delete files in cache one by one */
        while ((fname = g_dir_read_name(dir)) != NULL)
        {

            locfile = g_strconcat(cache, G_DIR_SEPARATOR_S, fname, NULL);
            if (g_remove(locfile))
                    sat_log_log(SAT_LOG_LEVEL_ERROR,
                                _("%s: Failed to remove %s"), __func__, locfile);
            g_free(locfile);
        }
        /* close cache */
        g_dir_close(dir);
    }

    g_free(cache);
    g_mutex_unlock(&tle_in_progress);
}

#ifndef WIN32
/**
 * Write TLE data block to file.
 *
 * @param ptr Pointer to the data block to be written.
 * @param size Size of data block.
 * @param nmemb Size multiplier?
 * @param stream Pointer to the file handle.
 * @return The number of bytes actually written.
 *
 * This function writes the received data to the file pointed to by stream.
 * It is used as write callback by to curl exec function.
 */
static size_t my_write_func(void *ptr, size_t size, size_t nmemb,
                            FILE * stream)
{
    /*** FIXME: TBC whether this works in wintendo */
    return fwrite(ptr, size, nmemb, stream);
}
#endif

/**
 * Check whether file is TLE file.
 * @param dir The directory.
 * @param fnam The file name.
 * 
 * This function checks whether the file with path dir/fnam is a potential
 * TLE file. Checks performed:
 *   - It is a real file
 *   - suffix is .txt or .tle
 */
static gboolean is_tle_file(const gchar * dir, const gchar * fnam)
{
    gchar          *path;
    gchar          *fname_lower;
    gboolean        fileIsOk = FALSE;

    path = g_strconcat(dir, G_DIR_SEPARATOR_S, fnam, NULL);
    fname_lower = g_ascii_strdown(fnam, -1);

    if (g_file_test(path, G_FILE_TEST_IS_REGULAR) &&
        (g_str_has_suffix(fname_lower, ".tle") ||
         g_str_has_suffix(fname_lower, ".txt")))
    {
        fileIsOk = TRUE;
    }
    g_free(fname_lower);
    g_free(path);

    return fileIsOk;
}

/**
 * Read fresh TLE data into hash table.
 *
 * @param dir The directory to read from.
 * @param fnam The name of the file to read from.
 * @param fresh_data Hash table where the data should be stored.
 * @return The number of satellites successfully read.
 * 
 * This function will read fresh TLE data from local files into memory.
 * If there is a saetllite category (.cat file) with the same name as the
 * input file it will also update the satellites in that category.
 */
static gint read_fresh_tle(const gchar * dir, const gchar * fnam,
                           GHashTable * data)
{
    new_tle_t      *ntle;
    tle_t           tle;
    gchar          *path;
    gchar           tle_str[3][80];
    gchar           tle_working[3][80];
    gchar           linetmp[80];
    guint           linesneeded = 3;
    gchar           catstr[6];
    gchar           idstr[7] = "\0\0\0\0\0\0\0", idyearstr[3];
    gchar          *b;
    FILE           *fp;
    gint            retcode = 0;
    guint           catnr, i, idyear;
    guint          *key = NULL;

    /* category sync related */
    gchar          *catname, *catpath, *buff, **buffv;
    FILE           *catfile;
    gchar           category[80];
    gboolean        catsync = FALSE;    /* whether .cat file should be synced. NB: not effective since 1.4 */

    /* 
       Normal cases to check
       1. 3 line tle file as in amateur.txt from celestrak
       2. 2 line tle file as in .... from celestrak

       corner cases to check 
       1. 3 line tle with something at the end. (nasa.all from amsat)
       2. 2 line tle with only one in the file
       3. 2 line tle file reading the last one.
     */

    path = g_strconcat(dir, G_DIR_SEPARATOR_S, fnam, NULL);
    fp = g_fopen(path, "r");
    if (fp != NULL)
    {
        /* Prepare .cat file for sync while we read data */
        buffv = g_strsplit(fnam, ".", 0);
        catname = g_strconcat(buffv[0], ".cat", NULL);
        g_strfreev(buffv);
        catpath = sat_file_name(catname);
        g_free(catname);

        /* read category name for catfile */
        catfile = g_fopen(catpath, "r");
        if (catfile != NULL)
        {
            b = fgets(category, 80, catfile);
            if (b == NULL)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s:%s: There is no category in %s"),
                            __FILE__, __func__, catpath);
            }
            fclose(catfile);
            catsync = TRUE;
        }
        else
        {
            /* There is no category with this name (could be update from custom file) */
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s:%s: There is no category called %s"),
                        __FILE__, __func__, fnam);
        }

        /* reopen a new catfile and write category name */
        if (catsync)
        {
            catfile = g_fopen(catpath, "w");
            if (catfile != NULL)
            {
                fputs(category, catfile);
            }
            else
            {
                catsync = FALSE;
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _
                            ("%s:%s: Could not reopen .cat file while reading TLE from %s"),
                            __FILE__, __func__, fnam);
            }

            /* .cat file now contains the category name;
               satellite catnums will be added during update in the while loop */
        }

        /* set b to non-null as a flag */
        b = path;

        /* read lines from tle file */
        while (fgets(linetmp, 80, fp))
        {
            /*read in the number of lines needed to potentially get to a new tle */
            switch (linesneeded)
            {
            case 3:
                strncpy(tle_working[0], linetmp, 80);
                tle_working[0][79] = 0;         // make coverity happy
                /* b is being used a flag here
                   if b==NULL then we only have one line read in 
                   and there is no way we have a full tle as there 
                   is only one line in the buffer.
                   A TLE must be two or three lines.
                 */
                b = fgets(tle_working[1], 80, fp);
                if (b == NULL)
                {
                    /* make sure there is no junk in tle_working[1] */
                    tle_working[1][0] = '\0';
                    break;
                }
                /* make sure there is no junk in tle_working[2] */
                if (fgets(tle_working[2], 80, fp) == NULL)
                {
                    tle_working[2][0] = '\0';
                }

                break;
            case 2:
                strncpy(tle_working[0], tle_working[2], 80);
                strncpy(tle_working[1], linetmp, 80);
                /* make sure there is no junk in tle_working[2] */
                if (fgets(tle_working[2], 80, fp) == NULL)
                {
                    tle_working[2][0] = '\0';
                }
                break;
            case 1:
                strncpy(tle_working[0], tle_working[1], 80);
                strncpy(tle_working[1], tle_working[2], 80);
                strncpy(tle_working[2], linetmp, 80);
                tle_working[2][79] = 0;         // make coverity happy
                break;
            default:
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _
                            ("%s:%s: Something wrote linesneeded to an illegal value %d"),
                            __FILE__, __func__, linesneeded);
                break;
            }
            /* b can only be null if there is only one line in the buffer */
            /* a tle must be two or three */
            if (b == NULL)
            {
                break;
            }
            /* remove leading and trailing whitespace to be more forgiving */
            g_strstrip(tle_working[0]);
            g_strstrip(tle_working[1]);
            g_strstrip(tle_working[2]);

            /* there are three possibilities at this point */
            /* first is that line 0 is a name and normal text for three line element and that lines 1 and 2 
               are the corresponding tle */
            /* second is that line 0 and line 1 are a tle for a bare tle */
            /* third is that neither of these is true and we are consuming either text at the top of the 
               file or a text file that happens to be in the update directory 
             */
            if ((tle_working[1][0] == '1') &&
                (tle_working[2][0] == '2') &&
                Checksum_Good(tle_working[1]) && Checksum_Good(tle_working[2]))
            {
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s:%s: Processing a three line TLE"),
                            __FILE__, __func__);

                /* it appears that the first line may be a name followed by a tle */
                strncpy(tle_str[0], tle_working[0], 80);
                tle_str[0][79] = 0;         // make coverity happy
                strncpy(tle_str[1], tle_working[1], 80);
                strncpy(tle_str[2], tle_working[2], 80);
                /* we consumed three lines so we need three lines */
                linesneeded = 3;

            }
            else if ((tle_working[0][0] == '1') &&
                     (tle_working[1][0] == '2') &&
                     Checksum_Good(tle_working[0]) &&
                     Checksum_Good(tle_working[1]))
            {
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s:%s: Processing a bare two line TLE"),
                            __FILE__, __func__);

                /* first line appears to belong to the start of bare TLE */
                /* put in a dummy name of form yyyy-nnaa base on international id */
                /* this special form will be overwritten if a three line tle ever has another name */

                strncpy(idstr, &tle_working[0][11], 6);
                g_strstrip(idstr);
                strncpy(idyearstr, &tle_working[0][9], 2);
                idstr[6] = '\0';
                idyearstr[2] = '\0';
                idyear = g_ascii_strtod(idyearstr, NULL);

                /* there is a two digit year field that started around sputnik */
                if (idyear >= 57)
                    idyear += 1900;
                else
                    idyear += 2000;

                snprintf(tle_str[0], 79, "%d-%s", idyear, idstr);
                strncpy(tle_str[1], tle_working[0], 80);
                strncpy(tle_str[2], tle_working[1], 80);

                /* we consumed two lines so we need two lines */
                linesneeded = 2;
            }
            else
            {
                /* we appear to have junk 
                   read another line in and do nothing else */
                linesneeded = 1;
                /* skip back to beginning of loop */
                continue;
            }


            tle_str[1][69] = '\0';
            tle_str[2][69] = '\0';

            /* copy catnum and convert to integer */
            for (i = 2; i < 7; i++)
            {
                catstr[i - 2] = tle_str[1][i];
            }
            catstr[5] = '\0';
            catnr = (guint) g_ascii_strtod(catstr, NULL);


            if (Get_Next_Tle_Set(tle_str, &tle) != 1)
            {
                /* TLE data not good */
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s:%s: Invalid data for %d"),
                            __FILE__, __func__, catnr);
            }
            else
            {
                if (catsync)
                {
                    /* store catalog number in catfile */
                    buff = g_strdup_printf("%d\n", catnr);
                    fputs(buff, catfile);
                    g_free(buff);
                }

                /* add data to hash table */
                key = g_try_new0(guint, 1);
                *key = catnr;

                ntle = g_hash_table_lookup(data, key);

                /* check if satellite already in hash table */
                if (ntle == NULL)
                {

                    /* create new_tle structure */
                    ntle = g_try_new(new_tle_t, 1);
                    ntle->catnum = catnr;
                    ntle->epoch = tle.epoch;
                    ntle->status = tle.status;
                    ntle->satname = g_strdup(tle.sat_name);
                    ntle->line1 = g_strdup(tle_str[1]);
                    ntle->line2 = g_strdup(tle_str[2]);
                    ntle->srcfile = g_strdup(fnam);
                    ntle->isnew = TRUE; /* flag will be reset when using data */

                    g_hash_table_insert(data, key, ntle);
                    retcode++;
                }
                else
                {
                    /* satellite is already in hash */
                    /* apply various merge routines */

                    /* time merge */
                    if (ntle->epoch == tle.epoch)
                    {
                        /* if satellite epoch has the same time,  merge status as appropriate */
                        if (ntle->status != tle.status)
                        {
                            /* log if there is something funny about the data coming in */
                            sat_log_log(SAT_LOG_LEVEL_WARN,
                                        _
                                        ("%s:%s: Two different statuses for %d (%s) at the same time."),
                                        __FILE__, __func__, ntle->catnum,
                                        ntle->satname);
                            if (tle.status != OP_STAT_UNKNOWN)
                                ntle->status = tle.status;
                        }
                    }
                    else if (ntle->epoch < tle.epoch)
                    {
                        /* if the satellite in the hash is older than 
                           the one just loaded, copy the values over. */

                        ntle->catnum = catnr;
                        ntle->epoch = tle.epoch;
                        ntle->status = tle.status;
                        g_free(ntle->line1);
                        ntle->line1 = g_strdup(tle_str[1]);
                        g_free(ntle->line2);
                        ntle->line2 = g_strdup(tle_str[2]);
                        g_free(ntle->srcfile);
                        ntle->srcfile = g_strdup(fnam);
                        ntle->isnew = TRUE;     /* flag will be reset when using data */
                    }

                    /* merge based on name */
                    if (is_computer_generated_name(ntle->satname) &&
                        !is_computer_generated_name(tle_str[0]))
                    {
                        g_free(ntle->satname);
                        ntle->satname = g_strdup(tle.sat_name);
                    }

                    /* free the key since we do not commit it to the cache */
                    g_free(key);
                }
            }

        }

        if (catsync)
        {
            /* close category file */
            fclose(catfile);
        }

        g_free(catpath);

        /* close input TLE file */
        fclose(fp);
    }

    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Failed to open %s"), __FILE__, __func__, path);
    }
    g_free(path);

    return retcode;
}

/**
 * Update TLE data in a file.
 *
 * @param ldname Directory name for gpredict tle files.
 * @param fname The name of the TLE file.
 * @param data The hash table containing the fresh data.
 * @param sat_upd OUT: number of sats updated.
 * @param sat_ski OUT: number of sats skipped.
 * @param sat_nod OUT: number of sats for which no data found
 * @param sat_tot OUT: total number of sats
 *
 * For each satellite in the TLE file ldname/fnam, this function
 * checks whether there is any newer data available in the hash table.
 * If yes, the function writes the fresh data to temp_file, if no, the
 * old data is copied to temp_file.
 * When all sats have been copied ldname/fnam is deleted and temp_file
 * is renamed to ldname/fnam.
 */
static void update_tle_in_file(const gchar * ldname,
                               const gchar * fname,
                               GHashTable * data,
                               guint * sat_upd,
                               guint * sat_ski,
                               guint * sat_nod, guint * sat_tot)
{
    gchar          *path;
    guint           updated = 0;        /* number of updated sats */
    guint           nodata = 0; /* no sats for which no fresh data available */
    guint           skipped = 0;        /* no. sats where fresh data is older */
    guint           total = 0;  /* total no. of sats in gpredict tle file */
    gchar         **catstr;
    guint           catnr;
    guint          *key = NULL;
    tle_t           tle;
    new_tle_t      *ntle;
    op_stat_t       status;
    GError         *error = NULL;
    GKeyFile       *satdata;
    gchar          *tlestr1, *tlestr2, *rawtle, *satname, *satnickname;
    gboolean        updateddata;

    /* get catalog number for this satellite */
    catstr = g_strsplit(fname, ".sat", 0);
    catnr = (guint) g_ascii_strtod(catstr[0], NULL);

    /* see if we have new data for this satellite */
    key = g_try_new0(guint, 1);
    *key = catnr;
    ntle = (new_tle_t *) g_hash_table_lookup(data, key);
    g_free(key);

    if (ntle == NULL)
    {
        /* no new data found for this sat => obsolete */
        nodata++;

        /* check if obsolete sats should be deleted */
        /**** FIXME: This is dangereous, so we omit it */
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _
                    ("%s: No new TLE data found for %d. Satellite might be obsolete."),
                    __func__, catnr);
    }
    else
    {
        /* open input file (file containing old tle) */
        path = g_strconcat(ldname, G_DIR_SEPARATOR_S, fname, NULL);
        satdata = g_key_file_new();
        if (!g_key_file_load_from_file
            (satdata, path, G_KEY_FILE_KEEP_COMMENTS, &error))
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Error loading %s (%s)"),
                        __func__, path, error->message);
            g_clear_error(&error);

            skipped++;
        }
        else
        {
            /* This satellite is not new */
            ntle->isnew = FALSE;

            /* get TLE data */
            tlestr1 =
                g_key_file_get_string(satdata, "Satellite", "TLE1", NULL);
            if (error != NULL)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: Error reading TLE line 2 from %s (%s)"),
                            __func__, path, error->message);
                g_clear_error(&error);
            }
            tlestr2 =
                g_key_file_get_string(satdata, "Satellite", "TLE2", NULL);
            if (error != NULL)
            {
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s: Error reading TLE line 2 from %s (%s)"),
                            __func__, path, error->message);
                g_clear_error(&error);
            }

            /* get name data */
            satname =
                g_key_file_get_string(satdata, "Satellite", "NAME", NULL);
            satnickname =
                g_key_file_get_string(satdata, "Satellite", "NICKNAME", NULL);

            /* get status data */
            if (g_key_file_has_key(satdata, "Satellite", "STATUS", NULL))
            {
                status =
                    g_key_file_get_integer(satdata, "Satellite", "STATUS",
                                           NULL);
            }
            else
            {
                status = OP_STAT_UNKNOWN;
            }

            rawtle = g_strconcat(tlestr1, tlestr2, NULL);

            if (!Good_Elements(rawtle))
            {
                sat_log_log(SAT_LOG_LEVEL_WARN,
                            _("%s: Current TLE data for %d appears to be bad"),
                            __func__, catnr);
                /* set epoch to zero so it gets overwritten */
                tle.epoch = 0;
            }
            else
            {
                Convert_Satellite_Data(rawtle, &tle);
            }
            g_free(tlestr1);
            g_free(tlestr2);
            g_free(rawtle);

            /* Initialize flag for update */
            updateddata = FALSE;

            if (ntle->satname != NULL)
            {
                /* when a satellite first appears in the elements it is sometimes referred to by the 
                   international designator which is awkward after it is given a name */
                if (!is_computer_generated_name(ntle->satname))
                {
                    if (is_computer_generated_name(satname))
                    {
                        sat_log_log(SAT_LOG_LEVEL_INFO,
                                    _("%s: Data for  %d updated for name."),
                                    __func__, catnr);
                        g_key_file_set_string(satdata, "Satellite", "NAME",
                                              ntle->satname);
                        updateddata = TRUE;
                    }

                    /* FIXME what to do about nickname Possibilities: */
                    /* clobber with name */
                    /* clobber if nickname and name were same before */
                    /* clobber if international designator */
                    if (is_computer_generated_name(satnickname))
                    {
                        sat_log_log(SAT_LOG_LEVEL_INFO,
                                    _
                                    ("%s: Data for  %d updated for nickname."),
                                    __func__, catnr);
                        g_key_file_set_string(satdata, "Satellite", "NICKNAME",
                                              ntle->satname);
                        updateddata = TRUE;
                    }
                }
            }

            g_free(satname);
            g_free(satnickname);

            if (tle.epoch < ntle->epoch)
            {
                /* new data is newer than what we already have */
                /* store new data */
                sat_log_log(SAT_LOG_LEVEL_INFO,
                            _("%s: Data for  %d updated for tle."),
                            __func__, catnr);
                g_key_file_set_string(satdata, "Satellite", "TLE1",
                                      ntle->line1);
                g_key_file_set_string(satdata, "Satellite", "TLE2",
                                      ntle->line2);
                g_key_file_set_integer(satdata, "Satellite", "STATUS",
                                       ntle->status);
                updateddata = TRUE;

            }
            else if (tle.epoch == ntle->epoch)
            {
                if ((status != ntle->status) &&
                    (ntle->status != OP_STAT_UNKNOWN))
                {
                    sat_log_log(SAT_LOG_LEVEL_INFO,
                                _
                                ("%s: Data for  %d updated for operational status."),
                                __func__, catnr);
                    g_key_file_set_integer(satdata, "Satellite", "STATUS",
                                           ntle->status);
                    updateddata = TRUE;
                }
            }

            if (updateddata == TRUE)
            {
                if (gpredict_save_key_file(satdata, path))
                    skipped++;
                else
                    updated++;
            }
            else
            {
                skipped++;
            }
        }

        g_key_file_free(satdata);
        g_free(path);
    }
    g_strfreev(catstr);

    /* update out parameters */
    *sat_upd = updated;
    *sat_ski = skipped;
    *sat_nod = nodata;
    *sat_tot = total;
}


const gchar    *freq_to_str[TLE_AUTO_UPDATE_NUM] = {
    N_("Never"),
    N_("Monthly"),
    N_("Weekly"),
    N_("Daily")
};

const gchar    *tle_update_freq_to_str(tle_auto_upd_freq_t freq)
{
    if ((freq <= TLE_AUTO_UPDATE_NEVER) || (freq >= TLE_AUTO_UPDATE_NUM))
        freq = TLE_AUTO_UPDATE_NEVER;

    return _(freq_to_str[freq]);
}

/**
 * Determine if name is generic.
 *
 * @param satname The satellite name that might be old.
 *
 * This function determines if the satellite name is generic. Examples
 * of this are the names YYYY-NNNAAA international ID names used by
 * Celestrak. Also space-track.org will give items names of OBJECT A as 
 * well until the name is advertised.  
 */
static gboolean is_computer_generated_name(gchar * satname)
{
    /* celestrak generic satellite name */
    if (g_regex_match_simple("\\d{4,}-\\d{3,}", satname, 0, 0))
    {
        return (TRUE);
    }
    /* space-track generic satellite name */
    if (g_regex_match_simple("OBJECT", satname, 0, 0))
    {
        return (TRUE);
    }
    return (FALSE);
}
