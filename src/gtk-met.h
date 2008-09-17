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
#ifndef __GTK_MET_H__
#define __GTK_MET_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gtk-sat-data.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




#define GTK_TYPE_MET          (gtk_met_get_type ())
#define GTK_MET(obj)          GTK_CHECK_CAST (obj,\
				                     gtk_met_get_type (),\
						     GtkMet)

#define GTK_MET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass,\
							 gtk_met_get_type (),\
							 GtkMetClass)

#define IS_GTK_MET(obj)       GTK_CHECK_TYPE (obj, gtk_met_get_type ())


typedef struct _gtk_met      GtkMet;
typedef struct _GtkMetClass   GtkMetClass;


struct _gtk_met
{
	GtkVBox vbox;

	GHashTable   *sats;
	
	GtkWidget    *metlabel;     /*!< Header label, ie. satellite name. */
	GtkWidget    *evlabel;      /*!< LAst event label */
	gdouble       launch;       /*!< Date and time of launch */
	gdouble       tstamp;       /*!< time stamp of calculations; update by GtkSatModule */

	guint         timerid;
	
	void (* update) (GtkWidget *widget);  /*!< update function */
};

struct _GtkMetClass
{
	GtkVBoxClass parent_class;
};



GtkType        gtk_met_get_type        (void);
GtkWidget*     gtk_met_new             (GKeyFile   *cfgdata,
										GHashTable *sats,
										qth_t      *qth);
void           gtk_met_update          (GtkWidget  *widget);
void           gtk_met_reconf          (GtkWidget  *widget,
										GKeyFile   *newcfg,
										GHashTable *sats,
										qth_t      *qth,
										gboolean    local);


void gtk_met_reload_sats (GtkWidget *met, GHashTable *sats);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_MET_H__ */
