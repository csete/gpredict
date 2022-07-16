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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include "gtk-rot-knob.h"

#define FMTSTR "<span size='xx-large'>%c</span>"


static GtkHBoxClass *parent_class = NULL;

/* Convert digit index (in the digit array) to amount of change. */
const gdouble   INDEX_TO_DELTA[] = { 0.0, 100.0, 10.0, 1.0, 0.0, 0.1, 0.01 };


static void gtk_rot_knob_destroy(GtkWidget * widget)
{
    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_rot_knob_class_init(GtkRotKnobClass * class,
				    gpointer class_data)
{
    GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

    (void)class_data;

    widget_class->destroy = gtk_rot_knob_destroy;

    parent_class = g_type_class_peek_parent(class);
}

static void gtk_rot_knob_init(GtkRotKnob * knob,
			      gpointer g_class)
{
    (void)knob;
    (void)g_class;
}

/*
 * Update rotor display widget.
 *
 * @param knob The rotor control widget.
 * 
 */
static void gtk_rot_knob_update(GtkRotKnob * knob)
{
    gchar           b[7];
    gchar          *buff;
    guint           i;

    g_ascii_formatd(b, 8, "%6.2f", fabs(knob->value));

    /* set label markups */
    for (i = 0; i < 6; i++)
    {
        buff = g_strdup_printf(FMTSTR, b[i]);
        gtk_label_set_markup(GTK_LABEL(knob->digits[i + 1]), buff);
        g_free(buff);
    }

    if (knob->value < 0)
        buff = g_strdup_printf(FMTSTR, '-');
    else
        buff = g_strdup_printf(FMTSTR, ' ');

    gtk_label_set_markup(GTK_LABEL(knob->digits[0]), buff);
    g_free(buff);
}

/*
 * Button clicked event.
 *
 * @param button The button that was clicked.
 * @param data Pointer to the GtkRotKnob widget.
 * 
 */
static void button_clicked_cb(GtkWidget * button, gpointer data)
{
    GtkRotKnob     *knob = GTK_ROT_KNOB(data);
    gdouble         delta =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "delta")) / 100.0;

    if ((delta > 0.0) && ((knob->value + delta) <= knob->max + .005))
    {
        knob->value += delta;
        if (knob->value > knob->max)
        {
            knob->value = knob->max;
        }
    }
    else if ((delta < 0.0) && ((knob->value + delta) >= knob->min - .005))
    {
        knob->value += delta;
        if (knob->value < knob->min)
        {
            knob->value = knob->min;
        }
    }

    gtk_rot_knob_update(knob);
}

/*
 * Manage button press events
 *
 * @param digit Pointer to the event box that received the event
 * @param event Pointer to the GdkEventButton that contains details for te event
 * @param data Pointer to the GtkRotKnob widget (we need it to update the value)
 * @return Always TRUE to prevent further propagation of the event
 *
 * This function is called when a mouse button is pressed on a digit. This is used
 * to increment or decrement the value:
 * - Left button: up
 * - Right button: down
 * - Middle button: set digit to 0 (TBC)
 * 
 * Wheel up/down are managed in a separate callback since these are treated as scroll events
 * rather than button press events (they used to be button press events though)
 * 
 * The digit labels are stored in an array that also contains the sign and the
 * decimal separator. To get the amount of change corresponding to the clicked label we
 * can use the INDEX_TO_DELTA[] array. The index to this table is attached to the evtbox.
 * Whether the delta is positive or negative depends on which mouse button triggered the event.
 */
static gboolean on_button_press(GtkWidget * evtbox,
                                GdkEventButton * event, gpointer data)
{
    GtkRotKnob     *knob = GTK_ROT_KNOB(data);
    guint           idx =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(evtbox), "index"));
    gdouble         delta = INDEX_TO_DELTA[idx];
    gdouble         value;

    if (delta < 0.01)
    {
        /* no change, user clicked on sign or decimal separator */
        return TRUE;
    }

    if (event->type != GDK_BUTTON_PRESS)
    {
        /* wrong event (not possible?) */
        return TRUE;
    }

    switch (event->button)
    {
        /* left button */
    case 1:
        value = gtk_rot_knob_get_value(knob) + delta;
        gtk_rot_knob_set_value(knob, value);
        break;

        /* middle button */
    case 2:
        break;

        /* right button */
    case 3:
        value = gtk_rot_knob_get_value(knob) - delta;
        gtk_rot_knob_set_value(knob, value);
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
 * @param data Pointer to the GtkRotKnob widget (we need it to update the value)
 * @return Always TRUE to prevent further propagation of the event
 *
 * This function is called when the mouse wheel is moved up or down. This is used to increment
 * or decrement the value.
 * 
 * Button presses are managed in a separate callback since these are treated as different
 * events.
 * 
 * The digit labels are stored in an array that also contains the sign and the
 * decimal separator. To get the amount of change corresponding to the clicked label we
 * can use the INDEX_TO_DELTA[] array. The index is attached to the evtbox.
 * Whether the delta is positive or negative depends on the scroll direction.
 */
static gboolean on_button_scroll(GtkWidget * evtbox,
                                 GdkEventScroll * event, gpointer data)
{
    GtkRotKnob     *knob = GTK_ROT_KNOB(data);
    guint           idx =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(evtbox), "index"));
    gdouble         delta = INDEX_TO_DELTA[idx];
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
        value = gtk_rot_knob_get_value(knob) - delta;
        gtk_rot_knob_set_value(knob, value);
        break;

        /* increase value by delta */
    case GDK_SCROLL_UP:
    case GDK_SCROLL_RIGHT:
        value = gtk_rot_knob_get_value(knob) + delta;
        gtk_rot_knob_set_value(knob, value);
        break;

    default:
        break;
    }

    return TRUE;
}

GType gtk_rot_knob_get_type()
{
    static GType    gtk_rot_knob_type = 0;

    if (!gtk_rot_knob_type)
    {
        static const GTypeInfo gtk_rot_knob_info = {
            sizeof(GtkRotKnobClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_rot_knob_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkRotKnob),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_rot_knob_init,
            NULL
        };

        gtk_rot_knob_type = g_type_register_static(GTK_TYPE_BOX,
                                                   "GtkRotKnob",
                                                   &gtk_rot_knob_info, 0);
    }

    return gtk_rot_knob_type;
}

/*
 * Set the range of the control widget
 *
 * @param knob The rotor control widget.
 * @param min The new lower limit of the control widget.
 * @param max The new upper limit of the control widget.
 */
void gtk_rot_knob_set_range(GtkRotKnob * knob, gdouble min, gdouble max)
{
    gtk_rot_knob_set_min(knob, min);
    gtk_rot_knob_set_max(knob, max);
}

/*
 * Set the value of the rotor control widget.
 *
 * @param knob The rotor control widget.
 * @param val The new value.
 * 
 */
void gtk_rot_knob_set_value(GtkRotKnob * knob, gdouble val)
{
    /* set the new value */
    if (val <= knob->min)
        knob->value = knob->min;
    else if (val >= knob->max)
        knob->value = knob->max;
    else
        knob->value = val;

    /* update the display */
    gtk_rot_knob_update(knob);
}

/*
 * Get the current value of the rotor control widget.
 *
 * @param knob The rotor control widget.
 * @return The current value.
 * 
 * Hint: For reading the value you can also access knob->value.
 * 
 */
gdouble gtk_rot_knob_get_value(GtkRotKnob * knob)
{
    return knob->value;
}

/*
 * Get the upper limit of the control widget
 *
 * @param knob The rotor control widget.
 * @return The upper limit of the control widget.
 */
gdouble gtk_rot_knob_get_max(GtkRotKnob * knob)
{
    return knob->max;
}

/*
 * Get the lower limit of the control widget
 *
 * @param knob The rotor control widget.
 * @return The lower limit of the control widget.
 */
gdouble gtk_rot_knob_get_min(GtkRotKnob * knob)
{
    return knob->min;
}

/*
 * Set the lower limit of the control widget
 *
 * @param knob The rotor control widget.
 * @param min The new lower limit of the control widget.
 */
void gtk_rot_knob_set_min(GtkRotKnob * knob, gdouble min)
{
    /* just some sanity check we have only 3 digits */
    if (min < 1000)
    {
        knob->min = min;

        /* ensure that current value is within range */
        if (knob->value < knob->min)
        {
            knob->value = knob->min;
            gtk_rot_knob_update(knob);
        }
    }
}

/*
 * Set the upper limit of the control widget
 *
 * @param knob The rotor control widget.
 * @param min The new upper limit of the control widget.
 */
void gtk_rot_knob_set_max(GtkRotKnob * knob, gdouble max)
{
    /* just some sanity check we have only 3 digits */
    if (max < 1000)
    {
        knob->max = max;

        /* ensure that current value is within range */
        if (knob->value > knob->max)
        {
            knob->value = knob->max;
            gtk_rot_knob_update(knob);
        }
    }
}

/*
 * Create a new rotor control widget.
 *
 * @param min The lower limit in decimal degrees.
 * @param max The upper limit in decimal degrees.
 * @param val The initial value of the control.
 * @return A new rotor control widget.
 * 
 */
GtkWidget      *gtk_rot_knob_new(gdouble min, gdouble max, gdouble val)
{
    GtkRotKnob     *knob;
    GtkWidget      *widget;
    GtkWidget      *table;
    GtkWidget      *label;
    guint           i;

    widget = g_object_new(GTK_TYPE_ROT_KNOB, NULL);
    knob = GTK_ROT_KNOB(widget);

    knob->min = min;
    knob->max = max;
    knob->value = val;

    table = gtk_grid_new();

    /* +100 deg */
    knob->buttons[0] = gtk_button_new_with_label("\342\226\264");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[0]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[0]),
                      "delta", GINT_TO_POINTER(10000));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[0], 1, 0, 1, 1);
    g_signal_connect(knob->buttons[0], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* +10 deg */
    knob->buttons[1] = gtk_button_new_with_label("\342\226\264");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[1]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[1]),
                      "delta", GINT_TO_POINTER(1000));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[1], 2, 0, 1, 1);
    g_signal_connect(knob->buttons[1], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* +1 deg */
    knob->buttons[2] = gtk_button_new_with_label("\342\226\264");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[2]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[2]),
                      "delta", GINT_TO_POINTER(100));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[2], 3, 0, 1, 1);
    g_signal_connect(knob->buttons[2], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* +0.1 deg */
    knob->buttons[3] = gtk_button_new_with_label("\342\226\264");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[3]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[3]),
                      "delta", GINT_TO_POINTER(10));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[3], 5, 0, 1, 1);
    g_signal_connect(knob->buttons[3], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* +0.01 deg */
    knob->buttons[4] = gtk_button_new_with_label("\342\226\264");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[4]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[4]), "delta", GINT_TO_POINTER(1));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[4], 6, 0, 1, 1);
    g_signal_connect(knob->buttons[4], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* -100 deg */
    knob->buttons[5] = gtk_button_new_with_label("\342\226\276");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[5]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[5]),
                      "delta", GINT_TO_POINTER(-10000));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[5], 1, 2, 1, 1);
    g_signal_connect(knob->buttons[5], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* -10 deg */
    knob->buttons[6] = gtk_button_new_with_label("\342\226\276");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[6]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[6]),
                      "delta", GINT_TO_POINTER(-1000));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[6], 2, 2, 1, 1);
    g_signal_connect(knob->buttons[6], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* -1 deg */
    knob->buttons[7] = gtk_button_new_with_label("\342\226\276");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[7]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[7]),
                      "delta", GINT_TO_POINTER(-100));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[7], 3, 2, 1, 1);
    g_signal_connect(knob->buttons[7], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* -0.1 deg */
    knob->buttons[8] = gtk_button_new_with_label("\342\226\276");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[8]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[8]),
                      "delta", GINT_TO_POINTER(-10));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[8], 5, 2, 1, 1);
    g_signal_connect(knob->buttons[8], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* -0.01 deg */
    knob->buttons[9] = gtk_button_new_with_label("\342\226\276");
    gtk_button_set_relief(GTK_BUTTON(knob->buttons[9]), GTK_RELIEF_NONE);
    g_object_set_data(G_OBJECT(knob->buttons[9]),
                      "delta", GINT_TO_POINTER(-1));
    gtk_grid_attach(GTK_GRID(table), knob->buttons[9], 6, 2, 1, 1);
    g_signal_connect(knob->buttons[9], "clicked",
                     G_CALLBACK(button_clicked_cb), widget);

    /* create labels */
    for (i = 0; i < 7; i++)
    {
        /* labels showing the digits */
        knob->digits[i] = gtk_label_new(NULL);
        gtk_widget_set_tooltip_text(knob->digits[i],
                                    _
                                    ("Use mouse buttons and wheel to change value"));

        /* Event boxes for catching mouse evetns */
        knob->evtbox[i] = gtk_event_box_new();
        g_object_set_data(G_OBJECT(knob->evtbox[i]), "index",
                          GUINT_TO_POINTER(i));
        gtk_container_add(GTK_CONTAINER(knob->evtbox[i]), knob->digits[i]);
        gtk_grid_attach(GTK_GRID(table), knob->evtbox[i], i, 1, 1, 1);

        g_signal_connect(knob->evtbox[i], "button_press_event",
                         (GCallback) on_button_press, widget);
        g_signal_connect(knob->evtbox[i], "scroll_event",
                         (GCallback) on_button_scroll, widget);

    }

    /* degree sign */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<span size='xx-large'>\302\260</span>");
    gtk_grid_attach(GTK_GRID(table), label, 7, 1, 1, 1);
    gtk_rot_knob_update(knob);
    gtk_container_add(GTK_CONTAINER(widget), table);
    gtk_widget_show_all(widget);

    return widget;
}
