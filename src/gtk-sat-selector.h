/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

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
#ifndef __GTK_SAT_SELECTOR_H__
#define __GTK_SAT_SELECTOR_H__ 1

#include <gtk/gtk.h>
//#include "gtk-sat-list.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** \brief Column definitions in the tree. */
typedef enum {
    GTK_SAT_SELECTOR_COL_NAME = 0,  /*!< Satellite name. */
    GTK_SAT_SELECTOR_COL_CATNUM,    /*!< Catalogue Number. */
    GTK_SAT_SELECTOR_COL_EPOCH,     /*!< Element set epoch. */
    GTK_SAT_SELECTOR_COL_SELECTED,  /*!< Track whether element is selected. */
    GTK_SAT_SELECTOR_COL_NUM        /*!< The number of columns. */
} gtk_sat_selector_col_t;


/** \brief Flags used to indicate which columns should be visible. */
typedef enum {
    GTK_SAT_SELECTOR_FLAG_NAME   = 1 << GTK_SAT_SELECTOR_COL_NAME,   /*!< Satellite name. */
    GTK_SAT_SELECTOR_FLAG_CATNUM = 1 << GTK_SAT_SELECTOR_COL_CATNUM, /*!< Catalogue Number. */
    GTK_SAT_SELECTOR_FLAG_EPOCH  = 1 << GTK_SAT_SELECTOR_COL_EPOCH,  /*!< Element set epoch. */
    GTK_SAT_SELECTOR_FLAG_SELECTED  = 1 << GTK_SAT_SELECTOR_COL_SELECTED,  /*!< Item selected or not. */
} gtk_sat_selector_flag_t;



#define GTK_SAT_SELECTOR_DEFAULT_FLAGS (GTK_SAT_SELECTOR_FLAG_NAME | GTK_SAT_SELECTOR_FLAG_CATNUM)


#define GTK_TYPE_SAT_SELECTOR  (gtk_sat_selector_get_type ())
#define GTK_SAT_SELECTOR(obj)  G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                               gtk_sat_selector_get_type (),\
                                               GtkSatSelector)

#define GTK_SAT_SELECTOR_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                                             gtk_sat_selector_get_type (),\
                                                             GtkSatSelectorClass)

#define IS_GTK_SAT_SELECTOR(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sat_selector_get_type ())


/** \brief The GtkSatSelector structure */
typedef struct _gtk_sat_selector  GtkSatSelector;
typedef struct _GtkSatSelectorClass GtkSatSelectorClass;


/** \brief The GtkSatSelector Structure definition */
struct _gtk_sat_selector
{
    GtkVBox vbox;

    GtkWidget   *tree;        /*!< The tree. */
    GtkWidget   *swin;        /*!< Scrolled window. */
    guint        flags;       /*!< Column visibility flags. */

    GtkWidget   *groups;      /*!< Combo box for selecting satellite group. */
    GtkWidget   *search;      /*!< Text entry for searching. */
    GSList      *models;      /*!< List of models with index corresponding to groups. */
};

struct _GtkSatSelectorClass
{
    GtkVBoxClass parent_class;

    void (* gtksatselector) (GtkSatSelector *sel);
};


GType      gtk_sat_selector_get_type  (void);
GtkWidget *gtk_sat_selector_new       (guint flags);
guint32    gtk_sat_selector_get_flags (GtkSatSelector *selector);
void       gtk_sat_selector_get_selected (GtkSatSelector *selector, gint *catnum, gchar **satname, gdouble *epoch);
gdouble    gtk_sat_selector_get_latest_epoch (GtkSatSelector *selector);
void       gtk_sat_selector_mark_selected (GtkSatSelector *selector, gint catnum);
void       gtk_sat_selector_mark_unselected (GtkSatSelector *selector, gint catnum);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
