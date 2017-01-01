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
#ifndef SAT_PREF_ROT_DATA_H
#define SAT_PREF_ROT_DATA_H 1

/** Coumn definitions for rotator list. */
typedef enum {
    ROT_LIST_COL_NAME = 0,      /*!< File name. */
    ROT_LIST_COL_HOST,          /*!< Hostname */
    ROT_LIST_COL_PORT,          /*!< Port number */
    ROT_LIST_COL_MINAZ,         /*!< Lower Az limit. */
    ROT_LIST_COL_MAXAZ,         /*!< Upper Az limit. */
    ROT_LIST_COL_MINEL,         /*!< Lower El limit. */
    ROT_LIST_COL_MAXEL,         /*!< Upper El limit. */
    ROT_LIST_COL_AZTYPE,        /*!< Azimuth type. */
    ROT_LIST_COL_AZSTOPPOS,     /*!< Position of the azimuth rotation stops.
                                   Should default to MINAZ, unless specified
                                   otherwise */
    ROT_LIST_COL_NUM            /*!< The number of fields in the list. */
} rotor_list_col_t;

#endif
