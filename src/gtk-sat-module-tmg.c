/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <sys/time.h>

#include "compat.h"
#include "gtk-sat-module.h"
#include "gtk-sat-module-tmg.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"


static gint     tmg_delete(GtkWidget *, GdkEvent *, gpointer);
static void     tmg_destroy(GtkWidget *, gpointer);

static void     tmg_stop(GtkWidget * widget, gpointer data);
static void     tmg_fwd(GtkWidget * widget, gpointer data);
static void     tmg_bwd(GtkWidget * widget, gpointer data);
static void     tmg_reset(GtkWidget * widget, gpointer data);
static void     tmg_throttle(GtkWidget * widget, gpointer data);
static void     tmg_time_set(GtkWidget * widget, gpointer data);
static void     slider_moved(GtkWidget * widget, gpointer data);
static void     tmg_hour_wrap(GtkWidget * widget, gpointer data);
static void     tmg_min_wrap(GtkWidget * widget, gpointer data);
static void     tmg_sec_wrap(GtkWidget * widget, gpointer data);
static void     tmg_msec_wrap(GtkWidget * widget, gpointer data);
static void     tmg_cal_add_one_day(GtkSatModule * mod);
static void     tmg_cal_sub_one_day(GtkSatModule * mod);

static gdouble  calculate_time(GtkSatModule * mod);

void tmg_create(GtkSatModule * mod)
{
    GtkWidget      *vbox, *hbox, *table;
    GtkWidget      *image;
    GtkWidget      *label;
    gchar          *title;
    gchar          *buff;

    /* make sure controller is not already active */
    if (mod->tmgActive)
    {
        sat_log_log(SAT_LOG_LEVEL_INFO,
                    _("%s: Time Controller for %s is already active"),
                    __func__, mod->name);

        /* try to make window visible in case it is covered
           by something else */
        gtk_window_present(GTK_WINDOW(mod->tmgWin));

        return;
    }

    /* create hbox containing the controls
       the controls are implemented as radiobuttons in order
       to inherit the mutual exclusion behaviour */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);

    /* FWD */
    mod->tmgFwd = gtk_radio_button_new(NULL);
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(mod->tmgFwd), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mod->tmgFwd), TRUE);
#ifdef G_OS_WIN32
    image = gtk_image_new_from_icon_name("media-seek-forward-symbolic",
                                         GTK_ICON_SIZE_BUTTON);
#else
    image = gtk_image_new_from_icon_name("media-seek-forward",
                                         GTK_ICON_SIZE_BUTTON);
#endif
    gtk_container_add(GTK_CONTAINER(mod->tmgFwd), image);
    gtk_widget_set_tooltip_text(mod->tmgFwd, _("Play forward"));
    g_signal_connect(mod->tmgFwd, "toggled", G_CALLBACK(tmg_fwd), mod);
    gtk_box_pack_end(GTK_BOX(hbox), mod->tmgFwd, FALSE, FALSE, 0);

    /* STOP */
    mod->tmgStop =
        gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(mod->tmgFwd));
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(mod->tmgStop), FALSE);
#ifdef G_OS_WIN32
    image = gtk_image_new_from_icon_name("media-playback-pause-symbolic",
                                         GTK_ICON_SIZE_BUTTON);
#else
    image = gtk_image_new_from_icon_name("media-playback-pause",
                                         GTK_ICON_SIZE_BUTTON);
#endif
    gtk_container_add(GTK_CONTAINER(mod->tmgStop), image);
    gtk_widget_set_tooltip_text(mod->tmgStop, _("Stop"));
    g_signal_connect(mod->tmgStop, "toggled", G_CALLBACK(tmg_stop), mod);
    gtk_box_pack_end(GTK_BOX(hbox), mod->tmgStop, FALSE, FALSE, 0);

    /* BWD */
    mod->tmgBwd =
        gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(mod->tmgFwd));
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(mod->tmgBwd), FALSE);
#ifdef G_OS_WIN32
    image = gtk_image_new_from_icon_name("media-seek-backward-symbolic",
                                         GTK_ICON_SIZE_BUTTON);
#else
    image = gtk_image_new_from_icon_name("media-seek-backward",
                                         GTK_ICON_SIZE_BUTTON);
#endif
    gtk_container_add(GTK_CONTAINER(mod->tmgBwd), image);
    gtk_widget_set_tooltip_text(mod->tmgBwd, _("Play backwards"));
    g_signal_connect(mod->tmgBwd, "toggled", G_CALLBACK(tmg_bwd), mod);
    gtk_box_pack_end(GTK_BOX(hbox), mod->tmgBwd, FALSE, FALSE, 0);

    /* reset time */
    mod->tmgReset = gtk_button_new_with_label(_("Reset"));
    gtk_widget_set_tooltip_text(mod->tmgReset,
                                _("Reset to current date and time"));
    g_signal_connect(mod->tmgReset, "clicked", G_CALLBACK(tmg_reset), mod);
    gtk_box_pack_end(GTK_BOX(hbox), mod->tmgReset, FALSE, FALSE, 10);

    /* status label */
    mod->tmgState = gtk_label_new(NULL);
    g_object_set(mod->tmgState, "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_label_set_markup(GTK_LABEL(mod->tmgState), _("<b>Real-Time</b>"));
    gtk_box_pack_start(GTK_BOX(hbox), mod->tmgState, TRUE, TRUE, 10);

    /* create table containing the date and time widgets */
    table = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(table), 0);
    gtk_grid_set_column_spacing(GTK_GRID(table), 5);

    mod->tmgCal = gtk_calendar_new();
    gtk_calendar_set_display_options(GTK_CALENDAR(mod->tmgCal),
                                     GTK_CALENDAR_SHOW_HEADING |
                                     GTK_CALENDAR_SHOW_DAY_NAMES);
    g_signal_connect(mod->tmgCal, "day-selected",
                     G_CALLBACK(tmg_time_set), mod);
    gtk_grid_attach(GTK_GRID(table), mod->tmgCal, 0, 0, 1, 5);

    /* Time controllers.
     * Note that the controllers for hours, minutes, and seconds have ranges;
     * however, they can wrap around their limits in order to ensure a smooth
     * and continuous control of the time
     */

    /* hour */
    label = gtk_label_new(_("Hour:"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 1, 0, 1, 1);
    mod->tmgHour = gtk_spin_button_new_with_range(0, 23, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mod->tmgHour), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mod->tmgHour),
                                      GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(mod->tmgHour), 0);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(mod->tmgHour), TRUE);
    gtk_widget_set_tooltip_text(mod->tmgHour,
                                _("Use this control to set the hour"));
    g_signal_connect(mod->tmgHour, "value-changed",
                     G_CALLBACK(tmg_time_set), mod);
    g_signal_connect(mod->tmgHour, "wrapped", G_CALLBACK(tmg_hour_wrap), mod);
    gtk_grid_attach(GTK_GRID(table), mod->tmgHour, 2, 0, 1, 1);

    /* minutes */
    label = gtk_label_new(_("Min:"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 1, 1, 1, 1);
    mod->tmgMin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mod->tmgMin), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mod->tmgMin),
                                      GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(mod->tmgMin), 0);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(mod->tmgMin), TRUE);
    gtk_widget_set_tooltip_text(mod->tmgMin,
                                _("Use this control to set the minutes"));
    g_signal_connect(mod->tmgMin, "value-changed",
                     G_CALLBACK(tmg_time_set), mod);
    g_signal_connect(mod->tmgMin, "wrapped", G_CALLBACK(tmg_min_wrap), mod);
    gtk_grid_attach(GTK_GRID(table), mod->tmgMin, 2, 1, 1, 1);

    /* seconds */
    label = gtk_label_new(_("Sec:"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 1, 2, 1, 1);
    mod->tmgSec = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mod->tmgSec), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mod->tmgSec),
                                      GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(mod->tmgSec), 0);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(mod->tmgSec), TRUE);
    gtk_widget_set_tooltip_text(mod->tmgSec,
                                _("Use this control to set the seconds"));
    g_signal_connect(mod->tmgSec, "value-changed",
                     G_CALLBACK(tmg_time_set), mod);
    g_signal_connect(mod->tmgSec, "wrapped", G_CALLBACK(tmg_sec_wrap), mod);
    gtk_grid_attach(GTK_GRID(table), mod->tmgSec, 2, 2, 1, 1);

    /* milliseconds */
    label = gtk_label_new(_("Msec:"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 1, 3, 1, 1);
    mod->tmgMsec = gtk_spin_button_new_with_range(0, 999, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mod->tmgMsec), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mod->tmgMsec),
                                      GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(mod->tmgMsec), 0);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(mod->tmgMsec), TRUE);
    gtk_widget_set_tooltip_text(mod->tmgMsec,
                                _("Use this control to set the milliseconds"));
    g_signal_connect(mod->tmgMsec, "value-changed",
                     G_CALLBACK(tmg_time_set), mod);
    g_signal_connect(mod->tmgMsec, "wrapped", G_CALLBACK(tmg_msec_wrap), mod);
    gtk_grid_attach(GTK_GRID(table), mod->tmgMsec, 2, 3, 1, 1);

    /* time throttle */
    label = gtk_label_new(_("Throttle:"));
    g_object_set(label, "xalign", 1.0, "yalign", 0.5, NULL);
    gtk_grid_attach(GTK_GRID(table), label, 1, 4, 1, 1);
    mod->tmgFactor = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mod->tmgFactor), TRUE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mod->tmgFactor),
                                      GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(mod->tmgFactor), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgFactor), 1);
    gtk_widget_set_tooltip_text(mod->tmgFactor,
                                _("Time throttle / compression factor"));
    g_signal_connect(mod->tmgFactor, "value-changed",
                     G_CALLBACK(tmg_throttle), mod);
    gtk_grid_attach(GTK_GRID(table), mod->tmgFactor, 2, 4, 1, 1);

    /* add slider */
    mod->tmgSlider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                              -0.1, +0.1, 0.0001);
    /*gtk_widget_set_tooltip_text (mod->tmgSlider,
       _("Drag the slider to change the time up to +/- 2.5 hours.\n"\
       "Resolution is ~ 8 seconds.")); */
    gtk_scale_set_draw_value(GTK_SCALE(mod->tmgSlider), FALSE);
    gtk_range_set_value(GTK_RANGE(mod->tmgSlider), 0.0);
    g_signal_connect(mod->tmgSlider, "value-changed",
                     G_CALLBACK(slider_moved), mod);

    /* create the vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);

    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), mod->tmgSlider, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* create main window */
    mod->tmgWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    title = g_strconcat(_("Time Controller"), " / ", mod->name, NULL);
    gtk_window_set_title(GTK_WINDOW(mod->tmgWin), title);
    g_free(title);
    gtk_window_set_transient_for(GTK_WINDOW(mod->tmgWin),
                                 GTK_WINDOW(gtk_widget_get_toplevel
                                            (GTK_WIDGET(mod))));
    g_signal_connect(G_OBJECT(mod->tmgWin), "delete_event",
                     G_CALLBACK(tmg_delete), mod);
    g_signal_connect(G_OBJECT(mod->tmgWin), "destroy", G_CALLBACK(tmg_destroy),
                     mod);

    /* window icon */
    buff = icon_file_name("gpredict-clock.png");
    gtk_window_set_icon_from_file(GTK_WINDOW(mod->tmgWin), buff, NULL);
    g_free(buff);

    gtk_container_add(GTK_CONTAINER(mod->tmgWin), vbox);
    gtk_widget_show_all(mod->tmgWin);

    mod->tmgActive = TRUE;

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Time Controller for %s launched"), __func__, mod->name);
}

/*
 * Manage tmg window delete events
 *
 * Returns FALSE to indicate that window should be destroyed.
 */
static gint tmg_delete(GtkWidget * tmg, GdkEvent * event, gpointer data)
{
    (void)tmg;
    (void)event;
    (void)data;

    return FALSE;
}

static void tmg_destroy(GtkWidget * tmg, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);

    (void)tmg;

    /* reset time keeping variables */
    mod->throttle = 1;
    mod->tmgActive = FALSE;

    /* reset time */
    tmg_reset(NULL, data);

    sat_log_log(SAT_LOG_LEVEL_INFO,
                _("%s: Time Controller for %s closed. Time reset."),
                __func__, mod->name);
}

/*
 * Manage STOP signals.
 *
 * @param widget The GtkButton that received the signal.
 * @param data Pointer to the GtkSatModule widget.
 *
 * This function is called when the user clicks on the STOP button.
 * If the button state is active (pressed in) the function sets the
 * throttle parameter of the module to 0.
 */
static void tmg_stop(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, __func__);
        mod->throttle = 0;
    }
}

/*
 * Manage FWD signals.
 *
 * @param widget The GtkButton that received the signal.
 * @param data Pointer to the GtkSatModule widget.
 *
 * This function is called when the user clicks on the FWD button.
 * If the button state is active (pressed in) the function sets the
 *  throttle parameter of the module to 1.
 */
static void tmg_fwd(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, __func__);
        mod->throttle =
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mod->tmgFactor));
    }
}

/*
 * Manage back signals.
 *
 * @param widget The GtkButton that received the signal.
 * @param data Pointer to the GtkSatModule widget.
 *
 * This function is called when the user clicks on the BWD button.
 * If the button state is active (pressed in) the function sets the
 * throttle parameter of the module to -1.
 */
static void tmg_bwd(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, __func__);

        /* set throttle to -1 */
        mod->throttle =
            -gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mod->tmgFactor));
    }
}

/**
 * Manage Reset button clicks.
 *
 * @param widget The button that received the "clicked" signal.
 * @param data Pointer to the GtkSatModule structure.
 *
 * This function is called when the user clicks on the Reset button.
 * It resets the current time by setting 
 *
 *     mod->tmgPdnum = mod->rtPrev;
 *     mod->tmgCdnum = mod->rtNow;
 *
 */
static void tmg_reset(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);

    (void)widget;

    /* set reset flag in order to block
       widget signals */
    mod->reset = TRUE;

    mod->tmgPdnum = mod->rtPrev;
    mod->tmgCdnum = mod->rtNow;

    /* RESET slider */
    gtk_range_set_value(GTK_RANGE(mod->tmgSlider), 0.0);

    /* update widgets; widget signals will have no effect
       since the tmgReset flag is TRUE     */
    tmg_update_widgets(mod);

    /* clear reset flag */
    mod->reset = FALSE;
}

static void tmg_throttle(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);

    /* FFWD mode */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mod->tmgFwd)))
    {
        mod->throttle =
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mod->tmgBwd)))
    {
        mod->throttle =
            -gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    }
}

/**
 * Set new date and time callback.
 *
 * @param widget The widget that was modified.
 * @param data Pointer to the GtkSatModule structure.
 *
 * This function is called when the user changes the date or time in the time
 * controller. If we are in manual time control mode, the function reads the
 * date and time set in the control widget and calculates the new time for
 * the module. The function does nothing in real time and simulated real
 * time modes.
 */
static void tmg_time_set(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);
    gdouble         slider;
    gdouble         jd;

    (void)widget;

    /* update time only if we are in manual time control */
    if (!mod->throttle && !mod->reset)
    {
        jd = calculate_time(mod);

        /* get slider offset */
        slider = gtk_range_get_value(GTK_RANGE(mod->tmgSlider));

        mod->tmgCdnum = jd + slider;
    }
}

/**
 * Signal handler for slider "value-changed" signals
 *
 * @param widget The widget that was modified.
 * @param data Pointer to the GtkSatModule structure.
 *
 * This function is called when the user moves the slider.
 * If we are in manual control mode, the function simply calls
 * tmg_time_set(). In the other modes, the function switches over
 * to amnual mode first.
 */
static void slider_moved(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);

    if (mod->throttle)
    {
        /* "press" the stop button to trigger a transition into manual mode */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mod->tmgStop), TRUE);
    }

    tmg_time_set(widget, data);
}

/**
 * Hour controller wrap
 *
 * @param widget Pointer to the hour controller widget
 * @param data Pointer to the GtkSatModule structure.
 *
 * This function is called when the hour controller wraps around its
 * limits. The current day is incremented or decremented according to
 * the wrap direction, then the current time is updated.
 */
static void tmg_hour_wrap(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);
    gint            hour;

    (void)widget;

    hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mod->tmgHour));

    if (hour == 0)
    {
        /* 23 -> 0 wrap: increment date */
        tmg_cal_add_one_day(mod);
    }
    else
    {
        /* 0 -> 23 wrap: decrement data */
        tmg_cal_sub_one_day(mod);
    }
}

/**
 * Minute controller wrap
 *
 * @param widget Pointer to the minute controller widget
 * @param data Pointer to the GtkSatModule structure.
 *
 * This function is called when the minute controller wraps around its
 * limits. The current hour is incremented or decremented according to
 * the wrap direction, then the current time is updated.
 */
static void tmg_min_wrap(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);
    gint            hr, min;

    (void)widget;

    hr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mod->tmgHour));
    min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mod->tmgMin));

    if (min == 0)
    {
        /* 59 -> 0 wrap: increment hour */
        if (hr == 23)
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgHour), 0);
            tmg_hour_wrap(mod->tmgHour, data);
        }
        else
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgHour), hr + 1);
        }
    }
    else
    {
        /* 0 -> 59 wrap: decrement hour */
        if (hr == 0)
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgHour), 23);
            tmg_hour_wrap(mod->tmgHour, data);
        }
        else
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgHour), hr - 1);
        }
    }

    /* Note that the time will be updated automatically since the
       _set_value(hour) will trigger the "value-changed" signal */
}

/**
 * Seconds controller wrap
 *
 * @param widget Pointer to the seconds controller widget
 * @param data Pointer to the GtkSatModule structure.
 *
 * This function is called when the seconds controller wraps around its
 * limits. The current minute is incremented or decremented according to
 * the wrap direction, then the current time is updated.
 */
static void tmg_sec_wrap(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);
    gint            min, sec;

    (void)widget;

    sec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mod->tmgSec));
    min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mod->tmgMin));

    if (sec == 0)
    {
        /* 59 -> 0 wrap: increment minute */
        if (min == 59)
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgMin), 0);
            tmg_min_wrap(mod->tmgMin, data);
        }
        else
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgMin), min + 1);
        }
    }
    else
    {
        /* 0 -> 59 wrap: decrement minute */
        if (min == 0)
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgMin), 59);
            tmg_min_wrap(mod->tmgMin, data);
        }
        else
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgMin), min - 1);
        }
    }

    /* Note that the time will be updated automatically since the
       _set_value(minute) will trigger the "value-changed" signal */
}

/**
 * Millisecond controller wrap
 *
 * @param widget Pointer to the millisecond controller widget
 * @param data Pointer to the GtkSatmodule structure.
 *
 * This function is called when the muillisecond controller wraps around its
 * limits. The current second is incremented or decremented according to
 * the wrap direction, then the current time is updated.
 */
static void tmg_msec_wrap(GtkWidget * widget, gpointer data)
{
    GtkSatModule   *mod = GTK_SAT_MODULE(data);
    gint            msec, sec;

    (void)widget;

    sec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mod->tmgSec));
    msec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mod->tmgMsec));

    if (msec == 0)
    {
        /* 999 -> 0 wrap: increment seconds */
        if (sec == 59)
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgSec), 0);
            tmg_sec_wrap(mod->tmgSec, data);
        }
        else
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgSec), sec + 1);
        }
    }
    else
    {
        /* 0 -> 999 wrap: decrement seconds */
        if (sec == 0)
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgSec), 59);
            tmg_sec_wrap(mod->tmgSec, data);
        }
        else
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgSec), sec - 1);
        }
    }

    /* Note that the time will be updated automatically since the
       _set_value(sec) will trigger the "value-changed" signal */
}

/**
 * Update Time controller widgets
 *
 * @param mod Pointer to the GtkSatModule widget
 */
void tmg_update_widgets(GtkSatModule * mod)
{
    struct tm       tim;
    time_t          t;

    /* update time widgets */
    t = (mod->tmgCdnum - 2440587.5) * 86400.;

    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_LOCAL_TIME))
        tim = *localtime(&t);
    else
        tim = *gmtime(&t);

    /* hour, min, sec, msec */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgHour), tim.tm_hour);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgMin), tim.tm_min);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgSec), tim.tm_sec);

    /* msec: always 0 in RT and SRT modes */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mod->tmgMsec), 0);

    /* calendar */
    gtk_calendar_select_month(GTK_CALENDAR(mod->tmgCal),
                              tim.tm_mon, tim.tm_year + 1900);
    gtk_calendar_select_day(GTK_CALENDAR(mod->tmgCal), tim.tm_mday);
}

/**
 * Update state label.
 *
 * @param mod Pointer to the GtkSatModule widget
 *
 * This function is used to update the state label showing
 * whether the time control is in RT, SRT, or MAN mode.
 *
 */
void tmg_update_state(GtkSatModule * mod)
{
    if (mod->rtPrev != mod->tmgPdnum)
        if (mod->throttle)
            gtk_label_set_markup(GTK_LABEL(mod->tmgState),
                                 _("<b>Simulated Real-Time</b>"));
        else
            gtk_label_set_markup(GTK_LABEL(mod->tmgState),
                                 _("<b>Manual Control</b>"));
    else
        gtk_label_set_markup(GTK_LABEL(mod->tmgState), _("<b>Real-Time</b>"));
}

/** Add one day to the calendar */
static void tmg_cal_add_one_day(GtkSatModule * mod)
{
    gdouble         jd;

    jd = calculate_time(mod);
    jd += 1;
    mod->tmgCdnum = jd;
    tmg_update_widgets(mod);
}

/** Subtract one day from the calendar */
static void tmg_cal_sub_one_day(GtkSatModule * mod)
{
    gdouble         jd;

    jd = calculate_time(mod);
    jd -= 1;
    mod->tmgCdnum = jd;
    tmg_update_widgets(mod);
}


/**
 * Calculate the time as Julian day (and fraction) from the TMG widgets.
 *
 * @param mod Pointer to the GtkSatModule this time manager belongs to.
 * @returns The current date and time in Julian days and fraction of days.
 */
static gdouble calculate_time(GtkSatModule * mod)
{
    guint           year, month, day;
    gint            hr, min, sec, msec;
    struct tm       tim, utim;
    gdouble         jd = 0.0;

    /* get date and time from widgets */
    gtk_calendar_get_date(GTK_CALENDAR(mod->tmgCal), &year, &month, &day);
    hr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mod->tmgHour));
    min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mod->tmgMin));
    sec = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mod->tmgSec));
    msec = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mod->tmgMsec));

    /* build struct_tm */
    tim.tm_year = (int)(year);
    tim.tm_mon = (int)(month + 1);
    tim.tm_mday = (int)day;
    tim.tm_hour = (int)hr;
    tim.tm_min = (int)min;
    tim.tm_sec = (int)sec;

    sat_log_log(SAT_LOG_LEVEL_DEBUG,
                _("%s: %d/%d/%d %d:%d:%d.%d"),
                __func__,
                tim.tm_year, tim.tm_mon, tim.tm_mday,
                tim.tm_hour, tim.tm_min, tim.tm_sec, msec);

    /* convert UTC time to Julian Date  */
    if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_LOCAL_TIME))
    {
        /* convert local time to UTC */

        /* tm_mon -1 / +1 is a temporary workaround for
         * https://github.com/csete/gpredict/issues/90
         * Also see
         * https://github.com/csete/gpredict/issues/106
         */
        tim.tm_mon -= 1;
        Time_to_UTC(&tim, &utim);
        utim.tm_mon += 1;

        /* Convert to JD */
        jd = Julian_Date(&utim);
    }
    else
    {
        /* Already UTC, just convert to JD */
        jd = Julian_Date(&tim);
    }

    jd = jd + (gdouble) msec / 8.64e+7;

    return jd;
}
