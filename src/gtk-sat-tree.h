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
#ifndef __GTK_SAT_TREE_H__
#define __GTK_SAT_TREE_H__ 1

#include <gtk/gtk.h>
#include "gtk-sat-list.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** \brief Column definitions in the tree. */
typedef enum {
     GTK_SAT_TREE_COL_NAME = 0,  /*!< Satellite name. */
     GTK_SAT_TREE_COL_CATNUM,    /*!< Catalogue Number. */
     GTK_SAT_TREE_COL_EPOCH,     /*!< Element set epoch. */
     GTK_SAT_TREE_COL_SEL,       /*!< Checkbox column, ie select satellite. */
     GTK_SAT_TREE_COL_VIS,       /*!< Hidden column used to node visibility */
     GTK_SAT_TREE_COL_NUM        /*!< The number of columns. */
} gtk_sat_tree_col_t;

     
/** \brief Flags used to indicate which columns should be visible. */
typedef enum {
     GTK_SAT_TREE_FLAG_NAME   = 1 << GTK_SAT_TREE_COL_NAME,   /*!< Satellite name. */
     GTK_SAT_TREE_FLAG_CATNUM = 1 << GTK_SAT_TREE_COL_CATNUM, /*!< Catalogue Number. */
     GTK_SAT_TREE_FLAG_EPOCH  = 1 << GTK_SAT_TREE_COL_EPOCH,  /*!< Element set epoch. */
     GTK_SAT_TREE_FLAG_SEL    = 1 << GTK_SAT_TREE_COL_SEL     /*!< Checkbox column. */
} gtk_sat_tree_flag_t;



#define GTK_SAT_TREE_DEFAULT_FLAGS (GTK_SAT_TREE_FLAG_NAME | GTK_SAT_TREE_FLAG_SEL)


#define GTK_TYPE_SAT_TREE  (gtk_sat_tree_get_type ())
#define GTK_SAT_TREE(obj)  G_TYPE_CHECK_INSTANCE_CAST (obj,\
                            gtk_sat_tree_get_type (),\
                            GtkSatTree)

#define GTK_SAT_TREE_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                    gtk_sat_tree_get_type (),\
                                    GtkSatTreeClass)

#define IS_GTK_SAT_TREE(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_sat_tree_get_type ())


/** \brief The GtkSatTree structure */
typedef struct _gtk_sat_tree  GtkSatTree;
typedef struct _GtkSatTreeClass GtkSatTreeClass;


/** \brief The GtkSatTree Structure definition */
struct _gtk_sat_tree
{
     GtkVBox vbox;

     GtkWidget   *tree;        /*!< The tree. */
     GtkWidget   *swin;        /*!< Scrolled window. */
     guint        flags;       /*!< Column visibility flags. */
     GSList      *selection;   /*!< List of selected satellites. */
     gulong       handler_id;  /*!< Toggle signale handler ID (FIXME): remove. */
};

struct _GtkSatTreeClass
{
     GtkVBoxClass parent_class;
};


GType      gtk_sat_tree_get_type  (void);
GtkWidget *gtk_sat_tree_new       (guint flags);
guint32    gtk_sat_tree_get_flags (GtkSatTree *tree);
void       gtk_sat_tree_select    (GtkSatTree *sat_tree, guint catnum);
guint     *gtk_sat_tree_get_selected (GtkSatTree *sat_tree, gsize *size);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
