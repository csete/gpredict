/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2013 Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>
    Charles Suprin <hamaa1vs@gmail.com>

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

/* This file contains section and key definitions in config files
 *
 * NOTE: System wide config is in the sat-cfg component, including
 *       key definitions. This file contains only key definitions
 *       for .qth .mod .rig and .rot files.

 */

#ifndef CONFIG_KEYS_H
#define CONFIG_KEYS_H 1


/* Main configuration file (gpredict.cfg) */
/* global section*/
#define SAT_CFG_GLOBAL_SECTION       "GLOBAL"
#define SAT_CFG_LOCAL_TIME_KEY       "LOCALTIME"
#define SAT_CFG_TIME_FORMAT_KEY      "TIMEFORMAT"
#define SAT_CFG_SHOW_NSEW_KEY        "NSEW"
#define SAT_CFG_IMPERIAL_KEY         "IMPERIAL"


/* QTH files (.qth) */
#define QTH_CFG_MAIN_SECTION   "QTH"
#define QTH_CFG_NAME_KEY       "NAME"
#define QTH_CFG_LOC_KEY        "LOCATION"
#define QTH_CFG_DESC_KEY       "DESCRIPTION"
#define QTH_CFG_WX_KEY         "WX"
#define QTH_CFG_LAT_KEY        "LAT"
#define QTH_CFG_LON_KEY        "LON"
#define QTH_CFG_ALT_KEY        "ALT"
#define QTH_CFG_GPSD_SERVER_KEY "GPSDSERVER"
#define QTH_CFG_GPSD_PORT_KEY  "GPSDPORT"
#define QTH_CFG_TYPE_KEY       "QTH_TYPE"

/* Module files (.mod) */

/* global */
#define MOD_CFG_GLOBAL_SECTION  "GLOBAL"
#define MOD_CFG_QTH_FILE_KEY    "QTHFILE"
#define MOD_CFG_SATS_KEY        "SATELLITES"
#define MOD_CFG_TIMEOUT_KEY     "TIMEOUT"
#define MOD_CFG_WARP_KEY        "WARP"
#define MOD_CFG_LAYOUT          "LAYOUT"      /* Old layout before v1.2 */
#define MOD_CFG_VIEW_1          "VIEW_1"      /* Old layout before v1.2 */
#define MOD_CFG_VIEW_2          "VIEW_2"      /* Old layout before v1.2 */
#define MOD_CFG_VIEW_3          "VIEW_3"      /* Old layout before v1.2 */
#define MOD_CFG_GRID            "GRID"        /* New grid layout since v1.2 */
#define MOD_CFG_STATE           "STATE"
#define MOD_CFG_WIN_POS_X       "WIN_POS_X"
#define MOD_CFG_WIN_POS_Y       "WIN_POS_Y"
#define MOD_CFG_WIN_WIDTH       "WIN_WIDTH"
#define MOD_CFG_WIN_HEIGHT      "WIN_HEIGHT"

/* list specific */
#define MOD_CFG_LIST_SECTION   "LIST"
#define MOD_CFG_LIST_COLUMNS   "COLUMNS"
#define MOD_CFG_LIST_REFRESH   "REFRESH"
#define MOD_CFG_LIST_SORT_COLUMN "SORT_COLUMN"
#define MOD_CFG_LIST_SORT_ORDER "SORT_ORDER"

/* map specific */
#define MOD_CFG_MAP_SECTION           "MAP"
#define MOD_CFG_MAP_CENTER            "CENTER"  /*!< Center longitude. */
#define MOD_CFG_MAP_REFRESH           "REFRESH"
#define MOD_CFG_MAP_FILE              "MAP_FILE" /* abs. path = home dir */
#define MOD_CFG_MAP_FONT              "TEXT_FONT"
#define MOD_CFG_MAP_SHOW_QTH_INFO     "QTH_INFO"
#define MOD_CFG_MAP_SHOW_NEXT_EVENT   "NEXT_EVENT"
#define MOD_CFG_MAP_SHOW_CURS_TRACK   "CURSOR_TRACK"
#define MOD_CFG_MAP_SHOW_GRID         "SHOW_GRID"
#define MOD_CFG_MAP_SHOW_TERMINATOR   "SHOW_TERMINATOR"
#define MOD_CFG_MAP_SAT_COL           "SAT_COLOUR"
#define MOD_CFG_MAP_SAT_SEL_COL       "SAT_SEL_COLOUR"
#define MOD_CFG_MAP_SAT_COV_COL       "COV_AREA_COLOUR"
#define MOD_CFG_MAP_QTH_COL           "QTH_COLOUR"
#define MOD_CFG_MAP_INFO_COL          "INFO_COLOUR"
#define MOD_CFG_MAP_INFO_BGD_COL      "INFO_BGD_COLOUR"
#define MOD_CFG_MAP_GRID_COL          "GRIG_COLOUR"
#define MOD_CFG_MAP_TERMINATOR_COL    "TERMINATOR_COLOUR"
#define MOD_CFG_MAP_GLOBAL_SHADOW_COL "GLOBAL_SHADOW_COLOUR"
#define MOD_CFG_MAP_TICK_COL          "TICK_COLOUR"
#define MOD_CFG_MAP_TRACK_COL         "TRACK_COLOUR"
#define MOD_CFG_MAP_TRACK_NUM         "TRACK_NUMBER"
#define MOD_CFG_MAP_KEEP_RATIO        "KEEP_RATIO"
#define MOD_CFG_MAP_SHADOW_ALPHA      "SHADOW_ALPHA"
#define MOD_CFG_MAP_SHOWTRACKS        "SHOWTRACKS"
#define MOD_CFG_MAP_HIDECOVS          "HIDECOVS"

/* polar view specific */
#define MOD_CFG_POLAR_SECTION          "POLAR"
#define MOD_CFG_POLAR_REFRESH          "REFRESH"
#define MOD_CFG_POLAR_ORIENTATION      "ORIENTATION"
#define MOD_CFG_POLAR_SHOW_QTH_INFO    "QTH_INFO"
#define MOD_CFG_POLAR_SHOW_NEXT_EVENT  "NEXT_EVENT"
#define MOD_CFG_POLAR_SHOW_CURS_TRACK  "CURSOR_TRACK"
#define MOD_CFG_POLAR_SHOW_EXTRA_AZ_TICKS "EXTRA_AZ_TICKS"
#define MOD_CFG_POLAR_SHOW_TRACK_AUTO  "SHOW_TRACK"
#define MOD_CFG_POLAR_BGD_COL          "BGD_COLOUR"
#define MOD_CFG_POLAR_AXIS_COL         "AXIS_COLOUR"
#define MOD_CFG_POLAR_TICK_COL         "TICK_COLOUR"
#define MOD_CFG_POLAR_SAT_COL          "SAT_COLOUR"
#define MOD_CFG_POLAR_SAT_SEL_COL      "SAT_SEL_COLOUR"
#define MOD_CFG_POLAR_TRACK_COL        "TRACK_COLOUR"
#define MOD_CFG_POLAR_INFO_COL         "INFO_COLOUR"
#define MOD_CFG_POLAR_FONT             "TEXT_FONT"
#define MOD_CFG_POLAR_SHOWTRACKS       "SHOWTRACKS"
#define MOD_CFG_POLAR_HIDETRACKS       "HIDETRACKS"

/* single sat */
#define MOD_CFG_SINGLE_SAT_SECTION "SINGLE_SAT"
#define MOD_CFG_SINGLE_SAT_REFRESH "REFRESH"
#define MOD_CFG_SINGLE_SAT_FIELDS  "FIELDS"
#define MOD_CFG_SINGLE_SAT_SELECT  "SELECTED"

/* event list */
#define MOD_CFG_EVENT_LIST_SECTION  "EVENT_LIST"
#define MOD_CFG_EVENT_LIST_REFRESH "REFRESH"
#define MOD_CFG_EVENT_LIST_SORT_COLUMN "SORT_COLUMN"
#define MOD_CFG_EVENT_LIST_SORT_ORDER "SORT_ORDER"

#endif
