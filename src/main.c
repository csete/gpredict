/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2010  Alexandru Csete, OZ9AEC.

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
/** \file    main.c
 *  \ingroup main
 *  \bief    Main program file.
 *
 */
#include <stdlib.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "sat-log.h"
#include "first-time.h"
#include "compat.h"
#include "gui.h"
#include "mod-mgr.h"
#include "tle-update.h"
#include "sat-cfg.h"
#include "gtk-sat-selector.h"
#include "sat-debugger.h"

#ifdef WIN32
#include <winsock2.h>
#endif

/** \brief Main application widget. */
GtkWidget *app;


/** \brief Command line flag for cleaning TLE data. */
static gboolean cleantle = FALSE;

/** \brief Command line flag for cleaning TRSP data */
static gboolean cleantrsp = FALSE;

/** \brief Command line options. */
static GOptionEntry entries[] =
{
  { "clean-tle", 0, 0, G_OPTION_ARG_NONE, &cleantle, "Clean the TLE data in user's configuration directory", NULL },
  { "clean-trsp", 0, 0, G_OPTION_ARG_NONE, &cleantrsp, "Clean the transponder data in user's configuration directory", NULL },
  { NULL }
};



const gchar *dummy = N_("just to have a pot");


/* ID of TLE monitoring task */
static guint tle_mon_id = 0;

/* flag indicating whether TLE update is running */
static gboolean tle_upd_running = FALSE;

/* flag indicating whether user has been notified of TLE update */
static gboolean tle_upd_note_sent = FALSE;


/* private funtion prototypes */
static void     gpredict_app_create   (void);
static gint     gpredict_app_delete   (GtkWidget *, GdkEvent *, gpointer);
static void     gpredict_app_destroy  (GtkWidget *, gpointer);
static gboolean gpredict_app_config   (GtkWidget *, GdkEventConfigure *, gpointer);
static void     gpredict_sig_handler  (int sig);
static gboolean tle_mon_task          (gpointer data);
static void     tle_mon_stop          (void);
static gpointer update_tle_thread     (gpointer data);
static void     clean_tle             (void);
static void     clean_trsp            (void);

#ifdef G_OS_WIN32
  static void InitWinSock2(void);
  static void CloseWinSock2(void);
#endif


int main (int argc, char *argv[])
{
    GError *err = NULL;
    GOptionContext *context;
    guint  error = 0;


#ifdef G_OS_WIN32
    printf ("Starting gpredict. This may take some time...\n");
#endif


#ifdef ENABLE_NLS
    bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif
    gtk_init (&argc, &argv);

    context = g_option_context_new ("");
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_set_summary (context,
                                  _("Gpredict is a graphical real-time satellite tracking "\
                                    "and orbit prediction program.\n"\
                                    "Gpredict does not require any command line options for "\
                                    "nominal operation."));
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (!g_option_context_parse (context, &argc, &argv, &err)) {
        g_print (_("Option parsing failed: %s\n"), err->message);
    }


    /* start logger first, so that we can catch error messages if any */
    sat_log_init ();
     
    if (!g_thread_supported ())
        g_thread_init (NULL);

    /* check command line options */
    if (cleantle)
        clean_tle ();

    if (cleantrsp)
        clean_trsp ();

    /* check that user settings are ok */
    error = first_time_check_run ();


    if (error) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: User config check failed (code %d). This is fatal.\n"\
                       "A possible solution would be to remove the .config/Gpredict data dir\n"\
                       "in your home directory"),
                     __FUNCTION__, error);

        return 1;
    }

    sat_cfg_load ();

    /* get logging level */
    sat_log_set_level (sat_cfg_get_int (SAT_CFG_INT_LOG_LEVEL));

    /* create application */
    gpredict_app_create ();
    gtk_widget_show_all (app);

    //sat_debugger_run ();

    /* launch TLE monitoring task; 10 min interval */
    tle_mon_id = g_timeout_add (600000, tle_mon_task, NULL);

     
#ifdef WIN32
     // Initializing Windozze Sockets
    InitWinSock2();
#endif
    
    gtk_main ();

     g_option_context_free(context);
#ifdef WIN32
    // Cleanup Windozze Sockets
    CloseWinSock2();
#endif
    return 0;
}

#ifdef WIN32
/* This code was given from MSDN */
static void InitWinSock2(void)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD( 2, 2 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        return;
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions later    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if ( LOBYTE( wsaData.wVersion ) != 2 ||
         HIBYTE( wsaData.wVersion ) != 2 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup( );
        return;
    }
}

static void CloseWinSock2(void)
{
    WSACleanup();
}
#endif



/** \brief Create main application window.
 *  \return A new top level window as as GtkWidget.
 *
 * This function creates and initialises the main application window.
 * This function does not create any contents; that part is done in the
 * gpredict_gui package.
 *
 */
static void gpredict_app_create ()
{
    gchar     *title;     /* window title */
    gchar     *icon;      /* icon file name */

    /* create window title and file name for window icon  */
    title = g_strdup (_("GPREDICT"));
    icon = icon_file_name ("gpredict-icon.png");

    /* ceate window, add title and icon, restore size and position */
    app = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title (GTK_WINDOW (app), title);

    /* restore window position and size if requested by config */
    /* trunk/gtk/gtkblist.c */
    /* size is always restored */
    gtk_window_set_default_size (GTK_WINDOW (app),
                                 sat_cfg_get_int (SAT_CFG_INT_WINDOW_WIDTH),
                                 sat_cfg_get_int (SAT_CFG_INT_WINDOW_HEIGHT));

    /* position restored only if requested in config */
    if (sat_cfg_get_bool (SAT_CFG_BOOL_MAIN_WIN_POS)) {
        gtk_window_move (GTK_WINDOW (app),
                         sat_cfg_get_int (SAT_CFG_INT_WINDOW_POS_X),
                         sat_cfg_get_int (SAT_CFG_INT_WINDOW_POS_Y));
    }

    gtk_container_add (GTK_CONTAINER (app),
                       gui_create (app));

    if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
        gtk_window_set_icon_from_file (GTK_WINDOW (app),
                                       icon,
                                       NULL);
    }

    g_free (title);
    g_free (icon);

    /* connect delete and destroy signals */
    g_signal_connect (G_OBJECT (app), "delete_event",
                      G_CALLBACK (gpredict_app_delete), NULL);
    g_signal_connect (G_OBJECT (app), "configure_event",
                      G_CALLBACK (gpredict_app_config), NULL);
    g_signal_connect (G_OBJECT (app), "destroy",
                      G_CALLBACK (gpredict_app_destroy), NULL);

    /* register UNIX signals as well so that we
           have a chance to clean up external resources.
        */
    signal (SIGTERM, (void *) gpredict_sig_handler);
    signal (SIGINT,  (void *) gpredict_sig_handler);
    signal (SIGABRT, (void *) gpredict_sig_handler);

}



/** \brief Handle terminate signals.
 *  \param sig The signal that has been received.
 *
 * This function is used to handle termination signals received by the program.
 * The currently caught signals are SIGTERM, SIGINT and SIGABRT. When one of these
 * signals is received, the function sends an error message to logger and tries
 * to make a clean exit.
 */
static void
gpredict_sig_handler (int sig)
{
    /*      satlog_log (SAT_LOG_ERROR, "Received signal: %d\n", sig); */
    /*      satlog_log (SAT_LOG_ERROR, "Trying clean exit...\n"); */

    gtk_widget_destroy (app);
}


/** \brief Handle delete events.
 *  \param widget The widget which received the delete event signal.
 *  \param event  Data structure describing the event.
 *  \param data   User data (NULL).
 *  \param return Always FALSE to indicate that the app should be destroyed.
 *
 * This function handles the delete event received by the main application
 * window (eg. when the window is closed by the WM). This function simply
 * returns FALSE indicating that the main application window should be
 * destroyed by emiting the destroy signal.
 *
 */
static gint
gpredict_app_delete      (GtkWidget *widget,
                          GdkEvent  *event,
                          gpointer   data)
{
    return FALSE;
}



/** \brief Handle destroy signals.
 *  \param widget The widget which received the signal.
 *  \param data   User data (NULL).
 *
 * This function is called when the main application window receives the
 * destroy signal, ie. it is destroyed. This function signals all daemons
 * and other threads to stop and exits the Gtk+ main loop.
 *
 */
static void
gpredict_app_destroy    (GtkWidget *widget,
                         gpointer   data)
{

    /* stop TLE monitoring task */
    tle_mon_stop ();

    /* GUI timers are stopped automatically */

    /* stop timeouts */

    /* configuration data */
    mod_mgr_save_state ();

    /* not good, have to use configure event instead (see API doc) */
    /*     gtk_window_get_size (GTK_WINDOW (app), &w, &h);
                sat_cfg_set_int (SAT_CFG_INT_WINDOW_WIDTH, w);
                sat_cfg_set_int (SAT_CFG_INT_WINDOW_HEIGHT, h);
        */
    sat_cfg_save ();
    sat_log_close ();
    sat_cfg_close ();

    /* exit Gtk+ */
    gtk_main_quit ();
}



/** \brief Snoop window position and size when main window receives configure event.
 *  \param widget Pointer to the gpredict main window.
 *  \param event  Pointer to the even structure.
 *  \param data   Pointer to user data (always NULL).
 *
 * This function is used to trap configure events in order to store the current
 * position and size of the main window.
 *
 * \note unfortunately GdkEventConfigure ignores the window gravity, while
 *       the only way we have of setting the position doesn't. We have to
 *       call get_position because it does pay attention to the gravity.
 *
 * \note The logic in the code has been borrowed from gaim/pidgin http://pidgin.im/
 *
 */
static gboolean
gpredict_app_config   (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    gint x, y;


    /* data is only useful when window is visible */
    if (GTK_WIDGET_VISIBLE (widget))
        gtk_window_get_position (GTK_WINDOW (widget), &x, &y);
    else
        return FALSE; /* carry on normally */

#ifdef G_OS_WIN32
    /* Workaround for GTK+ bug # 169811 - "configure_event" is fired
           when the window is being maximized */
    if (gdk_window_get_state (widget->window) & GDK_WINDOW_STATE_MAXIMIZED) {
        return FALSE;
    }
#endif

    /* don't save off-screen positioning */
    if (x + event->width < 0 || y + event->height < 0 ||
        x > gdk_screen_width() || y > gdk_screen_height()) {

        return FALSE; /* carry on normally */
    }

    /* store the position and size */
    sat_cfg_set_int (SAT_CFG_INT_WINDOW_POS_X, x);
    sat_cfg_set_int (SAT_CFG_INT_WINDOW_POS_Y, y);
    sat_cfg_set_int (SAT_CFG_INT_WINDOW_WIDTH, event->width);
    sat_cfg_set_int (SAT_CFG_INT_WINDOW_HEIGHT, event->height);

    /* continue to handle event normally */
    return FALSE;
}


/** \brief Monitor TLE age.
 *
 * This function is called periodically in order to check
 * whether it is time to update the TLE elements.
 *
 * If the time to update the TLE has come, it will either notify
 * the user, or fork a separate task which will update the TLE data
 * in the background (depending on user settings.
 *
 * In case of notification, the task will be removed in order to
 * avoid a new notification the next time the taks would be run.
 */
static gboolean
tle_mon_task          (gpointer data)
{
    /*GtkWidget *selector;*/
    glong last,now,thrld;
    GTimeVal   tval;
    GtkWidget *dialog;
    GError    *err = NULL;


    /*      sat_log_log (SAT_LOG_LEVEL_DEBUG, */
    /*                      _("%s: Checking whether TLE check should be executed..."), */
    /*                      __FUNCTION__); */

    /* get time of last update */
    last = sat_cfg_get_int (SAT_CFG_INT_TLE_LAST_UPDATE);
    /*if (last == 0) {
        selector = gtk_sat_selector_new (0);
        last = (glong) gtk_sat_selector_get_latest_epoch (GTK_SAT_SELECTOR (selector));
        gtk_widget_destroy (selector);
    }

    g_print ("LAST: %ld\n", last); */


    /* get current time */
    g_get_current_time (&tval);
    now = tval.tv_sec;

    /* threshold */
    switch (sat_cfg_get_int (SAT_CFG_INT_TLE_AUTO_UPD_FREQ)) {

    case TLE_AUTO_UPDATE_MONTHLY:
        thrld = 2592000;
        break;

    case TLE_AUTO_UPDATE_WEEKLY:
        thrld = 604800;
        break;

    case TLE_AUTO_UPDATE_DAILY:
        thrld = 86400;
        break;

        /* set default to "infinite" */
    default:
        thrld = G_MAXLONG;
        break;
    }

    if ((now - last) < thrld) {
        /* too early */
        /*           sat_log_log (SAT_LOG_LEVEL_DEBUG, */
        /*                           _("%s: Threshold has not been passed yet."), */
        /*                           __FUNCTION__, last, now, thrld); */
    }
    else {
        /* time to update */
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: Time threshold has been passed."),
                     __FUNCTION__);

        /* find out what to do */
        if (sat_cfg_get_int (SAT_CFG_INT_TLE_AUTO_UPD_ACTION) == TLE_AUTO_UPDATE_GOAHEAD) {

            /* start update process in separate thread */
            sat_log_log (SAT_LOG_LEVEL_DEBUG,
                         _("%s: Starting new update thread."),
                         __FUNCTION__);


            g_thread_create (update_tle_thread, NULL, FALSE, &err);

            if (err != NULL)
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: Failed to create TLE update thread (%s)"),
                             __FUNCTION__, err->message);

        }
        else if (!tle_upd_note_sent) {
            /* notify user */
            dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (app),
                                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                                         GTK_MESSAGE_INFO,
                                                         GTK_BUTTONS_OK,
                                                         _("Your TLE files are getting out of date.\n"\
                                                           "You can update them by selecting\n"\
                                                           "<b>Edit -> Update TLE</b>\n"\
                                                           "in the menubar."));

            /* Destroy the dialog when the user responds to it (e.g. clicks a button) */
            g_signal_connect_swapped (dialog, "response",
                                      G_CALLBACK (gtk_widget_destroy),
                                      dialog);

            gtk_widget_show_all (dialog);

            tle_upd_note_sent = TRUE;
        }
    }

    return TRUE;
}


/** \brief Stop TLE monitoring and any pending updates. */
static void
tle_mon_stop          ()
{
    gboolean retcode;

    if (tle_mon_id) {
        retcode = g_source_remove (tle_mon_id);

        if (!retcode)
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Could not find TLE monitoring task (ID = %d)"),
                         __FUNCTION__, tle_mon_id);

    }

    /* if TLE update is running wait until it is finished */
    while (tle_upd_running) {
        g_usleep (1000);
    }
}



/** \brief Thread function which invokes TLE update */
static gpointer
update_tle_thread     (gpointer data)
{
    tle_upd_running = TRUE;

    tle_update_from_network (TRUE, NULL, NULL, NULL);

    tle_upd_running = FALSE;

    return NULL;
}


/** \brief Clean TLE data.
  *
  * This function removes all .sat files from the user's configuration directory.
  * The function is called when gpreidict is executed with the --clean-tle
  * command line option.
  */
static void clean_tle (void)
{
    GDir     *targetdir;
    gchar    *targetdirname,*path;
    const gchar *filename;


    /* Get trsp directory */
    targetdirname = get_satdata_dir ();
    targetdir = g_dir_open (targetdirname, 0, NULL);

    sat_log_log (SAT_LOG_LEVEL_MSG,
                 _("%s: Cleaning TLE data in %s"), __FUNCTION__, targetdirname);


    while ((filename = g_dir_read_name (targetdir))) {
        if (g_str_has_suffix (filename, ".sat")) {
            /* remove .sat file */
            path = sat_file_name (filename);
            if G_UNLIKELY(g_unlink (path)) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: Failed to delete %s"), __FUNCTION__, filename);
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: Removed %s"), __FUNCTION__, filename);
            }
            g_free (path);
        }
    }
    g_free (targetdirname);

}


/** \brief Clean transponder data.
  *
  * This function removes all .trsp files from the user's configuration directory.
  * The function is called when gpredict is executed with the --clean-trsp
  * command line option.
  */
static void clean_trsp (void)
{
    GDir     *targetdir;
    gchar    *targetdirname,*path;
    const gchar *filename;


    /* Get trsp directory */
    targetdirname = get_trsp_dir ();
    targetdir = g_dir_open (targetdirname, 0, NULL);

    sat_log_log (SAT_LOG_LEVEL_MSG,
                 _("%s: Cleaning transponder data in %s"), __FUNCTION__, targetdirname);


    while ((filename = g_dir_read_name (targetdir))) {
        if (g_str_has_suffix (filename, ".trsp")) {
            /* remove .trsp file */
            path = trsp_file_name (filename);
            if G_UNLIKELY(g_unlink (path)) {
                sat_log_log (SAT_LOG_LEVEL_ERROR,
                             _("%s: Failed to delete %s"), __FUNCTION__, filename);
            }
            else {
                sat_log_log (SAT_LOG_LEVEL_MSG,
                             _("%s: Removed %s"), __FUNCTION__, filename);
            }
            g_free (path);
        }
    }
    g_free (targetdirname);

}
