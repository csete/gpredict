/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC
  Copyright (C)       2017  Patrick Dohmen, DL4PD
 
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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include "gtk-freq-knob.h"


G_LOCK_DEFINE_STATIC(updatelock);

static GtkHBoxClass *parent_class = NULL;

#define FMTSTR "<span size='xx-large'>%c</span>"

/* x-index in table for buttons and labels */
static const guint idx[] = {
    0,
    2,
    3,
    4,
    6,
    7,
    8,
    10,
    11,
    12
};

static guint    freq_changed_signal = 0;


static void gtk_freq_knob_destroy(GtkWidget * widget)
{
    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_freq_knob_class_init(GtkFreqKnobClass * class,
				     gpointer class_init)
{
    GtkWidgetClass *widget_class;

    (void)class_init;

    widget_class = (GtkWidgetClass *) class;
    parent_class = g_type_class_peek_parent(class);
    widget_class->destroy = gtk_freq_knob_destroy;

    /* create freq changed signal */
    freq_changed_signal = g_signal_new("freq-changed",
                                       G_TYPE_FROM_CLASS(class),
                                       G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                       0,
                                       NULL,
                                       NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE, 0);
}

static void gtk_freq_knob_init(GtkFreqKnob * knob,
			       gpointer g_class)
{
    (void)g_class;

    knob->min = 0.0;
    knob->max = 9999999999.0;
}

static gboolean gtk_freq_knob_update(GtkFreqKnob * knob)
{
    gchar           b[11];
    gchar          *buff;
    guint           i;

    G_LOCK(updatelock);
    g_ascii_formatd(b, 11, "%10.0f", fabs(knob->value));

    /* set label markups */
    for (i = 0; i < 10; i++)
    {
        buff = g_strdup_printf(FMTSTR, b[i]);
        gtk_label_set_markup(GTK_LABEL(knob->digits[i]), buff);
        g_free(buff);
    }

    G_UNLOCK(updatelock);

    return FALSE;
}

static void button_clicked_cb(GtkWidget * button, gpointer data)
{
    GtkFreqKnob    *knob = GTK_FREQ_KNOB(data);
    gdouble         delta =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "delta"));

    if ((delta > 0.0) && ((knob->value + delta) <= knob->max))
    {
        knob->value += delta;
    }
    else if ((delta < 0.0) && ((knob->value + delta) >= knob->min))
    {
        knob->value += delta;
    }

    g_idle_add((GSourceFunc) gtk_freq_knob_update, knob);

    /* emit "freq_changed" signal */
    g_signal_emit(G_OBJECT(data), freq_changed_signal, 0);
}

/*
 * Manage button press events
 *
 * @param digit Pointer to the event box that received the event
 * @param event Pointer to the GdkEventButton that contains details for te event
 * @param data Pointer to the GtkFreqKnob widget (we need it to update the value)
 *
 * Always returns TRUE to prevent further propagation of the event
 *
 * This function is called when a mouse button is pressed on a digit. This is used
 * to increment or decrement the value:
 * - Left button: up
 * - Right button: down
 * - Middle button: set digit to 0 (TBC)
 * 
 * Wheel up/down are managed in a separate callback since these are treated as
 * scroll events rather than button press events (they used to be button press
 * events though)
 * 
 * The digit labels are stored in an array. To get the amount of change corresponding
 * to the clicked label we can convert the index (attached to the evtbox):
 * delta = 10^(9-index)
 *
 * Whether the delta is positive or negative depends on which mouse button triggered
 * the event.
 */
static gboolean on_button_press(GtkWidget * evtbox,
                                GdkEventButton * event, gpointer data)
{
    GtkFreqKnob    *knob = GTK_FREQ_KNOB(data);
    guint           idx =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(evtbox), "index"));
    gdouble         delta = pow(10, 9 - idx);
    gdouble         value;


    if (delta < 1.0)            // no change
        return TRUE;

    if (event->type != GDK_BUTTON_PRESS)
        return TRUE;

    switch (event->button)
    {
        /* left button */
    case 1:
        value = gtk_freq_knob_get_value(knob) + delta;
        gtk_freq_knob_set_value(knob, value);
        g_signal_emit(G_OBJECT(data), freq_changed_signal, 0);
        break;

        /* middle button */
    case 2:
        break;

        /* right button */
    case 3:
        value = gtk_freq_knob_get_value(knob) - delta;
        gtk_freq_knob_set_value(knob, value);
        g_signal_emit(G_OBJECT(data), freq_changed_signal, 0);
        break;

    default:
        break;
    }

    return TRUE;
}

/*
 * Manage scroll wheel events
 *
 * @param digit Pointer to the event box that received the event
 * @param event Pointer to the GdkEventScroll that contains details for te event
 * @param data Pointer to the GtkFreqKnob widget (we need it to update the value)
 *
 * Always returns TRUE to prevent further propagation of the event
 *
 * This function is called when the mouse wheel is moved up or down. This is used
 * to increment or decrement the value.
 * 
 * Button presses are managed in a separate callback since these are treated as
 * different events.
 * 
 * The digit labels are stored in an array. To get the amount of change corresponding
 * to the clicked label we can convert the index (attached to the evtbox):
 * delta = 10^(9-index)
 *
 * Whether the delta is positive or negative depends on the scroll direction.
 */
static gboolean on_button_scroll(GtkWidget * evtbox,
                                 GdkEventScroll * event, gpointer data)
{
    GtkFreqKnob    *knob = GTK_FREQ_KNOB(data);
    guint           idx =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(evtbox), "index"));
    gdouble         delta = pow(10, 9 - idx);
    gdouble         value;


    if (delta < 0.01)
    {
        /* no change, user clicked on sign or decimal separator */
        return TRUE;
    }

    if (event->type != GDK_SCROLL)
    {
        /* wrong event (not possible?) */
        return TRUE;
    }

    switch (event->direction)
    {

        /* decrease value by delta */
    case GDK_SCROLL_DOWN:
    case GDK_SCROLL_LEFT:
        value = gtk_freq_knob_get_value(knob) - delta;
        gtk_freq_knob_set_value(knob, value);
        g_signal_emit(G_OBJECT(data), freq_changed_signal, 0);
        break;

        /* increase value by delta */
    case GDK_SCROLL_UP:
    case GDK_SCROLL_RIGHT:
        value = gtk_freq_knob_get_value(knob) + delta;
        gtk_freq_knob_set_value(knob, value);
        g_signal_emit(G_OBJECT(data), freq_changed_signal, 0);
        break;

    default:
        break;
    }

    return TRUE;
}

void gtk_freq_knob_set_value(GtkFreqKnob * knob, gdouble val)
{
    if ((val >= knob->min) && (val <= knob->max))
    {
        knob->value = val;
        g_idle_add((GSourceFunc) gtk_freq_knob_update, knob);
    }
}

gdouble gtk_freq_knob_get_value(GtkFreqKnob * knob)
{
    return knob->value;
}

GType gtk_freq_knob_get_type()
{
    static GType    gtk_freq_knob_type = 0;

    if (!gtk_freq_knob_type)
    {
        static const GTypeInfo gtk_freq_knob_info = {
            sizeof(GtkFreqKnobClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_freq_knob_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkFreqKnob),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_freq_knob_init,
            NULL,
        };

        gtk_freq_knob_type = g_type_register_static(GTK_TYPE_BOX,
                                                    "GtkFreqKnob",
                                                    &gtk_freq_knob_info, 0);
    }

    return gtk_freq_knob_type;
}

GtkWidget      *gtk_freq_knob_new(gdouble val, gboolean buttons)
{
    GtkFreqKnob    *knob;
    GtkWidget      *widget;
    GtkWidget      *table;
    GtkWidget      *label;
    guint           i;
    gint            delta;


    widget = g_object_new(GTK_TYPE_FREQ_KNOB, NULL);
    knob = GTK_FREQ_KNOB(widget);
    knob->value = val;

    table = gtk_grid_new();

    /* create buttons and labels */
    for (i = 0; i < 10; i++)
    {
        /* labels */
        knob->digits[i] = gtk_label_new(NULL);

        if (!buttons)
        {
            /* passive display widget without event boxes or buttons */
            gtk_grid_attach(GTK_GRID(table), knob->digits[i], idx[i], 1, 1, 1);
        }
        else
        {
            /* active widget that allows changing the value */

            gtk_widget_set_tooltip_text(knob->digits[i],
                                        _
                                        ("Use mouse buttons and wheel to change value"));

            /* Event boxes for catching mouse evetns */
            knob->evtbox[i] = gtk_event_box_new();
            g_object_set_data(G_OBJECT(knob->evtbox[i]),
                              "index", GUINT_TO_POINTER(i));
            gtk_container_add(GTK_CONTAINER(knob->evtbox[i]), knob->digits[i]);
            gtk_grid_attach(GTK_GRID(table), knob->evtbox[i], idx[i], 1, 1, 1);

            g_signal_connect(knob->evtbox[i],
                             "button_press_event", (GCallback) on_button_press,
                             widget);
            g_signal_connect(knob->evtbox[i], "scroll_event",
                             (GCallback) on_button_scroll, widget);


            /* UP buttons */
            knob->buttons[i] = gtk_button_new();

            label = gtk_label_new("\342\226\264");
            gtk_container_add(GTK_CONTAINER(knob->buttons[i]), label);
            gtk_button_set_relief(GTK_BUTTON(knob->buttons[i]),
                                  GTK_RELIEF_NONE);
            delta = (gint) pow(10, 9 - i);
            g_object_set_data(G_OBJECT(knob->buttons[i]),
                              "delta", GINT_TO_POINTER(delta));
            gtk_grid_attach(GTK_GRID(table), knob->buttons[i], idx[i], 0, 1,
                            1);
            g_signal_connect(knob->buttons[i], "clicked",
                             G_CALLBACK(button_clicked_cb), widget);

            /* DOWN buttons */
            knob->buttons[i + 10] = gtk_button_new();

            label = gtk_label_new("\342\226\276");
            gtk_container_add(GTK_CONTAINER(knob->buttons[i + 10]), label);
            gtk_button_set_relief(GTK_BUTTON(knob->buttons[i + 10]),
                                  GTK_RELIEF_NONE);
            g_object_set_data(G_OBJECT(knob->buttons[i + 10]),
                              "delta", GINT_TO_POINTER(-delta));
            gtk_grid_attach(GTK_GRID(table), knob->buttons[i + 10],
                            idx[i], 2, 1, 1);
            g_signal_connect(knob->buttons[i + 10], "clicked",
                             G_CALLBACK(button_clicked_cb), widget);
        }
    }

    /* Add misc labels */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span size='xx-large'>.</span>");
    gtk_grid_attach(GTK_GRID(table), label, 5, 1, 1, 1);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span size='xx-large'>.</span>");
    gtk_grid_attach(GTK_GRID(table), label, 9, 1, 1, 1);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span size='xx-large'> Hz</span>");
    gtk_grid_attach(GTK_GRID(table), label, 13, 1, 1, 1);

    g_idle_add((GSourceFunc) gtk_freq_knob_update, knob);

    gtk_container_add(GTK_CONTAINER(widget), table);
    gtk_widget_show_all(widget);

    return widget;
}
