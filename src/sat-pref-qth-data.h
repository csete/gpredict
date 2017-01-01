/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.
    
    Authors: Alexandru Csete <oz9aec@gmail.com>
             Charles Suprin  <hamaa1vs@gmail.com>

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

/** Column definitions for QTH list. */
typedef enum {
    QTH_LIST_COL_NAME = 0,      /*!< Name of the QTH. */
    QTH_LIST_COL_LOC,           /*!< Location, eg. "Copenhagen, Denmark". */
    QTH_LIST_COL_DESC,          /*!< Optional description. */
    QTH_LIST_COL_LAT,           /*!< Latitude in dec. deg. North. */
    QTH_LIST_COL_LON,           /*!< Longitude in dec. deg. East. */
    QTH_LIST_COL_ALT,           /*!< Altitude in meters. */
    QTH_LIST_COL_QRA,           /*!< QRA locator. */
    QTH_LIST_COL_WX,            /*!< 4 letter weather station. */
    QTH_LIST_COL_DEF,           /*!< Is this QTH the default one? */
    QTH_LIST_COL_TYPE,          /*!< Is this QTH the default one? */
    QTH_LIST_COL_GPSD_SERVER,   /*!< Is this QTH the default one? */
    QTH_LIST_COL_GPSD_PORT,     /*!< Is this QTH the default one? */
    QTH_LIST_COL_NUM            /*!< The number of fields. */
} qth_list_col_t;
