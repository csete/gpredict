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
#ifndef __GTK_ROT_CTRL_H__
#define __GTK_ROT_CTRL_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-sat-module.h"
#include "rotor-conf.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




#define GTK_TYPE_ROT_CTRL          (gtk_rot_ctrl_get_type ())
#define GTK_ROT_CTRL(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                   gtk_rot_ctrl_get_type (),\
                                   GtkRotCtrl)

#define GTK_ROT_CTRL_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                             gtk_rot_ctrl_get_type (),\
                             GtkRotCtrlClass)

#define IS_GTK_ROT_CTRL(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_rot_ctrl_get_type ())


typedef struct _gtk_rot_ctrl      GtkRotCtrl;
typedef struct _GtkRotCtrlClass   GtkRotCtrlClass;



struct _gtk_rot_ctrl
{
    GtkVBox vbox;
    
    /* Azimuth widgets */
    GtkWidget *AzSat,*AzSet,*AzRead,*AzDevSel;
    
    /* Elevation widgets */
    GtkWidget *ElSat,*ElSet,*ElRead,*ElDevSel,*ElDev;
    
    /* other widgets */
    GtkWidget *SatCnt;
    GtkWidget *DevSel;
    GtkWidget *plot;    /*!< Polar plot widget */
    GtkWidget *LockBut;
                        
    rotor_conf_t *conf;
    gdouble       t;  /*!< Time when sat data last has been updated. */
    
    /* satellites */
    GSList *sats;       /*!< List of sats in parent module */
    sat_t  *target;     /*!< Target satellite */
    pass_t *pass;       /*!< Next pass of target satellite */
    qth_t  *qth;        /*!< The QTH for this module */
    gboolean flipped;   /*!< Whether the current pass loaded is a flip pass or not */

    guint delay;       /*!< Timeout delay. */
    guint timerid;     /*!< Timer ID */
    gdouble tolerance;  /*!< Error tolerance */
    
    gboolean tracking;  /*!< Flag set when we are tracking a target. */
    GMutex   busy;      /*!< Flag set when control algorithm is busy. */
    gboolean engaged;   /*!< Flag indicating that rotor device is engaged. */
                        
    gint     errcnt;    /*!< Error counter. */
    gint     sock;      /*!< socket for connecting to rotctld. */
    
    /* debug related */
    guint    wrops;
    guint    rdops;
};

struct _GtkRotCtrlClass
{
    GtkVBoxClass parent_class;
};



GType      gtk_rot_ctrl_get_type (void);
GtkWidget* gtk_rot_ctrl_new      (GtkSatModule *module);
void       gtk_rot_ctrl_update   (GtkRotCtrl *ctrl, gdouble t);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_ROT_ctrl_H__ */
