#ifndef SAT_PREF_MAP_VIEW_H
#define SAT_PREF_MAP_VIEW_H 1

GtkWidget      *sat_pref_map_view_create(GKeyFile * cfg);
void            sat_pref_map_view_cancel(GKeyFile * cfg);
void            sat_pref_map_view_ok(GKeyFile * cfg);

#endif
