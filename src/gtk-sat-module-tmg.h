/*
 * NOTE: This file is an internal part of gtk-sat-module and should not
 * be used by other files than gtk-sat-module.c and gtk-sat-module-popup.c
 */

#ifndef __GTK_SAT_MODULE_TMG_H__
#define __GTK_SAT_MODULE_TMG_H__ 1

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

void            tmg_create(GtkSatModule * mod);
void            tmg_update_widgets(GtkSatModule * mod);
void            tmg_update_state(GtkSatModule * mod);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* __GTK_SAT_MODULE_H__ */
