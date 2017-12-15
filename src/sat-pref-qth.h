#ifndef SAT_PREF_QTH_H
#define SAT_PREF_QTH_H 1

GtkWidget      *sat_pref_qth_create(void);
void            sat_pref_qth_cancel(void);
void            sat_pref_qth_ok(void);

/* external hooks */
void            sat_pref_qth_sys_changed(gboolean imperial);

#endif
