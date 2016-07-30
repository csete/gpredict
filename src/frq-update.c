/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.
  Copyright (C)  2016       Baris DINC (TA7W)

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
 * Downloads Frequency list from SATNOGS db 
 * and exports json transmitter information into satellite files
 * identified by catalog id
 *
*/

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <curl/curl.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "sat-cfg.h"
#include "compat.h"
#include "frq-update.h"
#include "gpredict-utils.h"

//library to parse json
#include "nxjson.h"
#include <locale.h>


/* private function prototypes */
static size_t   my_write_func(void *ptr, size_t size, size_t nmemb,
                              FILE * stream);


void frq_update_files(gchar * frqfile)
{

    FILE           *mfp;        //transmitter information json file
    FILE           *ffile;      //transponder output file in gpredict formar
    char            symbol;     //characters to read from json file
    char            frq_object[10000];  //json array will be in this buffer before parsing
    m_state         jsn_state = START;  //json parsing state

    gchar          *userconfdir;
    gchar          *trspfile;
    gchar          *trspfolder;


    setlocale(LC_NUMERIC, "C"); // I had to set locale, orherwise float delimeter was comma instead of dot, and was hard to parse the json file
    userconfdir = get_user_conf_dir();
    trspfolder = g_strconcat(userconfdir, G_DIR_SEPARATOR_S, "trsp", NULL);


    sprintf(frq_object, " ");   //initialize buffer
    mfp = fopen(frqfile, "r");
    if (mfp != NULL)
    {
        while ((symbol = getc(mfp)) != EOF)
        {
            if (symbol == '[')
                jsn_state = START;
            if (((jsn_state == START) || (jsn_state == NXDLM)) &&
                (symbol == '{'))
                jsn_state = OBBGN;
            if ((jsn_state == OBBGN) && (symbol == '}'))
                jsn_state = OBEND;
            if ((jsn_state == OBEND) && (symbol == ','))
                jsn_state = NXDLM;
            if ((jsn_state == OBEND) && (symbol == ']'))
                jsn_state = FINISH;

            if (jsn_state == OBBGN)
                sprintf(frq_object, "%s%c", frq_object, symbol);

            if (jsn_state == OBEND)
            {
                sprintf(frq_object, "%s}", frq_object);
                //printf("%s \n\n",frq_object); 
                const nx_json  *json = nx_json_parse(frq_object, 0);

                if (json)
                {
                    struct transponder m_trsp;

                    strcpy(m_trsp.description,
                           nx_json_get(json, "description")->text_value);
                    m_trsp.catnum =
                        nx_json_get(json, "norad_cat_id")->int_value;
                    m_trsp.uplink_low =
                        nx_json_get(json, "uplink_low")->int_value;
                    m_trsp.uplink_high =
                        nx_json_get(json, "uplink_high")->int_value;
                    m_trsp.downlink_low =
                        nx_json_get(json, "downlink_low")->int_value;
                    m_trsp.downlink_high =
                        nx_json_get(json, "downlink_high")->int_value;
                    m_trsp.mode_id = nx_json_get(json, "mode_id")->int_value;
                    m_trsp.invert = nx_json_get(json, "invert")->int_value;
                    m_trsp.baud = nx_json_get(json, "baud")->dbl_value;
                    m_trsp.alive = nx_json_get(json, "alive")->int_value;
                    //strcpy(m_trsp.uuid,nx_json_get(json,              "uuid")->text_value);

                    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                _(">>> Preparing information for transponders of cat_id %d <<<"),
                                m_trsp.catnum, __func__);

                    //sat_log_log (SAT_LOG_LEVEL_INFO, _("         uuid : %s"), m_trsp.uuid, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("  description : %s"),
                                m_trsp.description, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("        alive : %s"),
                                m_trsp.alive ? "true" : "false", __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("   uplink_low : %Ld"),
                                m_trsp.uplink_low, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("  uplink_high : %Ld"),
                                m_trsp.uplink_high, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _(" downink_low  : %Ld"),
                                m_trsp.downlink_low, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("downlink_high : %Ld"),
                                m_trsp.downlink_high, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("      Mode ID : %Ld"),
                                m_trsp.mode_id, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("       Invert : %s"),
                                m_trsp.invert ? "true" : "false", __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _("         Baud : %lf"),
                                m_trsp.baud, __func__);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG, _(" norad_cat_id : %Ld"),
                                m_trsp.catnum, __func__);
                    char            m_catnum[20];

                    sprintf(m_catnum, "%d", m_trsp.catnum);
                    trspfile =
                        g_strconcat(trspfolder, G_DIR_SEPARATOR_S, m_catnum,
                                    ".trsp", NULL);
                    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                                _("%s: Writing to file : %s "), __func__,
                                trspfile);


                    //first lets delete the file we already have, to make space for the new version
                    g_remove(trspfile);

                    //now lets write the new version
                    ffile = g_fopen(trspfile, "a");
                    if (ffile != NULL)
                    {
                        char            fcontent[1000];

                        sprintf(fcontent, "[%s]\n", m_trsp.description);
                        sprintf(fcontent, "%sUP_LOW=%d\n", fcontent,
                                m_trsp.uplink_low);
                        sprintf(fcontent, "%sUP_HIGH=%d\n", fcontent,
                                m_trsp.uplink_high);
                        sprintf(fcontent, "%sDOWN_LOW=%d\n", fcontent,
                                m_trsp.downlink_low);
                        sprintf(fcontent, "%sDOWN_HIGH=%d\n", fcontent,
                                m_trsp.downlink_high);
                        sprintf(fcontent, "%sMODE=%d baud:%f\n", fcontent,
                                m_trsp.mode_id, m_trsp.baud);
                        sprintf(fcontent, "%sINVERT=%s\n\n", fcontent,
                                m_trsp.invert ? "true" : "false");

                        fputs(fcontent, ffile);
                        fclose(ffile);
                    }
                    else
                    {
                        sat_log_log(SAT_LOG_LEVEL_ERROR,
                                    _
                                    ("%s:%s: Could not open .trsp file for writing..."),
                                    __FILE__, __func__);
                    }

                    nx_json_free(json);
                }               //if(json)
                sprintf(frq_object, " ");       //empty the buffer
            }                   //if(OBEND)
        }                       //while(symbol)
        fclose(mfp);
    }                           //if(mfp)

    return;
}

/** \brief Update FRQ files from network.
 *  \param silent TRUE if function should execute without graphical status indicator.
 *  \param progress Pointer to a GtkProgressBar progress indicator (can be NULL)
 *  \param label1 GtkLabel for activity string.
 *  \param label2 GtkLabel for statistics string.
 */
void frq_update_from_network(gboolean silent,
                             GtkWidget * progress,
                             GtkWidget * label1, GtkWidget * label2)
{
    static GMutex   frq_in_progress;

    gchar          *server;
    gchar          *proxy = NULL;
    gchar          *files_tmp;
    gchar         **files;
    guint           numfiles;
    gchar          *curfile;
    gchar          *locfile;
    gchar          *userconfdir;
    CURL           *curl;
    CURLcode        res;
    gdouble         fraction, start = 0;
    FILE           *outfile;

    //GDir        *dir;
    gchar          *cache;

    //const gchar *fname;
    gchar          *text;

    //GError      *err = NULL;
    guint           success = 0;        /* no. of successfull downloads */

    /* bail out if we are already in an update process */
    if (g_mutex_trylock(&frq_in_progress) == FALSE)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _
                    ("%s: A FRQ update process is already running. Aborting."),
                    __func__);

        return;
    }

    /* get server, proxy, and list of files */
    server = sat_cfg_get_str(SAT_CFG_STR_FRQ_SERVER);
    proxy = sat_cfg_get_str(SAT_CFG_STR_FRQ_PROXY);
    files_tmp = sat_cfg_get_str(SAT_CFG_STR_FRQ_FILES);
    files = g_strsplit(files_tmp, ";", 0);
    numfiles = g_strv_length(files);

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("Ready to fetch satellite list from satnogs..."), __func__);


    /* initialise progress bar */
    if (!silent && (progress != NULL))
        start = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progress));

    /* initialise curl */
    curl = curl_easy_init();
    if (proxy != NULL)
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "gpredict/curl");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);

    /* get files */
    /* set URL */
    curfile = g_strconcat(server, "", NULL);
    curl_easy_setopt(curl, CURLOPT_URL, curfile);

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
    locfile =
        g_strconcat(userconfdir, G_DIR_SEPARATOR_S, "trsp", G_DIR_SEPARATOR_S,
                    "transmitters.json", NULL);
    sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: File to open %s "), __func__,
                locfile);

    outfile = g_fopen(locfile, "wb");
    if (outfile != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func);

        /* get file */
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Error fetching %s (%s)"),
                        __func__, curfile, curl_easy_strerror(res));
        }
        else
        {
            sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Successfully fetched %s"),
                        __func__, curfile);
            success++;
        }
        fclose(outfile);

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Failed to open %s preventing update"), __func__,
                    locfile);
    }
    /* update progress indicator */
    if (!silent && (progress != NULL))
    {

        /* complete download corresponds to 50% */
        fraction = start + (0.5 - start) / (1.0 * numfiles);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), fraction);

        /* Force the drawing queue to be processed otherwise there will
           not be any visual feedback, ie. frozen GUI
           - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
         */
        while (g_main_context_iteration(NULL, FALSE));

    }

    g_free(userconfdir);
    g_free(curfile);
    //g_free (locfile);

    curl_easy_cleanup(curl);

    /* continue update if we have fetched at least one file */
    if (success > 0)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Fetched %d files from network; updating..."),
                    __func__, success);
        /* call update_from_files */
        cache = sat_file_name("cache");
        //frq_update_from_files (cache, NULL, silent, progress, label1, label2);
        frq_update_files(locfile);
        g_free(cache);

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _
                    ("%s: Could not fetch any new FRQ files from network; aborting..."),
                    __func__);
    }


    /* clear cache and memory */
    g_free(server);
    g_strfreev(files);
    g_free(files_tmp);
    if (proxy != NULL)
        g_free(proxy);

    g_mutex_unlock(&frq_in_progress);

}


/** \brief Write FRQ data block to file.
 *  \param ptr Pointer to the data block to be written.
 *  \param size Size of data block.
 *  \param nmemb Size multiplier?
 *  \param stream Pointer to the file handle.
 *  \return The number of bytes actually written.
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



const gchar    *freq_to_str2[FRQ_AUTO_UPDATE_NUM] = {
    N_("Never"),
    N_("Monthly"),
    N_("Weekly"),
    N_("Daily")
};

const gchar    *frq_update_freq_to_str2(frq_auto_upd_freq_t freq)
{
    if ((freq <= FRQ_AUTO_UPDATE_NEVER) || (freq >= FRQ_AUTO_UPDATE_NUM))
    {

        freq = FRQ_AUTO_UPDATE_NEVER;

    }

    return _(freq_to_str2[freq]);
}
