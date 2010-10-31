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
#ifndef LOC_TREE_H
#define LOC_TREE_H 1

/** \brief Tree column definitions.
 */
typedef enum {
     TREE_COL_NAM = 0,  /*!< Location name column. */
     TREE_COL_LAT,      /*!< Location latitude column. */
     TREE_COL_LON,      /*!< Location longitude column. */
     TREE_COL_ALT,      /*!< Location altitude column. */
     TREE_COL_WX,       /*!< Weather station column. */
     TREE_COL_SELECT,   /*!< Invisible colindicating whether row may be selected */
     TREE_COL_NUM       /*!< The total number of columns. */
} loc_tree_col_t;


/** \brief Column flags. */
typedef enum {
     TREE_COL_FLAG_NAME = 1 << TREE_COL_NAM,  /*!< Location name column. */
     TREE_COL_FLAG_LAT  = 1 << TREE_COL_LAT,  /*!< Location latitude column. */
     TREE_COL_FLAG_LON  = 1 << TREE_COL_LON,  /*!< Location longitude column. */
     TREE_COL_FLAG_ALT  = 1 << TREE_COL_ALT,  /*!< Location altitude column. */
     TREE_COL_FLAG_WX   = 1 << TREE_COL_WX    /*!< Weather station column. */
} loc_tree_col_flag_t;



gboolean  loc_tree_create (const gchar *fname,
                  guint flags,
                  gchar  **loc,
                  gfloat  *lat,
                  gfloat  *lon,
                  guint   *alt,
                  gchar  **wx);

#endif
