#ifndef __GTK_FREQ_KNOB_H__
#define __GTK_FREQ_KNOB_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* *INDENT-ON* */


#define GTK_TYPE_FREQ_KNOB          (gtk_freq_knob_get_type ())
#define GTK_FREQ_KNOB(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                    gtk_freq_knob_get_type (),\
                                    GtkFreqKnob)

#define GTK_FREQ_KNOB_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                    gtk_freq_knob_get_type (),\
                                    GtkFreqKnobClass)

#define IS_GTK_FREQ_KNOB(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_freq_knob_get_type ())


typedef struct _gtk_freq_knob GtkFreqKnob;
typedef struct _GtkFreqKnobClass GtkFreqKnobClass;

struct _gtk_freq_knob {
    GtkVBox         vbox;

    GtkWidget      *digits[10]; /*!< Labels for the digits */
    GtkWidget      *evtbox[10]; /*!< Event boxes to catch mouse events over the digits */
    GtkWidget      *buttons[20];        /*!< Buttons; 0..9 up; 10..19 down */

    gdouble         min;
    gdouble         max;
    gdouble         value;
};

struct _GtkFreqKnobClass {
    GtkVBoxClass    parent_class;
};

GType           gtk_freq_knob_get_type(void);
GtkWidget      *gtk_freq_knob_new(gdouble val, gboolean buttons);
void            gtk_freq_knob_set_value(GtkFreqKnob * knob, gdouble val);
gdouble         gtk_freq_knob_get_value(GtkFreqKnob * knob);


/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif /* __cplusplus */
/* *INDENT-ON* */

#endif /* __GTK_FREQ_KNOB_H__ */
