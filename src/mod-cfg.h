#ifndef MOD_CFG_H
#define MOD_CFG_H 1

#include <glib.h>

typedef enum {
    MOD_CFG_OK = 0,
    MOD_CFG_CANCEL,
    MOD_CFG_ERROR
} mod_cfg_status_t;

gchar          *mod_cfg_new(void);
mod_cfg_status_t mod_cfg_edit(gchar * modname, GKeyFile * cfgdata,
                              GtkWidget * toplevel);
mod_cfg_status_t mod_cfg_save(gchar * modname, GKeyFile * cfgdata);
mod_cfg_status_t mod_cfg_delete(gchar * modname, gboolean needcfm);

#endif
