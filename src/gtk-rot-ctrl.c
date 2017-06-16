/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>
           Charles Suprin  <hamaa1vs@gmail.com>

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
 * \brief Antenna rotator control window.
 * \ingroup widgets
 *
 * The master rotator control UI is implemented as a Gtk+ Widget in order
 * to allow multiple instances. The widget is created from the module
 * popup menu and each module can have several rotator control windows
 * attached to it. Note, however, that current implementation only
 * allows one rotor control window per module.
 * 
 */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

/* NETWORK */
#ifndef WIN32
#include <arpa/inet.h>          /* htons() */
#include <netdb.h>              /* gethostbyname() */
#include <netinet/in.h>         /* struct sockaddr_in */
#include <sys/socket.h>         /* socket(), connect(), send() */
#else
#include <winsock2.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <math.h>

#include "compat.h"
#include "gpredict-utils.h"
#include "gtk-polar-plot.h"
#include "gtk-rot-knob.h"
#include "gtk-rot-ctrl.h"
#include "predict-tools.h"
#include "sat-log.h"


#define FMTSTR "%7.2f\302\260"
#define MAX_ERROR_COUNT 5

static void     gtk_rot_ctrl_class_init(GtkRotCtrlClass * class);
static void     gtk_rot_ctrl_init(GtkRotCtrl * list);
static void     gtk_rot_ctrl_destroy(GtkObject * object);

static GtkWidget *create_az_widgets(GtkRotCtrl * ctrl);
static GtkWidget *create_el_widgets(GtkRotCtrl * ctrl);
static GtkWidget *create_target_widgets(GtkRotCtrl * ctrl);
static GtkWidget *create_conf_widgets(GtkRotCtrl * ctrl);
static GtkWidget *create_plot_widget(GtkRotCtrl * ctrl);

static void     store_sats(gpointer key, gpointer value, gpointer user_data);

static void     sat_selected_cb(GtkComboBox * satsel, gpointer data);
static void     track_toggle_cb(GtkToggleButton * button, gpointer data);
static void     delay_changed_cb(GtkSpinButton * spin, gpointer data);
static void     toler_changed_cb(GtkSpinButton * spin, gpointer data);
static void     rot_selected_cb(GtkComboBox * box, gpointer data);
static void     rot_locked_cb(GtkToggleButton * button, gpointer data);
static gboolean rot_ctrl_timeout_cb(gpointer data);
static void     update_count_down(GtkRotCtrl * ctrl, gdouble t);

static gboolean get_pos(GtkRotCtrl * ctrl, gdouble * az, gdouble * el);
static gboolean set_pos(GtkRotCtrl * ctrl, gdouble az, gdouble el);

static gboolean send_rotctld_command(GtkRotCtrl * ctrl, gchar * buff,
                                     gchar * buffout, gint sizeout);
static gboolean open_rotctld_socket(GtkRotCtrl * ctrl);
static gboolean close_rotctld_socket(gint * sock);

static gboolean have_conf(void);
static gint     sat_name_compare(sat_t * a, sat_t * b);
static gint     rot_name_compare(const gchar * a, const gchar * b);

static gboolean is_flipped_pass(pass_t * pass, rot_az_type_t type,
                                gdouble azstoppos);
static inline void set_flipped_pass(GtkRotCtrl * ctrl);

static GtkVBoxClass *parent_class = NULL;


GType gtk_rot_ctrl_get_type()
{
    static GType    gtk_rot_ctrl_type = 0;

    if (!gtk_rot_ctrl_type)
    {
        static const GTypeInfo gtk_rot_ctrl_info = {
            sizeof(GtkRotCtrlClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_rot_ctrl_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkRotCtrl),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_rot_ctrl_init,
            NULL
        };

        gtk_rot_ctrl_type = g_type_register_static(GTK_TYPE_VBOX,
                                                   "GtkRotCtrl",
                                                   &gtk_rot_ctrl_info, 0);
    }

    return gtk_rot_ctrl_type;
}

static void gtk_rot_ctrl_class_init(GtkRotCtrlClass * class)
{
    GtkObjectClass *object_class;

    object_class = (GtkObjectClass *) class;
    parent_class = g_type_class_peek_parent(class);
    object_class->destroy = gtk_rot_ctrl_destroy;
}

static void gtk_rot_ctrl_init(GtkRotCtrl * ctrl)
{
    ctrl->sats = NULL;
    ctrl->target = NULL;
    ctrl->pass = NULL;
    ctrl->qth = NULL;
    ctrl->plot = NULL;
    ctrl->sock = 0;

    ctrl->tracking = FALSE;
    g_mutex_init(&(ctrl->busy));
    ctrl->engaged = FALSE;
    ctrl->delay = 1000;
    ctrl->timerid = 0;
    ctrl->tolerance = 5.0;
    ctrl->errcnt = 0;
}

static void gtk_rot_ctrl_destroy(GtkObject * object)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(object);

    /* stop timer */
    if (ctrl->timerid > 0)
        g_source_remove(ctrl->timerid);

    /* free configuration */
    if (ctrl->conf != NULL)
    {
        g_free(ctrl->conf->name);
        g_free(ctrl->conf->host);
        g_free(ctrl->conf);
        ctrl->conf = NULL;
    }

    /*close the socket if it is still open */
    if (ctrl->sock != 0)
        close_rotctld_socket(&(ctrl->sock));

    (*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

/**
 * \brief Create a new rotor control widget.
 * \return A new rotor control window.
 */
GtkWidget      *gtk_rot_ctrl_new(GtkSatModule * module)
{
    GtkWidget      *widget;
    GtkWidget      *table;

    /* check that we have rot conf */
    if (!have_conf())
        return NULL;

    widget = g_object_new(GTK_TYPE_ROT_CTRL, NULL);

    /* store satellites */
    g_hash_table_foreach(module->satellites, store_sats, widget);

    GTK_ROT_CTRL(widget)->target =
        SAT(g_slist_nth_data(GTK_ROT_CTRL(widget)->sats, 0));

    /* store current time (don't know if real or simulated) */
    GTK_ROT_CTRL(widget)->t = module->tmgCdnum;

    /* store QTH */
    GTK_ROT_CTRL(widget)->qth = module->qth;

    /* get next pass for target satellite */
    if (GTK_ROT_CTRL(widget)->target)
    {
        if (GTK_ROT_CTRL(widget)->target->el > 0.0)
        {
            GTK_ROT_CTRL(widget)->pass =
                get_current_pass(GTK_ROT_CTRL(widget)->target,
                                 GTK_ROT_CTRL(widget)->qth, 0.0);
        }
        else
        {
            GTK_ROT_CTRL(widget)->pass =
                get_next_pass(GTK_ROT_CTRL(widget)->target,
                              GTK_ROT_CTRL(widget)->qth, 3.0);
        }
    }

    /* create contents */
    table = gtk_table_new(2, 3, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 0);
    gtk_table_set_col_spacings(GTK_TABLE(table), 0);
    gtk_container_set_border_width(GTK_CONTAINER(table), 10);
    gtk_table_attach(GTK_TABLE(table), create_az_widgets(GTK_ROT_CTRL(widget)),
                     0, 1, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(table), create_el_widgets(GTK_ROT_CTRL(widget)),
                     1, 2, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(table),
                     create_target_widgets(GTK_ROT_CTRL(widget)), 0, 1, 1, 2,
                     GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(table),
                     create_conf_widgets(GTK_ROT_CTRL(widget)), 1, 2, 1, 2,
                     GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(table),
                     create_plot_widget(GTK_ROT_CTRL(widget)), 2, 3, 0, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

    gtk_container_add(GTK_CONTAINER(widget), table);

    GTK_ROT_CTRL(widget)->timerid = g_timeout_add(GTK_ROT_CTRL(widget)->delay,
                                                  rot_ctrl_timeout_cb,
                                                  GTK_ROT_CTRL(widget));

    if (module->target > 0)
        gtk_rot_ctrl_select_sat(GTK_ROT_CTRL(widget), module->target);

    return widget;
}

/**
 * \brief Update rotator control state.
 * \param ctrl Pointer to the GtkRotCtrl.
 * 
 * This function is called by the parent, i.e. GtkSatModule, indicating that
 * the satellite data has been updated. The function updates the internal state
 * of the controller and the rotator.
 */
void gtk_rot_ctrl_update(GtkRotCtrl * ctrl, gdouble t)
{
    gchar          *buff;

    ctrl->t = t;

    if (ctrl->target)
    {
        /* update target displays */
        buff = g_strdup_printf(FMTSTR, ctrl->target->az);
        gtk_label_set_text(GTK_LABEL(ctrl->AzSat), buff);
        g_free(buff);
        buff = g_strdup_printf(FMTSTR, ctrl->target->el);
        gtk_label_set_text(GTK_LABEL(ctrl->ElSat), buff);
        g_free(buff);

        update_count_down(ctrl, t);

        /*if the current pass is too far away */
        if ((ctrl->pass != NULL))
            if (qth_small_dist(ctrl->qth, ctrl->pass->qth_comp) > 1.0)
            {
                free_pass(ctrl->pass);
                ctrl->pass = NULL;
                ctrl->pass = get_pass(ctrl->target, ctrl->qth, t, 3.0);
                if (ctrl->pass)
                {
                    set_flipped_pass(ctrl);
                    /* update polar plot */
                    gtk_polar_plot_set_pass(GTK_POLAR_PLOT(ctrl->plot),
                                            ctrl->pass);
                }
            }

        /* update next pass if necessary */
        if (ctrl->pass != NULL)
        {
            /* if we are not in the current pass */
            if ((ctrl->pass->aos > t) || (ctrl->pass->los < t))
            {
                /* the pass may not have met the minimum 
                   elevation, calculate the pass and plot it */
                if (ctrl->target->el >= 0.0)
                {
                    /* inside an unexpected/unpredicted pass */
                    free_pass(ctrl->pass);
                    ctrl->pass = NULL;
                    ctrl->pass = get_current_pass(ctrl->target, ctrl->qth, t);
                    set_flipped_pass(ctrl);
                    gtk_polar_plot_set_pass(GTK_POLAR_PLOT(ctrl->plot),
                                            ctrl->pass);
                }
                else if ((ctrl->target->aos - ctrl->pass->aos) >
                         (ctrl->delay / secday / 1000 / 4.0))
                {
                    /* the target is expected to appear in a new pass 
                       sufficiently later after the current pass says */

                    /* converted milliseconds to gpredict time and took a 
                       fraction of it as a threshold for deciding a new pass */

                    /* if the next pass is not the one for the target */
                    free_pass(ctrl->pass);
                    ctrl->pass = NULL;
                    ctrl->pass = get_pass(ctrl->target, ctrl->qth, t, 3.0);
                    set_flipped_pass(ctrl);
                    /* update polar plot */
                    gtk_polar_plot_set_pass(GTK_POLAR_PLOT(ctrl->plot),
                                            ctrl->pass);
                }
            }
            else
            {
                /* inside a pass and target dropped below the 
                   horizon so look for a new pass */
                if (ctrl->target->el < 0.0)
                {
                    free_pass(ctrl->pass);
                    ctrl->pass = NULL;
                    ctrl->pass = get_pass(ctrl->target, ctrl->qth, t, 3.0);
                    set_flipped_pass(ctrl);
                    /* update polar plot */
                    gtk_polar_plot_set_pass(GTK_POLAR_PLOT(ctrl->plot),
                                            ctrl->pass);
                }
            }
        }
        else
        {
            /* we don't have any current pass; store the current one */
            if (ctrl->target->el > 0.0)
                ctrl->pass = get_current_pass(ctrl->target, ctrl->qth, t);
            else
                ctrl->pass = get_pass(ctrl->target, ctrl->qth, t, 3.0);

            set_flipped_pass(ctrl);
            /* update polar plot */
            gtk_polar_plot_set_pass(GTK_POLAR_PLOT(ctrl->plot), ctrl->pass);
        }
    }
}

/** Select a satellite. */
void gtk_rot_ctrl_select_sat(GtkRotCtrl * ctrl, gint catnum)
{
    sat_t          *sat;
    int             i, n;

    /* find index in satellite list */
    n = g_slist_length(ctrl->sats);
    for (i = 0; i < n; i++)
    {
        sat = SAT(g_slist_nth_data(ctrl->sats, i));
        if (sat && sat->tle.catnr == catnum)
        {
            /* assume the index is the same in sat selector */
            gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->SatSel), i);
            break;
        }
    }
}

/**
 * \brief Create azimuth control widgets.
 * \param ctrl Pointer to the GtkRotCtrl widget.
 * 
 * This function creates and initialises the widgets for controlling the
 * azimuth of the the rotator.
 */
static GtkWidget *create_az_widgets(GtkRotCtrl * ctrl)
{
    GtkWidget      *frame;
    GtkWidget      *table;
    GtkWidget      *label;

    frame = gtk_frame_new(_("Azimuth"));

    table = gtk_table_new(2, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_container_add(GTK_CONTAINER(frame), table);

    ctrl->AzSet = gtk_rot_knob_new(0.0, 360.0, 180.0);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->AzSet, 0, 2, 0, 1);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("Read:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                     GTK_SHRINK, GTK_SHRINK, 10, 0);

    ctrl->AzRead = gtk_label_new(" --- ");
    gtk_misc_set_alignment(GTK_MISC(ctrl->AzRead), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->AzRead, 1, 2, 1, 2);

    return frame;
}

/**
 * \brief Create elevation control widgets.
 * \param ctrl Pointer to the GtkRotCtrl widget.
 * 
 * This function creates and initialises the widgets for controlling the
 * elevation of the the rotator.
 */
static GtkWidget *create_el_widgets(GtkRotCtrl * ctrl)
{
    GtkWidget      *frame;
    GtkWidget      *table;
    GtkWidget      *label;

    frame = gtk_frame_new(_("Elevation"));

    table = gtk_table_new(2, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_container_add(GTK_CONTAINER(frame), table);

    ctrl->ElSet = gtk_rot_knob_new(0.0, 90.0, 45.0);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->ElSet, 0, 2, 0, 1);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("Read: "));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                     GTK_SHRINK, GTK_SHRINK, 10, 0);

    ctrl->ElRead = gtk_label_new(" --- ");
    gtk_misc_set_alignment(GTK_MISC(ctrl->ElRead), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->ElRead, 1, 2, 1, 2);

    return frame;
}

/**
 * \brief Create target widgets.
 * \param ctrl Pointer to the GtkRotCtrl widget.
 */
static GtkWidget *create_target_widgets(GtkRotCtrl * ctrl)
{
    GtkWidget      *frame, *table, *label, *track;
    gchar          *buff;
    guint           i, n;
    sat_t          *sat = NULL;

    buff = g_strdup_printf(FMTSTR, 0.0);

    table = gtk_table_new(4, 3, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);

    /* sat selector */
    ctrl->SatSel = gtk_combo_box_text_new();
    n = g_slist_length(ctrl->sats);

    for (i = 0; i < n; i++)
    {
        sat = SAT(g_slist_nth_data(ctrl->sats, i));
        if (sat)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctrl->SatSel),
                                           sat->nickname);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->SatSel), 0);
    gtk_widget_set_tooltip_text(ctrl->SatSel, _("Select target object"));
    g_signal_connect(ctrl->SatSel, "changed", G_CALLBACK(sat_selected_cb),
                     ctrl);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->SatSel, 0, 2, 0, 1);

    /* tracking button */
    track = gtk_toggle_button_new_with_label(_("Track"));
    gtk_widget_set_tooltip_text(track,
                                _
                                ("Track the satellite when it is within range"));
    gtk_table_attach_defaults(GTK_TABLE(table), track, 2, 3, 0, 1);
    g_signal_connect(track, "toggled", G_CALLBACK(track_toggle_cb), ctrl);

    /* Azimuth */
    label = gtk_label_new(_("Az:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    ctrl->AzSat = gtk_label_new(buff);
    gtk_misc_set_alignment(GTK_MISC(ctrl->AzSat), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->AzSat, 1, 2, 1, 2);

    /* Elevation */
    label = gtk_label_new(_("El:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

    ctrl->ElSat = gtk_label_new(buff);
    gtk_misc_set_alignment(GTK_MISC(ctrl->ElSat), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->ElSat, 1, 2, 2, 3);

    /* count down */
    label = gtk_label_new(_("\316\224T:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
    ctrl->SatCnt = gtk_label_new("00:00:00");
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatCnt), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->SatCnt, 1, 2, 3, 4);

    frame = gtk_frame_new(_("Target"));
    gtk_container_add(GTK_CONTAINER(frame), table);

    g_free(buff);

    return frame;
}

static GtkWidget *create_conf_widgets(GtkRotCtrl * ctrl)
{
    GtkWidget      *frame, *table, *label, *timer, *toler;
    GDir           *dir = NULL; /* directory handle */
    GError         *error = NULL;       /* error flag and info */
    gchar          *dirname;    /* directory name */
    gchar         **vbuff;
    const gchar    *filename;   /* file name */
    gchar          *rotname;

    table = gtk_table_new(3, 3, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);

    label = gtk_label_new(_("Device:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

    ctrl->DevSel = gtk_combo_box_text_new();
    gtk_widget_set_tooltip_text(ctrl->DevSel,
                                _("Select antenna rotator device"));

    /* open configuration directory */
    dirname = get_hwconf_dir();

    dir = g_dir_open(dirname, 0, &error);
    if (dir)
    {
        /* read each .rot file */
        GSList         *rots = NULL;
        gint            i;
        gint            n;

        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".rot"))
            {
                vbuff = g_strsplit(filename, ".rot", 0);
                rots =
                    g_slist_insert_sorted(rots, g_strdup(vbuff[0]),
                                          (GCompareFunc) rot_name_compare);
                g_strfreev(vbuff);
            }
        }
        n = g_slist_length(rots);
        for (i = 0; i < n; i++)
        {
            rotname = g_slist_nth_data(rots, i);
            if (rotname)
            {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (ctrl->DevSel), rotname);
                g_free(rotname);
            }
        }
        g_slist_free(rots);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open hwconf dir (%s)"),
                    __FILE__, __LINE__, error->message);
        g_clear_error(&error);
    }

    g_free(dirname);
    g_dir_close(dir);

    gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->DevSel), 0);
    g_signal_connect(ctrl->DevSel, "changed", G_CALLBACK(rot_selected_cb),
                     ctrl);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->DevSel, 1, 2, 0, 1);

    /* Engage button */
    ctrl->LockBut = gtk_toggle_button_new_with_label(_("Engage"));
    gtk_widget_set_tooltip_text(ctrl->LockBut,
                                _("Engage the selected rotor device"));
    g_signal_connect(ctrl->LockBut, "toggled", G_CALLBACK(rot_locked_cb),
                     ctrl);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->LockBut, 2, 3, 0, 1);

    /* Timeout */
    label = gtk_label_new(_("Cycle:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    timer = gtk_spin_button_new_with_range(1000, 10000, 10);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(timer), 0);
    gtk_widget_set_tooltip_text(timer,
                                _("This parameter controls the delay between "
                                  "commands sent to the rotator."));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(timer), ctrl->delay);
    g_signal_connect(timer, "value-changed", G_CALLBACK(delay_changed_cb),
                     ctrl);
    gtk_table_attach(GTK_TABLE(table), timer, 1, 2, 1, 2, GTK_FILL, GTK_FILL,
                     0, 0);

    label = gtk_label_new(_("msec"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 1, 2);

    /* Tolerance */
    label = gtk_label_new(_("Tolerance:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

    toler = gtk_spin_button_new_with_range(0.01, 50.0, 0.01);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(toler), 2);
    gtk_widget_set_tooltip_text(toler,
                                _
                                ("This parameter controls the tolerance between "
                                 "the target and rotator values for the rotator.\n"
                                 "If the difference between the target and rotator values "
                                 "is smaller than the tolerance, no new commands are sent"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(toler), ctrl->tolerance);
    g_signal_connect(toler, "value-changed", G_CALLBACK(toler_changed_cb),
                     ctrl);
    gtk_table_attach(GTK_TABLE(table), toler, 1, 2, 2, 3, GTK_FILL, GTK_FILL,
                     0, 0);

    label = gtk_label_new(_("deg"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 3);

    /* load initial rotator configuration */
    rot_selected_cb(GTK_COMBO_BOX(ctrl->DevSel), ctrl);

    frame = gtk_frame_new(_("Settings"));
    gtk_container_add(GTK_CONTAINER(frame), table);

    return frame;
}

/**
 * \brief Create target widgets.
 * \param ctrl Pointer to the GtkRotCtrl widget.
 */
static GtkWidget *create_plot_widget(GtkRotCtrl * ctrl)
{
    GtkWidget      *frame;

    ctrl->plot = gtk_polar_plot_new(ctrl->qth, ctrl->pass);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), ctrl->plot);

    return frame;
}

/** Copy satellite from hash table to singly linked list. */
static void store_sats(gpointer key, gpointer value, gpointer user_data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(user_data);
    sat_t          *sat = SAT(value);

    (void)key;                  /* avoid unused variable warning */

    ctrl->sats = g_slist_insert_sorted(ctrl->sats, sat,
                                       (GCompareFunc) sat_name_compare);
}

/**
 ' \brief Manage satellite selections
 * \param satsel Pointer to the GtkComboBox.
 * \param data Pointer to the GtkRotCtrl widget.
 * 
 * This function is called when the user selects a new satellite.
 */
static void sat_selected_cb(GtkComboBox * satsel, gpointer data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(data);
    gint            i;

    i = gtk_combo_box_get_active(satsel);
    if (i >= 0)
    {
        ctrl->target = SAT(g_slist_nth_data(ctrl->sats, i));

        /* update next pass */
        if (ctrl->pass != NULL)
            free_pass(ctrl->pass);

        if (ctrl->target->el > 0.0)
            ctrl->pass = get_current_pass(ctrl->target, ctrl->qth, ctrl->t);
        else
            ctrl->pass = get_pass(ctrl->target, ctrl->qth, ctrl->t, 3.0);

        set_flipped_pass(ctrl);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Invalid satellite selection: %d"),
                    __FILE__, __func__, i);

        /* clear pass just in case... */
        if (ctrl->pass != NULL)
        {
            free_pass(ctrl->pass);
            ctrl->pass = NULL;
        }
    }

    /* in either case, we set the new pass (even if NULL) on the polar plot */
    if (ctrl->plot != NULL)
        gtk_polar_plot_set_pass(GTK_POLAR_PLOT(ctrl->plot), ctrl->pass);
}

/**
 * \brief Manage toggle signals (tracking)
 * \param button Pointer to the GtkToggle button.
 * \param data Pointer to the GtkRotCtrl widget.
 */
static void track_toggle_cb(GtkToggleButton * button, gpointer data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(data);

    ctrl->tracking = gtk_toggle_button_get_active(button);
}

/**
 * \brief Manage cycle delay changes.
 * \param spin Pointer to the spin button.
 * \param data Pointer to the GtkRotCtrl widget.
 * 
 * This function is called when the user changes the value of the
 * cycle delay.
 */
static void delay_changed_cb(GtkSpinButton * spin, gpointer data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(data);

    ctrl->delay = (guint) gtk_spin_button_get_value(spin);

    if (ctrl->timerid > 0)
        g_source_remove(ctrl->timerid);

    ctrl->timerid = g_timeout_add(ctrl->delay, rot_ctrl_timeout_cb, ctrl);
}

/**
 * \brief Manage tolerance changes.
 * \param spin Pointer to the spin button.
 * \param data Pointer to the GtkRotCtrl widget.
 * 
 * This function is called when the user changes the value of the
 * tolerance.
 */
static void toler_changed_cb(GtkSpinButton * spin, gpointer data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(data);

    ctrl->tolerance = gtk_spin_button_get_value(spin);
}

/**
 * \brief New rotor device selected.
 * \param box Pointer to the rotor selector combo box.
 * \param data Pointer to the GtkRotCtrl widget.
 * 
 * This function is called when the user selects a new rotor controller
 * device.
 */
static void rot_selected_cb(GtkComboBox * box, gpointer data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(data);

    /* free previous configuration */
    if (ctrl->conf != NULL)
    {
        g_free(ctrl->conf->name);
        g_free(ctrl->conf->host);
        g_free(ctrl->conf);
    }

    ctrl->conf = g_try_new(rotor_conf_t, 1);
    if (ctrl->conf == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to allocate memory for rotator config"),
                    __FILE__, __LINE__);
        return;
    }

    /* load new configuration */
    ctrl->conf->name =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box));
    if (rotor_conf_read(ctrl->conf))
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("Loaded new rotator configuration %s"),
                    ctrl->conf->name);

        /* update new ranges of the Az and El controller widgets */
        gtk_rot_knob_set_range(GTK_ROT_KNOB(ctrl->AzSet), ctrl->conf->minaz,
                               ctrl->conf->maxaz);
        gtk_rot_knob_set_range(GTK_ROT_KNOB(ctrl->ElSet), ctrl->conf->minel,
                               ctrl->conf->maxel);

        /* Update flipped when changing rotor if there is a plot */
        set_flipped_pass(ctrl);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to load rotator configuration %s"),
                    __FILE__, __LINE__, ctrl->conf->name);

        g_free(ctrl->conf->name);
        if (ctrl->conf->host)
            g_free(ctrl->conf->host);
        g_free(ctrl->conf);
        ctrl->conf = NULL;
    }
}

/**
 * \brief Rotor locked.
 * \param button Pointer to the "Engage" button.
 * \param data Pointer to the GtkRotCtrl widget.
 * 
 * This function is called when the user toggles the "Engage" button.
 */
static void rot_locked_cb(GtkToggleButton * button, gpointer data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(data);
    gchar          *buff;
    gchar           buffback[128];
    gboolean        retcode;
    gint            retval;

    if (!gtk_toggle_button_get_active(button))
    {
        /* DL4PD: issue #51: stop moving rotor */
        buff = g_strdup_printf("S\x0a");

        retcode = send_rotctld_command(ctrl, buff, buffback, 128);

        g_free(buff);

        if (retcode == TRUE)
        {
            /* treat errors as soft errors */
            retval = (gint) g_strtod(buffback + 4, NULL);
            if (retval != 0)
            {
                g_strstrip(buffback);
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _
                            ("%s:%d: rotctld returned error %d with stop-cmd (%s)"),
                            __FILE__, __LINE__, retval, buffback);
            }
        }

        gtk_widget_set_sensitive(ctrl->DevSel, TRUE);
        ctrl->engaged = FALSE;
        close_rotctld_socket(&(ctrl->sock));
        gtk_label_set_text(GTK_LABEL(ctrl->AzRead), "---");
        gtk_label_set_text(GTK_LABEL(ctrl->ElRead), "---");
    }
    else
    {
        if (ctrl->conf == NULL)
        {
            /* we don't have a working configuration */
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _
                        ("%s: Controller does not have a valid configuration"),
                        __func__);
            return;
        }
        gtk_widget_set_sensitive(ctrl->DevSel, FALSE);
        ctrl->engaged = TRUE;
        open_rotctld_socket(ctrl);
        ctrl->wrops = 0;
        ctrl->rdops = 0;
    }
}


/**
 * \brief Rotator controller timeout function
 * \param data Pointer to the GtkRotCtrl widget.
 * \return Always TRUE to let the timer continue.
 */
static gboolean rot_ctrl_timeout_cb(gpointer data)
{
    GtkRotCtrl     *ctrl = GTK_ROT_CTRL(data);
    gdouble         rotaz = 0.0, rotel = 0.0;
    gdouble         setaz = 0.0, setel = 45.0;
    gchar          *text;
    gboolean        error = FALSE;
    sat_t           sat_working, *sat;

    /* parameters for path predictions */
    gdouble         time_delta;
    gdouble         step_size;

    if (g_mutex_trylock(&(ctrl->busy)) == FALSE)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s missed the deadline"),
                    __func__);
        return TRUE;
    }

    /* If we are tracking and the target satellite is within
       range, set the rotor position controller knob values to
       the target values. If the target satellite is out of range
       set the rotor controller to 0 deg El and to the Az where the
       target sat is expected to come up or where it last went down
     */
    if (ctrl->tracking && ctrl->target)
    {
        if (ctrl->target->el < 0.0)
        {
            if (ctrl->pass != NULL)
            {
                if (ctrl->t < ctrl->pass->aos)
                {
                    setaz = ctrl->pass->aos_az;
                    setel = 0;
                }
                else if (ctrl->t > ctrl->pass->los)
                {
                    setaz = ctrl->pass->los_az;
                    setel = 0;
                }
            }
        }
        else
        {
            setaz = ctrl->target->az;
            setel = ctrl->target->el;
        }
        /* if this is a flipped pass and the rotor supports it */
        if ((ctrl->flipped) && (ctrl->conf->maxel >= 180.0))
        {
            setel = 180 - setel;
            if (setaz > 180)
                setaz -= 180;
            else
                setaz += 180;

            while (setaz > ctrl->conf->maxaz)
                setaz -= 360;

            while (setaz < ctrl->conf->minaz)
                setaz += 360;
        }

        if ((ctrl->conf->aztype == ROT_AZ_TYPE_180) && (setaz > 180.0))
            setaz = setaz - 360.0;

        if (!(ctrl->engaged))
        {
            gtk_rot_knob_set_value(GTK_ROT_KNOB(ctrl->AzSet), setaz);
            gtk_rot_knob_set_value(GTK_ROT_KNOB(ctrl->ElSet), setel);
        }

    }
    else
    {
        setaz = gtk_rot_knob_get_value(GTK_ROT_KNOB(ctrl->AzSet));
        setel = gtk_rot_knob_get_value(GTK_ROT_KNOB(ctrl->ElSet));
    }

    if ((ctrl->engaged) && (ctrl->conf != NULL))
    {
        /* read back current value from device */
        if (get_pos(ctrl, &rotaz, &rotel))
        {
            /* update display widgets */
            text = g_strdup_printf("%.2f\302\260", rotaz);
            gtk_label_set_text(GTK_LABEL(ctrl->AzRead), text);
            g_free(text);
            text = g_strdup_printf("%.2f\302\260", rotel);
            gtk_label_set_text(GTK_LABEL(ctrl->ElRead), text);
            g_free(text);

            if ((ctrl->conf->aztype == ROT_AZ_TYPE_180) && (rotaz < 0.0))
            {
                gtk_polar_plot_set_rotor_pos(GTK_POLAR_PLOT(ctrl->plot),
                                             rotaz + 360.0, rotel);
            }
            else
            {
                gtk_polar_plot_set_rotor_pos(GTK_POLAR_PLOT(ctrl->plot), rotaz,
                                             rotel);
            }
        }
        else
        {
            gtk_label_set_text(GTK_LABEL(ctrl->AzRead), _("ERROR"));
            gtk_label_set_text(GTK_LABEL(ctrl->ElRead), _("ERROR"));
            error = TRUE;

            gtk_polar_plot_set_rotor_pos(GTK_POLAR_PLOT(ctrl->plot), -10.0,
                                         -10.0);
        }

        /* if tolerance exceeded */
        if ((fabs(setaz - rotaz) > ctrl->tolerance) ||
            (fabs(setel - rotel) > ctrl->tolerance))
        {
            if (ctrl->tracking)
            {
                /* if we are in a pass try to lead the satellite 
                   some so we are not always chasing it */
                if (ctrl->target && ctrl->target->el > 0.0)
                {
                    /* starting the rotator moving while we do some computation
                     * can lead to errors later */
                    /* compute a time in the future when the position is 
                       within tolerance so and send the rotor there.
                     */

                    /* use a working copy so data does not get corrupted */
                    sat = memcpy(&(sat_working), ctrl->target, sizeof(sat_t));

                    /* compute az/el in the future that is past end of pass 
                       or exceeds tolerance
                     */
                    if (ctrl->pass)
                    {
                        /* the next point is before the end of the pass 
                           if there is one. */
                        time_delta = ctrl->pass->los - ctrl->t;
                    }
                    else
                    {
                        /* otherwise look 20 minutes into the future */
                        time_delta = 1.0 / 72.0;
                    }

                    /* have a minimum time delta */
                    step_size = time_delta / 2.0;
                    if (step_size < ctrl->delay / 1000.0 / (secday))
                    {
                        step_size = ctrl->delay / 1000.0 / (secday);
                    }
                    /* find a time when satellite is above horizon and at the 
                       edge of tolerance. the final step size needs to be smaller
                       than delay. otherwise the az/el could be further away than
                       tolerance the next time we enter the loop and we end up 
                       pushing ourselves away from the satellite.
                     */
                    while (step_size > (ctrl->delay / 1000.0 / 4.0 / (secday)))
                    {
                        predict_calc(sat, ctrl->qth, ctrl->t + time_delta);
                        /*update sat->az and sat->el to account for flips and az range */
                        if ((ctrl->flipped) && (ctrl->conf->maxel >= 180.0))
                        {
                            sat->el = 180.0 - sat->el;
                            if (sat->az > 180.0)
                                sat->az -= 180.0;
                            else
                                sat->az += 180.0;
                        }
                        if ((ctrl->conf->aztype == ROT_AZ_TYPE_180) &&
                            (sat->az > 180.0))
                        {
                            sat->az = sat->az - 360.0;
                        }
                        if ((sat->el < 0.0) || (sat->el > 180.0) ||
                            (fabs(setaz - sat->az) > (ctrl->tolerance)) ||
                            (fabs(setel - sat->el) > (ctrl->tolerance)))
                        {
                            time_delta -= step_size;
                        }
                        else
                        {
                            time_delta += step_size;
                        }
                        step_size /= 2.0;
                    }
                    setel = sat->el;
                    if (setel < 0.0)
                        setel = 0.0;

                    if (setel > 180.0)
                        setel = 180.0;

                    setaz = sat->az;
                }
            }

            /* send controller values to rotator device */
            /* this is the newly computed value which should be ahead of the current position */
            if (!set_pos(ctrl, setaz, setel))
            {
                error = TRUE;
            }
            else
            {
                gtk_rot_knob_set_value(GTK_ROT_KNOB(ctrl->AzSet), setaz);
                gtk_rot_knob_set_value(GTK_ROT_KNOB(ctrl->ElSet), setel);
            }
        }

        /* check error status */
        if (!error)
        {
            /* reset error counter */
            ctrl->errcnt = 0;
        }
        else
        {
            if (ctrl->errcnt >= MAX_ERROR_COUNT)
            {
                /* disengage device */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctrl->LockBut),
                                             FALSE);
                ctrl->engaged = FALSE;
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _
                            ("%s: MAX_ERROR_COUNT (%d) reached. Disengaging device!"),
                            __func__, MAX_ERROR_COUNT);
                ctrl->errcnt = 0;
                //g_print ("ERROR. WROPS: %d   RDOPS: %d\n", ctrl->wrops, ctrl->rdops);
            }
            else
            {
                /* increment error counter */
                ctrl->errcnt++;
            }
        }
    }
    else
    {
        /* ensure rotor pos is not visible on plot */
        gtk_polar_plot_set_rotor_pos(GTK_POLAR_PLOT(ctrl->plot), -10.0, -10.0);
    }


    /* update target object on polar plot */
    if (ctrl->target != NULL)
    {
        gtk_polar_plot_set_target_pos(GTK_POLAR_PLOT(ctrl->plot),
                                      ctrl->target->az, ctrl->target->el);
    }

    /* update controller circle on polar plot */
    if (ctrl->conf != NULL)
    {
        if ((ctrl->conf->aztype == ROT_AZ_TYPE_180) && (setaz < 0.0))
        {
            gtk_polar_plot_set_ctrl_pos(GTK_POLAR_PLOT(ctrl->plot),
                                        gtk_rot_knob_get_value(GTK_ROT_KNOB
                                                               (ctrl->AzSet)) +
                                        360.0,
                                        gtk_rot_knob_get_value(GTK_ROT_KNOB
                                                               (ctrl->ElSet)));
        }
        else
        {
            gtk_polar_plot_set_ctrl_pos(GTK_POLAR_PLOT(ctrl->plot),
                                        gtk_rot_knob_get_value(GTK_ROT_KNOB
                                                               (ctrl->AzSet)),
                                        gtk_rot_knob_get_value(GTK_ROT_KNOB
                                                               (ctrl->ElSet)));
        }
    }
    g_mutex_unlock(&(ctrl->busy));

    return TRUE;
}

/**
 * \brief Read rotator position from device.
 * \param ctrl Pointer to the GtkRotCtrl widget.
 * \param az The current Az as read from the device
 * \param el The current El as read from the device
 * \return TRUE if the position was successfully retrieved, FALSE if an
 *         error occurred.
 */
static gboolean get_pos(GtkRotCtrl * ctrl, gdouble * az, gdouble * el)
{
    gchar          *buff, **vbuff;
    gchar           buffback[128];
    gboolean        retcode;

    if ((az == NULL) || (el == NULL))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: NULL storage."), __FILE__, __LINE__);
        return FALSE;
    }

    /* send command */
    buff = g_strdup_printf("p\x0a");
    retcode = send_rotctld_command(ctrl, buff, buffback, 128);

    /* try to read answer */
    if (retcode)
    {
        if (strncmp(buffback, "RPRT", 4) == 0)
        {
            g_strstrip(buffback);
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%d: rotctld returned error (%s)"),
                        __FILE__, __LINE__, buffback);
        }
        else
        {
            vbuff = g_strsplit(buffback, "\n", 3);
            if ((vbuff[0] != NULL) && (vbuff[1] != NULL))
            {
                *az = g_strtod(vbuff[0], NULL);
                *el = g_strtod(vbuff[1], NULL);
            }
            else
            {
                g_strstrip(buffback);
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s:%d: rotctld returned bad response (%s)"),
                            __FILE__, __LINE__, buffback);
            }

            g_strfreev(vbuff);
        }
    }

    g_free(buff);

    return retcode;
}

/**
 * \brief Send new position to rotator device
 * \param ctrl Pointer to the GtkRotCtrl widget
 * \param az The new Azimuth
 * \param el The new Elevation
 * \return TRUE if the new position has been sent successfully
 *         FALSE if an error occurred
 * 
 * \note The function does not perform any range check since the GtkRotKnob
 * should always keep its value within range.
 */
static gboolean set_pos(GtkRotCtrl * ctrl, gdouble az, gdouble el)
{
    gchar          *buff;
    gchar           buffback[128];
    gchar           azstr[8], elstr[8];
    gboolean        retcode;
    gint            retval;

    /* send command */
    g_ascii_formatd(azstr, 8, "%7.2f", az);
    g_ascii_formatd(elstr, 8, "%7.2f", el);
    buff = g_strdup_printf("P %s %s\x0a", azstr, elstr);

    retcode = send_rotctld_command(ctrl, buff, buffback, 128);

    g_free(buff);

    if (retcode == TRUE)
    {
        /* treat errors as soft errors */
        retval = (gint) g_strtod(buffback + 4, NULL);
        if (retval != 0)
        {
            g_strstrip(buffback);
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _
                        ("%s:%d: rotctld returned error %d with az %f el %f(%s)"),
                        __FILE__, __LINE__, retval, az, el, buffback);
        }
    }

    return (retcode);
}

/**
 * \brief Update count down label.
 * \param[in] ctrl Pointer to the RotCtrl widget.
 * \param[in] t The current time.
 * 
 * This function calculates the new time to AOS/LOS of the currently
 * selected target and updates the ctrl->SatCnt label widget.
 */
static void update_count_down(GtkRotCtrl * ctrl, gdouble t)
{
    gdouble         targettime;
    gdouble         delta;
    gchar          *buff;
    guint           h, m, s;

    /* select AOS or LOS time depending on target elevation */
    if (ctrl->target->el < 0.0)
        targettime = ctrl->target->aos;
    else
        targettime = ctrl->target->los;

    delta = targettime - t;

    /* convert julian date to seconds */
    s = (guint) (delta * 86400);

    /* extract hours */
    h = (guint) floor(s / 3600);
    s -= 3600 * h;

    /* extract minutes */
    m = (guint) floor(s / 60);
    s -= 60 * m;

    if (h > 0)
        buff = g_strdup_printf("%02d:%02d:%02d", h, m, s);
    else
        buff = g_strdup_printf("%02d:%02d", m, s);

    gtk_label_set_text(GTK_LABEL(ctrl->SatCnt), buff);

    g_free(buff);

}

/** Check that we have at least one .rot file */
static gboolean have_conf()
{
    GDir           *dir = NULL; /* directory handle */
    GError         *error = NULL;       /* error flag and info */
    gchar          *dirname;    /* directory name */
    const gchar    *filename;   /* file name */
    gint            i = 0;

    /* open configuration directory */
    dirname = get_hwconf_dir();

    dir = g_dir_open(dirname, 0, &error);
    if (dir)
    {
        /* read each .rot file */
        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".rot"))
            {
                i++;
                /*once we have one we need nothing else */
                break;
            }
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open hwconf dir (%s)"),
                    __FILE__, __LINE__, error->message);
        g_clear_error(&error);
    }

    g_free(dirname);
    g_dir_close(dir);

    return (i > 0) ? TRUE : FALSE;
}

/** Open the rotcld socket. return true if successful false otherwise.*/
static gboolean open_rotctld_socket(GtkRotCtrl * ctrl)
{
    struct sockaddr_in ServAddr;
    struct hostent *h;
    gint            status;

    ctrl->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctrl->sock < 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to create socket"), __func__);
        ctrl->sock = 0;
        return FALSE;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Network socket created successfully"), __func__);
    }

    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family = AF_INET;      /* Internet address family */
    h = gethostbyname(ctrl->conf->host);
    memcpy((char *)&ServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    ServAddr.sin_port = htons(ctrl->conf->port);        /* Server port */

    /* establish connection */
    status =
        connect(ctrl->sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr));
    if (status < 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to connect to %s:%d"),
                    __func__, ctrl->conf->host, ctrl->conf->port);
        ctrl->sock = 0;
        return FALSE;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Connection opened to %s:%d"),
                    __func__, ctrl->conf->host, ctrl->conf->port);
    }

    return TRUE;
}

/** Close a rotcld socket. First send a q command to cleanly shut down rotctld */
static gboolean close_rotctld_socket(gint * sock)
{
    gint            written;

    /*shutdown the rotctld connect */
    written = send(*sock, "q\x0a", 2, 0);
    if (written != 2)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Sent 2 bytes but sent %d."),
                    __FILE__, __func__, written);
    }
#ifndef WIN32
    shutdown(*sock, SHUT_RDWR);
    close(*sock);
#else
    shutdown(*sock, SD_BOTH);
    closesocket(*sock);
#endif

    *sock = 0;

    return TRUE;
}

/**
 * \brief  Send a command to rotctld
 *
 * Inputs are a controller, a string command, and a buffer and length for
 * returning the output from rotctld.
 */
gboolean send_rotctld_command(GtkRotCtrl * ctrl, gchar * buff, gchar * buffout,
                              gint sizeout)
{
    gint            written;
    gint            size;

    /* added by Marcel Cimander; win32 newline -> \10\13 */
#ifdef WIN32
    size = strlen(buff) - 1;
    /* added by Marcel Cimander; unix newline -> \10 (apple -> \13) */
#else
    size = strlen(buff);
#endif

    //sat_log_log (SAT_LOG_LEVEL_DEBUG,
    //             _("%s:%s: Sending %d bytes as %s."),
    //             __FILE__, __func__, size, buff);

    /* send command */
    written = send(ctrl->sock, buff, size, 0);
    if (written != size)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: SIZE ERROR %d / %d"), __func__, written, size);
    }
    if (written == -1)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: rotctld Socket Down"), __func__);
        return FALSE;
    }

    /* try to read answer */
    size = recv(ctrl->sock, buffout, sizeout, 0);

    if (size == -1)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: rotctld Socket Down"), __func__);
        return FALSE;
    }

    buffout[size] = '\0';
    if (size == 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Got 0 bytes from rotctld"), __FILE__, __func__);
    }
    else
    {
        //sat_log_log (SAT_LOG_LEVEL_DEBUG,
        //             _("%s:%s: Read %d bytes as %s from rotctld"),
        //             __FILE__, __func__, size, buffout);

    }

    ctrl->wrops++;

    return TRUE;
}

/**
 * \brief Compare Satellite Names.
 *
 * Simple function to sort the list of satellites in the combo box.
 */
static gint sat_name_compare(sat_t * a, sat_t * b)
{
    return (gpredict_strcmp(a->nickname, b->nickname));
}

/**
 * \brief  Compare Rotator Names.
 */
static gint rot_name_compare(const gchar * a, const gchar * b)
{
    return (gpredict_strcmp(a, b));
}

/**
 * \brief  Compute if a pass is flipped or not.
 *
 * This is a function of the rotator and the particular pass. 
 */
static gboolean is_flipped_pass(pass_t * pass, rot_az_type_t type,
                                gdouble azstoppos)
{
    gdouble         max_az = 0, min_az = 0, offset = 0;
    gdouble         caz, last_az = pass->aos_az;
    guint           num, i;
    pass_detail_t  *detail;
    gboolean        retval = FALSE;

    num = g_slist_length(pass->details);
    if (type == ROT_AZ_TYPE_360)
    {
        min_az = 0;
        max_az = 360;
    }
    else if (type == ROT_AZ_TYPE_180)
    {
        min_az = -180;
        max_az = 180;
    }

    /* Offset by (azstoppos-min_az) to handle
     * rotators with non-default positions.
     * Note that the default positions of the rotator stops
     * (eg. -180 for ROT_AZ_TYPE_180, and 0 for 
     * ROT_AZ_TYPE_360) will create an offset of 0, which
     * seems like a pretty sane default. */
    offset = azstoppos - min_az;
    min_az += offset;
    max_az += offset;

    /* Assume that min_az and max_az are atleat 360 degrees apart
       get the azimuth that is in a settable range */
    while (last_az > max_az)
        last_az -= 360;

    while (last_az < min_az)
        last_az += 360;

    if (num > 1)
    {
        for (i = 1; i < num - 1; i++)
        {
            detail = PASS_DETAIL(g_slist_nth_data(pass->details, i));
            caz = detail->az;

            while (caz > max_az)
                caz -= 360;

            while (caz < min_az)
                caz += 360;


            if (fabs(caz - last_az) > 180)
                retval = TRUE;

            last_az = caz;
        }
    }
    caz = pass->los_az;
    while (caz > max_az)
        caz -= 360;

    while (caz < min_az)
        caz += 360;

    if (fabs(caz - last_az) > 180)
        retval = TRUE;

    return retval;
}

static inline void set_flipped_pass(GtkRotCtrl * ctrl)
{
    if (ctrl->conf && ctrl->pass)
        ctrl->flipped = is_flipped_pass(ctrl->pass, ctrl->conf->aztype,
                                        ctrl->conf->azstoppos);
}
