#ifndef PASS_POPUP_MENU_H
#define PASS_POPUP_MENU_H 1

#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "predict-tools.h"
#include "sgpsdp/sgp4sdp4.h"

void            pass_popup_menu_exec(qth_t * qth, pass_t * pass,
                                     GdkEventButton * event,
                                     GtkWidget * toplevel);

#endif
