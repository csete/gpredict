#ifndef SAT_PREF_REFRESH_H
#define SAT_PREF_REFRESH_H 1

GtkWidget      *sat_pref_refresh_create(GKeyFile * cfg);
void            sat_pref_refresh_cancel(GKeyFile * cfg);
void            sat_pref_refresh_ok(GKeyFile * cfg);

#endif
