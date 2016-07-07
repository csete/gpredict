/*
 * Added by TA7W (Baris Dinc) on July 2016
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
#  include <build-config.h>
#endif
#include <curl/curl.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "sat-cfg.h"
#include "compat.h"
#include "frq-update.h"
#include "gpredict-utils.h"

#include "../../jsmn/jsmn.h"


gchar	*frq_filename;
char	frq_buffer[BUFSIZ];
int	dmp_state = 0;
int	dmp_tofile = 0;

/* private function prototypes */
static size_t  my_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream);

/*Copied from jsmn exmple... */
static inline void *realloc_it(void *ptrmem, size_t size) {
        void *p = realloc(ptrmem, size);
        if (!p)  {
                free (ptrmem);
                fprintf(stderr, "realloc(): err\n");
        }
        return p;
}

static int dump(const char *js, jsmntok_t *t, size_t count, int indent) {
        int i, j, k;
        char buf[BUFSIZ];
        //gchar	*buf;


        if (count == 0) {
                return 0;
        }
        if (t->type == JSMN_PRIMITIVE) {
                //printf("%.*s", t->end - t->start, js+t->start);
                if (dmp_tofile == 1) sprintf(frq_buffer,"%s%.*s", frq_buffer, t->end - t->start, js+t->start);
                sprintf(buf,"%.*s", t->end - t->start, js+t->start);
		if (dmp_state == 1) 
			{
			  frq_filename = g_strconcat (get_user_conf_dir(), G_DIR_SEPARATOR_S, "satdata", G_DIR_SEPARATOR_S, "frq_",buf, ".json", NULL);
                          printf("myfile : %s \n",frq_filename);
			}
                return 1;
        } else if (t->type == JSMN_STRING) {
                //printf("'%.*s'", t->end - t->start, js+t->start);
                if (dmp_tofile == 1) sprintf(frq_buffer,"%s'%.*s'", frq_buffer, t->end - t->start, js+t->start);
                sprintf(buf,"%.*s", t->end - t->start, js+t->start);
                if ( strcmp(buf,"norad_cat_id") == 0)  
			dmp_state = 1;
		else
			dmp_state = 0;
		if ( (strcmp(buf,"uplink_low") && strcmp(buf,"uplink_high") && strcmp(buf,"downlink_low") && strcmp(buf,"downlink_high")) == 0) dmp_tofile = 1 ; else dmp_tofile = 0;
                return 1;
        } else if (t->type == JSMN_OBJECT) {
                j = 0;
		sprintf(frq_buffer,"{ \n");
		//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                for (i = 0; i < t->size; i++) {
                        //printf(" ");
                        for (k = 0; k < indent; k++) printf("  ");
                        j += dump(js, t+1+j, count-j, indent+1);
                        //printf(": ");
                        //sprintf(frq_buffer,"%s : ",frq_buffer);
                        j += dump(js, t+1+j, count-j, indent+1);
			//sprintf(frq_buffer,"%s , \n",frq_buffer);
                }
		//printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
		sprintf(frq_buffer,"%s 'place_holder' : 'nope' \n}\n ",frq_buffer);
		//printf("OKUNAN : %s \n",frq_buffer);
		//printf("DOSYA  : %s \n",frq_filename);
		//write per satellite transceiver information to corresponding frq_CATID.json file
		FILE* pFile;
		pFile = fopen(frq_filename,"w");
		if (pFile) fprintf(pFile,"%s",frq_buffer);
		fclose(pFile);

                return j+1;
        } else if (t->type == JSMN_ARRAY) {
                j = 0;
                printf("SIZE : %d \n", t->size);
                for (i = 0; i < t->size; i++) {
                        for (k = 0; k < indent-1; k++) printf("  ");
                        printf(" ");
                        j += dump(js, t+1+j, count-j, indent+1);
                        printf("\n");
                }
                return j+1;
        }
        return 0;
}



void frq_update_files ( gchar *frqfile )
{
        int r;
        int eof_expected = 0;
        char *js = NULL;
        size_t jslen = 0;
        char buf[BUFSIZ];



        jsmn_parser p;
        jsmntok_t *tok;
        size_t tokcount = 2;

        /* Prepare parser */
        jsmn_init(&p);

        /* Allocate some tokens as a start */
        tok = malloc(sizeof(*tok) * tokcount);
        if (tok == NULL) {
                fprintf(stderr, "malloc(): err\n");
                return ;
        }
        FILE *mfp;
        //mfp = fopen("../../transmitters.txt","r");
        mfp = fopen(frqfile,"r");
        sat_log_log (SAT_LOG_LEVEL_INFO, _("%s: File Opened.............................."), __func__);


        for (;;) {
                /* Read another chunk */
                //r = fread(buf, 1, sizeof(buf), stdin);
                r = fread(buf, 1, sizeof(buf), mfp);
                if (r < 0) {
                        fprintf(stderr, "fread(): %d, err\n", r);
                        return ;
                }
                if (r == 0) {
                        if (eof_expected != 0) {
                                return ;
                        } else {
                                fprintf(stderr, "fread(): unexpected EOF\n");
                                return ;
                        }
                }

                js = realloc_it(js, jslen + r + 1);
                if (js == NULL) {
                        return ;
                }
                strncpy(js + jslen, buf, r);
                jslen = jslen + r;
again:
                r = jsmn_parse(&p, js, jslen, tok, tokcount);
                if (r < 0) {
                        if (r == JSMN_ERROR_NOMEM) {
                                tokcount = tokcount * 2;
                                tok = realloc_it(tok, sizeof(*tok) * tokcount);
                                if (tok == NULL) {
                                        return ;
                                }
                                goto again;
                        }
                } else {
                        dump(js, tok, p.toknext, 0);
                        eof_expected = 1;
                }
        }


}

/** \brief Update FRQ files from network.
 *  \param silent TRUE if function should execute without graphical status indicator.
 *  \param progress Pointer to a GtkProgressBar progress indicator (can be NULL)
 *  \param label1 GtkLabel for activity string.
 *  \param label2 GtkLabel for statistics string.
 */
void frq_update_from_network (gboolean   silent,
                              GtkWidget *progress,
                              GtkWidget *label1,
                              GtkWidget *label2)
{
    static GMutex frq_in_progress;

    gchar       *server;
    gchar       *proxy = NULL;
    gchar       *files_tmp;
    gchar      **files;
    guint        numfiles;
    gchar       *curfile;
    gchar       *locfile;
    gchar       *userconfdir;
    CURL        *curl;
    CURLcode     res;
    gdouble      fraction,start=0;
    FILE        *outfile;
    //GDir        *dir;
    gchar       *cache;
    //const gchar *fname;
    gchar       *text;
    //GError      *err = NULL;
    guint        success = 0; /* no. of successfull downloads */ 

    /* bail out if we are already in an update process */
    if (g_mutex_trylock(&frq_in_progress) == FALSE)
    {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: A FRQ update process is already running. Aborting."),
                     __func__);

        return;
    }

    /* get server, proxy, and list of files */
    server = sat_cfg_get_str (SAT_CFG_STR_FRQ_SERVER);
    proxy  = sat_cfg_get_str (SAT_CFG_STR_FRQ_PROXY);
    files_tmp = sat_cfg_get_str (SAT_CFG_STR_FRQ_FILES);
    files = g_strsplit (files_tmp, ";", 0);
    numfiles = g_strv_length (files);

    sat_log_log (SAT_LOG_LEVEL_INFO, _("Ready to fetch satellite list from satnogs..."), __func__);


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
        /* set URL */
        curfile = g_strconcat (server, "", NULL);
        curl_easy_setopt (curl, CURLOPT_URL, curfile);

        /* set activity message */
        if (!silent && (label1 != NULL)) {

        text = g_strdup_printf (_("Fetching %s"), "satellites.json");
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
        //locfile = g_strconcat (userconfdir, G_DIR_SEPARATOR_S, "satdata", G_DIR_SEPARATOR_S, "cache", G_DIR_SEPARATOR_S, files[i], NULL);
        locfile = g_strconcat (userconfdir, G_DIR_SEPARATOR_S, "satdata", G_DIR_SEPARATOR_S, "cache", G_DIR_SEPARATOR_S, "satellites.json", NULL);
	//baris
        locfile = g_strconcat (userconfdir, G_DIR_SEPARATOR_S, "satdata", G_DIR_SEPARATOR_S, "satellites.json", NULL);
        sat_log_log (SAT_LOG_LEVEL_INFO, _("%s: File to open %s "), __func__, locfile);

        outfile = g_fopen (locfile, "wb");
        if (outfile != NULL) {
           curl_easy_setopt (curl, CURLOPT_WRITEDATA, outfile);
           curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, my_write_func);
                
           /* get file */
           res = curl_easy_perform (curl);
                
           if (res != CURLE_OK) {
              sat_log_log (SAT_LOG_LEVEL_ERROR, _("%s: Error fetching %s (%s)"), __func__, curfile, curl_easy_strerror (res));
                }
                else {
                    sat_log_log (SAT_LOG_LEVEL_INFO, _("%s: Successfully fetched %s"), __func__, curfile);
                    success++;
                }
                fclose (outfile);

            } else {
                sat_log_log (SAT_LOG_LEVEL_INFO, _("%s: Failed to open %s preventing update"), __func__, locfile);
            }
            /* update progress indicator */
            if (!silent && (progress != NULL)) {

                /* complete download corresponds to 50% */
                fraction = start + (0.5-start)  / (1.0 * numfiles);
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), fraction);

                /* Force the drawing queue to be processed otherwise there will
                    not be any visual feedback, ie. frozen GUI
                    - see Gtk+ FAQ http://www.gtk.org/faq/#AEN602
                */
                while (g_main_context_iteration (NULL, FALSE));

            }

            g_free (userconfdir);
            g_free (curfile);
            //g_free (locfile);

        curl_easy_cleanup (curl);

        /* continue update if we have fetched at least one file */
        if (success > 0) {
            sat_log_log (SAT_LOG_LEVEL_INFO, _("%s: Fetched %d files from network; updating..."), __func__, success);
            /* call update_from_files */
            cache = sat_file_name ("cache");
            //frq_update_from_files (cache, NULL, silent, progress, label1, label2);
            frq_update_files (locfile);
            g_free (cache);

        }
        else {
            sat_log_log (SAT_LOG_LEVEL_ERROR, _("%s: Could not fetch any new FRQ files from network; aborting..."), __func__);
        }


    /* clear cache and memory */
    g_free (server);
    g_strfreev (files);
    g_free (files_tmp);
    if (proxy != NULL)
        g_free (proxy);

    /* open cache */
//    cache = sat_file_name ("cache");
//    dir = g_dir_open (cache, 0, &err);

//    if (err != NULL) {
        /* send an error message */
//        sat_log_log (SAT_LOG_LEVEL_ERROR, _("%s: Error opening %s (%s)"), __func__, dir, err->message);
//        g_clear_error (&err);
//    }
//    else {
        /* delete files in cache one by one */
//        while ((fname = g_dir_read_name (dir)) != NULL) {
//            locfile = g_strconcat (cache, G_DIR_SEPARATOR_S, fname, NULL);
//            g_remove (locfile);
//            g_free (locfile);
//        }
        /* close cache */
//        g_dir_close (dir);
//    }

//    g_free (cache);

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
static size_t my_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    /*** FIXME: TBC whether this works in wintendo */
    return fwrite (ptr, size, nmemb, stream);
}



const gchar *freq_to_str2[FRQ_AUTO_UPDATE_NUM] = {
    N_("Never"),
    N_("Monthly"),
    N_("Weekly"),
    N_("Daily")
};

const gchar *
        frq_update_freq_to_str2 (frq_auto_upd_freq_t freq)
{
    if ((freq <= FRQ_AUTO_UPDATE_NEVER) ||
        (freq >= FRQ_AUTO_UPDATE_NUM)) {

        freq = FRQ_AUTO_UPDATE_NEVER;

    }

    return _(freq_to_str2[freq]);
}


