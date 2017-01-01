/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2015  Alexandru Csete, OZ9AEC.

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
#ifndef SAT_PREF_RIG_DATA_H
#define SAT_PREF_RIG_DATA_H 1

/** \brief Coumn definitions for radio list. */
typedef enum {
    RIG_LIST_COL_NAME = 0,      /*!< File name. */
    RIG_LIST_COL_HOST,          /*!< Hostname, e.g. localhost */
    RIG_LIST_COL_PORT,          /*!< Port number */
    RIG_LIST_COL_TYPE,          /*!< Radio type */
    RIG_LIST_COL_PTT,           /*!< PTT */
    RIG_LIST_COL_VFOUP,         /*!< VFO Up */
    RIG_LIST_COL_VFODOWN,       /*!< VFO down */
    RIG_LIST_COL_LO,            /*!< Local oscillator freq (downlink) */
    RIG_LIST_COL_LOUP,          /*!< Local oscillato freq (uplink) */
    RIG_LIST_COL_SIGAOS,        /*!< Signal AOS */
    RIG_LIST_COL_SIGLOS,        /*!< Signal LOS */
    RIG_LIST_COL_NUM            /*!< The number of fields in the list. */
} rig_list_col_t;

#endif
