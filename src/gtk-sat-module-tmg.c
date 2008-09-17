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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <sys/time.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "gtk-sat-module.h"
#include "gtk-sat-module-tmg.h"
#include "compat.h"



static gint tmg_delete   (GtkWidget *, GdkEvent *, gpointer);
static void tmg_destroy  (GtkWidget *, gpointer);

static void tmg_stop     (GtkWidget *widget, gpointer data);
static void tmg_fwd      (GtkWidget *widget, gpointer data);
static void tmg_bwd      (GtkWidget *widget, gpointer data);
static void tmg_reset    (GtkWidget *widget, gpointer data);
static void tmg_throttle (GtkWidget *widget, gpointer data);
static void tmg_time_set (GtkWidget *widget, gpointer data);



/** \brief Create and initialise time controller widgets.
 *  \param module The parent GtkSatModule
 *
 */
void
tmg_create (GtkSatModule *mod)
{
    GtkWidget   *vbox, *hbox, *table;
    GtkWidget   *image;
	GtkWidget   *label;
    GtkTooltips *tips;
    gchar       *title;
	gchar       *buff;


    /* make sure controller is not already active */
    if (mod->tmgActive) {
		sat_log_log (SAT_LOG_LEVEL_MSG,
					 _("%s: Time Controller for %s is already active"),
					 __FUNCTION__, mod->name);

		/* try to make window visible in case it is covered
		   by something else */
		gtk_window_present (GTK_WINDOW (mod->tmgWin));

        return;
	}

    /* create hbox containing the controls
       the controls are implemented as radiobuttons in order
       to inherit the mutual exclusion behaviour
    */
    hbox = gtk_hbox_new (FALSE, 0);
	
    
    /* FWD */
    mod->tmgFwd = gtk_radio_button_new (NULL);
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (mod->tmgFwd), FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mod->tmgFwd), TRUE);
    image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (mod->tmgFwd), image);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, mod->tmgFwd, _("Play forward"), NULL);
    g_signal_connect (mod->tmgFwd, "toggled", G_CALLBACK (tmg_fwd), mod);
    gtk_box_pack_end (GTK_BOX (hbox), mod->tmgFwd, FALSE, FALSE, 0);

    /* STOP */
    mod->tmgStop = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (mod->tmgFwd));
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (mod->tmgStop), FALSE);
    image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (mod->tmgStop), image);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, mod->tmgStop, _("Stop"), NULL);
    g_signal_connect (mod->tmgStop, "toggled", G_CALLBACK (tmg_stop), mod);
    gtk_box_pack_end (GTK_BOX (hbox), mod->tmgStop, FALSE, FALSE, 0);

    /* BWD */
    mod->tmgBwd = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (mod->tmgFwd));
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (mod->tmgBwd), FALSE);
    image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_direction (image, GTK_TEXT_DIR_RTL);
    gtk_container_add (GTK_CONTAINER (mod->tmgBwd), image);
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, mod->tmgBwd, _("Play backwards"), NULL);
    g_signal_connect (mod->tmgBwd, "toggled", G_CALLBACK (tmg_bwd), mod);
    gtk_box_pack_end (GTK_BOX (hbox), mod->tmgBwd, FALSE, FALSE, 0);

	/* reset time */
	mod->tmgReset = gtk_button_new_with_label (_("Reset"));
    tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (tips, mod->tmgReset, _("Reset to current date and time"), NULL);
    g_signal_connect (mod->tmgReset, "clicked", G_CALLBACK (tmg_reset), mod);
    gtk_box_pack_end (GTK_BOX (hbox), mod->tmgReset, FALSE, FALSE, 10);

	/* status label */
	mod->tmgState = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (mod->tmgState), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (mod->tmgState), _("<b>Real-Time</b>"));
    gtk_box_pack_start (GTK_BOX (hbox), mod->tmgState, TRUE, TRUE, 10);
	

    /* create table containing the date and time widgets */
	table = gtk_table_new (5, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 0);

	mod->tmgCal = gtk_calendar_new ();
	gtk_calendar_set_display_options (GTK_CALENDAR (mod->tmgCal),
									  GTK_CALENDAR_SHOW_HEADING	|
									  GTK_CALENDAR_SHOW_DAY_NAMES |
									  GTK_CALENDAR_WEEK_START_MONDAY);
	g_signal_connect (mod->tmgCal, "day-selected",
					  G_CALLBACK (tmg_time_set), mod);
	gtk_table_attach (GTK_TABLE (table), mod->tmgCal,
					  0, 1, 0, 5, GTK_SHRINK, GTK_SHRINK,
					  0, 0);

	/* hour */
	label = gtk_label_new (_(" Hour:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
					  1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 5, 0);
	mod->tmgHour = gtk_spin_button_new_with_range (0, 23, 1);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (mod->tmgHour), TRUE);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (mod->tmgHour),
									   GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (mod->tmgHour), 0);
	//FIXME gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgHour), 2);
	tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tips, mod->tmgHour,
						  _("Use this control to set the hour"),
						  NULL);
	g_signal_connect (mod->tmgHour, "value-changed",
					  G_CALLBACK (tmg_time_set), mod);
	gtk_table_attach (GTK_TABLE (table), mod->tmgHour,
					  2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

	/* minutes */
	label = gtk_label_new (_(" Min:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
					  1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 5, 0);
	mod->tmgMin = gtk_spin_button_new_with_range (0, 59, 1);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (mod->tmgMin), TRUE);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (mod->tmgMin),
									   GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (mod->tmgMin), 0);
	//FIXME gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgMin), 2);
	tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tips, mod->tmgMin,
						  _("Use this control to set the minutes"),
						  NULL);
	g_signal_connect (mod->tmgMin, "value-changed",
					  G_CALLBACK (tmg_time_set), mod);
	gtk_table_attach (GTK_TABLE (table), mod->tmgMin,
					  2, 3, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);

	/* seconds */
	label = gtk_label_new (_(" Sec:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
					  1, 2, 2, 3, GTK_SHRINK, GTK_SHRINK, 5, 0);
	mod->tmgSec = gtk_spin_button_new_with_range (0, 59, 1);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (mod->tmgSec), TRUE);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (mod->tmgSec),
									   GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (mod->tmgSec), 0);
	//FIXME gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgSec), 2);
	tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tips, mod->tmgSec,
						  _("Use this control to set the seconds"),
						  NULL);
	g_signal_connect (mod->tmgSec, "value-changed",
					  G_CALLBACK (tmg_time_set), mod);
	gtk_table_attach (GTK_TABLE (table), mod->tmgSec,
					  2, 3, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);

	/* milliseconds */
	label = gtk_label_new (_(" Msec:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
					  1, 2, 3, 4, GTK_SHRINK, GTK_SHRINK, 5, 0);
	mod->tmgMsec = gtk_spin_button_new_with_range (0, 999, 1);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (mod->tmgMsec), TRUE);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (mod->tmgMsec),
									   GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (mod->tmgMsec), 0);
	//FIXME gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgMsec), 2);
	tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tips, mod->tmgMsec,
						  _("Use this control to set the milliseconds"),
						  NULL);
	g_signal_connect (mod->tmgMsec, "value-changed",
					  G_CALLBACK (tmg_time_set), mod);
	gtk_table_attach (GTK_TABLE (table), mod->tmgMsec,
					  2, 3, 3, 4, GTK_SHRINK, GTK_SHRINK, 0, 0);

	/* time throttle */
	label = gtk_label_new (_("Throttle:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
					  1, 2, 4, 5, GTK_SHRINK, GTK_SHRINK, 5, 0);
	mod->tmgFactor = gtk_spin_button_new_with_range (1, 100, 1);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (mod->tmgFactor), TRUE);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (mod->tmgFactor),
									   GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (mod->tmgFactor), 0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgFactor), 1);
	tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tips, mod->tmgFactor,
						  _("Time throttle / compression factor"),
						  NULL);
	g_signal_connect (mod->tmgFactor, "value-changed",
					  G_CALLBACK (tmg_throttle), mod);
	gtk_table_attach (GTK_TABLE (table), mod->tmgFactor,
					  2, 3, 4, 5, GTK_SHRINK, GTK_SHRINK, 0, 0);


    /* create the vertical box */
    vbox = gtk_vbox_new (FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new(), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    /* add slider */

  
    /* create main window */
    mod->tmgWin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    title = g_strconcat (_("Time Controller"), " / ", mod->name, NULL);
    gtk_window_set_title (GTK_WINDOW (mod->tmgWin), title);
    g_free (title);
    gtk_window_set_transient_for (GTK_WINDOW (mod->tmgWin),
                                  GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (mod))));
    g_signal_connect (G_OBJECT (mod->tmgWin), "delete_event",
                      G_CALLBACK (tmg_delete), mod);    
    g_signal_connect (G_OBJECT (mod->tmgWin), "destroy",
                      G_CALLBACK (tmg_destroy), mod);

    /* window icon */
	buff = icon_file_name ("gpredict-clock.png");
	gtk_window_set_icon_from_file (GTK_WINDOW (mod->tmgWin), buff, NULL);
	g_free (buff);

    gtk_container_add (GTK_CONTAINER (mod->tmgWin), vbox);
    gtk_widget_show_all (mod->tmgWin);
  
    mod->tmgActive = TRUE;

	sat_log_log (SAT_LOG_LEVEL_MSG,
				 _("%s: Time Controller for %s launched"),
				 __FUNCTION__, mod->name);
}



/** \brief Manage tmg window delete events
 *  \return FALSE to indicate that window should be destroyed.
 */ 
static gint
tmg_delete   (GtkWidget *tmg, GdkEvent *event, gpointer data)
{
    return FALSE;
}



static void
tmg_destroy  (GtkWidget *tmg, gpointer data)
{
    GtkSatModule *mod = GTK_SAT_MODULE (data);


    /* reset time keeping variables */
    mod->throttle = 1;
    mod->tmgActive = FALSE;

	/* reset time */
	tmg_reset (NULL, data);

	sat_log_log (SAT_LOG_LEVEL_MSG,
				 _("%s: Time Controller for %s closed. Time reset."),
				 __FUNCTION__, mod->name);

}


/** \brief Manage STOP signals.
 *  \param widget The GtkButton that received the signal.
 *  \param data Pointer to the GtkSatModule widget.
 *
 * This function is called when the user clicks on the STOP button.
 * If the button state is active (pressed in) the function sets the
 * throttle parameter of the module to 0.
 */
static void
tmg_stop     (GtkWidget *widget, gpointer data)
{
    GtkSatModule *mod = GTK_SAT_MODULE (data);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {

        sat_log_log (SAT_LOG_LEVEL_DEBUG, __FUNCTION__);

        /* set throttle to 0 */
        mod->throttle = 0;

    }

}



/** \brief Manage FWD signals.
 *  \param widget The GtkButton that received the signal.
 *  \param data Pointer to the GtkSatModule widget.
 *
 * This function is called when the user clicks on the FWD button.
 * If the button state is active (pressed in) the function sets the
 *  throttle parameter of the module to 1.
 */
static void
tmg_fwd      (GtkWidget *widget, gpointer data)
{
    GtkSatModule *mod = GTK_SAT_MODULE (data);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {

        sat_log_log (SAT_LOG_LEVEL_DEBUG, __FUNCTION__);

        /* set throttle */
        mod->throttle = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mod->tmgFactor));

    }

}


/** \brief Manage BWD signals.
 *  \param widget The GtkButton that received the signal.
 *  \param data Pointer to the GtkSatModule widget.
 *
 * This function is called when the user clicks on the BWD button.
 * If the button state is active (pressed in) the function sets the
 *  throttle parameter of the module to -1.
 */
static void
tmg_bwd      (GtkWidget *widget, gpointer data)
{
    GtkSatModule *mod = GTK_SAT_MODULE (data);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {

        sat_log_log (SAT_LOG_LEVEL_DEBUG, __FUNCTION__);

        /* set throttle to -1 */
        mod->throttle = -gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mod->tmgFactor));;

    }
}


/** \brief Manage Reset button clicks.
 *  \param widget The button that received the "clicked" signal.
 *  \param data Pointer to the GtkSatModule structure.
 *
 * This function is called when the user clicks on the Reset button.
 * It resets the current time by setting 
 *
 *	mod->tmgPdnum = mod->rtPrev;
 *	mod->tmgCdnum = mod->rtNow;
 *
 */
static void
tmg_reset      (GtkWidget *widget, gpointer data)
{
    GtkSatModule *mod = GTK_SAT_MODULE (data);

	/* set reset flag in order to block
	   widget signals */
	mod->reset = TRUE;

	mod->tmgPdnum = mod->rtPrev;
	mod->tmgCdnum = mod->rtNow;

	/* update widgets; widget signals will have no effect
	   since the tmgReset flag is TRUE	*/
	tmg_update_widgets (mod);

	/* clear reset flag */
	mod->reset = FALSE;
}



static void
tmg_throttle (GtkWidget *widget, gpointer data)
{
    GtkSatModule *mod = GTK_SAT_MODULE (data);


	/* FFWD mode */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (mod->tmgFwd))) {
		mod->throttle = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (mod->tmgBwd))) {
		mod->throttle = -gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
	}
}



static void
tmg_time_set (GtkWidget *widget, gpointer data)
{
    GtkSatModule *mod = GTK_SAT_MODULE (data);
	guint year, month, day;
	gint  hr, min, sec, msec;
	struct tm tim;
	gdouble jd;

	/* update time only if we are in manual time control */
	if (!mod->throttle && !mod->reset) {

		/* get date and time from widgets */
		gtk_calendar_get_date (GTK_CALENDAR (mod->tmgCal),
							   &year, &month, &day);

		hr = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mod->tmgHour));
		min = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mod->tmgMin));
		sec = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mod->tmgSec));
		msec = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mod->tmgMsec));

		/* build struct_tm */
		tim.tm_year = (int) (year);
		tim.tm_mon = (int) (month+1);
		tim.tm_mday = (int) day;
		tim.tm_hour = (int) hr;
		tim.tm_min = (int) min;
		tim.tm_sec = (int) sec;

		sat_log_log (SAT_LOG_LEVEL_DEBUG,
					 _("%s: %d/%d/%d %d:%d:%d.%d"),
					 __FUNCTION__,
					 tim.tm_year, tim.tm_mon, tim.tm_mday,
					 tim.tm_hour, tim.tm_min, tim.tm_sec, msec);

		/* convert to Julian Date (FIXME: only UTC?) */
		jd = Julian_Date (&tim);
		jd = jd + (gdouble)msec/8.64e+7;

		mod->tmgCdnum = jd;
	}
}



/** \brief Update Time controller widgets
 *  \param mod Pointer to the GtkSatModule widget
 */
void
tmg_update_widgets (GtkSatModule *mod)
{
	struct tm tim;
	time_t    t;

	/* update time widgets */
	t = (mod->tmgCdnum - 2440587.5)*86400.;
	
	if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME)) {
		tim = *localtime (&t);
	}
	else {
		tim = *gmtime (&t);
	}
	
	/* hour */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgHour),
							   tim.tm_hour);
	
	/* min */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgMin),
							   tim.tm_min);
	
	/* sec */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgSec),
							   tim.tm_sec);
	
	/* msec: alway 0 in RT and SRT modes */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (mod->tmgMsec), 0);


	/* calendar */
	gtk_calendar_select_month (GTK_CALENDAR (mod->tmgCal),
							   tim.tm_mon, tim.tm_year+1900);
	gtk_calendar_select_day (GTK_CALENDAR (mod->tmgCal), tim.tm_mday);

}


/** \brief Update state label.
 *  \param mod Pointer to the GtkSatModule widget
 *
 * This function is used to update the state label showing
 * whether the time control is in RT, SRT, or MAN mode.
 *
 */
void
tmg_update_state (GtkSatModule *mod)
{


	if (mod->rtPrev != mod->tmgPdnum) {
		if (mod->throttle) {
			gtk_label_set_markup (GTK_LABEL (mod->tmgState),
								  _("<b>Simulated Real-Time</b>"));
		}
		else {
			gtk_label_set_markup (GTK_LABEL (mod->tmgState),
								  _("<b>Manual Control</b>"));
		}
	}
	else
		gtk_label_set_markup (GTK_LABEL (mod->tmgState),
							  _("<b>Real-Time</b>"));


}
