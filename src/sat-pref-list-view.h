#ifndef SAT_PREF_LIST_VIEW_H
#define SAT_PREF_LIST_VIEW_H 1

GtkWidget      *sat_pref_list_view_create(GKeyFile * cfg);
void            sat_pref_list_view_cancel(GKeyFile * cfg);
void            sat_pref_list_view_ok(GKeyFile * cfg);

#endif
