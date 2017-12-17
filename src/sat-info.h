#ifndef SAT_INFO_H
#define SAT_INFO_H 1

#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "sgpsdp/sgp4sdp4.h"


void            show_sat_info_menu_cb(GtkWidget * menuitem, gpointer data);
void            show_sat_info(sat_t * sat, gpointer data);

#endif
