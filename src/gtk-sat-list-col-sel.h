/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>

    Comments, questions and bugreports should be submitted via
    http://sourceforge.net/projects/gpredict/
    More details can be found at the project home page:

            http://gpredict.oz9aec.net/
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License
    along with this program; if not, visit http://www.fsf.org/
*/
#ifndef __GTK_SAT_LIST_COL_SEL_H__
#define __GTK_SAT_LIST_COL_SEL_H__ 1

#include <gtk/gtk.h>
#include "gtk-sat-list.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_SAT_LIST_COL_SEL  (gtk_sat_list_col_sel_get_type ())
#define GTK_SAT_LIST_COL_SEL(obj)  GTK_CHECK_CAST (obj,\
                                 gtk_sat_list_col_sel_get_type (),\
                                 GtkSatListColSel)

#define GTK_SAT_LIST_COL_SEL_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass,\
                                    gtk_sat_list_col_sel_get_type (),\
                                    GtkSatListColSelClass)

#define IS_GTK_SAT_LIST_COL_SEL(obj)       GTK_CHECK_TYPE (obj, gtk_sat_list_col_sel_get_type ())



typedef struct _gtk_sat_list_col_sel  GtkSatListColSel;
typedef struct _GtkSatListColSelClass GtkSatListColSelClass;


struct _gtk_sat_list_col_sel
{
     GtkVBox vbox;

     GtkWidget   *list;     /*!< the list containing the toggles */
     GtkWidget   *swin;
     guint32      flags;    /*!< Flags indicating which boxes are checked */
     gulong     handler_id;
};

struct _GtkSatListColSelClass
{
     GtkVBoxClass parent_class;
};


GtkType    gtk_sat_list_col_sel_get_type  (void);
GtkWidget *gtk_sat_list_col_sel_new       (guint32 flags);
guint32    gtk_sat_list_col_sel_get_flags (GtkSatListColSel *sel);
void       gtk_sat_list_col_sel_set_flags (GtkSatListColSel *sel, guint32 flags);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
