/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>
           Patrick Dohmen <dl4pd@darc.de>

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
 * \brief RIG control window.
 * \ingroup widgets
 *
 * The master radio control UI is implemented as a Gtk+ Widget in order
 * to allow multiple instances. The widget is created from the module
 * popup menu and each module can have several radio control windows
 * attached to it. Note, however, that current implementation only
 * allows one control window per module.
 *
 * TODO Duplex TRX
 * TODO Transponder passband display somewhere, below Sat freq?
 * 
 */
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <math.h>

/* NETWORK */
#ifndef WIN32
#include <arpa/inet.h>          /* htons() */
#include <netdb.h>              /* gethostbyname() */
#include <netinet/in.h>         /* struct sockaddr_in */
#include <sys/socket.h>         /* socket(), connect(), send() */
#else
#include <winsock2.h>
#endif

#include "compat.h"
#include "gpredict-utils.h"
#include "gtk-freq-knob.h"
#include "gtk-rig-ctrl.h"
#include "predict-tools.h"
#include "radio-conf.h"
#include "sat-log.h"
#include "sat-cfg.h"
#include "trsp-conf.h"


#define AZEL_FMTSTR "%7.2f\302\260"
#define MAX_ERROR_COUNT 5
#define WR_DEL 5000             /* delay in usec to wait between write and read commands */


static void     gtk_rig_ctrl_class_init(GtkRigCtrlClass * class);
static void     gtk_rig_ctrl_init(GtkRigCtrl * list);
static void     gtk_rig_ctrl_destroy(GtkWidget * widget);

static GtkWidget *create_downlink_widgets(GtkRigCtrl * ctrl);
static GtkWidget *create_uplink_widgets(GtkRigCtrl * ctrl);
static GtkWidget *create_target_widgets(GtkRigCtrl * ctrl);
static GtkWidget *create_conf_widgets(GtkRigCtrl * ctrl);
static GtkWidget *create_count_down_widgets(GtkRigCtrl * ctrl);

/* callback functions */
static void     sat_selected_cb(GtkComboBox * satsel, gpointer data);
static void     track_toggle_cb(GtkToggleButton * button, gpointer data);
static void     delay_changed_cb(GtkSpinButton * spin, gpointer data);
static void     primary_rig_selected_cb(GtkComboBox * box, gpointer data);
static void     secondary_rig_selected_cb(GtkComboBox * box, gpointer data);
static void     rig_engaged_cb(GtkToggleButton * button, gpointer data);
static void     trsp_selected_cb(GtkComboBox * box, gpointer data);
static void     trsp_tune_cb(GtkButton * button, gpointer data);
static void     trsp_lock_cb(GtkToggleButton * button, gpointer data);
static gboolean rig_ctrl_timeout_cb(gpointer data);
static void     downlink_changed_cb(GtkFreqKnob * knob, gpointer data);
static void     uplink_changed_cb(GtkFreqKnob * knob, gpointer data);
static gboolean key_press_cb(GtkWidget * widget, GdkEventKey * pKey,
                             gpointer data);
static void     manage_ptt_event(GtkRigCtrl * ctrl);

/* radio control functions */
static void     exec_rx_cycle(GtkRigCtrl * ctrl);
static void     exec_tx_cycle(GtkRigCtrl * ctrl);
static void     exec_trx_cycle(GtkRigCtrl * ctrl);
static void     exec_toggle_cycle(GtkRigCtrl * ctrl);
static void     exec_toggle_tx_cycle(GtkRigCtrl * ctrl);
static void     exec_duplex_cycle(GtkRigCtrl * ctrl);
static void     exec_duplex_tx_cycle(GtkRigCtrl * ctrl);
static void     exec_dual_rig_cycle(GtkRigCtrl * ctrl);
static gboolean check_aos_los(GtkRigCtrl * ctrl);
static gboolean set_freq_simplex(GtkRigCtrl * ctrl, gint sock, gdouble freq);
static gboolean get_freq_simplex(GtkRigCtrl * ctrl, gint sock, gdouble * freq);
static gboolean set_freq_toggle(GtkRigCtrl * ctrl, gint sock, gdouble freq);
static gboolean set_toggle(GtkRigCtrl * ctrl, gint sock);
static gboolean unset_toggle(GtkRigCtrl * ctrl, gint sock);
static gboolean get_freq_toggle(GtkRigCtrl * ctrl, gint sock, gdouble * freq);
static gboolean get_ptt(GtkRigCtrl * ctrl, gint sock);
static gboolean set_ptt(GtkRigCtrl * ctrl, gint sock, gboolean ptt);

static gboolean setup_split(GtkRigCtrl * ctrl);
static void     update_count_down(GtkRigCtrl * ctrl, gdouble t);
static gboolean open_rigctld_socket(radio_conf_t * conf, gint * sock);
static gboolean close_rigctld_socket(gint * sock);

/* misc utility functions */
static void     load_trsp_list(GtkRigCtrl * ctrl);
static void     store_sats(gpointer key, gpointer value, gpointer user_data);
static gboolean have_conf(void);
static void     track_downlink(GtkRigCtrl * ctrl);
static void     track_uplink(GtkRigCtrl * ctrl);
static gboolean is_rig_tx_capable(const gchar * confname);
static gboolean send_rigctld_command(GtkRigCtrl * ctrl, gint sock,
                                     gchar * buff, gchar * buffout,
                                     gint sizeout);
static inline gboolean check_set_response(gchar * buff, gboolean retcode,
                                          const gchar * filename);
static inline gboolean check_get_response(gchar * buff, gboolean retcode,
                                          const gchar * filename);
static gint     sat_name_compare(sat_t * a, sat_t * b);
static gint     rig_name_compare(const gchar * a, const gchar * b);

/*  add thread for hamlib communication */
gpointer        rigctl_run(gpointer data);
static void     rigctrl_open(GtkRigCtrl * data);
static void     rigctrl_close(GtkRigCtrl * data);
static void     setconfig(gpointer data);
static void     remove_timer(GtkRigCtrl * data);
static void     start_timer(GtkRigCtrl * data);

static GtkVBoxClass *parent_class = NULL;


GType gtk_rig_ctrl_get_type()
{
    static GType    gtk_rig_ctrl_type = 0;

    if (!gtk_rig_ctrl_type)
    {

        static const GTypeInfo gtk_rig_ctrl_info = {
            sizeof(GtkRigCtrlClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_rig_ctrl_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkRigCtrl),
            2,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_rig_ctrl_init,
            NULL
        };

        gtk_rig_ctrl_type = g_type_register_static(GTK_TYPE_VBOX,
                                                   "GtkRigCtrl",
                                                   &gtk_rig_ctrl_info, 0);
    }

    return gtk_rig_ctrl_type;
}

static void gtk_rig_ctrl_class_init(GtkRigCtrlClass * class)
{
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) class;
    parent_class = g_type_class_peek_parent(class);
    widget_class->destroy = gtk_rig_ctrl_destroy;
}

static void gtk_rig_ctrl_init(GtkRigCtrl * ctrl)
{
    ctrl->sats = NULL;
    ctrl->target = NULL;
    ctrl->pass = NULL;
    ctrl->qth = NULL;
    ctrl->conf = NULL;
    ctrl->conf2 = NULL;
    ctrl->trsp = NULL;
    ctrl->trsplist = NULL;
    ctrl->trsplock = FALSE;
    ctrl->tracking = FALSE;
    ctrl->prev_ele = 0.0;
    ctrl->sock = 0;
    ctrl->sock2 = 0;
    g_mutex_init(&(ctrl->busy));
    ctrl->engaged = FALSE;
    ctrl->delay = 1000;
    ctrl->timerid = 0;
    ctrl->errcnt = 0;
    ctrl->lastrxf = 0.0;
    ctrl->lasttxf = 0.0;
    ctrl->last_toggle_tx = -1;
}

static void gtk_rig_ctrl_destroy(GtkWidget * widget)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(widget);

    if (ctrl->rigctl_thread != NULL)
    {
        g_mutex_lock(&ctrl->widgetsync);

        ctrl->engaged = 0;
        setconfig(ctrl);

        /* synchronization */
        g_cond_wait(&ctrl->widgetready, &ctrl->widgetsync);
        g_mutex_unlock(&ctrl->widgetsync);
        ctrl->rigctl_thread = NULL;
    }

    if (ctrl->conf != NULL)
    {
        g_free(ctrl->conf->name);
        g_free(ctrl->conf->host);
        g_free(ctrl->conf);
        ctrl->conf = NULL;
    }
    if (ctrl->conf2 != NULL)
    {
        g_free(ctrl->conf2->name);
        g_free(ctrl->conf2->host);
        g_free(ctrl->conf2);
        ctrl->conf2 = NULL;
    }

    if (ctrl->trsplist != NULL)
    {
        free_transponders(ctrl->trsplist);
        ctrl->trsplist = NULL;
    }

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

GtkWidget      *gtk_rig_ctrl_new(GtkSatModule * module)
{
    GtkWidget      *widget;
    GtkWidget      *table;

    if (!have_conf())
        return NULL;

    widget = g_object_new(GTK_TYPE_RIG_CTRL, NULL);

    g_signal_connect(widget, "key-press-event", G_CALLBACK(key_press_cb),
                     NULL);

    g_hash_table_foreach(module->satellites, store_sats, widget);
    GTK_RIG_CTRL(widget)->target =
        SAT(g_slist_nth_data(GTK_RIG_CTRL(widget)->sats, 0));

    GTK_RIG_CTRL(widget)->qth = module->qth;

    if (GTK_RIG_CTRL(widget)->target != NULL)
    {
        /* get next pass for target satellite */
        GTK_RIG_CTRL(widget)->pass =
            get_next_pass(GTK_RIG_CTRL(widget)->target,
                          GTK_RIG_CTRL(widget)->qth, 3.0);
    }

    /* create contents */
    table = gtk_table_new(3, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_container_set_border_width(GTK_CONTAINER(table), 10);
    gtk_table_attach(GTK_TABLE(table),
                     create_downlink_widgets(GTK_RIG_CTRL(widget)), 0, 1, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
    gtk_table_attach(GTK_TABLE(table),
                     create_uplink_widgets(GTK_RIG_CTRL(widget)), 1, 2, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
    gtk_table_attach(GTK_TABLE(table),
                     create_target_widgets(GTK_RIG_CTRL(widget)), 0, 1, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
    gtk_table_attach(GTK_TABLE(table),
                     create_conf_widgets(GTK_RIG_CTRL(widget)), 1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
    gtk_table_attach(GTK_TABLE(table),
                     create_count_down_widgets(GTK_RIG_CTRL(widget)), 0, 2, 2,
                     3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

    gtk_container_add(GTK_CONTAINER(widget), table);

    if (module->target > 0)
        gtk_rig_ctrl_select_sat(GTK_RIG_CTRL(widget), module->target);

    return widget;
}

/*
 * Update rig control state.
 * 
 * This function is called by the parent, i.e. GtkSatModule, indicating that
 * the satellite data has been updated. The function updates the internal state
 * of the controller and the rigator.
 */
void gtk_rig_ctrl_update(GtkRigCtrl * ctrl, gdouble t)
{
    gdouble         satfreq;
    gchar          *buff;

    g_mutex_lock(&ctrl->rig_ctrl_updatelock);

    if (ctrl->target)
    {
        buff = g_strdup_printf(AZEL_FMTSTR, ctrl->target->az);
        gtk_label_set_text(GTK_LABEL(ctrl->SatAz), buff);
        g_free(buff);
        buff = g_strdup_printf(AZEL_FMTSTR, ctrl->target->el);
        gtk_label_set_text(GTK_LABEL(ctrl->SatEl), buff);
        g_free(buff);

        update_count_down(ctrl, t);

        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(ctrl->target->range));
        }
        else
        {
            buff = g_strdup_printf("%.0f km", ctrl->target->range);
        }
        gtk_label_set_text(GTK_LABEL(ctrl->SatRng), buff);
        g_free(buff);

        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.3f mi/s",
                                   KM_TO_MI(ctrl->target->range_rate));
        }
        else
        {
            buff = g_strdup_printf("%.3f km/s", ctrl->target->range_rate);
        }
        gtk_label_set_text(GTK_LABEL(ctrl->SatRngRate), buff);
        g_free(buff);

        /* Doppler shift down */
        satfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqDown));
        ctrl->dd = -satfreq * (ctrl->target->range_rate / 299792.4580); // Hz
        buff = g_strdup_printf("%.0f Hz", ctrl->dd);
        gtk_label_set_text(GTK_LABEL(ctrl->SatDopDown), buff);
        g_free(buff);

        /* Doppler shift up */
        satfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqUp));
        ctrl->du = satfreq * (ctrl->target->range_rate / 299792.4580);  // Hz
        buff = g_strdup_printf("%.0f Hz", ctrl->du);
        gtk_label_set_text(GTK_LABEL(ctrl->SatDopUp), buff);
        g_free(buff);

        /* update next pass if necessary */
        if (ctrl->pass != NULL)
        {
            if (ctrl->target->aos > ctrl->pass->aos)
            {
                free_pass(ctrl->pass);
                ctrl->pass = get_next_pass(ctrl->target, ctrl->qth, 3.0);
            }
        }
        else
        {
            /* we don't have any current pass; store the current one */
            ctrl->pass = get_next_pass(ctrl->target, ctrl->qth, 3.0);
        }
    }

    g_mutex_unlock(&ctrl->rig_ctrl_updatelock);
}

void gtk_rig_ctrl_select_sat(GtkRigCtrl * ctrl, gint catnum)
{
    sat_t          *sat;
    int             i, n;

    /* find index in satellite list */
    n = g_slist_length(ctrl->sats);
    for (i = 0; i < n; i++)
    {
        sat = SAT(g_slist_nth_data(ctrl->sats, i));
        if (sat)
        {
            if (sat->tle.catnr == catnum)
            {
                /* assume the index is the same in sat selector */
                gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->SatSel), i);
                break;
            }
        }
    }
}

/*
 * Create freq control widgets for downlink.
 * 
 * This function creates and initialises the widgets for controlling the
 * downlink frequency. It consists of a controller widget showing the
 * satellite frequency with the radio frequency below it.
 * 
 */
static GtkWidget *create_downlink_widgets(GtkRigCtrl * ctrl)
{
    GtkWidget      *frame;
    GtkWidget      *vbox;
    GtkWidget      *hbox1, *hbox2;
    GtkWidget      *label;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b> Downlink </b>"));
    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);

    vbox = gtk_vbox_new(FALSE, 5);
    hbox1 = gtk_hbox_new(FALSE, 5);
    hbox2 = gtk_hbox_new(FALSE, 5);

    /* satellite downlink frequency */
    ctrl->SatFreqDown = gtk_freq_knob_new(145890000.0, TRUE);
    g_signal_connect(ctrl->SatFreqDown, "freq-changed",
                     G_CALLBACK(downlink_changed_cb), ctrl);
    gtk_box_pack_start(GTK_BOX(vbox), ctrl->SatFreqDown, TRUE, TRUE, 0);

    /* Downlink doppler */
    label = gtk_label_new(_("Doppler:"));
    gtk_widget_set_tooltip_text(label,
                                _
                                ("The Doppler shift according to the range rate and "
                                 "the currently selected downlink frequency"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, FALSE, 0);
    ctrl->SatDopDown = gtk_label_new("---- Hz");
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatDopDown), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox1), ctrl->SatDopDown, FALSE, TRUE, 0);

    /* Downconverter LO */
    ctrl->LoDown = gtk_label_new("0 MHz");
    gtk_misc_set_alignment(GTK_MISC(ctrl->LoDown), 1.0, 0.5);
    gtk_box_pack_end(GTK_BOX(hbox1), ctrl->LoDown, FALSE, FALSE, 2);
    label = gtk_label_new(_("LO:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_end(GTK_BOX(hbox1), label, FALSE, FALSE, 0);

    /* Radio downlink frequency */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<span size='large'><b>Radio:</b></span>");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, TRUE, 0);
    ctrl->RigFreqDown = gtk_freq_knob_new(145890000.0, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox2), ctrl->RigFreqDown, TRUE, TRUE, 0);

    /* finish packing ... */
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    return frame;
}

/*
 * Create uplink frequency display widgets.
 * 
 * This function creates and initialises the widgets for displaying the
 * uplink frequency of the satellite and the radio.
 */
static GtkWidget *create_uplink_widgets(GtkRigCtrl * ctrl)
{
    GtkWidget      *frame;
    GtkWidget      *vbox;
    GtkWidget      *hbox1, *hbox2;
    GtkWidget      *label;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<b> Uplink </b>"));
    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);

    vbox = gtk_vbox_new(FALSE, 5);
    hbox1 = gtk_hbox_new(FALSE, 5);
    hbox2 = gtk_hbox_new(FALSE, 5);

    /* satellite uplink frequency */
    ctrl->SatFreqUp = gtk_freq_knob_new(145890000.0, TRUE);
    g_signal_connect(ctrl->SatFreqUp, "freq-changed",
                     G_CALLBACK(uplink_changed_cb), ctrl);
    gtk_box_pack_start(GTK_BOX(vbox), ctrl->SatFreqUp, TRUE, TRUE, 0);

    /* Uplink doppler */
    label = gtk_label_new(_("Doppler:"));
    gtk_widget_set_tooltip_text(label,
                                _("The Doppler shift according to the range "
                                  "rate and the currently selected downlink "
                                  "frequency"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, FALSE, 0);
    ctrl->SatDopUp = gtk_label_new("---- Hz");
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatDopUp), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox1), ctrl->SatDopUp, FALSE, TRUE, 0);

    /* Upconverter LO */
    ctrl->LoUp = gtk_label_new("0 MHz");
    gtk_misc_set_alignment(GTK_MISC(ctrl->LoUp), 1.0, 0.5);
    gtk_box_pack_end(GTK_BOX(hbox1), ctrl->LoUp, FALSE, FALSE, 2);
    label = gtk_label_new(_("LO:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_end(GTK_BOX(hbox1), label, FALSE, FALSE, 0);

    /* Radio uplink frequency */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<span size='large'><b>Radio:</b></span>");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, TRUE, 0);
    ctrl->RigFreqUp = gtk_freq_knob_new(145890000.0, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox2), ctrl->RigFreqUp, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    return frame;
}

static GtkWidget *create_target_widgets(GtkRigCtrl * ctrl)
{
    GtkWidget      *frame, *table, *label, *track;
    GtkWidget      *tune, *trsplock, *hbox;
    gchar          *buff;
    guint           i, n;
    sat_t          *sat = NULL;

    buff = g_strdup_printf(AZEL_FMTSTR, 0.0);

    table = gtk_table_new(4, 4, FALSE);
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
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->SatSel, 0, 3, 0, 1);

    /* tracking button */
    track = gtk_toggle_button_new_with_label(_("Track"));
    gtk_widget_set_tooltip_text(track,
                                _("Track the satellite transponder.\n"
                                  "Enabling this button will apply Doppler "
                                  "correction to the frequency of the radio."));
    gtk_table_attach_defaults(GTK_TABLE(table), track, 3, 4, 0, 1);
    g_signal_connect(track, "toggled", G_CALLBACK(track_toggle_cb), ctrl);

    /* Transponder selector, tune, and trsplock buttons */
    ctrl->TrspSel = gtk_combo_box_text_new();
    gtk_widget_set_tooltip_text(ctrl->TrspSel, _("Select a transponder"));
    load_trsp_list(ctrl);
    //gtk_combo_box_set_active (GTK_COMBO_BOX (ctrl->TrspSel), 0);
    g_signal_connect(ctrl->TrspSel, "changed", G_CALLBACK(trsp_selected_cb),
                     ctrl);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->TrspSel, 0, 3, 1, 2);

    /* buttons */
    tune = gtk_button_new_with_label(_("T"));
    gtk_widget_set_tooltip_text(tune,
                                _("Tune the radio to this transponder. "
                                  "The uplink and downlink will be set to the "
                                  "center of the transponder passband. In case "
                                  "of beacons, only the downlink will be tuned "
                                  "to the beacon frequency."));
    g_signal_connect(tune, "clicked", G_CALLBACK(trsp_tune_cb), ctrl);

    trsplock = gtk_toggle_button_new_with_label(_("L"));
    gtk_widget_set_tooltip_text(trsplock,
                                _("Lock the uplink and the downlink to each "
                                  "other. Whenever you change the downlink "
                                  "(in the controller or on the dial, the "
                                  "uplink will track it according to whether "
                                  "the transponder is inverting or not. "
                                  "Similarly, if you change the uplink the "
                                  "downlink will track it automatically.\n\n"
                                  "If the downlink and uplink are initially "
                                  "out of sync when you enable this function, "
                                  "the current downlink frequency will be used "
                                  "as baseline for setting the new uplink "
                                  "frequency."));
    g_signal_connect(trsplock, "toggled", G_CALLBACK(trsp_lock_cb), ctrl);

    /* box for packing buttons */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), tune, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), trsplock, TRUE, TRUE, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox, 3, 4, 1, 2);

    /* Azimuth */
    label = gtk_label_new(_("Az:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
    ctrl->SatAz = gtk_label_new(buff);
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatAz), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->SatAz, 1, 2, 2, 3);

    /* Elevation */
    label = gtk_label_new(_("El:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
    ctrl->SatEl = gtk_label_new(buff);
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatEl), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->SatEl, 1, 2, 3, 4);

    /* Range */
    label = gtk_label_new(_(" Range:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 3);
    ctrl->SatRng = gtk_label_new("0 km");
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatRng), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->SatRng, 3, 4, 2, 3);

    gtk_widget_set_tooltip_text(label,
                                _("This is the current distance between the "
                                  "satellite and the observer."));
    gtk_widget_set_tooltip_text(ctrl->SatRng,
                                _("This is the current distance between the "
                                  "satellite and the observer."));

    /* Range rate */
    label = gtk_label_new(_(" Rate:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 3, 4);
    ctrl->SatRngRate = gtk_label_new("0.0 km/s");
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatRngRate), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->SatRngRate, 3, 4, 3, 4);

    gtk_widget_set_tooltip_text(label,
                                _("The rate of change for the distance between"
                                  " the satellite and the observer."));
    gtk_widget_set_tooltip_text(ctrl->SatRngRate,
                                _("The rate of change for the distance between"
                                  " the satellite and the observer."));

    frame = gtk_frame_new(_("Target"));
    gtk_container_add(GTK_CONTAINER(frame), table);
    g_free(buff);

    return frame;
}

static GtkWidget *create_conf_widgets(GtkRigCtrl * ctrl)
{
    GtkWidget      *frame, *table, *label, *timer;
    GDir           *dir = NULL; /* directory handle */
    GError         *error = NULL;       /* error flag and info */
    gchar          *dirname;    /* directory name */
    gchar         **vbuff;
    const gchar    *filename;   /* file name */
    gchar          *rigname;


    table = gtk_table_new(4, 3, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);

    /* Primary device */
    label = gtk_label_new(_("1. Device:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

    ctrl->DevSel = gtk_combo_box_text_new();
    gtk_widget_set_tooltip_text(ctrl->DevSel,
                                _("Select primary radio device."
                                  "This device will be used for downlink and "
                                  "uplink unless you select a secondary device"
                                  " for uplink"));

    /* open configuration directory */
    dirname = get_hwconf_dir();

    dir = g_dir_open(dirname, 0, &error);
    if (dir)
    {
        /* read each .rig file */
        GSList         *rigs = NULL;
        gint            i;
        gint            n;

        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".rig"))
            {
                vbuff = g_strsplit(filename, ".rig", 0);
                rigs =
                    g_slist_insert_sorted(rigs, g_strdup(vbuff[0]),
                                          (GCompareFunc) rig_name_compare);
                g_strfreev(vbuff);
            }
        }
        n = g_slist_length(rigs);
        for (i = 0; i < n; i++)
        {
            rigname = g_slist_nth_data(rigs, i);
            if (rigname)
            {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (ctrl->DevSel), rigname);
                g_free(rigname);
            }
        }
        g_slist_free(rigs);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to open hwconf dir (%s)"),
                    __FILE__, __LINE__, error->message);
        g_clear_error(&error);
    }

    g_dir_close(dir);

    gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->DevSel), 0);
    g_signal_connect(ctrl->DevSel, "changed",
                     G_CALLBACK(primary_rig_selected_cb), ctrl);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->DevSel, 1, 2, 0, 1);
    /* config will be force-loaded after LO spin is created */

    /* Secondary device */
    label = gtk_label_new(_("2. Device:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    ctrl->DevSel2 = gtk_combo_box_text_new();
    gtk_widget_set_tooltip_text(ctrl->DevSel2,
                                _("Select secondary radio device\n"
                                  "This device will be used for uplink"));

    /* load config */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctrl->DevSel2),
                                   _("None"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->DevSel2), 0);

    dir = g_dir_open(dirname, 0, &error);
    if (dir)
    {
        /* read each .rig file */
        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".rig"))
            {
                /* only add TX capable rigs */
                vbuff = g_strsplit(filename, ".rig", 0);
                if (is_rig_tx_capable(vbuff[0]))
                {
                    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                                   (ctrl->DevSel2), vbuff[0]);
                }
                g_strfreev(vbuff);
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

    //gtk_combo_box_set_active (GTK_COMBO_BOX (ctrl->DevSel), 0);
    g_signal_connect(ctrl->DevSel2, "changed",
                     G_CALLBACK(secondary_rig_selected_cb), ctrl);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->DevSel2, 1, 2, 1, 2);

    /* Engage button */
    ctrl->LockBut = gtk_toggle_button_new_with_label(_("Engage"));
    gtk_widget_set_tooltip_text(ctrl->LockBut,
                                _("Engage the selected radio device"));
    g_signal_connect(ctrl->LockBut, "toggled", G_CALLBACK(rig_engaged_cb),
                     ctrl);
    gtk_table_attach_defaults(GTK_TABLE(table), ctrl->LockBut, 2, 3, 0, 1);

    /* Now, load config */
    primary_rig_selected_cb(GTK_COMBO_BOX(ctrl->DevSel), ctrl);

    /* Timeout */
    label = gtk_label_new(_("Cycle:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);

    timer = gtk_spin_button_new_with_range(100, 5000, 10);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(timer), 0);
    gtk_widget_set_tooltip_text(timer,
                                _("This parameter controls the delay between "
                                  "commands sent to the rig."));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(timer), ctrl->delay);
    g_signal_connect(timer, "value-changed", G_CALLBACK(delay_changed_cb),
                     ctrl);
    gtk_table_attach(GTK_TABLE(table), timer, 1, 2, 3, 4, GTK_FILL, GTK_FILL,
                     0, 0);

    label = gtk_label_new(_("msec"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 3, 4);

    frame = gtk_frame_new(_("Settings"));
    gtk_container_add(GTK_CONTAINER(frame), table);

    return frame;
}


/** Create count down widget */
static GtkWidget *create_count_down_widgets(GtkRigCtrl * ctrl)
{
    GtkWidget      *frame;

    /* create delta-t label */
    ctrl->SatCnt = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(ctrl->SatCnt), 0.5, 0.5);
    gtk_label_set_markup(GTK_LABEL(ctrl->SatCnt),
                         _("<span size='large'><b>\316\224T: "
                           "00:00:00</b></span>"));
    gtk_widget_set_tooltip_text(ctrl->SatCnt,
                                _("The time remaining until the next AOS or "
                                  "LOS event, depending on which one comes "
                                  "first."));

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), ctrl->SatCnt);

    return frame;
}

/* Copy satellite from hash table to singly linked list. */
static void store_sats(gpointer key, gpointer value, gpointer user_data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(user_data);
    sat_t          *sat = SAT(value);

    (void)key;

    ctrl->sats = g_slist_insert_sorted(ctrl->sats, sat,
                                      (GCompareFunc) sat_name_compare);
}

/* Called when the user selects a new satellite. */
static void sat_selected_cb(GtkComboBox * satsel, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);
    gint            i;

    i = gtk_combo_box_get_active(satsel);
    if (i >= 0)
    {
        ctrl->target = SAT(g_slist_nth_data(ctrl->sats, i));

        ctrl->prev_ele = ctrl->target->el;

        /* update next pass */
        if (ctrl->pass != NULL)
            free_pass(ctrl->pass);
        ctrl->pass = get_next_pass(ctrl->target, ctrl->qth, 3.0);

        /* read transponders for new target */
        load_trsp_list(ctrl);
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
}

/*
 * Called when a new transponder is selected.
 * It updates ctrl->trsp with the new selection and issues a "tune" event.
 */
static void trsp_selected_cb(GtkComboBox * box, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);
    gint            i, n;

    i = gtk_combo_box_get_active(box);
    n = g_slist_length(ctrl->trsplist);

    if (i == -1)
    {
        /* clear transponder data */
        ctrl->trsp = NULL;
    }
    else if (i < n)
    {
        ctrl->trsp = (trsp_t *) g_slist_nth_data(ctrl->trsplist, i);
        trsp_tune_cb(NULL, data);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Inconsistency detected in internal transponder "
                      "data (%d,%d)"), __func__, i, n);
    }
}

/*
 * Manage "Tune" events
 *
 * @param button Pointer to the GtkButton that received the signal.
 * @param data Pointer to the GtkRigCtrl structure.
 *
 * This function is called when the user clicks on the Tune button next to the
 * transponder selector. When clicked, the radio controller will set the RX and TX
 * frequencies to the middle of the transponder uplink/downlink bands.
 *
 * To avoid conflicts with manual frequency changes on the radio, the sync between
 * RIG and GPREDICT is invalidated after the tuning operation is performed.
 */
static void trsp_tune_cb(GtkButton * button, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);
    gdouble         freq;

    (void)button;

    if (ctrl->trsp == NULL)
        return;

    /* tune downlink */
    if ((ctrl->trsp->downlow > 0) && (ctrl->trsp->downhigh > 0))
    {
        freq = ctrl->trsp->downlow +
            abs(ctrl->trsp->downhigh - ctrl->trsp->downlow) / 2;
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqDown), freq);

        /* invalidate RIG<->GPREDICT sync */
        ctrl->lastrxf = 0.0;
    }

    /* tune uplink */
    if ((ctrl->trsp->uplow > 0) && (ctrl->trsp->uphigh > 0))
    {
        freq = ctrl->trsp->uplow +
            abs(ctrl->trsp->uphigh - ctrl->trsp->uplow) / 2;
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqUp), freq);

        /* invalidate RIG<->GPREDICT sync */
        ctrl->lasttxf = 0.0;
    }
}

/*
 * Manage lock transponder signals.
 *
 * @param button Pointer to the GtkToggleButton that received the signal.
 * @param data Pointer to the GtkRigCtrl structure.
 *
 * This function is called when the user toggles the "Lock Transponder" button.
 * When ON, the uplink and downlink are locked according to the current transponder
 * data, i.e. when user changes the downlink, the uplink will follow automatically
 * taking into account whether the transponder is inverting or not.
 */
static void trsp_lock_cb(GtkToggleButton * button, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    ctrl->trsplock = gtk_toggle_button_get_active(button);

    /* set uplink according to downlink */
    if (ctrl->trsplock)
        track_downlink(ctrl);
}

static void track_toggle_cb(GtkToggleButton * button, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    ctrl->tracking = gtk_toggle_button_get_active(button);

    /* invalidate sync with radio */
    ctrl->lastrxf = 0.0;
    ctrl->lasttxf = 0.0;
}

/* Called when the user changes the value of the cycle delay */
static void delay_changed_cb(GtkSpinButton * spin, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    ctrl->delay = (guint) gtk_spin_button_get_value(spin);

    if (ctrl->engaged)
        start_timer(ctrl);
}

static void primary_rig_selected_cb(GtkComboBox * box, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);
    gchar          *buff;

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%s: Primary device selected: %d"),
                __FILE__, __func__, gtk_combo_box_get_active(box));

    if (ctrl->conf != NULL)
    {
        g_free(ctrl->conf->name);
        g_free(ctrl->conf->host);
        g_free(ctrl->conf);
    }

    ctrl->conf = g_try_new(radio_conf_t, 1);
    if (ctrl->conf == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to allocate memory for radio config"),
                    __FILE__, __LINE__);
        return;
    }

    ctrl->conf->name =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box));
    if (radio_conf_read(ctrl->conf))
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%s: Loaded new radio configuration %s"),
                    __FILE__, __func__, ctrl->conf->name);
        /* update LO widgets */
        buff = g_strdup_printf(_("%.0f MHz"), ctrl->conf->lo / 1.0e6);
        gtk_label_set_text(GTK_LABEL(ctrl->LoDown), buff);
        g_free(buff);
        /* uplink LO only if single device */
        if (ctrl->conf2 == NULL)
        {
            buff = g_strdup_printf(_("%.0f MHz"), ctrl->conf->loup / 1.0e6);
            gtk_label_set_text(GTK_LABEL(ctrl->LoUp), buff);
            g_free(buff);
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Failed to load radio configuration %s"),
                    __FILE__, __func__, ctrl->conf->name);

        g_free(ctrl->conf->name);
        if (ctrl->conf->host)
            g_free(ctrl->conf->host);
        g_free(ctrl->conf);
        ctrl->conf = NULL;
    }
}

static void secondary_rig_selected_cb(GtkComboBox * box, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);
    gchar          *buff;
    gchar          *name1, *name2;


    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%s: Secondary device selected: %d"),
                __FILE__, __func__, gtk_combo_box_get_active(box));

    if (ctrl->conf2 != NULL)
    {
        g_free(ctrl->conf2->name);
        g_free(ctrl->conf2->host);
        g_free(ctrl->conf2);
        ctrl->conf2 = NULL;
    }

    if (gtk_combo_box_get_active(box) == 0)
    {
        /* first entry is "None" */

        /* reset uplink LO to what's in ctrl->conf */
        if (ctrl->conf != NULL)
        {
            buff = g_strdup_printf(_("%.0f MHz"), ctrl->conf->loup / 1.0e6);
            gtk_label_set_text(GTK_LABEL(ctrl->LoUp), buff);
            g_free(buff);
        }

        return;
    }

    /* ensure that selected secondary rig is not the same as the primary */
    name1 =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(ctrl->DevSel));
    name2 =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(ctrl->DevSel2));
    if (!g_strcmp0(name1, name2))
    {
        /* selected conf is the same as the primary one */
        g_free(name1);
        g_free(name2);
        if (ctrl->conf != NULL)
        {
            buff = g_strdup_printf(_("%.0f MHz"), ctrl->conf->loup / 1.0e6);
            gtk_label_set_text(GTK_LABEL(ctrl->LoUp), buff);
            g_free(buff);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->DevSel2), 0);

        return;
    }

    g_free(name1);
    g_free(name2);

    /* else load new device */
    ctrl->conf2 = g_try_new(radio_conf_t, 1);
    if (ctrl->conf2 == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Failed to allocate memory for radio config"),
                    __FILE__, __func__);
        return;
    }

    /* load new configuration */
    ctrl->conf2->name =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box));
    if (radio_conf_read(ctrl->conf2))
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%s: Loaded new radio configuration %s"),
                    __FILE__, __func__, ctrl->conf2->name);

        buff = g_strdup_printf(_("%.0f MHz"), ctrl->conf2->loup / 1.0e6);
        gtk_label_set_text(GTK_LABEL(ctrl->LoUp), buff);
        g_free(buff);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Failed to load radio configuration %s"),
                    __FILE__, __func__, ctrl->conf->name);

        g_free(ctrl->conf2->name);
        if (ctrl->conf2->host)
            g_free(ctrl->conf2->host);
        g_free(ctrl->conf2);
        ctrl->conf2 = NULL;
    }
}

static void rig_engaged_cb(GtkToggleButton * button, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    if (ctrl->conf == NULL)
    {
        /* we don't have a working configuration */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Controller does not have a valid configuration"),
                    __func__);
        return;
    }

    if (!gtk_toggle_button_get_active(button))
    {
        /* close socket */
        gtk_widget_set_sensitive(ctrl->DevSel, TRUE);
        gtk_widget_set_sensitive(ctrl->DevSel2, TRUE);
        ctrl->engaged = FALSE;

        /*  stop worker thread... */
        setconfig(ctrl);
        ctrl->rigctl_thread = NULL;
    }
    else
    {
        gtk_widget_set_sensitive(ctrl->DevSel, FALSE);
        gtk_widget_set_sensitive(ctrl->DevSel2, FALSE);
        ctrl->engaged = TRUE;

        /*  start worker thread... */
        ctrl->rigctlq = g_async_queue_new();
        ctrl->rigctl_thread = g_thread_new("rigctl_run", rigctl_run, ctrl);
        setconfig(ctrl);
    }
}


/* Setup VFOs for split operation (simplex or duplex) */
static gboolean setup_split(GtkRigCtrl * ctrl)
{
    gchar          *buff;
    gchar           buffback[256];
    gboolean        retcode;

    switch (ctrl->conf->vfoUp)
    {
    case VFO_A:
        buff = g_strdup("S 1 VFOA\x0a");
        break;

    case VFO_B:
        buff = g_strdup("S 1 VFOB\x0a");
        break;

    case VFO_MAIN:
        buff = g_strdup("S 1 Main\x0a");
        break;

    case VFO_SUB:
        buff = g_strdup("S 1 Sub\x0a");
        break;

    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s called but TX VFO is %d."), __func__,
                    ctrl->conf->vfoUp);
        return FALSE;
    }

    retcode = send_rigctld_command(ctrl, ctrl->sock, buff, buffback, 128);
    g_free(buff);

    return (check_set_response(buffback, retcode, __func__));
}

static void downlink_changed_cb(GtkFreqKnob * knob, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);
    (void)knob;

    if (ctrl->trsplock)
        track_downlink(ctrl);
}

static void uplink_changed_cb(GtkFreqKnob * knob, gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    (void)knob;

    if (ctrl->trsplock)
        track_uplink(ctrl);
}

static gboolean rig_ctrl_timeout_cb(gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    if (ctrl->conf == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Controller does not have a valid configuration"),
                    __func__);
        return FALSE;
    }

    if (g_mutex_trylock(&(ctrl->busy)) == FALSE)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s missed the deadline"),
                    __func__);
        return TRUE;
    }

    setconfig(ctrl);
    g_mutex_unlock(&(ctrl->busy));

    return TRUE;
}

static void exec_rx_cycle(GtkRigCtrl * ctrl)
{
    gdouble         readfreq = 0.0, tmpfreq, satfreqd, satfrequ;
    gboolean        ptt = FALSE;
    gboolean        dialchanged = FALSE;

    /* get PTT status */
    if (ctrl->engaged && ctrl->conf->ptt)
        ptt = get_ptt(ctrl, ctrl->sock);

    /* Dial feedback:
       If radio device is engaged read frequency from radio and compare it to the
       last set frequency. If different, it means that user has changed frequency
       on the radio dial => update transponder knob

       Note: If ctrl->lastrxf = 0.0 the sync has been invalidated (e.g. user pressed "tune")
       and no need to execute the dial feedback.
     */
    if ((ctrl->engaged) && (ctrl->lastrxf > 0.0))
    {
        if (ptt == FALSE)
        {
            if (!get_freq_simplex(ctrl, ctrl->sock, &readfreq))
            {
                /* error => use a passive value */
                readfreq = ctrl->lastrxf;
                ctrl->errcnt++;
            }
        }
        else
        {
            readfreq = ctrl->lastrxf;
        }

        if (fabs(readfreq - ctrl->lastrxf) >= 1.0)
        {
            dialchanged = TRUE;

            /* user might have altered radio frequency => update transponder knob */
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                    readfreq);
            ctrl->lastrxf = readfreq;

            /* doppler shift; only if we are tracking */
            if (ctrl->tracking)
            {
                satfreqd = (readfreq - ctrl->dd + ctrl->conf->lo);
            }
            else
            {
                satfreqd = readfreq + ctrl->conf->lo;
            }
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqDown),
                                    satfreqd);

            /* Update uplink if locked to downlink */
            if (ctrl->trsplock)
            {
                track_downlink(ctrl);
            }
        }
    }

    /* now, forward tracking */
    if (dialchanged)
    {
        /* no need to forward track */
        return;
    }

    /* If we are tracking, calculate the radio freq by applying both dopper shift
       and tranverter LO frequency. If we are not tracking, apply only LO frequency.
     */
    satfreqd = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqDown));
    satfrequ = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqUp));
    if (ctrl->tracking)
    {
        /* downlink */
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                satfreqd + ctrl->dd - ctrl->conf->lo);
        /* uplink */
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                satfrequ + ctrl->du - ctrl->conf->loup);
    }
    else
    {
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                satfreqd - ctrl->conf->lo);
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                satfrequ - ctrl->conf->loup);
    }

    tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqDown));

    /* if device is engaged, send freq command to radio */
    if ((ctrl->engaged) && (ptt == FALSE) &&
        (fabs(ctrl->lastrxf - tmpfreq) >= 1.0))
    {
        if (set_freq_simplex(ctrl, ctrl->sock, tmpfreq))
        {
            /* reset error counter */
            ctrl->errcnt = 0;

            /* give radio a chance to set frequency */
            g_usleep(WR_DEL);

            /* The actual frequency might be different from what we have set because
               the tuning step is larger than what we work with (e.g. FT-817 has a
               smallest tuning step of 10 Hz). Therefore we read back the actual
               frequency from the rig. */
            get_freq_simplex(ctrl, ctrl->sock, &tmpfreq);
            ctrl->lastrxf = tmpfreq;
        }
        else
        {
            ctrl->errcnt++;
        }
    }
}

static void exec_tx_cycle(GtkRigCtrl * ctrl)
{
    gdouble         readfreq = 0.0, tmpfreq, satfreqd, satfrequ;
    gboolean        ptt = TRUE;
    gboolean        dialchanged = FALSE;

    /* get PTT status */
    if (ctrl->engaged && ctrl->conf->ptt)
    {
        ptt = get_ptt(ctrl, ctrl->sock);
    }

    /* Dial feedback:
       If radio device is engaged read frequency from radio and compare it to the
       last set frequency. If different, it means that user has changed frequency
       on the radio dial => update transponder knob

       Note: If ctrl->lasttxf = 0.0 the sync has been invalidated (e.g. user pressed "tune")
       and no need to execute the dial feedback.
     */
    if ((ctrl->engaged) && (ctrl->lasttxf > 0.0))
    {
        if (ptt == TRUE)
        {
            if (!get_freq_simplex(ctrl, ctrl->sock, &readfreq))
            {
                /* error => use a passive value */
                readfreq = ctrl->lasttxf;
                ctrl->errcnt++;
            }
        }
        else
        {
            readfreq = ctrl->lasttxf;
        }

        if (fabs(readfreq - ctrl->lasttxf) >= 1.0)
        {
            dialchanged = TRUE;

            /* user might have altered radio frequency => update transponder knob */
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp), readfreq);
            ctrl->lasttxf = readfreq;

            /* doppler shift; only if we are tracking */
            if (ctrl->tracking)
            {
                satfrequ = readfreq - ctrl->du + ctrl->conf->loup;
            }
            else
            {
                satfrequ = readfreq + ctrl->conf->loup;
            }
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqUp), satfrequ);

            /* Follow with downlink if transponder is locked */
            if (ctrl->trsplock)
            {
                track_uplink(ctrl);
            }
        }
    }

    /* now, forward tracking */
    if (dialchanged)
    {
        /* no need to forward track */
        return;
    }

    /* If we are tracking, calculate the radio freq by applying both dopper shift
       and tranverter LO frequency. If we are not tracking, apply only LO frequency.
     */
    satfreqd = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqDown));
    satfrequ = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqUp));
    if (ctrl->tracking)
    {
        /* downlink */
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                satfreqd + ctrl->dd - ctrl->conf->lo);
        /* uplink */
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                satfrequ + ctrl->du - ctrl->conf->loup);
    }
    else
    {
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                satfreqd - ctrl->conf->lo);
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                satfrequ - ctrl->conf->loup);
    }

    tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqUp));

    /* if device is engaged, send freq command to radio */
    if ((ctrl->engaged) && (ptt == TRUE) &&
        (fabs(ctrl->lasttxf - tmpfreq) >= 1.0))
    {
        if (set_freq_simplex(ctrl, ctrl->sock, tmpfreq))
        {
            /* reset error counter */
            ctrl->errcnt = 0;

            /* give radio a chance to set frequency */
            g_usleep(WR_DEL);

            /* The actual frequency migh be different from what we have set because
               the tuning step is larger than what we work with (e.g. FT-817 has a
               smallest tuning step of 10 Hz). Therefore we read back the actual
               frequency from the rig. */
            get_freq_simplex(ctrl, ctrl->sock, &tmpfreq);
            ctrl->lasttxf = tmpfreq;
        }
        else
        {
            ctrl->errcnt++;
        }
    }
}

static void exec_trx_cycle(GtkRigCtrl * ctrl)
{
    exec_rx_cycle(ctrl);
    exec_tx_cycle(ctrl);
}

static void exec_toggle_cycle(GtkRigCtrl * ctrl)
{
    exec_rx_cycle(ctrl);

    /* TX cycle is executed only if user selected RIG_TYPE_TOGGLE_AUTO
     * In manual mode the TX freq update is performed only when TX is activated. 
     * Even in auto mode, the toggling is performed only once every 10 seconds.
     */
    if (ctrl->conf->type == RIG_TYPE_TOGGLE_AUTO)
    {
        GTimeVal        current_time;

        /* get the current time */
        g_get_current_time(&current_time);

        if ((ctrl->last_toggle_tx == -1) ||
            ((current_time.tv_sec - ctrl->last_toggle_tx) >= 10))
        {
            /* it's time to update TX freq */
            exec_toggle_tx_cycle(ctrl);

            /* store current time */
            ctrl->last_toggle_tx = current_time.tv_sec;
        }
    }
}

/*
 * Execute TX mode cycle.
 *
 * This function executes a transmit cycle when the primary device is of
 * RIG_TYPE_TOGGLE_AUTO. This applies to radios that support split operation
 * (e.g. TX on VHF, RX on UHF) where the frequency can not be set via CAT while
 * PTT is active.
 * 
 * If PTT=TRUE we are in TX mode and hence there is nothing to do since the
 * frequency is kept constant.
 * 
 * If PTT=FALSE we are in RX mode and we should update the TX frequency by
 * using set_freq_toggle()
 * 
 * For these kind of radios there is no dial-feedback for the TX frequency.
 */

static void exec_toggle_tx_cycle(GtkRigCtrl * ctrl)
{
    gdouble         tmpfreq;
    gboolean        ptt = TRUE;

    if (ctrl->engaged && ctrl->conf->ptt)
    {
        ptt = get_ptt(ctrl, ctrl->sock);
    }

    /* if we are in TX mode do nothing */
    if (ptt == TRUE)
    {
        return;
    }

    /* Get the desired uplink frequency from controller */
    tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqUp));

    /* if device is engaged, send freq command to radio */
    if ((ctrl->engaged) && (fabs(ctrl->lasttxf - tmpfreq) >= 10.0))
    {
        if (set_freq_toggle(ctrl, ctrl->sock, tmpfreq))
        {
            /* reset error counter */
            ctrl->errcnt = 0;
        }
        else
        {
            ctrl->errcnt++;
        }

        /* store the last sent frequency even if an error occurred */
        ctrl->lasttxf = tmpfreq;
    }

}

static void exec_duplex_tx_cycle(GtkRigCtrl * ctrl)
{
    gdouble         readfreq = 0.0, tmpfreq, satfreqd, satfrequ;
    gboolean        dialchanged = FALSE;

    /* Dial feedback:
       If radio device is engaged read frequency from radio and compare it to the
       last set frequency. If different, it means that user has changed frequency
       on the radio dial => update transponder knob

       Note: If ctrl->lasttxf = 0.0 the sync has been invalidated (e.g. user pressed "tune")
       and no need to execute the dial feedback.
     */
    if ((ctrl->engaged) && (ctrl->lasttxf > 0.0))
    {
        if (!get_freq_toggle(ctrl, ctrl->sock, &readfreq))
        {
            /* error => use a passive value */
            readfreq = ctrl->lasttxf;
            ctrl->errcnt++;
        }

        if (fabs(readfreq - ctrl->lasttxf) >= 1.0)
        {
            dialchanged = TRUE;

            /* user might have altered radio frequency => update transponder knob */
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp), readfreq);
            ctrl->lasttxf = readfreq;

            /* doppler shift; only if we are tracking */
            if (ctrl->tracking)
            {
                satfrequ = readfreq - ctrl->du + ctrl->conf->loup;
            }
            else
            {
                satfrequ = readfreq + ctrl->conf->loup;
            }
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqUp), satfrequ);

            /* Follow with downlink if transponder is locked */
            if (ctrl->trsplock)
            {
                track_uplink(ctrl);
            }
        }
    }

    /* now, forward tracking */
    if (dialchanged)
    {
        /* no need to forward track */
        return;
    }

    /* If we are tracking, calculate the radio freq by applying both dopper shift
       and tranverter LO frequency. If we are not tracking, apply only LO frequency.
     */
    satfreqd = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqDown));
    satfrequ = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqUp));
    if (ctrl->tracking)
    {
        /* downlink */
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                satfreqd + ctrl->dd - ctrl->conf->lo);
        /* uplink */
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                satfrequ + ctrl->du - ctrl->conf->loup);
    }
    else
    {
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                satfreqd - ctrl->conf->lo);
        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                satfrequ - ctrl->conf->loup);
    }

    tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqUp));

    /* if device is engaged, send freq command to radio */
    if ((ctrl->engaged) && (fabs(ctrl->lasttxf - tmpfreq) >= 1.0))
    {
        if (set_freq_toggle(ctrl, ctrl->sock, tmpfreq))
        {
            /* reset error counter */
            ctrl->errcnt = 0;

            /* give radio a chance to set frequency */
            g_usleep(WR_DEL);

            /* The actual frequency migh be different from what we have set because
               the tuning step is larger than what we work with (e.g. FT-817 has a
               smallest tuning step of 10 Hz). Therefore we read back the actual
               frequency from the rig. */
            get_freq_toggle(ctrl, ctrl->sock, &tmpfreq);
            ctrl->lasttxf = tmpfreq;
        }
        else
        {
            ctrl->errcnt++;
        }
    }
}

static void exec_duplex_cycle(GtkRigCtrl * ctrl)
{
    exec_rx_cycle(ctrl);
    exec_duplex_tx_cycle(ctrl);
}

static void exec_dual_rig_cycle(GtkRigCtrl * ctrl)
{
    gdouble         tmpfreq, readfreq, satfreqd, satfrequ;
    gboolean        dialchanged = FALSE;

    /* Execute downlink cycle using ctrl->conf */
    if (ctrl->engaged && (ctrl->lastrxf > 0.0))
    {
        /* get frequency from receiver */
        if (!get_freq_simplex(ctrl, ctrl->sock, &readfreq))
        {
            /* error => use a passive value */
            readfreq = ctrl->lastrxf;
            ctrl->errcnt++;
        }

        if (fabs(readfreq - ctrl->lastrxf) >= 1.0)
        {
            dialchanged = TRUE;

            /* user might have altered radio frequency => update transponder knob */
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                    readfreq);
            ctrl->lastrxf = readfreq;

            /* doppler shift; only if we are tracking */
            if (ctrl->tracking)
            {
                satfreqd = readfreq - ctrl->dd + ctrl->conf->lo;
            }
            else
            {
                satfreqd = readfreq + ctrl->conf->lo;
            }
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqDown),
                                    satfreqd);

            /* Update uplink if locked to downlink */
            if (ctrl->trsplock)
            {
                track_downlink(ctrl);
            }
        }
    }

    if (dialchanged)
    {
        /* update uplink */
        satfrequ = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqUp));
        if (ctrl->tracking)
        {
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                    satfrequ + ctrl->du - ctrl->conf2->loup);
        }
        else
        {
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                    satfrequ - ctrl->conf2->loup);
        }

        tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqUp));

        /* if device is engaged, send freq command to radio */
        if ((ctrl->engaged) && (fabs(ctrl->lasttxf - tmpfreq) >= 1.0))
        {
            if (set_freq_simplex(ctrl, ctrl->sock2, tmpfreq))
            {
                /* reset error counter */
                ctrl->errcnt = 0;

                /* give radio a chance to set frequency */
                g_usleep(WR_DEL);

                /* The actual frequency migh be different from what we have set */
                get_freq_simplex(ctrl, ctrl->sock2, &tmpfreq);
                ctrl->lasttxf = tmpfreq;
            }
            else
            {
                ctrl->errcnt++;
            }
        }
    }                           /* dialchanged on downlink */
    else
    {
        /* if no dial change on downlink perform forward tracking on downlink
           and execute uplink controller too */
        satfreqd = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqDown));
        if (ctrl->tracking)
        {
            /* downlink */
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                    satfreqd + ctrl->dd - ctrl->conf->lo);
        }
        else
        {
            gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                    satfreqd - ctrl->conf->lo);
        }

        tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqDown));

        /* if device is engaged, send freq command to radio */
        if ((ctrl->engaged) && (fabs(ctrl->lastrxf - tmpfreq) >= 1.0))
        {
            if (set_freq_simplex(ctrl, ctrl->sock, tmpfreq))
            {
                /* reset error counter */
                ctrl->errcnt = 0;

                /* give radio a chance to set frequency */
                g_usleep(WR_DEL);

                /* The actual frequency migh be different from what we have set */
                get_freq_simplex(ctrl, ctrl->sock, &tmpfreq);
                ctrl->lastrxf = tmpfreq;
            }
            else
            {
                ctrl->errcnt++;
            }
        }

        /* Now execute uplink controller */

        /* check if uplink dial has changed */
        if ((ctrl->engaged) && (ctrl->lasttxf > 0.0))
        {
            if (!get_freq_simplex(ctrl, ctrl->sock2, &readfreq))
            {
                /* error => use a passive value */
                readfreq = ctrl->lasttxf;
                ctrl->errcnt++;
            }

            if (fabs(readfreq - ctrl->lasttxf) >= 1.0)
            {
                dialchanged = TRUE;

                gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                        readfreq);
                ctrl->lasttxf = readfreq;

                /* doppler shift; only if we are tracking */
                if (ctrl->tracking)
                {
                    satfrequ = readfreq - ctrl->du + ctrl->conf2->loup;
                }
                else
                {
                    satfrequ = readfreq + ctrl->conf2->loup;
                }
                gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqUp),
                                        satfrequ);

                /* Follow with downlink if transponder is locked */
                if (ctrl->trsplock)
                {
                    track_uplink(ctrl);
                }
            }
        }

        if (dialchanged)
        {                       /* on uplink */
            /* update downlink */
            satfreqd =
                gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqDown));
            if (ctrl->tracking)
            {
                gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                        satfreqd + ctrl->dd - ctrl->conf->lo);
            }
            else
            {
                gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqDown),
                                        satfreqd - ctrl->conf->lo);
            }

            tmpfreq =
                gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqDown));

            /* if device is engaged, send freq command to radio */
            if ((ctrl->engaged) && (fabs(ctrl->lastrxf - tmpfreq) >= 1.0))
            {
                if (set_freq_simplex(ctrl, ctrl->sock, tmpfreq))
                {
                    /* reset error counter */
                    ctrl->errcnt = 0;

                    /* give radio a chance to set frequency */
                    g_usleep(WR_DEL);

                    /* The actual frequency migh be different from what we have set */
                    get_freq_simplex(ctrl, ctrl->sock, &tmpfreq);
                    ctrl->lastrxf = tmpfreq;
                }
                else
                {
                    ctrl->errcnt++;
                }
            }
        }                       /* dialchanged on uplink */
        else
        {
            /* perform forward tracking on uplink */
            satfrequ = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqUp));
            if (ctrl->tracking)
            {
                gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                        satfrequ + ctrl->du -
                                        ctrl->conf2->loup);
            }
            else
            {
                gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->RigFreqUp),
                                        satfrequ - ctrl->conf2->loup);
            }

            tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqUp));

            /* if device is engaged, send freq command to radio */
            if ((ctrl->engaged) && (fabs(ctrl->lasttxf - tmpfreq) >= 1.0))
            {
                if (set_freq_simplex(ctrl, ctrl->sock2, tmpfreq))
                {
                    /* reset error counter */
                    ctrl->errcnt = 0;

                    /* give radio a chance to set frequency */
                    g_usleep(WR_DEL);

                    /* The actual frequency might be different from what we have set. */
                    get_freq_simplex(ctrl, ctrl->sock2, &tmpfreq);
                    ctrl->lasttxf = tmpfreq;
                }
                else
                {
                    ctrl->errcnt++;
                }
            }
        }                       /* else dialchange on uplink */
    }                           /* else dialchange on downlink */
}

static gboolean get_ptt(GtkRigCtrl * ctrl, gint sock)
{
    gchar          *buff, **vbuff;
    gchar           buffback[128];
    gboolean        retcode;
    guint64         pttstat = 0;

    if (ctrl->conf->ptt == PTT_TYPE_CAT)
    {
        /* send command get_ptt (t) */
        buff = g_strdup_printf("t\x0a");
    }
    else
    {
        /* send command \get_dcd */
        buff = g_strdup_printf("%c\x0a", 0x8b);
    }

    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    if (retcode)
    {
        vbuff = g_strsplit(buffback, "\n", 3);
        if (vbuff[0])
            pttstat = g_ascii_strtoull(vbuff[0], NULL, 0);      //FIXME base = 0 ok?
        g_strfreev(vbuff);
    }

    g_free(buff);

    return (pttstat == 1) ? TRUE : FALSE;
}

static gboolean set_ptt(GtkRigCtrl * ctrl, gint sock, gboolean ptt)
{
    gchar          *buff;
    gchar           buffback[128];
    gboolean        retcode;

    /* send command */
    if (ptt == TRUE)
        buff = g_strdup_printf("T 1\x0aq\x0a");
    else
        buff = g_strdup_printf("T 0\x0aq\x0a");

    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    g_free(buff);

    return (check_set_response(buffback, retcode, __func__));

}

/*
 * Check for AOS and LOS and send signal if enabled for rig.
 *
 * @param ctrl Pointer to the GtkRigCtrl handle.
 * @return TRUE if the operation was successful, FALSE if a connection error
 *         occurred.
 *
 * This function checks whether AOS or LOS just happened and sends the
 * apropriate signal to the RIG if this signalling is enabled.
 */
static gboolean check_aos_los(GtkRigCtrl * ctrl)
{
    gboolean        retcode = TRUE;
    gchar           retbuf[10];

    if (ctrl->engaged && ctrl->tracking)
    {
        if (ctrl->prev_ele < 0.0 && ctrl->target->el >= 0.0)
        {
            /* AOS has occurred */
            if (ctrl->conf->signal_aos)
            {
                retcode &= send_rigctld_command(ctrl, ctrl->sock, "AOS\n",
                                                retbuf, 10);
            }
            if (ctrl->conf2 != NULL)
            {
                if (ctrl->conf2->signal_aos)
                {
                    retcode &= send_rigctld_command(ctrl, ctrl->sock2, "AOS\n",
                                                    retbuf, 10);
                }
            }
        }
        else if (ctrl->prev_ele >= 0.0 && ctrl->target->el < 0.0)
        {
            /* LOS has occurred */
            if (ctrl->conf->signal_los)
            {
                retcode &= send_rigctld_command(ctrl, ctrl->sock, "LOS\n",
                                                retbuf, 10);
            }
            if (ctrl->conf2 != NULL)
            {
                if (ctrl->conf2->signal_los)
                {
                    retcode &= send_rigctld_command(ctrl, ctrl->sock2, "LOS\n",
                                                    retbuf, 10);
                }
            }
        }
    }

    ctrl->prev_ele = ctrl->target->el;

    return retcode;
}

/*
 * Set frequency in simplex mode
 *
 * Returns TRUE if the operation was successful, FALSE otherwise
 */
static gboolean set_freq_simplex(GtkRigCtrl * ctrl, gint sock, gdouble freq)
{
    gchar          *buff;
    gchar           buffback[128];
    gboolean        retcode;

    buff = g_strdup_printf("F %10.0f\x0a", freq);
    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    g_free(buff);

    return (check_set_response(buffback, retcode, __func__));
}


/*
 * Set frequency in toggle mode
 *
 * Returns TRUE if the operation was successful, FALSE otherwise
 */
static gboolean set_freq_toggle(GtkRigCtrl * ctrl, gint sock, gdouble freq)
{
    gchar          *buff;
    gchar           buffback[128];
    gboolean        retcode;

    /* send command */
    buff = g_strdup_printf("I %10.0f\x0a", freq);
    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    g_free(buff);

    return (check_set_response(buffback, retcode, __func__));

}

/*
 * Turn on the radios toggle mode
 *
 * Returns TRUE if the operation was successful
 */
static gboolean set_toggle(GtkRigCtrl * ctrl, gint sock)
{
    gchar          *buff;
    gchar           buffback[128];
    gboolean        retcode;

    buff = g_strdup_printf("S 1 %d\x0a", ctrl->conf->vfoDown);
    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    g_free(buff);

    return (check_set_response(buffback, retcode, __func__));
}

/*
 * Turn off the radios toggle mode
 *
 * Returns TRUE if the operation was successful
 */
static gboolean unset_toggle(GtkRigCtrl * ctrl, gint sock)
{
    gchar          *buff;
    gchar           buffback[128];
    gboolean        retcode;

    /* send command */
    buff = g_strdup_printf("S 0 %d\x0a", ctrl->conf->vfoDown);
    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    g_free(buff);

    return (check_set_response(buffback, retcode, __func__));
}

/*
 * Get frequency
 *
 * Returns TRUE if the operation was successful, FALSE otherwise
 */
static gboolean get_freq_simplex(GtkRigCtrl * ctrl, gint sock, gdouble * freq)
{
    gchar          *buff, **vbuff;
    gchar           buffback[128];
    gboolean        retcode;
    gboolean        retval = TRUE;

    buff = g_strdup_printf("f\x0a");
    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    retcode = check_get_response(buffback, retcode, __func__);
    if (retcode)
    {
        vbuff = g_strsplit(buffback, "\n", 3);
        if (vbuff[0])
            *freq = g_ascii_strtod(vbuff[0], NULL);
        else
            retval = FALSE;
        g_strfreev(vbuff);
    }
    else
    {
        retval = FALSE;
    }

    //g_free(buff);
    return retval;
}

/*
 * Get frequency when the radio is working toggle
 *
 * Returns TRUE if the operation was successful, FALSE otherwise
 */
static gboolean get_freq_toggle(GtkRigCtrl * ctrl, gint sock, gdouble * freq)
{
    gchar          *buff, **vbuff;
    gchar           buffback[128];
    gboolean        retcode;
    gboolean        retval = TRUE;

    if (freq == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: NULL storage."), __FILE__, __LINE__);
        return FALSE;
    }

    /* send command */
    buff = g_strdup_printf("i\x0a");
    retcode = send_rigctld_command(ctrl, sock, buff, buffback, 128);
    retcode = check_get_response(buffback, retcode, __func__);
    if (retcode)
    {
        vbuff = g_strsplit(buffback, "\n", 3);
        if (vbuff[0])
            *freq = g_ascii_strtod(vbuff[0], NULL);
        else
            retval = FALSE;

        g_strfreev(vbuff);
    }
    else
    {
        retval = FALSE;
    }

    //g_free(buff);
    return retval;
}

static void update_count_down(GtkRigCtrl * ctrl, gdouble t)
{
    gdouble         targettime;
    gdouble         delta;
    gchar          *buff;
    guint           h, m, s;
    gchar          *aoslos;

    /* select AOS or LOS time depending on target elevation */
    if (ctrl->target->el < 0.0)
    {
        targettime = ctrl->target->aos;
        aoslos = g_strdup_printf(_("AOS in"));
    }
    else
    {
        targettime = ctrl->target->los;
        aoslos = g_strdup_printf(_("LOS in"));
    }

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
        buff =
            g_strdup_printf
            ("<span size='xx-large'><b>%s %02d:%02d:%02d</b></span>", aoslos,
             h, m, s);
    else
        buff =
            g_strdup_printf("<span size='xx-large'><b>%s %02d:%02d</b></span>",
                            aoslos, m, s);

    gtk_label_set_markup(GTK_LABEL(ctrl->SatCnt), buff);

    g_free(buff);
    g_free(aoslos);
}

static void load_trsp_list(GtkRigCtrl * ctrl)
{
    trsp_t         *trsp = NULL;
    guint           i, n;

    if (ctrl->trsplist != NULL)
    {
        n = g_slist_length(ctrl->trsplist);
        for (i = 0; i < n; i++)
            gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(ctrl->TrspSel), 0);

        free_transponders(ctrl->trsplist);
        ctrl->trsp = NULL;
    }

    /* check if there is a target satellite */
    if (ctrl->target == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s:%s: GtkSatModule has no target satellite."),
                    __FILE__, __func__);
        return;
    }

    /* read transponders for new target */
    ctrl->trsplist = read_transponders(ctrl->target->tle.catnr);
    n = g_slist_length(ctrl->trsplist);
    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%s: Satellite %d has %d transponder modes."),
                __FILE__, __func__, ctrl->target->tle.catnr, n);

    if (n == 0)
        return;

    for (i = 0; i < n; i++)
    {
        trsp = (trsp_t *) g_slist_nth_data(ctrl->trsplist, i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctrl->TrspSel),
                                       trsp->name);

        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%s: Read transponder '%s' for satellite %d"),
                    __FILE__, __func__, trsp->name, ctrl->target->tle.catnr);
    }

    ctrl->trsp = (trsp_t *) g_slist_nth_data(ctrl->trsplist, 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctrl->TrspSel), 0);
}

static gboolean have_conf()
{
    GDir           *dir = NULL; /* directory handle */
    GError         *error = NULL;       /* error flag and info */
    gchar          *dirname;    /* directory name */
    const gchar    *filename;   /* file name */
    gint            i = 0;

    dirname = get_hwconf_dir();
    dir = g_dir_open(dirname, 0, &error);
    if (dir)
    {
        while ((filename = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(filename, ".rig"))
            {
                i++;
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

/*
 * Track the downlink frequency by setting the uplink frequency
 * according to the lower limit of the downlink passband.
 */
static void track_downlink(GtkRigCtrl * ctrl)
{
    gdouble         delta, down, up;

    if (ctrl->trsp == NULL)
        return;

    /* ensure that we have a useable transponder config */
    if ((ctrl->trsp->downlow > 0) && (ctrl->trsp->uplow > 0))
    {
        down = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqDown));
        delta = down - ctrl->trsp->downlow;

        if (ctrl->trsp->invert)
            up = ctrl->trsp->uphigh - delta;
        else
            up = ctrl->trsp->uplow + delta;

        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqUp), up);
    }
}

/*
 * Track the uplink frequency by setting the downlink frequency
 * according to the offset from the lower limit on the uplink passband.
 */
static void track_uplink(GtkRigCtrl * ctrl)
{
    gdouble         delta, down, up;

    if (ctrl->trsp == NULL)
        return;

    /* ensure that we have a useable transponder config */
    if ((ctrl->trsp->downlow > 0) && (ctrl->trsp->uplow > 0))
    {
        up = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->SatFreqUp));
        delta = up - ctrl->trsp->uplow;

        if (ctrl->trsp->invert)
            down = ctrl->trsp->downhigh - delta;
        else
            down = ctrl->trsp->downlow + delta;

        gtk_freq_knob_set_value(GTK_FREQ_KNOB(ctrl->SatFreqDown), down);
    }
}

static gboolean is_rig_tx_capable(const gchar * confname)
{
    radio_conf_t   *conf = NULL;
    gboolean        cantx = FALSE;

    conf = g_try_new(radio_conf_t, 1);
    if (conf == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to allocate memory for radio config"),
                    __FILE__, __LINE__);
        return FALSE;
    }

    /* load new configuration */
    conf->name = g_strdup(confname);
    if (radio_conf_read(conf))
    {
        cantx = (conf->type == RIG_TYPE_RX) ? FALSE : TRUE;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Error reading radio configuration %s"),
                    __FILE__, __LINE__, confname);

        cantx = FALSE;
    }

    g_free(conf->name);
    if (conf->host)
        g_free(conf->host);
    g_free(conf);

    return cantx;
}

gboolean send_rigctld_command(GtkRigCtrl * ctrl, gint sock, gchar * buff,
                              gchar * buffout, gint sizeout)
{
    gint            written;
    gint            size;

    /* Enter critical section! */
    g_mutex_lock(&ctrl->writelock);

    /* added by Marcel Cimander; win32 newline -> \10\13 */
#ifdef WIN32
    size = strlen(buff) - 1;
    /* added by Marcel Cimander; unix newline -> \10 (apple -> \13) */
#else
    size = strlen(buff);
#endif

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s:%s: sending %d bytes to rigctld as \"%s\""),
                __FILE__, __func__, size, buff);
    /* send command */
    written = send(sock, buff, strlen(buff), 0);
    if (written != size)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: SIZE ERROR %d / %d"), __func__, written, size);
    }
    if (written == -1)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: rigctld port closed"), __func__);
        return FALSE;
    }
    /* try to read answer */
    size = recv(sock, buffout, sizeout - 1, 0);
    if (size == -1)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: rigctld port closed"), __func__);
        return FALSE;
    }

    buffout[size] = '\0';
    if (size == 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%s: Got 0 bytes from rigctld"), __FILE__, __func__);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%s: Read %d bytes from rigctld"),
                    __FILE__, __func__, size);
    }
    ctrl->wrops++;

    /* Leave critical section! */
    g_mutex_unlock(&ctrl->writelock);

    return TRUE;
}

/*
 * Catch events when the user presses the SPACE key on the keyboard.
 * This is used to toggle betweer RX/TX when using FT817/857/897 in manual mode.
 */
static gboolean key_press_cb(GtkWidget * widget, GdkEventKey * pKey,
                             gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(widget);
    gboolean        event_managed = FALSE;

    (void)data;

    if (pKey->type == GDK_KEY_PRESS)
    {
        switch (pKey->keyval)
        {
            /* keyvals not in API docs. See <gdk/gdkkeysyms.h> for a complete list */
        case GDK_KEY_space:
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Detected SPACEBAR pressed event"), __func__);

            /* manage PTT event but only if rig is of type TOGGLE_MAN */
            if (ctrl->conf->type == RIG_TYPE_TOGGLE_MAN)
            {
                manage_ptt_event(ctrl);
                event_managed = TRUE;
            }
            break;

        default:
            sat_log_log(SAT_LOG_LEVEL_DEBUG,
                        _
                        ("%s:%s: Keypress value %i not managed by this function"),
                        __FILE__, __func__, pKey->keyval);
            break;
        }
    }

    return event_managed;
}

/*
 * This function is used to manage PTT events, e.g. the user presses
 * the spacebar. It is only useful for RIG_TYPE_TOGGLE_MAN and possibly for 
 * RIG_TYPE_TOGGLE_AUTO.
 * 
 * First, the function will try to lock the controller. If the lock is
 * acquired the function checks the current PTT status.
 * If PTT status is FALSE (off), it will set the TX frequency and set PTT to
 * TRUE (on). If PTT status is TRUE (on) it will simply set the PTT to FALSE
 * (off).
 * 
 * This function assumes that the radio supprot set/get PTT, otherwise it makes
 * no sense to use it!
 */
static void manage_ptt_event(GtkRigCtrl * ctrl)
{
    guint           timeout = 1;
    gboolean        ptt = FALSE;

    /* wait for controller to be idle or until the timeout triggers */
    while (timeout < 5)
    {
        if (g_mutex_trylock(&(ctrl->busy)) == TRUE)
        {
            timeout = 17;       /* use an arbitrary value that is large enough */
        }
        else
        {
            /* wait for 100 msec */
            g_usleep(100000);
            timeout++;
        }
    }

    if (timeout == 17)
    {
        /* timeout did not expire, we've got the controller lock */
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Acquired controller lock"), __func__);

        if (ctrl->engaged == FALSE)
        {
            sat_log_log(SAT_LOG_LEVEL_INFO,
                        _("%s: Controller not engaged; PTT event ignored "
                          "(Hint: Enable the Engage button)"), __func__);
        }
        else
        {
            ptt = get_ptt(ctrl, ctrl->sock);

            if (ptt == FALSE)
            {
                /* PTT is OFF => set TX freq then set PTT to ON */
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s: PTT is OFF => Set TX freq and PTT=ON"),
                            __func__);

                exec_toggle_tx_cycle(ctrl);
                set_ptt(ctrl, ctrl->sock, TRUE);
            }
            else
            {
                /* PTT is ON => set to OFF */
                sat_log_log(SAT_LOG_LEVEL_DEBUG,
                            _("%s: PTT is ON = Set PTT=OFF"), __func__);

                set_ptt(ctrl, ctrl->sock, FALSE);
            }
        }

        g_mutex_unlock(&(ctrl->busy));
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to acquire controller lock; PTT event "
                      "not handled"), __func__);
    }

}

static gboolean open_rigctld_socket(radio_conf_t * conf, gint * sock)
{
    struct sockaddr_in ServAddr;
    struct hostent *h;
    gint            status;

    *sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*sock < 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to create socket"), __func__);
        *sock = 0;
        return FALSE;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Network socket created successfully"), __func__);
    }

    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family = AF_INET;      /* Internet address family */
    h = gethostbyname(conf->host);
    memcpy((char *)&ServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    ServAddr.sin_port = htons(conf->port);      /* Server port */

    /* establish connection */
    status = connect(*sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr));
    if (status < 0)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to connect to %s:%d"),
                    __func__, conf->host, conf->port);
        *sock = 0;
        return FALSE;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s: Connection opened to %s:%d"),
                    __func__, conf->host, conf->port);
    }

    return TRUE;
}

static gboolean close_rigctld_socket(gint * sock)
{
    gint            written;

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

/* Sort the list of satellites in the combo box. */
static gint sat_name_compare(sat_t * a, sat_t * b)
{
    return (gpredict_strcmp(a->nickname, b->nickname));
}

/* Sort the list of rigs in the combo box */
static gint rig_name_compare(const gchar * a, const gchar * b)
{
    return (gpredict_strcmp(a, b));
}

static inline gboolean check_set_response(gchar * buffback, gboolean retcode,
                                          const gchar * function)
{
    if (retcode == TRUE)
    {
        if (strncmp(buffback, "RPRT 0", 6) != 0)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%s: %s rigctld returned error (%s)"),
                        __FILE__, __func__, function, buffback);

            retcode = FALSE;
        }
    }

    return retcode;
}

static inline gboolean check_get_response(gchar * buffback, gboolean retcode,
                                          const gchar * function)
{
    if (retcode == TRUE)
    {
        if (strncmp(buffback, "RPRT", 4) == 0)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%s: %s rigctld returned error (%s)"),
                        __FILE__, __func__, function, buffback);

            retcode = FALSE;
        }
    }

    return retcode;
}

static void rigctrl_close(GtkRigCtrl * data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    ctrl->lasttxf = 0.0;
    ctrl->lastrxf = 0.0;

    remove_timer(ctrl);

    if ((ctrl->conf->type == RIG_TYPE_TOGGLE_AUTO) ||
        (ctrl->conf->type == RIG_TYPE_TOGGLE_MAN))
    {
        unset_toggle(ctrl, ctrl->sock);
    }

    if (ctrl->conf2 != NULL)
    {
        close_rigctld_socket(&(ctrl->sock2));
    }
    close_rigctld_socket(&(ctrl->sock));
}

static void rigctrl_open(GtkRigCtrl * data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    ctrl->wrops = 0;

    start_timer(ctrl);

    open_rigctld_socket(ctrl->conf, &(ctrl->sock));

    /* set initial frequency */
    if (ctrl->conf2 != NULL)
    {
        open_rigctld_socket(ctrl->conf2, &(ctrl->sock2));
        /* set initial dual mode */
        exec_dual_rig_cycle(ctrl);
    }
    else
    {
        switch (ctrl->conf->type)
        {

        case RIG_TYPE_RX:
            exec_rx_cycle(ctrl);
            break;

        case RIG_TYPE_TX:
            exec_tx_cycle(ctrl);
            break;

        case RIG_TYPE_TRX:
            exec_trx_cycle(ctrl);
            break;

        case RIG_TYPE_DUPLEX:
            /* set rig into SAT mode (hamlib needs it even if rig already in SAT) */
            setup_split(ctrl);
            exec_duplex_cycle(ctrl);
            break;

        case RIG_TYPE_TOGGLE_AUTO:
        case RIG_TYPE_TOGGLE_MAN:
            set_toggle(ctrl, ctrl->sock);
            ctrl->last_toggle_tx = -1;
            exec_toggle_cycle(ctrl);
            break;

        default:
            /* this is an error! */
            ctrl->conf->type = RIG_TYPE_RX;
            exec_rx_cycle(ctrl);
            break;
        }
    }
}

/* Communication thread for hamlib rigctld */
gpointer rigctl_run(gpointer data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);
    GtkRigCtrl     *t_ctrl = GTK_RIG_CTRL(data);

    while (1)
    {
        t_ctrl = GTK_RIG_CTRL(g_async_queue_pop(ctrl->rigctlq));
        ctrl = t_ctrl;
        while (g_main_context_iteration(NULL, FALSE));

        if (t_ctrl == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%s: ERROR: NO VALID ctrl-struct"), __FILE__,
                        __func__);
            continue;
        }

        if (t_ctrl->engaged)
        {
            if (!t_ctrl->sock)
                rigctrl_open(t_ctrl);

            if (!t_ctrl->timerid)
                start_timer(t_ctrl);
        }
        else
        {
            g_mutex_lock(&t_ctrl->widgetsync);

            if (t_ctrl->sock > 0)
                rigctrl_close(t_ctrl);

            if (t_ctrl->timerid)
                remove_timer(t_ctrl);

            g_cond_signal(&t_ctrl->widgetready);
            g_mutex_unlock(&t_ctrl->widgetsync);
            break;
        }

        check_aos_los(t_ctrl);

        if (t_ctrl->conf2 != NULL)
        {
            exec_dual_rig_cycle(t_ctrl);
        }
        else
        {
            /* Execute controller cycle depending on primary radio type */
            switch (t_ctrl->conf->type)
            {

            case RIG_TYPE_RX:
                exec_rx_cycle(t_ctrl);
                break;

            case RIG_TYPE_TX:
                exec_tx_cycle(t_ctrl);
                break;

            case RIG_TYPE_TRX:
                exec_trx_cycle(t_ctrl);
                break;

            case RIG_TYPE_DUPLEX:
                exec_duplex_cycle(t_ctrl);
                break;

            case RIG_TYPE_TOGGLE_AUTO:
            case RIG_TYPE_TOGGLE_MAN:
                exec_toggle_cycle(t_ctrl);
                break;

            default:
                /* invalid mode */
                sat_log_log(SAT_LOG_LEVEL_ERROR,
                            _("%s:%s: Invalid radio type %d. Setting type to "
                              "RIG_TYPE_RX"), __FILE__, __func__,
                            t_ctrl->conf->type);
                t_ctrl->conf->type = RIG_TYPE_RX;
            }
        }

        /* perform error count checking */
        if (t_ctrl->errcnt >= MAX_ERROR_COUNT)
        {
            /* disengage device */
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_ctrl->LockBut),
                                         FALSE);
            t_ctrl->engaged = FALSE;
            t_ctrl->errcnt = 0;
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _
                        ("%s:%s: MAX_ERROR_COUNT (%d) reached. Disengaging device!"),
                        __FILE__, __func__, MAX_ERROR_COUNT);

            //g_print ("ERROR. WROPS = %d\n", ctrl->wrops);
        }

        //g_print ("       WROPS = %d\n", ctrl->wrops);
    }

    if (t_ctrl->sock > 0)
        rigctrl_close(t_ctrl);

    if (t_ctrl->timerid)
        remove_timer(t_ctrl);

    return NULL;
}

void start_timer(GtkRigCtrl * data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    /*  start timeout timer here ("Cycle")! */
    if (ctrl->timerid > 0)
        g_source_remove(ctrl->timerid);

    ctrl->timerid =
        gdk_threads_add_timeout(ctrl->delay, rig_ctrl_timeout_cb, ctrl);
}

void remove_timer(GtkRigCtrl * data)
{
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    /* stop timer */
    if (ctrl->timerid > 0)
        g_source_remove(ctrl->timerid);
    ctrl->timerid = 0;
}

void setconfig(gpointer data)
{
    /* something has changed... */
    GtkRigCtrl     *ctrl = GTK_RIG_CTRL(data);

    if (ctrl != NULL)
    {
        g_async_queue_push(ctrl->rigctlq, ctrl);
    }
}
