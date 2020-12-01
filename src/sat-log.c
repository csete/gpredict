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
/**
 * Gpredict debug message logger.
 *
 * This component is responsible for logging the debug messages
 * coming from gpredict and hamlib. Debug messages are stored
 * in USER_CONF_DIR/logs/gpredict.log during runtime.
 *
 * During initialisation, gpredict removes the previous gpredict.log
 * file to allow the creation of the new one. However, the user can
 * choose to keep old log files. In that case the old files are kept
 * under gpredict-X.log file name, where X is the file age in seconds
 * (unix time as returned by g_get_current_time).
 * 
 */
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <time.h>

#include "compat.h"
#include "sat-cfg.h"
#include "sat-log.h"


static gboolean initialised = FALSE;
static GIOChannel *logfile = NULL;
static sat_log_level_t loglevel = SAT_LOG_LEVEL_DEBUG;
static gboolean debug_to_stderr = FALSE; // whether to also send debug msg to stderr

/** String representation of debug levels. */
const gchar    *debug_level_str[] = {
    N_(" --- "),
    N_("ERROR"),
    N_(" WARN"),
    N_(" INFO"),
    N_("DEBUG")
};

static void     manage_debug_message(sat_log_level_t debug_level,
                                     const gchar * message);
static void     log_rotate(void);
static void     clean_log_dir(const gchar * dirname, glong age);


/**
 * Initialise message logger.
 *
 * This function initialises the debug message logger. First, it
 * checks that the directory USER_CONF_DIR/logs/ exists, if not it
 * creates it.
 * Then, if there is a gpredict.log file it is either deleted or
 * renamed, depending on the sat-cfg settings.
 * Finally, a new gpredict.log file is created and opened.
 */
void sat_log_init()
{
    gchar          *dirname, *filename, *confdir;
    gboolean        err = FALSE;
    GError         *error = NULL;

    /* Check whether log directory exists, if not, create it */
    confdir = get_user_conf_dir();
    dirname = g_strconcat(confdir, G_DIR_SEPARATOR_S, "logs", NULL);
    g_free(confdir);

    if (!g_file_test(dirname, G_FILE_TEST_IS_DIR))
    {
        if (g_mkdir_with_parents(dirname, 0755))
        {
            /* print an error message */
            g_print(_("ERROR: Could not create %s\n"), dirname);
            err = TRUE;
        }
    }

    /* check if gpredict.log already exists */
    filename = g_strconcat(dirname, G_DIR_SEPARATOR_S, "gpredict.log", NULL);
    if (g_file_test(filename, G_FILE_TEST_EXISTS))
    {
        /* Note that logger shall clean up and perform log rotation
           upon exit, so we don't do anything here except removing
           the old file.
           The reason for this is that we need sat-cfg parameters for
           log rotation, but sat-cfg is not initialised at this time.
         */
        if (g_remove(filename))
            g_print(_("%s: Failed to delete old log file %s\n"),
                    __func__, filename);
    }

    /* create new gpredict.log file */
    logfile = g_io_channel_new_file(filename, "w", &error);
    if (error != NULL)
    {
        g_print(_("\n\nERROR: Failed to create %s\n%s\n\n"),
                filename, error->message);
        g_clear_error(&error);
        err = 1;
    }

    /* clean up */
    g_free(dirname);
    g_free(filename);

    if (!err)
    {
        initialised = TRUE;
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Session started"), __func__);
    }
}

/** Close message logger. */
void sat_log_close()
{
    if (initialised)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO, _("%s: Session ended"), __func__);
        g_io_channel_shutdown(logfile, TRUE, NULL);
        g_io_channel_unref(logfile);
        logfile = NULL;
        initialised = FALSE;

        /* Always call log_rotate to get rid of old logs */
        log_rotate();
    }
}


/** Log messages from gpredict */
void sat_log_log(sat_log_level_t level, const gchar * fmt, ...)
{
    gchar          *msg;        /* formatted debug message */
    gchar         **msgv;       /* debug message line by line */
    guint           numlines;   /* the number of lines in the message */
    guint           i;
    va_list         ap;

    if (level > loglevel)
        return;

    va_start(ap, fmt);

    /* create character string and split it in case
       it is a multi-line message */
    msg = g_strdup_vprintf(fmt, ap);

    /* remove trailing \n */
    g_strchomp(msg);

    /* split the message in case it is a multiline message */
    msgv = g_strsplit_set(msg, "\n", 0);
    numlines = g_strv_length(msgv);

    g_free(msg);

    /* for each line in msgv, call the real debug handler
       which will print the debug message and save it to
       a logfile
     */
    for (i = 0; i < numlines; i++)
        manage_debug_message(level, msgv[i]);

    va_end(ap);
    g_strfreev(msgv);
}

void sat_log_set_visible(gboolean visible)
{
    (void)visible;
}

void sat_log_set_level(sat_log_level_t level)
{
    if G_LIKELY
        (level <= SAT_LOG_LEVEL_DEBUG) loglevel = level;
}

static void manage_debug_message(sat_log_level_t debug_level,
                                 const gchar * message)
{
    gchar          *msg_time;
    GDateTime      *now;
    gchar          *msg;
    gsize           written;
    GError         *error = NULL;

    /* get the time */
    now = g_date_time_new_now_local();
    msg_time = g_date_time_format(now, "%Y/%m/%d %H:%M:%S");
    g_date_time_unref(now);

    /* send debug messages to stderr */
    if G_UNLIKELY(debug_to_stderr)
            g_fprintf(stderr, "%s  %s  %s\n", msg_time,
                      debug_level_str[debug_level], message);

    msg = g_strdup_printf("%s%s%d%s%s\n", msg_time, SAT_LOG_MSG_SEPARATOR,
                          debug_level, SAT_LOG_MSG_SEPARATOR, message);

    g_free(msg_time);

    /* print debug message */
    if G_LIKELY(initialised)
    {
        /* save to file */
        g_io_channel_write_chars(logfile, msg, -1, &written, &error);
        if G_UNLIKELY
            (error != NULL)
        {
            g_fprintf(stderr, "CRITICAL: LOG ERROR\n");
            g_clear_error(&error);
        }
        g_io_channel_flush(logfile, NULL);
    }
    else
    {
        /* send to stderr */
        g_fprintf(stderr, "%s", msg);
    }

    g_free(msg);
}

/** Perform log rotation and other maintenance in log directory */
static void log_rotate()
{
    gint64          now;        /* current time */
    glong           age;        /* age for cleaning */
    glong           then;       /* time in sec corresponding to age */
    gchar          *confdir, *dirname, *fname1, *fname2;

    /* get cleaning age; if age non-zero do not rename log */
    age = sat_cfg_get_int(SAT_CFG_INT_LOG_CLEAN_AGE);

    /* initialise some vars */
    now = g_get_real_time() / G_USEC_PER_SEC;
    confdir = get_user_conf_dir();
    dirname = g_strconcat(confdir, G_DIR_SEPARATOR_S, "logs", NULL);

    fname1 = g_strconcat(dirname, G_DIR_SEPARATOR_S, "gpredict.log", NULL);
    fname2 = g_strdup_printf("%s%sgpredict-%ld.log",
                             dirname, G_DIR_SEPARATOR_S, now);

    if (age > 0)
    {
        if (g_rename(fname1, fname2))
            g_print(_("%s: Failed to rename %s to %s\n"), __func__,
                    fname1, fname2);
    }

    /* calculate age for files that should be removed */
    then = now - age;

    /* cleanup every time as old log files should be deleted if they exist */
    clean_log_dir(dirname, then);

    g_free(dirname);
    g_free(fname1);
    g_free(fname2);
    g_free(confdir);
}

/** Scan directory for .log files and remove those which are older than age. */
static void clean_log_dir(const gchar * dirname, glong age)
{
    GDir           *dir;        /* directory handle */
    GError         *error = NULL;       /* error handle */
    const gchar    *fname;      /* current file name */
    gchar         **vbuf;       /* string vector buffer */
    gchar          *ages;       /* age as string */
    gchar          *buff;


    ages = g_strdup_printf("%ld", age);

    dir = g_dir_open(dirname, 0, &error);

    /* failed to open dir */
    if (error != NULL)
    {
        g_clear_error(&error);
        return;
    }

    while ((fname = g_dir_read_name(dir)) != NULL)
    {
        /* ensure this is a .log file */
        if G_LIKELY
            (g_str_has_suffix(fname, ".log"))
        {
            vbuf = g_strsplit_set(fname, "-.", -1);

            /* Remove file if too old */
            if (g_ascii_strcasecmp(vbuf[1], ages) <= 0)
            {
                buff = g_strconcat(dirname, G_DIR_SEPARATOR_S, fname, NULL);
                if (g_remove(buff))
                    g_print(_("%s: Failed to delete old log file %s\n"),
                            __func__, buff);

                g_free(buff);
            }

            g_strfreev(vbuf);
        }

    }

    g_dir_close(dir);
    g_free(ages);
}
