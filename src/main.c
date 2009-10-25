/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.

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
 * Add some more text.
 *
 * \bug Change to glib getopt in 2.6
 *
 */
#include <stdlib.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#ifdef HAVE_GETOPT_H
#  include <getopt.h>
#endif
#include "sat-log.h"
#include "first-time.h"
#include "compat.h"
#include "gui.h"
#include "mod-mgr.h"
#include "tle-update.h"
#include "sat-cfg.h"
#include "sat-debugger.h"

/** \brief Main application widget. */
GtkWidget *app;


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

static void test_ui (void);


int main (int argc, char *argv[])
{
    guint  error = 0;

#ifdef G_OS_WIN32
    printf ("Starting gpredict. This may take some time...\n");
#endif


#ifdef ENABLE_NLS
    bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif
    gtk_set_locale ();
    gtk_init (&argc, &argv);

    /* start logger first, so that we can catch error messages if any */
    sat_log_init ();
	
	if (!g_thread_supported ())
		g_thread_init (NULL);

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

    //test_ui ();
    
    gtk_main ();

    return 0;
}

static void test_ui (void)
{
    gchar *fnam;
    
    fnam = g_strconcat (g_get_home_dir(), G_DIR_SEPARATOR_S,
                        "test.ui", NULL);
    
    g_free (fnam);
}


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
    /* 	satlog_log (SAT_LOG_ERROR, "Received signal: %d\n", sig); */
    /* 	satlog_log (SAT_LOG_ERROR, "Trying clean exit...\n"); */

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
    /*	gtk_window_get_size (GTK_WINDOW (app), &w, &h);
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
    glong last,now,thrld;
    GTimeVal   tval;
    GtkWidget *dialog;
    GError    *err = NULL;


    /* 	sat_log_log (SAT_LOG_LEVEL_DEBUG, */
    /* 				 _("%s: Checking whether TLE check should be executed..."), */
    /* 				 __FUNCTION__); */

    /* get time of last update */
    last = sat_cfg_get_int (SAT_CFG_INT_TLE_LAST_UPDATE);

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
        /* 		sat_log_log (SAT_LOG_LEVEL_DEBUG, */
        /* 					 _("%s: Threshold has not been passed yet."), */
        /* 					 __FUNCTION__, last, now, thrld); */
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
