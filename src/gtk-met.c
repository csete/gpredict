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
/** \brief Satellite List Widget.
 *
 * More info...
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-met.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "gtk-sat-data.h"
#include "gpredict-utils.h"
#include "mod-mgr.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif




static void gtk_met_class_init (GtkMetClass *class);
static void gtk_met_init       (GtkMet      *list);
static void gtk_met_destroy    (GtkObject       *object);

static gboolean refresh_tle    (gpointer usrdata);


static GtkVBoxClass *parent_class = NULL;


GType
gtk_met_get_type ()
{
	static GType gtk_met_type = 0;

	if (!gtk_met_type) {

		static const GTypeInfo gtk_met_info = {
			sizeof (GtkMetClass),
			NULL,  /* base_init */
			NULL,  /* base_finalize */
			(GClassInitFunc) gtk_met_class_init,
			NULL,  /* class_finalize */
			NULL,  /* class_data */
			sizeof (GtkMet),
			5,     /* n_preallocs */
			(GInstanceInitFunc) gtk_met_init,
		};

		gtk_met_type = g_type_register_static (GTK_TYPE_VBOX,
													  "GtkMet",
													  &gtk_met_info,
													  0);
	}

	return gtk_met_type;
}


static void
gtk_met_class_init (GtkMetClass *class)
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

	object_class->destroy = gtk_met_destroy;
 
}



static void
gtk_met_init (GtkMet *list)
{
	/* 	GtkWidget *vbox,*hbox; */




}

static void
gtk_met_destroy (GtkObject *object)
{
	g_source_remove (GTK_MET (object)->timerid);
	
	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}




GtkWidget *
gtk_met_new (GKeyFile *cfgdata, GHashTable *sats, qth_t *qth)
{
	GtkWidget *widget;
	GtkWidget *vbox;     
	struct tm  cdate;


	widget = g_object_new (GTK_TYPE_MET, NULL);

	GTK_MET (widget)->update = gtk_met_update;

	/* Read configuration data. */
	/* ... */



	/* initialise launch date */
	cdate.tm_year = 2007;
	cdate.tm_mon = 12;
	cdate.tm_mday = 7;
	cdate.tm_hour = 21;
	cdate.tm_min = 9;
	cdate.tm_sec = 12;
	GTK_MET (widget)->launch = Julian_Date (&cdate);

	/* store pointer to sats */
	GTK_MET (widget)->sats = sats;


	/* create MET label */
	GTK_MET (widget)->metlabel = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (GTK_MET (widget)->metlabel),
						  "<span size='xx-large'>MET: -999:59:59</span>");
	
	/* create last event label */
	GTK_MET (widget)->evlabel = gtk_label_new ("Last event: -");
	gtk_misc_set_alignment (GTK_MISC (GTK_MET (widget)->evlabel),
							0.0, 0.5);


	/* pack widgets */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), GTK_MET (widget)->metlabel,
						TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (),
						FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), GTK_MET (widget)->evlabel,
						FALSE, FALSE, 3);
	gtk_container_add (GTK_CONTAINER (widget), vbox);

	gtk_widget_show_all (widget);

	/* start TLE refresher timeout */
	GTK_MET (widget)->timerid = g_timeout_add_seconds (10, refresh_tle, widget);

	return widget;
}







/** \brief Update satellites */
void
gtk_met_update          (GtkWidget *widget)
{
	GtkMet   *met = GTK_MET (widget);
	gdouble   number;
	gboolean  neg;    /* countdown instead of MET */
	gchar    *buff;
	guint     h, m, s;
	gchar    *ch, *cm, *cs;


	
	/* Update MET / Countdown */
	if (met->launch >= met->tstamp) {
		number = met->launch - met->tstamp;
		neg = TRUE;
	}
	else {
		number = met->tstamp - met->launch;
		neg = FALSE;
	}

	/* convert julian date to seconds */
	s = (guint) (number * 86400);

	/* extract hours */
	h = (guint) floor (s/3600);
	s -= 3600*h;

	/* leading zero */
	if (h < 10)
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

		buff = g_strdup_printf ("<span size='xx-large'><big><big><big><big>MET: "\
								"%c%s%d:%s%d%s%d</big></big></big></big></span>",
								neg ? '-' : ' ',
								ch, h, cm, m, cs, s);

	gtk_label_set_markup (GTK_LABEL (met->metlabel), buff);
	g_free (buff);
}







/** \brief Refresh internal references to the satellites.
 */
void
gtk_met_reload_sats (GtkWidget *met, GHashTable *sats)
{
	/* store new pointer to sats */
	GTK_MET (met)->sats = sats;
}


/** \brief Reload configuration.
 *  \param widget The GtkMet widget.
 *  \param newcfg The new configuration data for the module
 *  \param sats   The satellites.
 *  \param local  Flag indicating whether reconfiguration is requested from 
 *                local configuration dialog.
 */
void
gtk_met_reconf          (GtkWidget    *widget,
						 GKeyFile     *newcfg,
						 GHashTable   *sats,
						 qth_t        *qth,
						 gboolean      local)
{

}


/** \brief Refresh Shuttle TLE data if necessary.
 *  \bug This is a last minute hack!
 */
static gboolean
refresh_tle    (gpointer usrdata)
{
	GtkMet   *met = GTK_MET (usrdata);
	gchar    *buff;
	gchar   **data;
	gchar     epb[15];
	gsize     len;
	guint     n,i,j;
	gdouble   epoch,epochjd,satepochjd;

	sat_t    *sat;
	guint    *key = NULL;


	/* get pointer to the satellite */
	key = g_new0 (guint, 1);
	*key = 99122;
	sat = SAT (g_hash_table_lookup (met->sats, GINT_TO_POINTER (key)));
	g_free (key);
	
	/* load trajectory file */
	if (!g_file_get_contents ("/home/alexc/.gpredict2/sts-122.traj",
							  &buff, &len, NULL)) {
		sat_log_log (SAT_LOG_LEVEL_ERROR,
					 _("%s: %s failed"),
					 __FILE__, __FUNCTION__);
		g_free (buff);
		return FALSE;
	}

	
	data = g_strsplit (buff, "\n", 0);
	n = g_strv_length (data);
 	g_free (buff);

	/* loop over each line */
	i = 0;
	j = 0;
	while (i < n) {

		if (g_strrstr (data[i], "TWO LINE MEAN ELEMENT SET")) {

			/* TLE starts two lines ahead with satname */
			i += 2;

			/* copy the next 3 lines excluding leading whitespaces
			   we can't make a simple copy because of variable length
			   in source and fixed length in target
			*/
			/* remove leading and trailing whitespace */
			data[i] = g_strstrip (data[i]);
			data[i+1] = g_strstrip (data[i+1]);
			data[i+2] = g_strstrip (data[i+2]);

			/* get EPOCH of TLE */
			strncpy (epb, &data[i+1][18], 14);
			epb[14] = '\0';
			epoch = g_ascii_strtod (epb, NULL);

			/* convert to JD */
			epochjd = Julian_Date_of_Epoch (epoch);
			satepochjd = Julian_Date_of_Epoch (sat->tle.epoch);

			/* IF
			     the TLE is newer than the one currently loaded
			     but not in the future
			   OR
			     we are in the first iteration and satepoch is in
				 the future (e.g. we have used the simulator)
			*/
			if (((epoch > sat->tle.epoch) && (epochjd < met->tstamp)) ||
				((j == 0) && (satepochjd > met->tstamp))) {

				/* save this TLE data to sts-122.tle */
				buff = g_strdup_printf ("%s\n%s\n%s\n",
										data[i], data[i+1], data[i+2]);

				if (!g_file_set_contents ("/home/alexc/.gpredict2/tle/sts-122.tle",
										  buff, -1, NULL)) {
					sat_log_log (SAT_LOG_LEVEL_ERROR,
								 _("%s: Failed to save TLE for %.8f"),
								 __FILE__, epoch);
				}

				j++;

				g_free (buff);
			}


			i += 3;
		}
		else {
			i++;
		}

	}

	g_strfreev (data);

	/* trig sat_reload if there has been at least one update */
	if (j > 0) {
		sat_log_log (SAT_LOG_LEVEL_MSG,
					 _("%s: %s refreshed TLE for STS-122 (j=%d)\n"),
					 __FILE__, __FUNCTION__, j);

		/* reload satellites */
		mod_mgr_reload_sats ();

		/* add info to even log */
	}


	return TRUE;
}

