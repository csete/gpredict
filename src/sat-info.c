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
/** \brief Satellite info
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-pass-dialogs.h"
#include "sat-info.h"






static gchar *epoch_to_str     (sat_t *sat);


/** \brief Show satellite info in a dialog 
 *  \param menuitem The menuitem from where the function is invoked.
 *  \param data Pointer to parent window or NULL.
 *
 * FIXME: see nice drawing at http://www.amsat.org/amsat-new/tools/keps_tutorial.php
 *
*/
void
show_sat_info (GtkWidget *menuitem, gpointer data)
{
	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *label;
	GtkWindow *toplevel = NULL;
	sat_t     *sat;
	gchar     *str;


	sat = SAT(g_object_get_data (G_OBJECT (menuitem), "sat"));

	if (data != NULL)
		toplevel = GTK_WINDOW (data);

	/* create table */
	table = gtk_table_new (20, 4, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_container_set_border_width (GTK_CONTAINER (table), 10);

	/* create table contents and add them to table */

	/* satellite name */
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Satellite name:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	label = gtk_label_new (NULL);
	str = g_strdup_printf (_("<b>%s</b>"), sat->tle.sat_name);
	gtk_label_set_markup (GTK_LABEL (label), str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
	g_free (str);

	/* operational status */
	label = gtk_label_new (_("Operational Status:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

	switch (sat->tle.status) {

	case OP_STAT_OPERATIONAL:
		label = gtk_label_new (_("Operational"));
		break;

	case OP_STAT_NONOP:
		label = gtk_label_new (_("Non-operational"));
		break;

	case OP_STAT_PARTIAL:
		label = gtk_label_new (_("Partially operational"));
		break;

	case OP_STAT_STDBY:
		label = gtk_label_new (_("Backup/Standby"));
		break;

	case OP_STAT_SPARE:
		label = gtk_label_new (_("Spare"));
		break;

	case OP_STAT_EXTENDED:
		label = gtk_label_new (_("Extended Mission"));
		break;

	default:
		label = gtk_label_new (_("Unknown"));
		break;

	}

	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 1, 2);

	/* Catnum */
	label = gtk_label_new (_("Catalogue number:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

	str = g_strdup_printf ("%d", sat->tle.catnr);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 2, 3);
	g_free (str);

	/* international designator */
	label = gtk_label_new (_("Internation designator:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);

	label = gtk_label_new (sat->tle.idesg);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 3, 4);

	/* elset number */
	label = gtk_label_new (_("Element set number:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);

	str = g_strdup_printf ("%d", sat->tle.elset);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 4, 5);
	g_free (str);

	/* elset epoch */
	label = gtk_label_new (_("Epoch time:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 5, 6);

	str = epoch_to_str (sat);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 5, 6);
	g_free (str);

	/* Revolution Number @ Epoch */
	label = gtk_label_new (_("Orbit number @ epoch:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 6, 7);

	str = g_strdup_printf ("%d", sat->tle.revnum);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 6, 7);
	g_free (str);

	/* ephermis type left out, since it is always 0 */

	/* separator */
	gtk_table_attach_defaults (GTK_TABLE (table),
							   gtk_hseparator_new (),
							   0, 4, 7, 8);

	/* Orbit inclination */
	label = gtk_label_new (_("Inclination:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 8, 9);

	str = g_strdup_printf ("%.4f\302\260", sat->tle.xincl/de2ra);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 8, 9);
	g_free (str);

	/* RAAN */
	label = gtk_label_new (_("RAAN:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 9, 10);

	str = g_strdup_printf ("%.4f\302\260", sat->tle.xnodeo/de2ra);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 9, 10);
	g_free (str);

	/* Eccentricity */
	label = gtk_label_new (_("Eccentricity:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 10, 11);

	str = g_strdup_printf ("%.7f", sat->tle.eo);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 10, 11);
	g_free (str);

	/* Argument of perigee */
	label = gtk_label_new (_("Arg. of perigee:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 11, 12);

	str = g_strdup_printf ("%.4f\302\260", sat->tle.omegao/de2ra);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 11, 12);
	g_free (str);

	/* Mean Anomaly */
	label = gtk_label_new (_("Mean anomaly:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 12, 13);

	str = g_strdup_printf ("%.4f\302\260", sat->tle.xmo/de2ra);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 12, 13);
	g_free (str);

	/* Mean Motion */
	label = gtk_label_new (_("Mean motion:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 13, 14);

	//str = g_strdup_printf ("%.4f [rev/day]", sat->tle.xno/(xmnpda*(twopi/xmnpda/xmnpda)));
	str = g_strdup_printf ("%.8f [rev/day]", sat->meanmo);
	label = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 13, 14);
	g_free (str);

	/* one half of the first time derivative of mean motion */
	label = gtk_label_new (_("\302\275 d/dt (mean motion):"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 14, 15);

	str = g_strdup_printf ("%.5e [rev/day<sup>2</sup>]",
			       sat->tle.xndt2o/(twopi/xmnpda/xmnpda));
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 14, 15);
	g_free (str);

	/* one sixth of the second time derivative of mean motion */
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label),
				  _("1/6 d<sup>2</sup>/dt<sup>2</sup> (mean motion):"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 15, 16);

	str = g_strdup_printf ("%.5e [rev/day<sup>3</sup>]",
			       sat->tle.xndd6o*xmnpda/(twopi/xmnpda/xmnpda));
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 15, 16);
	g_free (str);

	/* B* drag term */
	label = gtk_label_new (_("B* drag term:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 16, 17);

	str = g_strdup_printf ("%.5e [R<sub>E</sub><sup>-1</sup>]", sat->tle.bstar*ae);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 16, 17);
	g_free (str);

	/* Orbit type */

	/* Next Event */



	gtk_widget_show_all (table);

	/* create dialog window with NULL parent */
	dialog = gtk_dialog_new_with_buttons (_("Satellite Info"),
					      toplevel,
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);

	/* allow interaction with other windows */
	gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);

	g_signal_connect (dialog, "response",
			  G_CALLBACK (gtk_widget_destroy), NULL);
 	g_signal_connect (dialog, "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &dialog);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), table);

	gtk_widget_show_all (dialog);

					      
}



/*  BBBBB.BBBBBBBB] - Epoch Time -- 2-digit year, followed by 3-digit sequential
                      day of the year, followed by the time represented as the
                      fractional portion of one day, but...

    we now have the converted fields, tle->epoch_year, tle->epoch_day and tle->epoch_fod
		      
*/
static gchar *
epoch_to_str (sat_t *sat)
{
	GDate     *epd;
	guint      h,m,s,sec;
	gchar     *buff;
	gchar     *fmt;
	struct tm  tms;
	time_t     t;
	guint      size;

	/* http://celestrak.com/columns/v04n03/#FAQ02
	   ... While talking about the epoch, this is perhaps a good place to answer
	   the other time-related questions. First, how is the epoch time format
	   interpreted? This question is best answered by using an example. An epoch
	   of 98001.00000000 corresponds to 0000 UT on 1998 January 01st in other
	   words, midnight between 1997 December 31 and 1998 January 01. An epoch of
	   98000.00000000 would actually correspond to the beginning of 1997 December
	   31st strange as that might seem. Note that the epoch day starts at UT
	   midnight (not noon) and that all times are measured mean solar rather than
	   sidereal time units (the answer to our third question).
	*/
	epd = g_date_new_dmy (1, 1, sat->tle.epoch_year);
	g_date_add_days (epd, sat->tle.epoch_day-1);

	/* convert date to struct tm */
	g_date_to_struct_tm (epd, &tms);

	/* add HMS */
	sec = (guint) floor (sat->tle.epoch_fod * 86400);   /* fraction of day in seconds */
	
	/* hour */
	h = (guint) floor (sec / 3600);
	tms.tm_hour = h;

	/* minutes */
	m = (guint) floor ((sec - (h*3600)) / 60);
	tms.tm_min  = m;

	s = (guint) floor (sec - (h*3600) - (m*60));
	tms.tm_sec  = s;

	/* get format string */
	fmt = sat_cfg_get_str (SAT_CFG_STR_TIME_FORMAT);

	/* format either local time or UTC depending on check box */
	t = mktime (&tms);
	buff = g_try_malloc (51);

	if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
		size = strftime (buff, 50, fmt, localtime (&t));
	else
		size = strftime (buff, 50, fmt, gmtime (&t));

	if (size < 50)
		buff[size]='\0';
	else
		buff[50]='\0';

	g_date_free (epd);
	g_free (fmt);

	return buff;
}

