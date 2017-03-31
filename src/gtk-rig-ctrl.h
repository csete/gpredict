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
#ifndef __GTK_RIG_CTRL_H__
#define __GTK_RIG_CTRL_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtk-sat-module.h"
#include "predict-tools.h"
#include "radio-conf.h"
#include "sgpsdp/sgp4sdp4.h"
#include "trsp-conf.h"


#define GTK_TYPE_RIG_CTRL          (gtk_rig_ctrl_get_type ())
#define GTK_RIG_CTRL(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                   gtk_rig_ctrl_get_type (),\
                                   GtkRigCtrl)

#define GTK_RIG_CTRL_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                   gtk_rig_ctrl_get_type (),\
                                   GtkRigCtrlClass)

#define IS_GTK_RIG_CTRL(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_rig_ctrl_get_type ())

typedef struct _gtk_rig_ctrl GtkRigCtrl;
typedef struct _GtkRigCtrlClass GtkRigCtrlClass;

struct _gtk_rig_ctrl {
    GtkVBox         vbox;

    GtkWidget      *SatFreqDown;
    GtkWidget      *RigFreqDown;
    GtkWidget      *SatFreqUp;
    GtkWidget      *RigFreqUp;
    GtkWidget      *SatDopDown; /*!< Doppler shift down */
    GtkWidget      *SatDopUp;   /*!< Doppler shift up */
    GtkWidget      *LoDown;     /*!< LO of downconverter */
    GtkWidget      *LoUp;       /*!z LO of upconverter */

    /* target status labels */
    GtkWidget      *SatAz, *SatEl, *SatCnt;
    GtkWidget      *SatRng, *SatRngRate, *SatDop;

    /* other widgets */
    GtkWidget      *SatSel;     /*!< Satellite selector */
    GtkWidget      *TrspSel;    /*!< Transponder selector */
    GtkWidget      *DevSel;     /*!< Device selector */
    GtkWidget      *DevSel2;    /*!< Second device selector */
    GtkWidget      *LockBut;

    radio_conf_t   *conf;       /*!< Radio configuration */
    radio_conf_t   *conf2;      /*!< Secondary radio configuration */
    GSList         *trsplist;   /*!< List of available transponders */
    trsp_t         *trsp;       /*!< Pointer to the current transponder configuration */
    gboolean        trsplock;   /*!< Flag indicating whether uplink and downlink are lockled */

    GSList         *sats;       /*!< List of sats in parent module */
    sat_t          *target;     /*!< Target satellite */
    pass_t         *pass;       /*!< Next pass of target satellite */
    qth_t          *qth;        /*!< The QTH for this module */

    double          prev_ele;   /*!< Previous elevation (used for AOS/LOS signalling) */

    guint           delay;      /*!< Timeout delay. */
    guint           timerid;    /*!< Timer ID */

    gboolean        tracking;   /*!< Flag set when we are tracking a target. */
    GMutex          busy;       /*!< Flag set when control algorithm is busy. */
    gboolean        engaged;    /*!< Flag indicating that rig device is engaged. */
    gint            errcnt;     /*!< Error counter. */

    gdouble         lastrxf;    /*!< Last frequency sent to receiver. */
    gdouble         lasttxf;    /*!< Last frequency sent to tranmitter. */
    gdouble         du, dd;     /*!< Last computed up/down Doppler shift; computed in update() */

    glong           last_toggle_tx;     /*!< Last time when exec_toggle_tx_cycle() was executed (seconds)
                                           -1 indicates that an update should be performed ASAP */

    gint            sock, sock2;        /*!< Sockets for controlling the radio(s). */

    /* debug related */
    guint           wrops;
    guint           rdops;

    /* DL4PD */
    /* threads related stuff */
    /* add mutexes etc, to make threads reentrant! */
    GMutex          writelock;  /*!< Mutex for blocking write operation */
    GMutex          rig_ctrl_updatelock;        /*!< Mutex wile updating widgets etc */
    GMutex          widgetsync; /*!< Mutex used while leaving (sync stuff) */
    GCond           widgetready;        /*!< Condition when work is done (sync stuff) */
    GAsyncQueue    *rigctlq;    /*!< Message queue to indicate something has changed */
    GThread        *rigctl_thread;      /*!< Pointer to current rigctl-thread */
};

struct _GtkRigCtrlClass {
    GtkVBoxClass    parent_class;
};

GType           gtk_rig_ctrl_get_type(void);
GtkWidget      *gtk_rig_ctrl_new(GtkSatModule * module);
void            gtk_rig_ctrl_update(GtkRigCtrl * ctrl, gdouble t);
void            gtk_rig_ctrl_select_sat(GtkRigCtrl * ctrl, gint catnum);

#endif /* __GTK_RIG_CTRL_H__ */
