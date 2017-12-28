#ifndef __GTK_ROT_KNOB_H__
#define __GTK_ROT_KNOB_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */


#define GTK_TYPE_ROT_KNOB          (gtk_rot_knob_get_type ())
#define GTK_ROT_KNOB(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                       gtk_rot_knob_get_type (),\
                                         GtkRotKnob)
#define GTK_ROT_KNOB_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                    gtk_rot_knob_get_type (),\
                                    GtkRotKnobClass)
#define IS_GTK_ROT_KNOB(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_rot_knob_get_type ())


typedef struct _gtk_rot_knob GtkRotKnob;
typedef struct _GtkRotKnobClass GtkRotKnobClass;

struct _gtk_rot_knob {
    GtkBox          box;

    GtkWidget      *digits[7];  /*!< Labels for the digits */
    GtkWidget      *evtbox[7];  /*!< Event boxes to catch mouse events over the digits */
    GtkWidget      *buttons[10];        /*!< Buttons; 0..4 up; 5..9 down */

    gdouble         min;
    gdouble         max;
    gdouble         value;
};

struct _GtkRotKnobClass {
    GtkVBoxClass    parent_class;
};

GType           gtk_rot_knob_get_type(void);
GtkWidget      *gtk_rot_knob_new(gdouble min, gdouble max, gdouble val);
void            gtk_rot_knob_set_value(GtkRotKnob * knob, gdouble val);
gdouble         gtk_rot_knob_get_value(GtkRotKnob * knob);
void            gtk_rot_knob_set_max(GtkRotKnob * knob, gdouble max);
gdouble         gtk_rot_knob_get_max(GtkRotKnob * knob);
gdouble         gtk_rot_knob_get_min(GtkRotKnob * knob);
void            gtk_rot_knob_set_min(GtkRotKnob * knob, gdouble min);
void            gtk_rot_knob_set_max(GtkRotKnob * knob, gdouble max);
void            gtk_rot_knob_set_range(GtkRotKnob * knob, gdouble min,
                                       gdouble max);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* __GTK_ROT_knob_H__ */
