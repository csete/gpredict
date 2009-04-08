/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2008  Alexandru Csete, OZ9AEC.

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
/** \brief RIG control window.
 *  \ingroup widgets
 *
 * The master radio control UI is implemented as a Gtk+ Widget in order
 * to allow multiple instances. The widget is created from the module
 * popup menu and each module can have several radio control windows
 * attached to it. Note, however, that current implementation only
 * allows one control window per module.
 * 
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include "compat.h"
#include "sat-log.h"
#include "predict-tools.h"
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "gtk-freq-knob.h"
#include "radio-conf.h"
#include "trsp-conf.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif

/* NETWORK */
//#include <sys/types.h>
#include <sys/socket.h>     /* socket(), connect(), send() */
#include <netinet/in.h>     /* struct sockaddr_in */
#include <arpa/inet.h>      /* htons() */
#include <netdb.h>          /* gethostbyname() */
/* END */
#include "gtk-rig-ctrl.h"


#define AZEL_FMTSTR "%7.2f\302\260"
#define MAX_ERROR_COUNT 5


static void gtk_rig_ctrl_class_init (GtkRigCtrlClass *class);
static void gtk_rig_ctrl_init       (GtkRigCtrl      *list);
static void gtk_rig_ctrl_destroy    (GtkObject       *object);


static GtkWidget *create_downlink_widgets (GtkRigCtrl *ctrl);
static GtkWidget *create_uplink_widgets (GtkRigCtrl *ctrl);
static GtkWidget *create_target_widgets (GtkRigCtrl *ctrl);
static GtkWidget *create_conf_widgets (GtkRigCtrl *ctrl);
static GtkWidget *create_count_down_widgets (GtkRigCtrl *ctrl);

/* callback functions */
static void sat_selected_cb (GtkComboBox *satsel, gpointer data);
static void track_toggle_cb (GtkToggleButton *button, gpointer data);
static void delay_changed_cb (GtkSpinButton *spin, gpointer data);
static void rig_selected_cb (GtkComboBox *box, gpointer data);
static void rig_locked_cb (GtkToggleButton *button, gpointer data);
static void trsp_selected_cb (GtkComboBox *box, gpointer data);
static gboolean rig_ctrl_timeout_cb (gpointer data);

/* radio control functions */
static void exec_rx_cycle (GtkRigCtrl *ctrl);
static void exec_tx_cycle (GtkRigCtrl *ctrl);
static void exec_trx_cycle (GtkRigCtrl *ctrl);
static void exec_duplex_cycle (GtkRigCtrl *ctrl);
static gboolean set_freq_simplex (GtkRigCtrl *ctrl, gdouble freq);
static gboolean get_freq_simplex (GtkRigCtrl *ctrl, gdouble *freq);
static gboolean get_ptt (GtkRigCtrl *ctrl);
static void update_count_down (GtkRigCtrl *ctrl, gdouble t);

/* misc utility functions */
static void load_trsp_list (GtkRigCtrl *ctrl);
static void store_sats (gpointer key, gpointer value, gpointer user_data);
static gboolean have_conf (void);


static GtkVBoxClass *parent_class = NULL;

static GdkColor ColBlack = { 0, 0, 0, 0};
static GdkColor ColWhite = { 0, 0xFFFF, 0xFFFF, 0xFFFF};
static GdkColor ColRed =   { 0, 0xFFFF, 0, 0};
static GdkColor ColGreen = {0, 0, 0xFFFF, 0};


GType
gtk_rig_ctrl_get_type ()
{
    static GType gtk_rig_ctrl_type = 0;

    if (!gtk_rig_ctrl_type) {

        static const GTypeInfo gtk_rig_ctrl_info = {
            sizeof (GtkRigCtrlClass),
            NULL,  /* base_init */
            NULL,  /* base_finalize */
            (GClassInitFunc) gtk_rig_ctrl_class_init,
            NULL,  /* class_finalize */
            NULL,  /* class_data */
            sizeof (GtkRigCtrl),
            2,     /* n_preallocs */
            (GInstanceInitFunc) gtk_rig_ctrl_init,
        };

        gtk_rig_ctrl_type = g_type_register_static (GTK_TYPE_VBOX,
                                                    "GtkRigCtrl",
                                                    &gtk_rig_ctrl_info,
                                                     0);
    }

    return gtk_rig_ctrl_type;
}


static void
gtk_rig_ctrl_class_init (GtkRigCtrlClass *class)
{
    GObjectClass      *gobject_class;
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    gobject_class   = G_OBJECT_CLASS (class);
    object_class    = (GtkObjectClass*) class;
    widget_class    = (GtkWidgetClass*) class;
    container_class = (GtkContainerClass*) class;

    parent_class = g_type_class_peek_parent (class);

    object_class->destroy = gtk_rig_ctrl_destroy;
 
}



static void
gtk_rig_ctrl_init (GtkRigCtrl *ctrl)
{
    ctrl->sats = NULL;
    ctrl->target = NULL;
    ctrl->pass = NULL;
    ctrl->qth = NULL;
    ctrl->conf = NULL;
    ctrl->trsp = NULL;
    ctrl->trsplist = NULL;
    ctrl->tracking = FALSE;
    ctrl->busy = FALSE;
    ctrl->engaged = FALSE;
    ctrl->delay = 1000;
    ctrl->timerid = 0;
    ctrl->errcnt = 0;
    ctrl->lastrxf = 0.0;
    ctrl->lasttxf = 0.0;
}

static void
gtk_rig_ctrl_destroy (GtkObject *object)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (object);
    
    /* stop timer */
    if (ctrl->timerid > 0) 
        g_source_remove (ctrl->timerid);

    /* free configuration */
    if (ctrl->conf != NULL) {
        g_free (ctrl->conf->name);
        g_free (ctrl->conf->host);
        g_free (ctrl->conf);
        ctrl->conf = NULL;
    }
    
    /* free transponder */
    if (ctrl->trsplist != NULL) {
        free_transponders (ctrl->trsplist);
        ctrl->trsplist = NULL;   /* destroy might is called twice (?) so we need to NULL it */
    }

    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



/** \brief Create a new rig control widget.
 * \return A new rig control window.
 * 
 */
GtkWidget *
gtk_rig_ctrl_new (GtkSatModule *module)
{
    GtkWidget *widget;
    GtkWidget *table;
    
    /* check that we have rig conf */
    if (!have_conf()) {
        return NULL;
    }

    widget = g_object_new (GTK_TYPE_RIG_CTRL, NULL);
    
    /* store satellites */
    g_hash_table_foreach (module->satellites, store_sats, widget);
    
    GTK_RIG_CTRL (widget)->target = SAT (g_slist_nth_data (GTK_RIG_CTRL (widget)->sats, 0));
    
    /* store QTH */
    GTK_RIG_CTRL (widget)->qth = module->qth;
    
    /* get next pass for target satellite */
    GTK_RIG_CTRL (widget)->pass = get_next_pass (GTK_RIG_CTRL (widget)->target,
                                                 GTK_RIG_CTRL (widget)->qth,
                                                 3.0);
    
    /* initialise custom colors */
    gdk_rgb_find_color (gtk_widget_get_colormap (widget), &ColBlack);
    gdk_rgb_find_color (gtk_widget_get_colormap (widget), &ColWhite);
    gdk_rgb_find_color (gtk_widget_get_colormap (widget), &ColRed);
    gdk_rgb_find_color (gtk_widget_get_colormap (widget), &ColGreen);

    /* create contents */
    table = gtk_table_new (3, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
    gtk_container_set_border_width (GTK_CONTAINER (table), 10);
    gtk_table_attach (GTK_TABLE (table), create_downlink_widgets (GTK_RIG_CTRL (widget)),
                      0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (table), create_uplink_widgets (GTK_RIG_CTRL (widget)),
                      1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (table), create_target_widgets (GTK_RIG_CTRL (widget)),
                      0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (table), create_conf_widgets (GTK_RIG_CTRL (widget)),
                      1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (table), create_count_down_widgets (GTK_RIG_CTRL (widget)),
                      0, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

    gtk_container_add (GTK_CONTAINER (widget), table);
    
    GTK_RIG_CTRL (widget)->timerid = g_timeout_add (GTK_RIG_CTRL (widget)->delay,
                                                    rig_ctrl_timeout_cb,
                                                    GTK_RIG_CTRL (widget));
    
	return widget;
}


/** \brief Update rig control state.
 * \param ctrl Pointer to the GtkRigCtrl.
 * 
 * This function is called by the parent, i.e. GtkSatModule, indicating that
 * the satellite data has been updated. The function updates the internal state
 * of the controller and the rigator.
 */
void
gtk_rig_ctrl_update   (GtkRigCtrl *ctrl, gdouble t)
{
    gdouble satfreq, doppler;
    gchar *buff;
    
    if (ctrl->target) {
        
        /* update Az/El */
        buff = g_strdup_printf (AZEL_FMTSTR, ctrl->target->az);
        gtk_label_set_text (GTK_LABEL (ctrl->SatAz), buff);
        g_free (buff);
        buff = g_strdup_printf (AZEL_FMTSTR, ctrl->target->el);
        gtk_label_set_text (GTK_LABEL (ctrl->SatEl), buff);
        g_free (buff);
        
        update_count_down (ctrl, t);
        
        /* update range */
        if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_IMPERIAL)) {
            buff = g_strdup_printf ("%.0f mi", KM_TO_MI (ctrl->target->range));
        }
        else {
            buff = g_strdup_printf ("%.0f km", ctrl->target->range);
        }
        gtk_label_set_text (GTK_LABEL (ctrl->SatRng), buff);
        g_free (buff);
        
        /* update range rate */
        if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_IMPERIAL)) {
            buff = g_strdup_printf ("%.3f mi/s", KM_TO_MI (ctrl->target->range_rate));
        }
        else {
            buff = g_strdup_printf ("%.3f km/s", ctrl->target->range_rate);
        }
        gtk_label_set_text (GTK_LABEL (ctrl->SatRngRate), buff);
        g_free (buff);
        
        /* Doppler shift down */
        satfreq = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->SatFreqDown));
        doppler = -satfreq * (ctrl->target->range_rate / 299792.4580); // Hz
        buff = g_strdup_printf ("%.0f Hz", doppler);
        gtk_label_set_text (GTK_LABEL (ctrl->SatDopDown), buff);
        g_free (buff);
        
        /* Doppler shift up */
        satfreq = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->SatFreqUp));
        doppler = -satfreq * (ctrl->target->range_rate / 299792.4580); // Hz
        buff = g_strdup_printf ("%.0f Hz", doppler);
        gtk_label_set_text (GTK_LABEL (ctrl->SatDopUp), buff);
        g_free (buff);

        /* update next pass if necessary */
        if (ctrl->pass != NULL) {
            if (ctrl->target->aos > ctrl->pass->aos) {
                /* update pass */
                free_pass (ctrl->pass);
                ctrl->pass = get_next_pass (ctrl->target, ctrl->qth, 3.0);
            }
        }
        else {
            /* we don't have any current pass; store the current one */
            ctrl->pass = get_next_pass (ctrl->target, ctrl->qth, 3.0);
        }
    }
}


/** \brief Create freq control widgets for downlink.
 * \param ctrl Pointer to the GtkRigCtrl widget.
 * 
 * This function creates and initialises the widgets for controlling the
 * downlink frequency. It consists of a controller widget showing the
 * satellite frequency with the radio frequency below it.
 * 
 */
static
GtkWidget *create_downlink_widgets (GtkRigCtrl *ctrl)
{
    GtkWidget   *frame;
    GtkWidget   *vbox;
    GtkWidget   *hbox1,*hbox2;
    GtkWidget   *label;
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b> Downlink </b>"));
    frame = gtk_frame_new (NULL);
    gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    
    vbox = gtk_vbox_new (FALSE, 5);
    hbox1 = gtk_hbox_new (FALSE, 5);
    hbox2 = gtk_hbox_new (FALSE, 5);
    
    /* satellite downlink frequency */
    ctrl->SatFreqDown = gtk_freq_knob_new (145890000.0, TRUE);
    gtk_box_pack_start_defaults (GTK_BOX (vbox), ctrl->SatFreqDown);
    
    /* Downlink doppler */
    label = gtk_label_new (_("Doppler:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    ctrl->SatDopDown = gtk_label_new ("---- Hz");
    gtk_misc_set_alignment (GTK_MISC (ctrl->SatDopDown), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox1), ctrl->SatDopDown, FALSE, TRUE, 0);
    
    /* Downconverter LO */
    ctrl->LoDown = gtk_label_new ("0 MHz");
    gtk_misc_set_alignment (GTK_MISC (ctrl->LoDown), 1.0, 0.5);
    gtk_box_pack_end (GTK_BOX (hbox1), ctrl->LoDown, FALSE, FALSE, 2);
    label = gtk_label_new (_("LO:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_end (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    
    /* Radio downlink frequency */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label),"<span size='large'><b>Radio:</b></span>");
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_start_defaults (GTK_BOX (hbox2), label); 
    ctrl->RigFreqDown = gtk_freq_knob_new (145890000.0, FALSE);
    gtk_box_pack_start_defaults (GTK_BOX (hbox2), ctrl->RigFreqDown);
    
    /* finish packing ... */
    gtk_box_pack_start_defaults (GTK_BOX (vbox), hbox1);
    gtk_box_pack_start_defaults (GTK_BOX (vbox), hbox2);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    
    return frame;
}


/** \brief Create uplink frequency display widgets.
 * \param ctrl Pointer to the GtkRigCtrl widget.
 * 
 * This function creates and initialises the widgets for displaying the
 * uplink frequency of the satellite and the radio.
 */
static
GtkWidget *create_uplink_widgets (GtkRigCtrl *ctrl)
{
    GtkWidget   *frame;
    GtkWidget   *vbox;
    GtkWidget   *hbox1,*hbox2;
    GtkWidget   *label;
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<b> Uplink </b>"));
    frame = gtk_frame_new (NULL);
    gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    
    vbox = gtk_vbox_new (FALSE, 5);
    hbox1 = gtk_hbox_new (FALSE, 5);
    hbox2 = gtk_hbox_new (FALSE, 5);
    
    /* satellite uplink frequency */
    ctrl->SatFreqUp = gtk_freq_knob_new (145890000.0, TRUE);
    gtk_box_pack_start_defaults (GTK_BOX (vbox), ctrl->SatFreqUp);
    
    /* Uplink doppler */
    label = gtk_label_new (_("Doppler:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    ctrl->SatDopUp = gtk_label_new ("---- Hz");
    gtk_misc_set_alignment (GTK_MISC (ctrl->SatDopUp), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox1), ctrl->SatDopUp, FALSE, TRUE, 0);
    
    /* Upconverter LO */
    ctrl->LoUp = gtk_label_new ("0 MHz");
    gtk_misc_set_alignment (GTK_MISC (ctrl->LoUp), 1.0, 0.5);
    gtk_box_pack_end (GTK_BOX (hbox1), ctrl->LoUp, FALSE, FALSE, 2);
    label = gtk_label_new (_("LO:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_end (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    
    /* Radio uplink frequency */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label),"<span size='large'><b>Radio:</b></span>");
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_start_defaults (GTK_BOX (hbox2), label); 
    ctrl->RigFreqUp = gtk_freq_knob_new (145890000.0, FALSE);
    gtk_box_pack_start_defaults (GTK_BOX (hbox2), ctrl->RigFreqUp);
    
   
    gtk_box_pack_start_defaults (GTK_BOX (vbox), hbox1);
    gtk_box_pack_start_defaults (GTK_BOX (vbox), hbox2);
    gtk_container_add (GTK_CONTAINER (frame), vbox);


    return frame;
}

/** \brief Create target widgets.
 * \param ctrl Pointer to the GtkRigCtrl widget.
 */
static
GtkWidget *create_target_widgets (GtkRigCtrl *ctrl)
{
    GtkWidget *frame,*table,*label,*satsel,*track;
    gchar *buff;
    guint i, n;
    sat_t *sat = NULL;
    

    buff = g_strdup_printf (AZEL_FMTSTR, 0.0);
    
    table = gtk_table_new (4, 4, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);

    /* sat selector */
    satsel = gtk_combo_box_new_text ();
    n = g_slist_length (ctrl->sats);
    for (i = 0; i < n; i++) {
        sat = SAT (g_slist_nth_data (ctrl->sats, i));
        if (sat) {
            gtk_combo_box_append_text (GTK_COMBO_BOX (satsel), sat->tle.sat_name);
        }
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (satsel), 0);
    gtk_widget_set_tooltip_text (satsel, _("Select target object"));
    g_signal_connect (satsel, "changed", G_CALLBACK (sat_selected_cb), ctrl);
    gtk_table_attach_defaults (GTK_TABLE (table), satsel, 0, 3, 0, 1);
    
    /* tracking button */
    track = gtk_toggle_button_new_with_label (_("Track"));
    gtk_widget_set_tooltip_text (track, _("Track the satellite when it is within range"));
    gtk_table_attach_defaults (GTK_TABLE (table), track, 3, 4, 0, 1);
    g_signal_connect (track, "toggled", G_CALLBACK (track_toggle_cb), ctrl);
    
    /* Transponder selector */
    ctrl->TrspSel = gtk_combo_box_new_text ();
    gtk_widget_set_tooltip_text (ctrl->TrspSel, _("Select a transponder"));
    load_trsp_list (ctrl);
    //gtk_combo_box_set_active (GTK_COMBO_BOX (ctrl->TrspSel), 0);
    g_signal_connect (ctrl->TrspSel, "changed", G_CALLBACK (trsp_selected_cb), ctrl);
    gtk_table_attach_defaults (GTK_TABLE (table), ctrl->TrspSel, 0, 3, 1, 2);
    
    /* Azimuth */
    label = gtk_label_new (_("Az:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
    ctrl->SatAz = gtk_label_new (buff);
    gtk_misc_set_alignment (GTK_MISC (ctrl->SatAz), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), ctrl->SatAz, 1, 2, 2, 3);
    
    /* Elevation */
    label = gtk_label_new (_("El:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
    ctrl->SatEl = gtk_label_new (buff);
    gtk_misc_set_alignment (GTK_MISC (ctrl->SatEl), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), ctrl->SatEl, 1, 2, 3, 4);
    
    /* Range */
    label = gtk_label_new (_(" Range:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 2, 3);
    ctrl->SatRng = gtk_label_new ("0 km");
    gtk_misc_set_alignment (GTK_MISC (ctrl->SatRng), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), ctrl->SatRng, 3, 4, 2, 3);
    
    /* Range rate */
    label = gtk_label_new (_(" Rate:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 3, 4);
    ctrl->SatRngRate = gtk_label_new ("0.0 km/s");
    gtk_misc_set_alignment (GTK_MISC (ctrl->SatRngRate), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), ctrl->SatRngRate, 3, 4, 3, 4);
    
    frame = gtk_frame_new (_("Target"));

    gtk_container_add (GTK_CONTAINER (frame), table);
    
    g_free (buff);
    
    return frame;
}


static GtkWidget *
create_conf_widgets (GtkRigCtrl *ctrl)
{
    GtkWidget *frame,*table,*label,*timer;
    GDir        *dir = NULL;   /* directory handle */
    GError      *error = NULL; /* error flag and info */
    gchar       *cfgdir;
    gchar       *dirname;      /* directory name */
    gchar      **vbuff;
    const gchar *filename;     /* file name */

    
    
    table = gtk_table_new (3, 3, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    
    
    label = gtk_label_new (_("Device:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    
    ctrl->DevSel = gtk_combo_box_new_text ();
    gtk_widget_set_tooltip_text (ctrl->DevSel, _("Select radio device"));
    
    /* open configuration directory */
    cfgdir = get_conf_dir ();
    dirname = g_strconcat (cfgdir, G_DIR_SEPARATOR_S,
                           "hwconf", NULL);
    g_free (cfgdir);
    
    dir = g_dir_open (dirname, 0, &error);
    if (dir) {
        /* read each .rig file */
        while ((filename = g_dir_read_name (dir))) {
            
            if (g_strrstr (filename, ".rig")) {
                
                vbuff = g_strsplit (filename, ".rig", 0);
                gtk_combo_box_append_text (GTK_COMBO_BOX (ctrl->DevSel), vbuff[0]);
                g_strfreev (vbuff);
            }
        }
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to open hwconf dir (%s)"),
                       __FILE__, __LINE__, error->message);
        g_clear_error (&error);
    }

    g_free (dirname);
    g_dir_close (dir);

    gtk_combo_box_set_active (GTK_COMBO_BOX (ctrl->DevSel), 0);
    g_signal_connect (ctrl->DevSel, "changed", G_CALLBACK (rig_selected_cb), ctrl);
    gtk_table_attach_defaults (GTK_TABLE (table), ctrl->DevSel, 1, 2, 0, 1);
    /* config will be force-loaded after LO spin is created */

    /* Engage button */
    ctrl->LockBut = gtk_toggle_button_new_with_label (_("Engage"));
    gtk_widget_set_tooltip_text (ctrl->LockBut, _("Engage the selcted radio device"));
    g_signal_connect (ctrl->LockBut, "toggled", G_CALLBACK (rig_locked_cb), ctrl);
    gtk_table_attach_defaults (GTK_TABLE (table), ctrl->LockBut, 2, 3, 0, 1);
    
    /* Now, load config*/
    rig_selected_cb (GTK_COMBO_BOX (ctrl->DevSel), ctrl);
    
    /* Timeout */
    label = gtk_label_new (_("Cycle:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
    
    timer = gtk_spin_button_new_with_range (100, 5000, 10);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (timer), 0);
    gtk_widget_set_tooltip_text (timer,
                                 _("This parameter controls the delay between "\
                                   "commands sent to the rigator."));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (timer), ctrl->delay);
    g_signal_connect (timer, "value-changed", G_CALLBACK (delay_changed_cb), ctrl);
    gtk_table_attach (GTK_TABLE (table), timer, 1, 2, 1, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    
    label = gtk_label_new (_("msec"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 1, 2);

    
    frame = gtk_frame_new (_("Settings"));
    gtk_container_add (GTK_CONTAINER (frame), table);
    
    return frame;
}


/** \brief Create count down widget */
static GtkWidget *
create_count_down_widgets (GtkRigCtrl *ctrl)
{
    GtkWidget *frame;
    
    /* create delta-t label */
    ctrl->SatCnt = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (ctrl->SatCnt), 0.5, 0.5);
    gtk_label_set_markup (GTK_LABEL (ctrl->SatCnt),
                          _("<span size='large'><b>\316\224T: 00:00:00</b></span>"));
    
    frame = gtk_frame_new (NULL);
    gtk_container_add (GTK_CONTAINER (frame), ctrl->SatCnt);
    
    return frame;
}



/** \brief Copy satellite from hash table to singly linked list.
 */
static void
store_sats (gpointer key, gpointer value, gpointer user_data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL( user_data);
    sat_t        *sat = SAT (value);

    ctrl->sats = g_slist_append (ctrl->sats, sat);
}


/** \brief Manage satellite selections
 * \param satsel Pointer to the GtkComboBox.
 * \param data Pointer to the GtkRigCtrl widget.
 * 
 * This function is called when the user selects a new satellite.
 */
static void
sat_selected_cb (GtkComboBox *satsel, gpointer data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (data);
    GSList *trsplist = NULL;
    trsp_t *trsp;
    gint i,n;
    
    i = gtk_combo_box_get_active (satsel);
    if (i >= 0) {
        ctrl->target = SAT (g_slist_nth_data (ctrl->sats, i));
        
        /* update next pass */
        if (ctrl->pass != NULL)
            free_pass (ctrl->pass);
        ctrl->pass = get_next_pass (ctrl->target, ctrl->qth, 3.0);
        
        /* read transponders for new target */
        load_trsp_list (ctrl);
        
#if 0
        /* get transponders */
        trsplist = read_transponders (ctrl->target->tle.catnr);
        
        n = g_slist_length (trsplist);
        //g_print ("*** %d\n", n);
        for (i = 0; i < n; i++) {
            trsp = (trsp_t *) g_slist_nth_data (trsplist, i);
            g_print ("%s: %.0f/%.0f/%.0f/%.0f/%s\n",
                      trsp->name,
                      trsp->uplow, trsp->uphigh,
                      trsp->downlow, trsp->downhigh,
                      trsp->invert ? "INV" : "NONINV");
            
        }
        
        free_transponders (trsplist);
#endif
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Invalid satellite selection: %d"),
                     __FILE__, __FUNCTION__, i);
        
        /* clear pass just in case... */
        if (ctrl->pass != NULL) {
            free_pass (ctrl->pass);
            ctrl->pass = NULL;
        }
        
    }
}


/** \brief Manage transponder selections.
 *  \param box Pointer to the transponder selector widget.
 *  \param data Pointer to the GtkRigCtrl structure
 *
 * This function is called when the user selects a new transponder.
 * It updates ctrl->trsp with the new selection.
 */
static void trsp_selected_cb (GtkComboBox *box, gpointer data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (data);
    gint i, n;
    
    i = gtk_combo_box_get_active (box) - 1; /* 0th element is "Transponder" */
    n = g_slist_length (ctrl->trsplist);
    
    if (i == -1) {
        /* clear transponder data */
        ctrl->trsp = NULL;
    }
    else if (i < n) {
        ctrl->trsp = (trsp_t *) g_slist_nth_data (ctrl->trsplist, i);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                      _("%s: Inconsistency detected in internal transponder data (%d,%d)"),
                      __FUNCTION__, i, n);
    }
}

/** \brief Manage toggle signals (tracking)
 * \param button Pointer to the GtkToggle button.
 * \param data Pointer to the GtkRigCtrl widget.
 */
static void
track_toggle_cb (GtkToggleButton *button, gpointer data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (data);
    
    ctrl->tracking = gtk_toggle_button_get_active (button);
}


/** \brief Manage cycle delay changes.
 * \param spin Pointer to the spin button.
 * \param data Pointer to the GtkRigCtrl widget.
 * 
 * This function is called when the user changes the value of the
 * cycle delay.
 */
static void
delay_changed_cb (GtkSpinButton *spin, gpointer data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (data);
    
    
    ctrl->delay = (guint) gtk_spin_button_get_value (spin);

    if (ctrl->timerid > 0) 
        g_source_remove (ctrl->timerid);

    ctrl->timerid = g_timeout_add (ctrl->delay, rig_ctrl_timeout_cb, ctrl);
}




/** \brief New rig device selected.
 * \param box Pointer to the rigor selector combo box.
 * \param data Pointer to the GtkRigCtrl widget.
 * 
 * This function is called when the user selects a new rigor controller
 * device.
 */
static void
rig_selected_cb (GtkComboBox *box, gpointer data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (data);
    gchar      *buff;
    
    
    /* free previous configuration */
    if (ctrl->conf != NULL) {
        g_free (ctrl->conf->name);
        g_free (ctrl->conf->host);
        g_free (ctrl->conf);
    }
    
    ctrl->conf = g_try_new (radio_conf_t, 1);
    if (ctrl->conf == NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to allocate memory for radio config"),
                     __FILE__, __LINE__);
        return;
    }
    
    /* load new configuration */
    ctrl->conf->name = gtk_combo_box_get_active_text (box);
    if (radio_conf_read (ctrl->conf)) {
        sat_log_log (SAT_LOG_LEVEL_MSG,
                     _("Loaded new radio configuration %s"),
                     ctrl->conf->name);
        /* update LO widgets */
        buff = g_strdup_printf (_("%.0f MHz"), ctrl->conf->lo/1.0e6);
        gtk_label_set_text (GTK_LABEL (ctrl->LoDown), buff);
        g_free (buff);
        buff = g_strdup_printf (_("%.0f MHz"), ctrl->conf->loup/1.0e6);
        gtk_label_set_text (GTK_LABEL (ctrl->LoUp), buff);
        g_free (buff);

        //gtk_spin_button_set_value (GTK_SPIN_BUTTON (ctrl->LO), ctrl->conf->lo/1.0e6);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to load radio configuration %s"),
                     __FILE__, __LINE__, ctrl->conf->name);

        g_free (ctrl->conf->name);
        if (ctrl->conf->host)
            g_free (ctrl->conf->host);
        g_free (ctrl->conf);
        ctrl->conf = NULL;
    }

}



/** \brief Rig locked.
 * \param button Pointer to the "Engage" button.
 * \param data Pointer to the GtkRigCtrl widget.
 * 
 * This function is called when the user toggles the "Engage" button.
 */
static void
rig_locked_cb (GtkToggleButton *button, gpointer data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (data);

    
    if (!gtk_toggle_button_get_active (button)) {
        /* close socket */
        gtk_widget_set_sensitive (ctrl->DevSel, TRUE);
        ctrl->engaged = FALSE;
        ctrl->lasttxf = 0.0;
        ctrl->lastrxf = 0.0;
    }
    else {
        if (ctrl->conf == NULL) {
            /* we don't have a working configuration */
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                         _("%s: Controller does not have a valid configuration"),
                           __FUNCTION__);
            return;
        }
        
        gtk_widget_set_sensitive (ctrl->DevSel, FALSE);
        ctrl->engaged = TRUE;
        ctrl->wrops = 0;
        
        /* set initial frequency */

        switch (ctrl->conf->type) {
            case RIG_TYPE_RX:
                ctrl->lastrxf = gtk_freq_knob_get_value (GTK_FREQ_KNOB(ctrl->RigFreqDown));
                set_freq_simplex (ctrl, ctrl->lastrxf);
                break;
            case RIG_TYPE_TX:
                ctrl->lasttxf = gtk_freq_knob_get_value (GTK_FREQ_KNOB(ctrl->RigFreqUp));
                set_freq_simplex (ctrl, ctrl->lasttxf);
                break;
            case RIG_TYPE_TRX:
            case RIG_TYPE_DUPLEX:
                if (get_ptt (ctrl)) {
                    ctrl->lasttxf = gtk_freq_knob_get_value (GTK_FREQ_KNOB(ctrl->RigFreqUp));
                    set_freq_simplex (ctrl, ctrl->lasttxf);
                }
                else {
                    ctrl->lastrxf = gtk_freq_knob_get_value (GTK_FREQ_KNOB(ctrl->RigFreqDown));
                    set_freq_simplex (ctrl, ctrl->lastrxf);
                }
                break;
            default:
                /* this is an error! */
                ctrl->conf->type = RIG_TYPE_RX;
                ctrl->lastrxf = gtk_freq_knob_get_value (GTK_FREQ_KNOB(ctrl->RigFreqDown));
                set_freq_simplex (ctrl, ctrl->lastrxf);
                break;
        }
    }
}


/** \brief Rigator controller timeout function
 * \param data Pointer to the GtkRigCtrl widget.
 * \return Always TRUE to let the timer continue.
 */
static gboolean
rig_ctrl_timeout_cb (gpointer data)
{
    GtkRigCtrl *ctrl = GTK_RIG_CTRL (data);
    
    
    if (ctrl->busy) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,_("%s missed the deadline"),__FUNCTION__);
        return TRUE;
    }
    
    ctrl->busy = TRUE;
    
    /* Execute controller cycle depending on radio type */
    switch (ctrl->conf->type) {
        
        case RIG_TYPE_RX:
            exec_rx_cycle (ctrl);
            break;
            
        case RIG_TYPE_TX:
            exec_tx_cycle (ctrl);
            break;
            
        case RIG_TYPE_TRX:
            exec_trx_cycle (ctrl);
            break;
            
        case RIG_TYPE_DUPLEX:
            exec_duplex_cycle (ctrl);
            break;
            
        default:
            /* invalid mode */
            sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s: Invalid radio type %d. Setting type to RIG_TYPE_RX"),
                          __FUNCTION__, ctrl->conf->type);
            ctrl->conf->type = RIG_TYPE_RX;
            
    }
    
    //g_print ("       WROPS = %d\n", ctrl->wrops);
    
    ctrl->busy = FALSE;
    
    return TRUE;
}


/** \brief Execute RX mode cycle.
 *  \param ctrl Pointer to the GtkRigCtrl widget.
 *
 * This function executes a controller cycle when the device is of RIG_TYPE_RX.
 */
static void exec_rx_cycle (GtkRigCtrl *ctrl)
{
    gdouble readfreq=0.0, tmpfreq, satfreqd, satfrequ, doppler;
    gboolean ptt = FALSE;
    gboolean dialchanged = FALSE;
    
    
    /* Dial feedback:
       If radio device is engaged read frequency from radio and compare it to the
       last set frequency. If different, it means that user has changed frequency
       on the radio dial => update transponder knob
    */
    if (ctrl->engaged) {
        // This is no good because it gets out of sync while PTT = ON
        //lastfreq = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->RigFreqDown));
        
        /* check whether PTT is ON */
        ptt = ctrl->conf->ptt ? get_ptt (ctrl) : FALSE;
        
        if (ptt == FALSE) {
            if (!get_freq_simplex (ctrl, &readfreq)) {
                /* error => use a passive value */
                readfreq = ctrl->lastrxf;
                ctrl->errcnt++;
            }
        }
        else {
            readfreq = ctrl->lastrxf;
        }
        
        if (fabs (readfreq - ctrl->lastrxf) > 0.99) {
            dialchanged = TRUE;
            
            /* user might have altered radio frequency => update transponder knob */
            gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqDown), readfreq);
            
            /* doppler shift; only if we are tracking */
            if (ctrl->tracking) {
                satfreqd = (readfreq + ctrl->conf->lo) /
                           (1 - (ctrl->target->range_rate/299792.4580));
            }
            else {
                satfreqd = readfreq + ctrl->conf->lo;
            }
            gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->SatFreqDown), satfreqd);
            
            /* TODO Update uplink if locked to downlink */
        }
    }

    /* TODO: Follow with uplink if transponder is locked */

    /* now, forward tracking */
    if (dialchanged) {
        /* no need to forward track */
        return;
    }
    
    /* If we are tracking, calculate the radio freq by applying both dopper shift
       and tranverter LO frequency. If we are not tracking, apply only LO frequency.
    */
    satfreqd = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->SatFreqDown));
    satfrequ = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->SatFreqUp));
    if (ctrl->tracking) {
        /* downlink */
        doppler = -satfreqd * (ctrl->target->range_rate / 299792.4580);
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqDown),
                                  satfreqd + doppler - ctrl->conf->lo);
        /* uplink */
        doppler = -satfrequ * (ctrl->target->range_rate / 299792.4580);
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqUp),
                                  satfrequ + doppler - ctrl->conf->lo);
    }
    else {
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqDown),
                                  satfreqd - ctrl->conf->lo);
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqUp),
                                  satfrequ - ctrl->conf->lo);
    }

    tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqDown));

    /* if device is engaged, send freq command to radio */
    if ((ctrl->engaged) && (ptt == FALSE) && (fabs(ctrl->lastrxf - tmpfreq) > 0.99)) {
        if (set_freq_simplex (ctrl, tmpfreq)) {
            /* reset error counter */
            ctrl->errcnt = 0;
            ctrl->lastrxf = tmpfreq;
        }
        else {
            ctrl->errcnt++;
        }
    }
    
    if (ctrl->errcnt >= MAX_ERROR_COUNT) {
        /* disengage device */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctrl->LockBut), FALSE);
        ctrl->engaged = FALSE;
        ctrl->errcnt = 0;
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                        _("%s: MAX_ERROR_COUNT (%d) reached. Disengaging device!"),
                        __FUNCTION__, MAX_ERROR_COUNT);
        
        //g_print ("ERROR. WROPS = %d\n", ctrl->wrops);
    }
}


/** \brief Execute TX mode cycle.
 *  \param ctrl Pointer to the GtkRigCtrl widget.
 *
 * This function executes a controller cycle when the device is of RIG_TYPE_TX.
 */
static void exec_tx_cycle (GtkRigCtrl *ctrl)
{
    gdouble readfreq=0.0, tmpfreq, satfreqd, satfrequ, doppler;
    gboolean ptt = FALSE;
    gboolean dialchanged = FALSE;
    
    
    /* Dial feedback:
       If radio device is engaged read frequency from radio and compare it to the
       last set frequency. If different, it means that user has changed frequency
       on the radio dial => update transponder knob
    */
    if (ctrl->engaged) {
        // This is no good because it gets out of sync while PTT = ON
        //lastfreq = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->RigFreqDown));
        
        /* check whether PTT is ON */
        ptt = ctrl->conf->ptt ? get_ptt (ctrl) : FALSE;
        
        if (ptt == TRUE) {
            if (!get_freq_simplex (ctrl, &readfreq)) {
                /* error => use a passive value */
                readfreq = ctrl->lasttxf;
                ctrl->errcnt++;
            }
        }
        else {
            readfreq = ctrl->lasttxf;
        }
        
        if (fabs (readfreq - ctrl->lasttxf) > 0.99) {
            dialchanged = TRUE;
            
            /* user might have altered radio frequency => update transponder knob */
            gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqUp), readfreq);
            
            /* doppler shift; only if we are tracking */
            if (ctrl->tracking) {
                satfrequ = (readfreq + ctrl->conf->loup) /
                           (1 - (ctrl->target->range_rate/299792.4580));
            }
            else {
                satfrequ = readfreq + ctrl->conf->loup;
            }
            gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->SatFreqUp), satfrequ);
            
            /* TODO Update downlink if locked to uplink */
        }
    }

    /* TODO: Follow with downlink if transponder is locked */

    /* now, forward tracking */
    if (dialchanged) {
        /* no need to forward track */
        return;
    }
    
    /* If we are tracking, calculate the radio freq by applying both dopper shift
       and tranverter LO frequency. If we are not tracking, apply only LO frequency.
    */
    satfreqd = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->SatFreqDown));
    satfrequ = gtk_freq_knob_get_value (GTK_FREQ_KNOB (ctrl->SatFreqUp));
    if (ctrl->tracking) {
        /* downlink */
        doppler = -satfreqd * (ctrl->target->range_rate / 299792.4580);
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqDown),
                                  satfreqd + doppler - ctrl->conf->lo);
        /* uplink */
        doppler = -satfrequ * (ctrl->target->range_rate / 299792.4580);
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqUp),
                                  satfrequ + doppler - ctrl->conf->lo);
    }
    else {
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqDown),
                                  satfreqd - ctrl->conf->lo);
        gtk_freq_knob_set_value (GTK_FREQ_KNOB (ctrl->RigFreqUp),
                                  satfrequ - ctrl->conf->lo);
    }

    tmpfreq = gtk_freq_knob_get_value(GTK_FREQ_KNOB(ctrl->RigFreqUp));

    /* if device is engaged, send freq command to radio */
    if ((ctrl->engaged) && (ptt == TRUE) && (fabs(ctrl->lasttxf - tmpfreq) > 0.99)) {
        if (set_freq_simplex (ctrl, tmpfreq)) {
            /* reset error counter */
            ctrl->errcnt = 0;
            ctrl->lasttxf = tmpfreq;
        }
        else {
            ctrl->errcnt++;
        }
    }
    
    if (ctrl->errcnt >= MAX_ERROR_COUNT) {
        /* disengage device */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctrl->LockBut), FALSE);
        ctrl->engaged = FALSE;
        ctrl->errcnt = 0;
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                        _("%s: MAX_ERROR_COUNT (%d) reached. Disengaging device!"),
                        __FUNCTION__, MAX_ERROR_COUNT);
        
        //g_print ("ERROR. WROPS = %d\n", ctrl->wrops);
    }

}


/** \brief Execute simplex mode cycle.
 *  \param ctrl Pointer to the GtkRigCtrl widget.
 *
 * This function executes a controller cycle when the device is of RIG_TYPE_TRX (simplex).
 */
static void exec_trx_cycle (GtkRigCtrl *ctrl)
{

}


/** \brief Execute duplex mode cycle.
 *  \param ctrl Pointer to the GtkRigCtrl widget.
 *
 * This function executes a controller cycle when the device is of RIG_TYPE_DUPLEX.
 */
static void exec_duplex_cycle (GtkRigCtrl *ctrl)
{
    //FIXME implement
    exec_trx_cycle (ctrl);
}


/** \brief Get PTT status
 *  \param ctrl Pointer to the GtkRigVtrl widget.
 *  \return TRUE if PTT is ON, FALSE if PTT is OFF or an error occurred.
 *
 */
static gboolean get_ptt (GtkRigCtrl *ctrl)
{
    gchar  *buff,**vbuff;
    gint    written,size;
    gint    status;
    struct hostent *h;
    struct sockaddr_in ServAddr;
    gint  sock;          /*!< Network socket */
    guint64  pttstat = 0;

    
    /* create socket */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to create socket"),
                       __FILE__, __LINE__);
        return FALSE;
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%d Network socket created successfully"),
                       __FILE__, __LINE__);
    }
        
    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family = AF_INET;             /* Internet address family */
    h = gethostbyname(ctrl->conf->host);
    memcpy((char *) &ServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    ServAddr.sin_port = htons(ctrl->conf->port); /* Server port */

    /* establish connection */
    status = connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr));
    if (status < 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to connect to %s:%d"),
                       __FILE__, __LINE__, ctrl->conf->host, ctrl->conf->port);
        return FALSE;
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%d: Connection opened to %s:%d"),
                       __FILE__, __LINE__, ctrl->conf->host, ctrl->conf->port);
    }
    
    /* send command (get_ptt: t) */
    buff = g_strdup_printf ("t");
    
    size = 1;
    written = send(sock, buff, size, 0);
    if (written != size) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: SIZE ERROR %d / %d"),
                       __FILE__, __LINE__, written, size);
    }
    g_free (buff);
    
    
    /* try to read answer */
    buff = g_try_malloc (128);
    if (buff == NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Failed to allocate 128 bytes (yes, this means trouble)"),
                       __FILE__, __FUNCTION__);
        shutdown (sock, SHUT_RDWR);
        close (sock);
        return FALSE;
    }
        
    size = read (sock, buff, 127);
    if (size == 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Got 0 bytes from rigctld"),
                       __FILE__, __FUNCTION__);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%s: Read %d bytes from rigctld"),
                       __FILE__, __FUNCTION__, size);
        
        buff[size] = 0;
        vbuff = g_strsplit (buff, "\n", 3);
        pttstat = g_ascii_strtoull (vbuff[0], NULL, 0);  //FIXME base = 0 ok?
                
        g_free (buff);
        g_strfreev (vbuff);
    }
    
    shutdown (sock, SHUT_RDWR);
    close (sock);


    return (pttstat == 1) ? TRUE : FALSE;

}


/** \brief Set frequency in simplex mode
 * \param ctrl Pointer to the GtkRigCtrl structure.
 * \param freq The new frequency.
 * \return TRUE if the operation was successful, FALSE if a connection error
 *         occurred.
 * 
 * \note freq is not strictly necessary for normal use since we could have
 *       gotten the current frequency from the ctrl; however, the param
 *       might become useful in the future.
 */
static gboolean set_freq_simplex (GtkRigCtrl *ctrl, gdouble freq)
{
    gchar  *buff;
    gint    written,size;
    gint    status;
    struct hostent *h;
    struct sockaddr_in ServAddr;
    gint  sock;          /*!< Network socket */
        
    /* create socket */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Failed to create socket"),
                       __FUNCTION__);
        return FALSE;
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: Network socket created successfully"),
                       __FUNCTION__);
    }
        
    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family = AF_INET;             /* Internet address family */
    h = gethostbyname(ctrl->conf->host);
    memcpy((char *) &ServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    ServAddr.sin_port = htons(ctrl->conf->port); /* Server port */

    /* establish connection */
    status = connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr));
    if (status < 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: Failed to connect to %s:%d"),
                       __FUNCTION__, ctrl->conf->host, ctrl->conf->port);
        return FALSE;
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s: Connection opened to %s:%d"),
                       __FUNCTION__, ctrl->conf->host, ctrl->conf->port);
    }
    
    /* send command */
    buff = g_strdup_printf ("F %10.0f", freq);
    
    size = 12;
    written = send(sock, buff, size, 0);
    if (written != size) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s: SIZE ERROR %d / %d"),
                       __FUNCTION__, written, size);
    }
    g_free (buff);
    shutdown (sock, SHUT_RDWR);
    close (sock);
    
    ctrl->wrops++;
    
    return TRUE;
}


/** \brief Get frequency
 * \param ctrl Pointer to the GtkRigCtrl structure.
 * \param freq The current frequency of the radio.
 * \return TRUE if the operation was successful, FALSE if a connection error
 *         occurred.
 */
static gboolean get_freq_simplex (GtkRigCtrl *ctrl, gdouble *freq)
{
    gchar  *buff,**vbuff;
    gint    written,size;
    gint    status;
    struct hostent *h;
    struct sockaddr_in ServAddr;
    gint  sock;          /*!< Network socket */

                         
    if (freq == NULL) {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s:%d: NULL storage."),
                       __FILE__, __LINE__);
        return FALSE;
    }
    
    /* create socket */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to create socket"),
                       __FILE__, __LINE__);
        return FALSE;
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%d Network socket created successfully"),
                       __FILE__, __LINE__);
    }
        
    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family = AF_INET;             /* Internet address family */
    h = gethostbyname(ctrl->conf->host);
    memcpy((char *) &ServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    ServAddr.sin_port = htons(ctrl->conf->port); /* Server port */

    /* establish connection */
    status = connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr));
    if (status < 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to connect to %s:%d"),
                       __FILE__, __LINE__, ctrl->conf->host, ctrl->conf->port);
        return FALSE;
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%d: Connection opened to %s:%d"),
                       __FILE__, __LINE__, ctrl->conf->host, ctrl->conf->port);
    }
    
    /* send command */
    buff = g_strdup_printf ("f");
    
    size = 1;
    written = send(sock, buff, size, 0);
    if (written != size) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: SIZE ERROR %d / %d"),
                       __FILE__, __LINE__, written, size);
    }
    g_free (buff);
    
    
    /* try to read answer */
    buff = g_try_malloc (128);
    if (buff == NULL) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Failed to allocate 128 bytes (yes, this means trouble)"),
                       __FILE__, __FUNCTION__);
        shutdown (sock, SHUT_RDWR);
        close (sock);
        return FALSE;
    }
        
    size = read (sock, buff, 127);
    if (size == 0) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%s: Got 0 bytes from rigctld"),
                       __FILE__, __FUNCTION__);
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_DEBUG,
                     _("%s:%s: Read %d bytes from rigctld"),
                       __FILE__, __FUNCTION__, size);
        
        buff[size] = 0;
        vbuff = g_strsplit (buff, "\n", 3);
        *freq = g_strtod (vbuff[0], NULL);
                
        g_free (buff);
        g_strfreev (vbuff);
    }
    
    shutdown (sock, SHUT_RDWR);
    close (sock);


    return TRUE;
}


/** \brief Update count down label.
 * \param[in] ctrl Pointer to the RigCtrl widget.
 * \param[in] t The current time.
 * 
 * This function calculates the new time to AOS/LOS of the currently
 * selected target and updates the ctrl->SatCnt label widget.
 */
static void update_count_down (GtkRigCtrl *ctrl, gdouble t)
{
    gdouble  targettime;
    gdouble  delta;
    gchar   *buff;
    guint    h,m,s;
    gchar   *ch,*cm,*cs;
    gchar   *aoslos;

    
    /* select AOS or LOS time depending on target elevation */
    if (ctrl->target->el < 0.0) {
        targettime = ctrl->target->aos;
        aoslos = g_strdup_printf (_("AOS in"));
    }
    else {
        targettime = ctrl->target->los;
        aoslos = g_strdup_printf (_("LOS in"));
    }
    
    delta = targettime - t;
    
    /* convert julian date to seconds */
    s = (guint) (delta * 86400);

    /* extract hours */
    h = (guint) floor (s/3600);
    s -= 3600*h;

    /* leading zero */
    if ((h > 0) && (h < 10))
        ch = g_strdup ("0");
    else
        ch = g_strdup ("");

    /* extract minutes */
    m = (guint) floor (s/60);
    s -= 60*m;

    /* leading zero */
    if (m < 10)
        cm = g_strdup ("0");
    else
        cm = g_strdup ("");

    /* leading zero */
    if (s < 10)
        cs = g_strdup (":0");
    else
        cs = g_strdup (":");

    if (h > 0) 
        buff = g_strdup_printf ("<span size='xx-large'><b>%s %s%d:%s%d%s%d</b></span>",
                                aoslos, ch, h, cm, m, cs, s);
    else
        buff = g_strdup_printf ("<span size='xx-large'><b>%s %s%d%s%d</b></span>",
                                aoslos, cm, m, cs, s);

    gtk_label_set_markup (GTK_LABEL (ctrl->SatCnt), buff);

    g_free (buff);
    g_free (aoslos);
    g_free (ch);
    g_free (cm);
    g_free (cs);

}


/** \brief Load the transponder list for the target satellite.
 *  \param ctrl Pointer to the GtkRigCtrl structure.
 *
 * This function loads the transponder list for the currently selected
 * satellite. The transponder list is loaded into ctrl->trsplist and the
 * transponder names are added to the ctrl->TrspSel combo box. If any of
 * these already contain data, it is cleared. The combo box is also cleared
 * if there are no transponders for the current target, or if there is no
 * target.
 */
static void load_trsp_list (GtkRigCtrl *ctrl)
{
    trsp_t *trsp = NULL;
    guint i,n;
    
    if (ctrl->trsplist != NULL) {
        /* clear combo box */
        n = g_slist_length (ctrl->trsplist);
        for (i = 0; i < n; i++) {
            gtk_combo_box_remove_text (GTK_COMBO_BOX (ctrl->TrspSel), 0);
        }
        
        /* clear transponder list */
        free_transponders (ctrl->trsplist);
    }

    /* check if there is a target satellite */
    if G_UNLIKELY (ctrl->target == NULL) {
        sat_log_log (SAT_LOG_LEVEL_MSG,
                      _("%s:%s: GtkSatModule has no target satellite."),
                      __FILE__, __FUNCTION__);
        return;
    }

    /* read transponders for new target */
    ctrl->trsplist = read_transponders (ctrl->target->tle.catnr);
    
    /* append transponder names to combo box */
    n = g_slist_length (ctrl->trsplist);
    
    if (n == 0)
        return;
    
    for (i = 0; i < n; i++) {
        trsp = (trsp_t *) g_slist_nth_data (ctrl->trsplist, i);
        gtk_combo_box_append_text (GTK_COMBO_BOX (ctrl->TrspSel), trsp->name);
    }
    
    /* make an initial selection */
    ctrl->trsp = (trsp_t *) g_slist_nth_data (ctrl->trsplist, 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (ctrl->TrspSel), 0);
}


/** \brief Check that we have at least one .rig file */
static gboolean have_conf ()
{
    GDir        *dir = NULL;   /* directory handle */
    GError      *error = NULL; /* error flag and info */
    gchar       *cfgdir;
    gchar       *dirname;      /* directory name */
    const gchar *filename;     /* file name */
    gint         i = 0;

    
    /* open configuration directory */
    cfgdir = get_conf_dir ();
    dirname = g_strconcat (cfgdir, G_DIR_SEPARATOR_S,
                           "hwconf", NULL);
    g_free (cfgdir);
    
    dir = g_dir_open (dirname, 0, &error);
    if (dir) {
        /* read each .rig file */
        while ((filename = g_dir_read_name (dir))) {
            
            if (g_strrstr (filename, ".rig")) {
                i++;
            }
        }
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     _("%s:%d: Failed to open hwconf dir (%s)"),
                       __FILE__, __LINE__, error->message);
        g_clear_error (&error);
    }

    g_free (dirname);
    g_dir_close (dir);
    
    return (i > 0) ? TRUE : FALSE;
}
