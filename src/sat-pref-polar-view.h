#ifndef SAT_PREF_POLAR_VIEW_H
#define SAT_PREF_POLAR_VIEW_H 1

GtkWidget      *sat_pref_polar_view_create(GKeyFile * cfg);
void            sat_pref_polar_view_cancel(GKeyFile * cfg);
void            sat_pref_polar_view_ok(GKeyFile * cfg);

#endif
