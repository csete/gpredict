/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sat-log.h"
#include "sat-log-browser.h"
#include "compat.h"


/* columns in the message list */
typedef enum {
    MSG_LIST_COL_TIME = 0,
    MSG_LIST_COL_LEVEL,
    MSG_LIST_COL_MSG,
    MSG_LIST_COL_NUMBER
} msg_list_col_t;

/* data structure to hold one message */
typedef struct {
    time_t          time;       /* time stamp */
    sat_log_level_t level;      /* debug level */
    gchar          *message;    /* the message itself */
} message_t;

/* Easy access to column titles */
const gchar    *MSG_LIST_COL_TITLE[MSG_LIST_COL_NUMBER] = {
    N_("Time"),
    N_("Level"),
    N_("Message")
};

const gfloat    MSG_LIST_COL_TITLE_ALIGN[MSG_LIST_COL_NUMBER] = {
    0.5, 0.5, 0.0
};

const gchar    *DEBUG_STR[6] = {
    N_("NONE"),
    N_("ERROR"),
    N_("WARNING"),
    N_("INFO"),
    N_("DEBUG")
};


extern GtkWidget *app;


static gboolean initialised = FALSE;    /* Is module initialised? */

/* counters */
static guint32  errors = 0;     /* Number of error messages */
static guint32  warnings = 0;   /* Number of warning messages */
static guint32  infos = 0;      /* Number of verbose messages */
static guint32  debugs = 0;     /* Number of trace messages */

/* summary labels; they need to be accessible at runtime */
static GtkWidget *errorlabel, *warnlabel, *infolabel, *debuglabel, *sumlabel;

/* The message window itself */
static GtkWidget *window;

/* the tree view model */
GtkTreeModel   *model;


static gint     message_window_delete(GtkWidget *, GdkEvent *, gpointer);
static void     message_window_destroy(GtkWidget *, gpointer);
static void     message_window_response(GtkWidget *, gint, gpointer);

/* message list and tree widget functions */
static GtkWidget *create_message_list(void);
static GtkTreeModel *create_list_model(void);
static GtkWidget *create_message_summary(void);

/* load debug file related */
static void     load_debug_file(GtkWidget * parent);
static int      read_debug_file(const gchar * filename);
static void     clear_message_list(void);

static void     add_debug_message(const gchar * datetime,
                                  sat_log_level_t debug_level,
                                  const char *message);

/* Initialise message window.
 *
 * This function creates the message window and allocates all the internal
 * data structures. The function should be called when the main program
 * is initialised.
 */
void sat_log_browser_open()
{
    GtkWidget      *hbox;
    gchar          *fname;
    gchar          *confdir;
    gchar          *title;
    gint            error;      /* error code returned by by read_debug_file */

    if (!initialised)
    {

        hbox = gtk_hbox_new(FALSE, 10);
        gtk_box_pack_start(GTK_BOX(hbox), create_message_list(),
                           TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(hbox),
                           create_message_summary(), FALSE, TRUE, 0);

        /* create dialog window; we use "fake" stock responses to catch user
           button clicks (save_as and pause)
         */
        window = gtk_dialog_new_with_buttons(_("Log Browser"),
                                             GTK_WINDOW(app),
                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_STOCK_OPEN,
                                             GTK_RESPONSE_YES,
                                             GTK_STOCK_CLEAR,
                                             GTK_RESPONSE_NO,
                                             GTK_STOCK_CLOSE,
                                             GTK_RESPONSE_CLOSE, NULL);

        gtk_window_set_default_size(GTK_WINDOW(window), 800, 300);

        gtk_container_add(GTK_CONTAINER
                          (gtk_dialog_get_content_area(GTK_DIALOG(window))),
                          hbox);

        /* connect response signal */
        g_signal_connect(G_OBJECT(window), "response",
                         G_CALLBACK(message_window_response), NULL);

        /* connect delete and destroy signals */
        g_signal_connect(G_OBJECT(window), "delete_event",
                         G_CALLBACK(message_window_delete), NULL);
        g_signal_connect(G_OBJECT(window), "destroy",
                         G_CALLBACK(message_window_destroy), NULL);

        gtk_widget_show_all(window);

        /* read gpredict.log by default */
        confdir = get_user_conf_dir();
        fname = g_strconcat(confdir, G_DIR_SEPARATOR_S,
                            "logs", G_DIR_SEPARATOR_S, "gpredict.log", NULL);

        error = read_debug_file(fname);

        if (error == 0)
        {
            /* update window title if file read cleanly */
            title = g_strdup_printf(_("Log Browser: %s"), fname);
            gtk_window_set_title(GTK_WINDOW(window), title);

            g_free(title);
        }
        else
        {
            /* Remove filename if file not read */
            gtk_window_set_title(GTK_WINDOW(window), _("Log Browser"));
        }

        g_free(fname);
        g_free(confdir);

        initialised = TRUE;
    }
}


/** \brief Add a message to message list */
static void add_debug_message(const gchar * datetime,
                              sat_log_level_t debug_level, const char *message)
{
    guint           total;      /* totalt number of messages */
    gchar          *str;        /* string to show message count */
    GtkTreeIter     item;       /* new item added to the list store */

    gtk_list_store_append(GTK_LIST_STORE(model), &item);
    gtk_list_store_set(GTK_LIST_STORE(model), &item,
                       MSG_LIST_COL_TIME, datetime,
                       MSG_LIST_COL_LEVEL, _(DEBUG_STR[debug_level]),
                       MSG_LIST_COL_MSG, message, -1);

    /* increment severity counter */
    switch (debug_level)
    {

        /* runtime error */
    case SAT_LOG_LEVEL_ERROR:
        errors++;
        str = g_strdup_printf("%d", errors);
        gtk_label_set_text(GTK_LABEL(errorlabel), str);
        g_free(str);
        break;

        /* warning */
    case SAT_LOG_LEVEL_WARN:
        warnings++;
        str = g_strdup_printf("%d", warnings);
        gtk_label_set_text(GTK_LABEL(warnlabel), str);
        g_free(str);
        break;

        /* info */
    case SAT_LOG_LEVEL_INFO:
        infos++;
        str = g_strdup_printf("%d", infos);
        gtk_label_set_text(GTK_LABEL(infolabel), str);
        g_free(str);
        break;

        /* debug */
    case SAT_LOG_LEVEL_DEBUG:
        debugs++;
        str = g_strdup_printf("%d", debugs);
        gtk_label_set_text(GTK_LABEL(debuglabel), str);
        g_free(str);
        break;

    default:
        break;
    }

    /* the sum does not have to be updated for each line */
    total = errors + warnings + infos + debugs;
    str = g_strdup_printf("<b>%d</b>", total);
    gtk_label_set_markup(GTK_LABEL(sumlabel), str);
    g_free(str);

}

/*** FIXME: does not seem to be necessary */
static gint message_window_delete(GtkWidget * widget, GdkEvent * event,
                                  gpointer data)
{
    (void)widget;
    (void)event;
    (void)data;

    /* return FALSE to indicate that message window
       should be destroyed */
    return FALSE;
}


/* callback function called when the dialog window is destroyed */
static void message_window_destroy(GtkWidget * widget, gpointer data)
{

    (void)widget;
    (void)data;

    /* clean up memory */
    /* GSList, ... */

    initialised = FALSE;
}


/* callback function called when a dialog button is clicked */
static void
message_window_response(GtkWidget * widget, gint response, gpointer data)
{
    (void)data;                 /* avoid unused parameter compiler warning */

    switch (response)
    {
        /* close button */
    case GTK_RESPONSE_CLOSE:
        gtk_widget_destroy(widget);
        break;

        /* OPEN button */
    case GTK_RESPONSE_YES:
        load_debug_file(widget);
        break;

        /* CLEAR button */
    case GTK_RESPONSE_NO:
        clear_message_list();
        break;

    default:
        break;
    }
}


/** \brief Load debug file.
 *
 * This function creates the file chooser dialog, which can be used to select
 * a file containing debug messages. When the dialog returns, the selected
 * file is checked and, if the file exists, is read line by line.
 */
static void load_debug_file(GtkWidget * parent)
{
    gchar          *confdir;
    gchar          *filename;
    gchar          *title;
    gint            error;      /* error code returned by by read_debug_file */


    GtkWidget      *dialog;

    /* create file chooser dialog */
    dialog = gtk_file_chooser_dialog_new(_("Select Log File"),
                                         GTK_WINDOW(parent),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    confdir = get_user_conf_dir();
    filename = g_strconcat(confdir, G_DIR_SEPARATOR_S, "logs", NULL);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), filename);
    g_free(filename);
    g_free(confdir);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {

        clear_message_list();

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        /* sanity check of filename will be performed
           in read_debug_file */
        error = read_debug_file(filename);
        if (error == 0)
        {
            /* Update title with filename */
            title = g_strdup_printf(_("Log Browser: %s"), filename);
            gtk_window_set_title(GTK_WINDOW(window), title);
            g_free(title);
        }
        else
        {
            /* clear filename from title if unable to read file */
            gtk_window_set_title(GTK_WINDOW(window), _("Log Browser"));
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);

}


/** \brief Read contents of debug file. */
static int read_debug_file(const gchar * filename)
{
    GIOChannel     *logfile = NULL;     /* the log file */
    GError         *error = NULL;       /* error structure */
    gint            errorcode = 0;      /* error code returned by function */
    gchar          *line;       /* line read from file */
    gsize           length;     /* length of line read from file */
    gchar         **buff;


    /* check file and read contents */
    if (g_file_test(filename, G_FILE_TEST_EXISTS))
    {

        /* open file */
        logfile = g_io_channel_new_file(filename, "r", &error);

        if (logfile)
        {
            /* read the file line by line */
            while (g_io_channel_read_line(logfile,
                                          &line,
                                          &length,
                                          NULL, NULL) != G_IO_STATUS_EOF)
            {

                /* trim line and split it */
                line = g_strdelimit(line, "\n", '\0');

                buff = g_strsplit(line, SAT_LOG_MSG_SEPARATOR, MSG_LIST_COL_NUMBER + 1); // +1 because we used to have a msg source (pre 1.4)

                /* Gpredict 1.3 and earlier had 4 fields:
                 *   buff[0]: Date and time
                 *   buff[1]: Message source
                 *   buff[2]: Message type
                 *   buff[3]: Message
                 * As of 1.4 we no longer have message source:
                 *   buff[0]: Date and time
                 *   buff[1]: Message type
                 *   buff[2]: Message
                 * 
                 * Gpredicxt 1.4+ will be able to read logs generated by earlier versions
                 * but not the other way around.
                 */
                switch (g_strv_length(buff))
                {

                case 1:
                    add_debug_message("", SAT_LOG_LEVEL_ERROR, buff[0]);
                    break;

                case 3:
                    /* v1.4 and later */
                    add_debug_message(buff[0],
                                      (guint) g_ascii_strtod(buff[1], NULL),
                                      buff[2]);
                    break;

                case 4:
                    /* v1.3 and earlier with message source */
                    add_debug_message(buff[0],  /* buff[1], - used to be message src */
                                      (guint) g_ascii_strtod(buff[2], NULL),
                                      buff[3]);
                    break;
                default:
                    add_debug_message("", SAT_LOG_LEVEL_ERROR,
                                      _("Log file is corrupt"));
                    break;
                }

                /* clean up */
                g_free(line);
                g_strfreev(buff);
            }

            errorcode = 0;

            /* Close IO chanel; don't care about status.
               Shutdown will flush the stream and close the channel
               as soon as the reference count is dropped. Order matters!
             */
            g_io_channel_shutdown(logfile, TRUE, NULL);
            g_io_channel_unref(logfile);

        }
        else
        {
            /* an error occured */
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%d: Error open debug log (%s)"),
                        __FILE__, __LINE__, error->message);

            g_clear_error(&error);

            errorcode = 1;
        }
    }
    else
    {
        errorcode = 1;
    }

    return errorcode;
}


/** \brief Clear the message list
 *
 * Besides clearing the message list, the function also resets
 * the counters and set the text of the corresponding widgets
 * to zero.
 */
static void clear_message_list()
{
    /* clear the meaase list */
    gtk_list_store_clear(GTK_LIST_STORE(model));

    /* reset the counters and text widgets */
    errors = 0;
    warnings = 0;
    debugs = 0;

    gtk_label_set_text(GTK_LABEL(errorlabel), "0");
    gtk_label_set_text(GTK_LABEL(warnlabel), "0");
    gtk_label_set_text(GTK_LABEL(infolabel), "0");
    gtk_label_set_text(GTK_LABEL(debuglabel), "0");
    gtk_label_set_markup(GTK_LABEL(sumlabel), "<b>0</b>");
}



/* Create list view */
static GtkWidget *create_message_list()
{
    GtkWidget      *treeview;   /* high level treev iew widget */
    GtkWidget      *swin;       /* scrolled window containing the tree view */
    GtkCellRenderer *renderer;  /* cell renderer used to create a column */
    GtkTreeViewColumn *column;  /* place holder for a tree view column */

    guint           i;

    treeview = gtk_tree_view_new();
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);

    for (i = 0; i < MSG_LIST_COL_NUMBER; i++)
    {
        renderer = gtk_cell_renderer_text_new();
        column =
            gtk_tree_view_column_new_with_attributes(_(MSG_LIST_COL_TITLE[i]),
                                                     renderer, "text", i,
                                                     NULL);
        gtk_tree_view_insert_column(GTK_TREE_VIEW(treeview), column, -1);

        /* only aligns the headers? */
        gtk_tree_view_column_set_alignment(column,
                                           MSG_LIST_COL_TITLE_ALIGN[i]);
    }

    /* create tree view model and finalise tree view */
    model = create_list_model();
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);
    g_object_unref(model);

    /* treeview is packed into a scroleld window */
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swin), treeview);

    return swin;
}


/* create tree view model; we actually create a GtkListStore because we are
   only interested in a flat list. A GtkListStore can be cast to a GtkTreeModel
   without any problems.
*/
static GtkTreeModel *create_list_model()
{
    GtkListStore   *liststore;

    liststore = gtk_list_store_new(MSG_LIST_COL_NUMBER, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING);

    /*** Fill existing data into the list here ***/

    return GTK_TREE_MODEL(liststore);
}


/* create summary */
static GtkWidget *create_message_summary()
{
    GtkWidget      *vbox;
    GtkWidget      *table;      /* table containing everything */
    GtkWidget      *frame;      /* surrounding frame */
    GtkWidget      *label;      /* dummy label */

    /* create labels */
    errorlabel = gtk_label_new("0");
    gtk_misc_set_alignment(GTK_MISC(errorlabel), 1.0, 0.5);

    warnlabel = gtk_label_new("0");
    gtk_misc_set_alignment(GTK_MISC(warnlabel), 1.0, 0.5);

    infolabel = gtk_label_new("0");
    gtk_misc_set_alignment(GTK_MISC(infolabel), 1.0, 0.5);

    debuglabel = gtk_label_new("0");
    gtk_misc_set_alignment(GTK_MISC(debuglabel), 1.0, 0.5);

    sumlabel = gtk_label_new(NULL);
    gtk_label_set_use_markup(GTK_LABEL(sumlabel), TRUE);
    gtk_label_set_markup(GTK_LABEL(sumlabel), "<b>0</b>");
    gtk_misc_set_alignment(GTK_MISC(sumlabel), 1.0, 0.5);

    /* create table and add widgets */
    table = gtk_table_new(6, 2, TRUE);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_container_set_border_width(GTK_CONTAINER(table), 10);

    label = gtk_label_new(_("Errors"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

    label = gtk_label_new(_("Warnings"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    label = gtk_label_new(_("Info"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

    label = gtk_label_new(_("Debug"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);

    gtk_table_attach_defaults(GTK_TABLE(table),
                              gtk_hseparator_new(), 0, 2, 4, 5);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Total</b>"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 5, 6);

    gtk_table_attach_defaults(GTK_TABLE(table), errorlabel, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), warnlabel, 1, 2, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), infolabel, 1, 2, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), debuglabel, 1, 2, 3, 4);
    gtk_table_attach_defaults(GTK_TABLE(table), sumlabel, 1, 2, 5, 6);

    /* frame around the table */
    frame = gtk_frame_new(_(" Summary "));
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(frame), table);

    /* pack frame into vbox so that it doesn't gets streched vertically */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    return vbox;
}
