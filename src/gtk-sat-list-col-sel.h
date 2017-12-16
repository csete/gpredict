#ifndef __GTK_SAT_LIST_COL_SEL_H__
#define __GTK_SAT_LIST_COL_SEL_H__ 1

#include <gtk/gtk.h>
#include "gtk-sat-list.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define GTK_TYPE_SAT_LIST_COL_SEL  (gtk_sat_list_col_sel_get_type ())
#define GTK_SAT_LIST_COL_SEL(obj)  G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                   gtk_sat_list_col_sel_get_type (),\
                                   GtkSatListColSel)

#define GTK_SAT_LIST_COL_SEL_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                    gtk_sat_list_col_sel_get_type (),\
                                    GtkSatListColSelClass)

#define IS_GTK_SAT_LIST_COL_SEL(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sat_list_col_sel_get_type ())

typedef struct _gtk_sat_list_col_sel GtkSatListColSel;
typedef struct _GtkSatListColSelClass GtkSatListColSelClass;

struct _gtk_sat_list_col_sel {
    GtkVBox         vbox;

    GtkWidget      *list;       /*!< the list containing the toggles */
    GtkWidget      *swin;
    guint32         flags;      /*!< Flags indicating which boxes are checked */
    gulong          handler_id;
};

struct _GtkSatListColSelClass {
    GtkVBoxClass    parent_class;
};


GType           gtk_sat_list_col_sel_get_type(void);
GtkWidget      *gtk_sat_list_col_sel_new(guint32 flags);
guint32         gtk_sat_list_col_sel_get_flags(GtkSatListColSel * sel);
void            gtk_sat_list_col_sel_set_flags(GtkSatListColSel * sel,
                                               guint32 flags);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
