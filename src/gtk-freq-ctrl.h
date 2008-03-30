/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>

  Comments, questions and bugreports should be submitted via
  http://sourceforge.net/projects/groundstation/
  More details can be found at the project home page:

  http://groundstation.sourceforge.net/
 
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
#ifndef __GTK_FREQ_CTRL_H__
#define __GTK_FREQ_CTRL_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




#define GTK_TYPE_FREQ_CTRL          (gtk_freq_ctrl_get_type ())
#define GTK_FREQ_CTRL(obj)          GTK_CHECK_CAST (obj,\
				                       gtk_freq_ctrl_get_type (),\
						               GtkFreqCtrl)

#define GTK_FREQ_CTRL_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass,\
							 gtk_freq_ctrl_get_type (),\
							 GtkFreqCtrlClass)

#define IS_GTK_FREQ_CTRL(obj)       GTK_CHECK_TYPE (obj, gtk_freq_ctrl_get_type ())


typedef struct _gtk_freq_ctrl      GtkFreqCtrl;
typedef struct _GtkFreqCtrlClass   GtkFreqCtrlClass;



struct _gtk_freq_ctrl
{
	GtkVBox vbox;
	
    gdouble value;
};

struct _GtkFreqCtrlClass
{
	GtkVBoxClass parent_class;
};



GtkType    gtk_freq_ctrl_get_type  (void);
GtkWidget* gtk_freq_ctrl_new       (gdouble val);
void       gtk_freq_ctrl_set_value (GtkFreqCtrl *ctrl, gdouble val);
gdouble    gtk_freq_ctrl_get_value (GtkFreqCtrl *ctrl);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_FREQ_CTRL_H__ */
