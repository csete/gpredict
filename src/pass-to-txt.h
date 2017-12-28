#ifndef PASS_TO_TXT_H
#define PASS_TO_TXT_H 1

#include <gtk/gtk.h>
#include "sat-pass-dialogs.h"
#include "predict-tools.h"
#include "gtk-sat-data.h"


gchar          *pass_to_txt_pgheader(pass_t * pass, qth_t * qth, gint fields);
gchar          *pass_to_txt_tblheader(pass_t * pass, qth_t * qth, gint fields);
gchar          *pass_to_txt_tblcontents(pass_t * pass, qth_t * qth,
                                        gint fields);

gchar          *passes_to_txt_pgheader(GSList * passes, qth_t * qth,
                                       gint fields);
gchar          *passes_to_txt_tblheader(GSList * passes, qth_t * qth,
                                        gint fields);
gchar          *passes_to_txt_tblcontents(GSList * passes, qth_t * qth,
                                          gint fields);


#endif
