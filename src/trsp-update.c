/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.
  Copyright (C)  2016       Baris DINC (TA7W)
  Copyright (C)  2017       Mario Haustein (DM5AHA)

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

/*
 * Downloads Frequency list from SATNOGS db and exports json transponder
 * information into satellite files identified by catalog id
 */
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <locale.h>

#include "compat.h"
#include "trsp-update.h"
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "nxjson/nxjson.h"

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#ifdef WIN32
#include "win32-fetch.h"
#else
#include <curl/curl.h>
#endif

/* TRSP auto update frequency. */
typedef enum {
    TRSP_AUTO_UPDATE_NEVER = 0, /* No auto-update, just warn after one week. */
    TRSP_AUTO_UPDATE_MONTHLY = 1,
    TRSP_AUTO_UPDATE_WEEKLY = 2,
    TRSP_AUTO_UPDATE_DAILY = 3,
    TRSP_AUTO_UPDATE_NUM
} trsp_auto_upd_freq_t;

/* Data structure to hold a TRSP set. */
struct transponder {
    char            uuid[30];   /* uuid */
    int             catnum;     /* Catalog number. */
    char            description[80];    /* Transponder descriptoion */
    long long       uplink_low; /* Uplink starting frequency */
    long long       uplink_high;        /* uplink end frequency  */
    long long       downlink_low;       /* downlink starting frequency */
    long long       downlink_high;      /* downlink end frequency */
    char            mode[20];   /* mode (from modes files) */
    int             invert;     /* inverting / noninverting */
    double          baud;       /* baudrate */
    int             alive;      /* alive or dead */
};

/* Data structure to hold a MODES set. */
struct modes {
    int             id;         /* id */
    char            name[80];   /* Mode name */
};

/* Data structure to hold a MODE set. */
typedef struct {
    guint           modnum;     /* Mode number. */
    gchar          *modname;    /* Mode description. */
} new_mode_t;

/* Data structure to hold a TRSP list. */
typedef struct {
    guint           catnum;     /* Catalog number. */
    guint           numtrsp;    /* Number of transponders. */
} new_trsp_t;

#ifndef WIN32
/* private function prototypes */
static size_t   my_write_func(void *ptr, size_t size, size_t nmemb,
                              FILE * stream);
#endif

//int getMODElist_intoHashMap();

//static void check_and_print_mode(gpointer key, gpointer value, gpointer user_data)
//{
//    new_mode_t  *nmode = (new_mode_t *) value;
//    guint      *num = user_data;
//
//    *num += 1;
//
//    (void) key; /* avoid unused parameter compiler warning */
//
//    printf("MODES LIST : %d %s \n", nmode->modnum, nmode->modname);
//
//}


/** Free a new_mode_t structure. */
static void free_new_mode(gpointer data)
{
    //TODO: remove this.
    new_mode_t     *mmode;

    mmode = (new_mode_t *) data;

    g_free(mmode->modname);
    g_free(mmode);
}

/** Free a new_trsp_t structure. */
static void free_new_trsp(gpointer data)
{
    //TODO: remove this.
    new_trsp_t     *mtrsp;

    mtrsp = (new_trsp_t *) data;

    g_free(mtrsp);
}

void trsp_update_files(gchar * input_file)
{

    FILE           *mfp;        /* transmitter information json file */
    size_t          mfplen;     /* size of transmitter information json file */
    long            flen;       /* holds the file size returned by ftell or -1 on error */
    int             result;
    FILE           *ffile;      /* transponder output file in gpredict format */
    char           *jsn_object; /* json array will be in this buffer before parsing */
    unsigned int    idx;        /* object index in JSON-Array */
    new_mode_t     *nmode;
    new_trsp_t     *ntrsp;
    GHashTable     *modes_hash; /* hash table to store modes */
    GHashTable     *trsp_hash;  /* hash table to store satellite list to generate trsp files */
    guint          *key = NULL;

    gchar          *userconfdir;
    gchar          *trspfile;
    gchar          *modesfile;
    gchar          *trspfolder;

    /* force decimal mark to dot when parsing JSON file */
    setlocale(LC_NUMERIC, "C");

    modes_hash =
        g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_new_mode);
    trsp_hash =
        g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_new_trsp);

    userconfdir = get_user_conf_dir();
    trspfolder = g_strconcat(userconfdir, G_DIR_SEPARATOR_S, "trsp", NULL);
    modesfile = g_strconcat(trspfolder, "/modes.json", NULL);

    mfp = fopen(modesfile, "r");

    if (mfp != NULL)
    {
        mfplen = 0;
        fseek(mfp, 0, SEEK_END);
        flen = ftell(mfp);

        if (flen > 1)
        {
            mfplen = flen;
            rewind(mfp);
            jsn_object = g_malloc(mfplen);
            result = fread(jsn_object, mfplen, 1, mfp);

            if (result == 1)
            {
                const nx_json  *json = nx_json_parse(jsn_object, 0);

                if (json)
                {
                    idx = 0;
                    while (1)
                    {
                        const nx_json  *json_obj = nx_json_item(json, idx++);
                        struct modes    m_modes;

                        if (json_obj->type == NX_JSON_NULL)
                            break;

                        m_modes.id = nx_json_get(json_obj, "id")->int_value;
                        strncpy(m_modes.name,
                                nx_json_get(json_obj, "name")->text_value, 79);
                        m_modes.name[79] = 0;

                        /* add data to hash table */
                        key = g_try_new0(guint, 1);
                        *key = m_modes.id;

                        nmode = g_hash_table_lookup(modes_hash, key);

                        if (nmode == NULL)
                        {
                            /* create new_mode structure */
                            nmode = g_try_new(new_mode_t, 1);
                            nmode->modnum = m_modes.id;
                            nmode->modname = g_strdup(m_modes.name);

                            g_hash_table_insert(modes_hash, key, nmode);

                        }

                        sat_log_log(SAT_LOG_LEVEL_INFO, _("MODE %d %s"),
                                    m_modes.id, m_modes.name);
                    }           // while(1)
                    nx_json_free(json);
                }               // if(json)
            }                   // if(result == 1)
            g_free(jsn_object);
        }                       // if(flen > 1)
        fclose(mfp);
    }                           // if(mfp)

//    guint num = 0;
//    printf("---------- PRINTING MODES LIST ------- \n");
//    g_hash_table_foreach (modes_hash, check_and_print_mode, &num);

    mfp = fopen(input_file, "r");
    if (mfp != NULL)
    {
        mfplen = 0;
        fseek(mfp, 0, SEEK_END);
        flen = ftell(mfp);

        if (flen > 1)
        {
            mfplen = flen;
            rewind(mfp);
            jsn_object = g_malloc(mfplen);
            result = fread(jsn_object, mfplen, 1, mfp);

            if (result == 1)
            {
                const nx_json  *json = nx_json_parse(jsn_object, 0);

                if (json)
                {
                    idx = 0;
                    while (1)
                    {
                        const nx_json  *json_obj = nx_json_item(json, idx++);
                        struct transponder m_trsp;

                        if (json_obj->type == NX_JSON_NULL)
                            break;

                        strncpy(m_trsp.description,
                                nx_json_get(json_obj,
                                            "description")->text_value, 79);
                        m_trsp.description[79] = 0;
                        m_trsp.catnum =
                            nx_json_get(json_obj, "norad_cat_id")->int_value;
                        m_trsp.uplink_low =
                            nx_json_get(json_obj, "uplink_low")->int_value;
                        m_trsp.uplink_high =
                            nx_json_get(json_obj, "uplink_high")->int_value;
                        m_trsp.downlink_low =
                            nx_json_get(json_obj, "downlink_low")->int_value;
                        m_trsp.downlink_high =
                            nx_json_get(json_obj, "downlink_high")->int_value;

                        key = g_try_new0(guint, 1);
                        *key = nx_json_get(json_obj, "mode_id")->int_value;
                        nmode = g_hash_table_lookup(modes_hash, key);
                        if (nmode != NULL)
                            sprintf(m_trsp.mode, "%s", nmode->modname);
                        else
                            sprintf(m_trsp.mode, "%lli",
                                    nx_json_get(json_obj,
                                                "mode_id")->int_value);

                        m_trsp.invert =
                            nx_json_get(json_obj, "invert")->int_value;
                        m_trsp.baud = nx_json_get(json_obj, "baud")->dbl_value;
                        m_trsp.alive =
                            nx_json_get(json_obj, "alive")->int_value;

                        //strcpy(m_trsp.uuid,nx_json_get(json_obj,          "uuid")->text_value);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _(">>> Preparing information for transponders of cat_id %d <<<"), m_trsp.catnum, __func__);
                        ////sat_log_log (SAT_LOG_LEVEL_INFO, _("         uuid : %s"), m_trsp.uuid, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("  description : %s"), m_trsp.description, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("        alive : %s"), m_trsp.alive ? "true" : "false", __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("   uplink_low : %Ld"),m_trsp.uplink_low, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("  uplink_high : %Ld"),m_trsp.uplink_high, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _(" downink_low  : %Ld"),m_trsp.downlink_low, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("downlink_high : %Ld"),m_trsp.downlink_high, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("         Mode : %s"), m_trsp.mode, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("       Invert : %s"), m_trsp.invert ? "true" : "false", __func__);
                        // sat_log_log(SAT_LOG_LEVEL_DEBUG, _("         Baud : %lf"),m_trsp.baud, __func__);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _(" norad_cat_id : %Ld"),m_trsp.catnum, __func__);
                        char            m_catnum[20];

                        sprintf(m_catnum, "%d", m_trsp.catnum);
                        trspfile = g_strconcat(trspfolder, G_DIR_SEPARATOR_S,
                                               m_catnum, ".trsp", NULL);
                        //sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Writing to file : %s "), __func__, trspfile);

                        //first lets delete the file we already have, to make space for the new version
                        /* check existence and add data to hash table for future check */
                        key = g_try_new0(guint, 1);
                        *key = m_trsp.catnum;
                        ntrsp = g_hash_table_lookup(trsp_hash, key);

                        if (ntrsp == NULL)
                        {
                            /* create new_trsp structure */
                            ntrsp = g_try_new(new_trsp_t, 1);
                            ntrsp->catnum = m_trsp.catnum;
                            ntrsp->numtrsp = 1; //our first insertion of transponder to this file
                            g_hash_table_insert(trsp_hash, key, ntrsp);
                            if (g_remove(trspfile))
                                sat_log_log(SAT_LOG_LEVEL_ERROR,
                                            _("%s: Failed to remove %s"),
                                            __func__, trspfile);
                        }
                        else
                        {
                            //TODO: increase number of transponders here
                            //ntrsp->numtrsp += 1; //number of transponder info in this file
                            //g_hash_table_replace(trsp_hash, key, ntrsp);
                        }

                        //now lets write the new version
                        ffile = g_fopen(trspfile, "a");
                        if (ffile != NULL)
                        {
                            fprintf(ffile, "\n[%s]\n", m_trsp.description);
                            if (m_trsp.uplink_low > 0)
                                fprintf(ffile, "UP_LOW=%lld\n",
                                        m_trsp.uplink_low);
                            if (m_trsp.uplink_high > 0)
                                fprintf(ffile, "UP_HIGH=%lld\n",
                                        m_trsp.uplink_high);
                            if (m_trsp.downlink_low > 0)
                                fprintf(ffile, "DOWN_LOW=%lld\n",
                                        m_trsp.downlink_low);
                            if (m_trsp.downlink_high > 0)
                                fprintf(ffile, "DOWN_HIGH=%lld\n",
                                        m_trsp.downlink_high);
                            fprintf(ffile, "MODE=%s\n", m_trsp.mode);
                            if (m_trsp.baud > 0.0)
                                fprintf(ffile, "BAUD=%.0f\n", m_trsp.baud);
                            if (m_trsp.invert)
                                fprintf(ffile, "INVERT=%s\n", "true");
                            fclose(ffile);
                        }
                        else
                        {
                            sat_log_log(SAT_LOG_LEVEL_ERROR,
                                        _
                                        ("%s: Could not open trsp file for write"),
                                        __func__);
                        }
                    }           // while(1)
                    nx_json_free(json);
                }               // if(json)
            }                   // if(result == 1)
            g_free(jsn_object);
        }                       // if(flen > 1)
        fclose(mfp);
    }                           // if(mfp)

    g_hash_table_destroy(modes_hash);
}

/** Update MODES files from network. */
void modes_update_from_network()
{
    gchar          *server;
    gchar          *proxy = NULL;
    gchar          *modes_file;
    gchar          *file_url;
    gchar          *locfile;
    gchar          *userconfdir;
#ifdef WIN32
    int             res;
#else
    CURL           *curl;
    CURLcode        res;
#endif
    FILE           *outfile;
    guint           success = 0;        /* no. of successful downloads */

    server = sat_cfg_get_str(SAT_CFG_STR_TRSP_SERVER);
    proxy = sat_cfg_get_str(SAT_CFG_STR_TRSP_PROXY);
    modes_file = sat_cfg_get_str(SAT_CFG_STR_TRSP_MODE_FILE);

#ifndef WIN32
    /* initialise curl */
    curl = curl_easy_init();
    if (proxy != NULL)
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "gpredict/curl");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
#endif

    /* get files */
    /* set URL */
    file_url = g_strconcat(server, modes_file, NULL);
#ifndef WIN32
    curl_easy_setopt(curl, CURLOPT_URL, file_url);
    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Ready to fetch modes list from %s"),
                __func__, file_url);
#endif

    /* create local cache file */
    userconfdir = get_user_conf_dir();
    locfile = g_strconcat(userconfdir, G_DIR_SEPARATOR_S, "trsp",
                          G_DIR_SEPARATOR_S, "modes.json", NULL);
    sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: File to open %s "), __func__,
                locfile);

    outfile = g_fopen(locfile, "wb");
    if (outfile != NULL)
    {
#ifdef WIN32
        res = win32_fetch(file_url, outfile, proxy, "gpredict/win32");
        if (res != 0)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Error fetching %s (%x)"),
                        __func__, file_url, res);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Successfully fetched %s"),
                        __func__, file_url);
            success++;
        }
#else
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func);

        /* get file */
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Error fetching %s (%s)"),
                        __func__, file_url, curl_easy_strerror(res));
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Successfully fetched %s"),
                        __func__, file_url);
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

    g_free(userconfdir);
    g_free(file_url);
    g_free(server);
    g_free(proxy);
    g_free(modes_file);

#ifndef WIN32
    curl_easy_cleanup(curl);
#endif

    /* continue update if we have fetched at least one file */
    if (success > 0)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Fetched %d files from network; updating..."),
                    __func__, success);
        //here we can get the modes json file into memory as hashmap..
        //g_free(cache);

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not fetch frequency files from network"),
                    __func__);
    }
}

/**
 * Update TRSP files from network.
 *
 * @param silent TRUE if function should execute without graphical status indicator.
 * @param progress Pointer to a GtkProgressBar progress indicator (can be NULL)
 * @param label1 GtkLabel for activity string.
 * @param label2 GtkLabel for statistics string.
 */
void trsp_update_from_network(gboolean silent,
                              GtkWidget * progress,
                              GtkWidget * label1, GtkWidget * label2)
{
    static GMutex   trsp_in_progress;

    gchar          *server;
    gchar          *proxy = NULL;
    gchar          *freq_file;
    gchar          *file_url;
    gchar          *locfile_trsp;
    gchar          *userconfdir;
#ifdef WIN32
    int             res;
#else
    CURL           *curl;
    CURLcode        res;
#endif
    gdouble         fraction;
    FILE           *outfile;
    gchar          *cache;
    gchar          *text;

    guint           success = 0;        /* no. of successful downloads */

    (void)label2;

    /* bail out if we are already in an update process */
    if (g_mutex_trylock(&trsp_in_progress) == FALSE)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: A frequency update process is already running"),
                    __func__);
        return;
    }

    /* get list with modes */
    modes_update_from_network();

    /* get server, proxy, and list of files */
    //server = sprintf("%stransmitters/?format=json", sat_cfg_get_str(SAT_CFG_STR_TRSP_SERVER));
    server = sat_cfg_get_str(SAT_CFG_STR_TRSP_SERVER);
    proxy = sat_cfg_get_str(SAT_CFG_STR_TRSP_PROXY);
    freq_file = sat_cfg_get_str(SAT_CFG_STR_TRSP_FREQ_FILE);

#ifndef WIN32
    /* initialise curl */
    curl = curl_easy_init();
    if (proxy != NULL)
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "gpredict/curl");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
#endif

    /* get files */
    /* set URL */
    file_url = g_strconcat(server, freq_file, NULL);
#ifndef WIN32
    curl_easy_setopt(curl, CURLOPT_URL, file_url);
    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Ready to fetch transponder list from %s"),
                __func__, file_url);
#endif

    /* set activity message */
    if (!silent && (label1 != NULL))
    {

        text = g_strdup_printf(_("Fetching %s"), "transmitters.json");
        gtk_label_set_text(GTK_LABEL(label1), text);
        g_free(text);

        /* Force the drawing queue to be processed otherwise there will
           not be any visual feedback, ie. frozen GUI
           - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
         */
        while (g_main_context_iteration(NULL, FALSE));
    }

    /* create local cache file */
    userconfdir = get_user_conf_dir();

    locfile_trsp = g_strconcat(userconfdir, G_DIR_SEPARATOR_S, "trsp",
                               G_DIR_SEPARATOR_S, "transmitters.json", NULL);
    sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: File to open %s "), __func__,
                locfile_trsp);

    outfile = g_fopen(locfile_trsp, "wb");
    if (outfile != NULL)
    {
#ifdef WIN32
        res = win32_fetch(file_url, outfile, proxy, "gpredict/win32");
        if (res != 0)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Error fetching %s (%x)"),
                        __func__, file_url, res);
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Successfully fetched %s"),
                        __func__, file_url);
            success++;
        }
#else
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func);

        /* get file */
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Error fetching %s (%s)"),
                        __func__, file_url, curl_easy_strerror(res));
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Successfully fetched %s"),
                        __func__, file_url);
            success++;
        }
#endif
        fclose(outfile);

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Failed to open %s preventing update"),
                    __func__, locfile_trsp);
    }

    /* update progress indicator */
    if (!silent && (progress != NULL))
    {
        /* complete download corresponds to 50% */
        fraction = 1;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), fraction);

        /* Force the drawing queue to be processed otherwise there will
           not be any visual feedback, ie. frozen GUI
           - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
         */
        while (g_main_context_iteration(NULL, FALSE));

    }

    g_free(userconfdir);
    g_free(file_url);
    g_free(server);
    g_free(freq_file);
    g_free(proxy);

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
        trsp_update_files(locfile_trsp);
        g_free(cache);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not fetch frequency files from network"),
                    __func__);
    }

    g_mutex_unlock(&trsp_in_progress);
}

#ifndef WIN32
/**
 * Write TRSP data block to file.
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

const gchar    *freq_to_str2[TRSP_AUTO_UPDATE_NUM] = {
    N_("Never"),
    N_("Monthly"),
    N_("Weekly"),
    N_("Daily")
};

const gchar    *trsp_update_freq_to_str2(trsp_auto_upd_freq_t freq)
{
    if ((freq <= TRSP_AUTO_UPDATE_NEVER) || (freq >= TRSP_AUTO_UPDATE_NUM))
    {
        freq = TRSP_AUTO_UPDATE_NEVER;
    }

    return _(freq_to_str2[freq]);
}
