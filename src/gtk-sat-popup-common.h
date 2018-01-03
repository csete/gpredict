#ifndef __GTK_SAT_POPUP_COMMON_H__
#define __GTK_SAT_POPUP_COMMON_H__ 1

#include "qth-data.h"
#include "sgpsdp/sgp4sdp4.h"

void            add_pass_menu_items(GtkWidget * menu, sat_t * sat,
                                    qth_t * qth, gdouble * tstamp,
                                    GtkWidget * widget);
void            show_current_pass_cb(GtkWidget * menuitem, gpointer data);
void            show_next_pass_cb(GtkWidget * menuitem, gpointer data);
void            show_future_passes_cb(GtkWidget * menuitem, gpointer data);
void            show_next_pass_dialog(sat_t * sat, qth_t * qth,
                                      gdouble tstamp, GtkWindow * toplevel);
void            show_future_passes_dialog(sat_t * sat, qth_t * qth,
                                          gdouble tstamp,
                                          GtkWindow * toplevel);

#endif
