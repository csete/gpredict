#ifndef SAT_PREF_LAYOUT_H
#define SAT_PREF_LAYOUT_H 1

GtkWidget      *sat_pref_layout_create(GKeyFile * cfg);
void            sat_pref_layout_cancel(GKeyFile * cfg);
void            sat_pref_layout_ok(GKeyFile * cfg);

#endif
