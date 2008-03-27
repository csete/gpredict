/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>

    Comments, questions and bugreports should be submitted via
    http://sourceforge.net/projects/groundstation/
    More details can be found at the project home page:

            http://groundstation.sourceforge.net/
 
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
    RIG_LIST_COL_NAME = 0,  /*!< File name. */
    RIG_LIST_COL_MODEL,     /*!< Model, e.g. FT-847. */
    RIG_LIST_COL_ID,        /*!< Hamlib ID. */
    RIG_LIST_COL_TYPE,      /*!< Radio type (RX, TX, TRX, FULL_DUP. */
    RIG_LIST_COL_PORT,      /*!< Port / Device, e.g. /dev/ttyS0. */
    RIG_LIST_COL_SPEED,     /*!< Serial speed. */
    RIG_LIST_COL_CIV,       /*!< CI-V address for Icom rigs. */
    RIG_LIST_COL_EXT,       /*!< Use built-in extensions. */
    RIG_LIST_COL_DTR,       /*!< DTR line usage */
    RIG_LIST_COL_RTS,       /*!< RTS line usage */
    RIG_LIST_COL_NUM        /*!< The number of fields in the list. */
} rig_list_col_t;


#endif
