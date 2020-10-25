/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
#ifndef SAT_CFG_H
#define SAT_CFG_H 1

#include <glib.h>

#define TIME_FORMAT_MAX_LENGTH 50

/** Symbolic references for boolean config values. */
typedef enum {
    SAT_CFG_BOOL_USE_LOCAL_TIME = 0,    /*!< Display local time instead of UTC. */
    SAT_CFG_BOOL_USE_NSEW,      /*!< Use N, S, E and W instead of sign */
    SAT_CFG_BOOL_USE_IMPERIAL,  /*!< Use Imperial units instead of Metric */
    SAT_CFG_BOOL_MAIN_WIN_POS,  /*!< Restore position of main window */
    SAT_CFG_BOOL_MOD_WIN_POS,   /*!< Restore size and position of module windows */
    SAT_CFG_BOOL_MOD_STATE,     /*!< Restore module state */
    SAT_CFG_BOOL_RULES_HINT_OBSOLETE,    /*!< Enable rules hint in GtkSatList. */
    SAT_CFG_BOOL_MAP_SHOW_QTH_INFO,     /*!< Show QTH info on map */
    SAT_CFG_BOOL_MAP_SHOW_NEXT_EV,      /*!< Show next event on map */
    SAT_CFG_BOOL_MAP_SHOW_CURS_TRACK,   /*!< Track mouse cursor on map. */
    SAT_CFG_BOOL_MAP_SHOW_GRID, /*!< Show grid on map. */
    SAT_CFG_BOOL_MAP_SHOW_TERMINATOR,   /*!< Show solar terminator on map. */
    SAT_CFG_BOOL_MAP_KEEP_RATIO,        /*!< Keep original aspect ratio */
    SAT_CFG_BOOL_POL_SHOW_QTH_INFO,     /*!< Show QTH info on polar plot */
    SAT_CFG_BOOL_POL_SHOW_NEXT_EV,      /*!< Show next event on polar plot */
    SAT_CFG_BOOL_POL_SHOW_CURS_TRACK,   /*!< Track mouse cursor on polar plot. */
    SAT_CFG_BOOL_POL_SHOW_EXTRA_AZ_TICKS,       /*!< Extra Az ticks at every 30 deg. */
    SAT_CFG_BOOL_POL_SHOW_TRACK_AUTO,   /*!< Automatically show the sky track. */
    SAT_CFG_BOOL_TLE_SERVER_AUTH,       /*!< TLE server requires authentication. */
    SAT_CFG_BOOL_TLE_PROXY_AUTH,        /*!< Proxy requires authentication. */
    SAT_CFG_BOOL_TLE_ADD_NEW,   /*!< Add new satellites to database. */
    SAT_CFG_BOOL_KEEP_LOG_FILES,        /*!< Whether to keep old log files */
    SAT_CFG_BOOL_PRED_USE_REAL_T0,      /*!< Whether to use current time as T0 fro predictions */
    SAT_CFG_BOOL_NUM            /*!< Number of boolean parameters */
} sat_cfg_bool_e;

/** Symbolic references for integer config values. */
typedef enum {
    SAT_CFG_INT_VERSION_MAJOR = 0,      /*!< Configuration version number (major) */
    SAT_CFG_INT_VERSION_MINOR,  /*!< Configuration version number (minor) */
    SAT_CFG_INT_MODULE_TIMEOUT, /*!< Module refresh rate */
    SAT_CFG_INT_MODULE_LAYOUT,  /*!< Module layout */
    SAT_CFG_INT_MODULE_VIEW_1,  /*!< Type of view 1 */
    SAT_CFG_INT_MODULE_VIEW_2,  /*!< Type of view 2 */
    SAT_CFG_INT_MODULE_VIEW_3,  /*!< Type of view 3 */
    SAT_CFG_INT_MODULE_CURRENT_PAGE,    /*!< Number of integer parameters. */
    SAT_CFG_INT_WARP_FACTOR,    /*!< Time compression factor. */
    SAT_CFG_INT_LIST_REFRESH,   /*!< List refresh rate (cycle). */
    SAT_CFG_INT_LIST_COLUMNS,   /*!< List column visibility. */
    SAT_CFG_INT_MAP_CENTER,     /*!< Longitude around which the map is centered. */
    SAT_CFG_INT_MAP_REFRESH,    /*!< Map refresh rate (cycle). */
    SAT_CFG_INT_MAP_INFO_COL,   /*!< Info text colour on maps. */
    SAT_CFG_INT_MAP_INFO_BGD_COL,       /*!< Info text bgd colour on maps. */
    SAT_CFG_INT_MAP_QTH_COL,    /*!< QTH mark colour on map. */
    SAT_CFG_INT_MAP_SAT_COL,    /*!< Satellite colour on maps */
    SAT_CFG_INT_MAP_SAT_SEL_COL,        /*!< Selected satellite colour */
    SAT_CFG_INT_MAP_SAT_COV_COL,        /*!< Map coverage area colour */
    SAT_CFG_INT_MAP_GRID_COL,   /*!< Grid colour. */
    SAT_CFG_INT_MAP_TERMINATOR_COL,     /*!< Solar terminator colour. */
    SAT_CFG_INT_MAP_GLOBAL_SHADOW_COL,  /*!< Earth shadow colour. */
    SAT_CFG_INT_MAP_TICK_COL,   /*!< Tick labels colour. */
    SAT_CFG_INT_MAP_TRACK_COL,  /*!< Ground Track colour. */
    SAT_CFG_INT_MAP_TRACK_NUM,  /*!< Number of orbits to show ground track for */
    SAT_CFG_INT_MAP_SHADOW_ALPHA,       /*!< Tranparency of shadow under satellite marker. */
    SAT_CFG_INT_POLAR_REFRESH,  /*!< Polar refresh rate (cycle). */
    SAT_CFG_INT_POLAR_ORIENTATION,      /*!< Orientation of the polar charts. */
    SAT_CFG_INT_POLAR_BGD_COL,  /*!< Polar view, background colour. */
    SAT_CFG_INT_POLAR_AXIS_COL, /*!< Polar view, axis colour. */
    SAT_CFG_INT_POLAR_TICK_COL, /*!< Tick label colour, e.g. N/W/S/E */
    SAT_CFG_INT_POLAR_SAT_COL,  /*!< Satellite colour. */
    SAT_CFG_INT_POLAR_SAT_SEL_COL,      /*!< Selected satellite colour. */
    SAT_CFG_INT_POLAR_TRACK_COL,        /*!< Track colour. */
    SAT_CFG_INT_POLAR_INFO_COL, /*!< Info colour. */
    SAT_CFG_INT_SINGLE_SAT_REFRESH,     /*!< Single-sat refresh rate (cycle). */
    SAT_CFG_INT_SINGLE_SAT_FIELDS,      /*!< Single-sat fields. */
    SAT_CFG_INT_SINGLE_SAT_SELECT,      /*!< Single-sat selected satellite. */
    SAT_CFG_INT_EVENT_LIST_REFRESH,     /*!< Event list refresh rate (cycle). */
    SAT_CFG_INT_PRED_MIN_EL,    /*!< Minimum elevation for passes. */
    SAT_CFG_INT_PRED_NUM_PASS,  /*!< Number of passes to predict. */
    SAT_CFG_INT_PRED_LOOK_AHEAD,        /*!< Look-ahead time limit in days. */
    SAT_CFG_INT_PRED_RESOLUTION,        /*!< Time resolution in seconds */
    SAT_CFG_INT_PRED_NUM_ENTRIES,       /*!< Number of entries in single pass. */
    SAT_CFG_INT_PRED_SINGLE_COL,        /*!< Visible columns in single-pass dialog */
    SAT_CFG_INT_PRED_MULTI_COL, /*!< Visible columns in multi-pass dialog */
    SAT_CFG_INT_PRED_SAVE_FORMAT,       /*!< Last used save format for predictions */
    SAT_CFG_INT_PRED_SAVE_CONTENTS,     /*!< Last selection for save file contents */
    SAT_CFG_INT_PRED_TWILIGHT_THLD,     /*!< Twilight zone threshold */
    SAT_CFG_INT_SKYATGL_TIME,   /*!< Time span for sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_01, /*!< Colour 1 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_02, /*!< Colour 2 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_03, /*!< Colour 3 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_04, /*!< Colour 4 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_05, /*!< Colour 5 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_06, /*!< Colour 6 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_07, /*!< Colour 7 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_08, /*!< Colour 8 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_09, /*!< Colour 9 in sky at a glance predictions */
    SAT_CFG_INT_SKYATGL_COL_10, /*!< Colour 10 in sky at a glance predictions */
    SAT_CFG_INT_WINDOW_POS_X,   /*!< Main window X during last session */
    SAT_CFG_INT_WINDOW_POS_Y,   /*!< Main window Y during last session */
    SAT_CFG_INT_WINDOW_WIDTH,   /*!< Main window width during last session */
    SAT_CFG_INT_WINDOW_HEIGHT,  /*!< Main window height during last session */
    SAT_CFG_INT_WEB_BROWSER_TYPE,       /*!< Web browser type, see browser_type_t */
    SAT_CFG_INT_TLE_AUTO_UPD_FREQ,      /*!< TLE auto-update frequency. */
    SAT_CFG_INT_TLE_AUTO_UPD_ACTION,    /*!< TLE auto-update action. */
    SAT_CFG_INT_TLE_LAST_UPDATE,        /*!< Date and time of last update, Unix seconds. */
    SAT_CFG_INT_LOG_CLEAN_AGE,  /*!< Age of log file to delete (seconds) */
    SAT_CFG_INT_LOG_LEVEL,      /*!< Logging level */
    SAT_CFG_INT_NUM             /*!< Number of integer parameters. */
} sat_cfg_int_e;

/** Symbolic references for string config values. */
typedef enum {
    SAT_CFG_STR_TIME_FORMAT = 0,        /*!< Time format. */
    SAT_CFG_STR_DEF_QTH,        /*!< Default QTH file. */
    SAT_CFG_STR_OPEN_MODULES,   /*!< Open modules. */
    SAT_CFG_STR_WEB_BROWSER,    /*!< Web browser string. */
    SAT_CFG_STR_MODULE_GRID,    /*!< The grid layout of the module */
    SAT_CFG_STR_MAP_FILE,       /*!< Map file (abs or rel). */
    SAT_CFG_STR_MAP_FONT,       /*!< Map font. */
    SAT_CFG_STR_POL_FONT,       /*!< Polar view font. */
    SAT_CFG_STR_TRSP_SERVER,    /*!< Server for transponder updates (since 1.4) */
    SAT_CFG_STR_TRSP_FREQ_FILE, /*!< The file with frequency data on the server (since 1.4) */
    SAT_CFG_STR_TRSP_MODE_FILE, /*!< The file with mode descriptions on the server (since 1.4) */
    SAT_CFG_STR_TRSP_PROXY,     /*!< Proxy server. */
    SAT_CFG_STR_TLE_SERVER,     /*!< Server for TLE updates (replaced in 1.4 by TLE_URLS) */
    SAT_CFG_STR_TLE_FILES,      /*!< ; separated list of files on server (replaced in 1.4 by TLE_URLS) */
    SAT_CFG_STR_TLE_PROXY,      /*!< Proxy server. */
    SAT_CFG_STR_TLE_URLS,       /*!< ; separated list of TLE file URLs (since 1.4) */
    SAT_CFG_STR_TLE_FILE_DIR,   /*!< Local directory from which tle were last updated. */
    SAT_CFG_STR_PRED_SAVE_DIR,  /*!< Last used save directory for pass predictions */
    SAT_CFG_STR_NUM             /*!< Number of string parameters */
} sat_cfg_str_e;

guint           sat_cfg_load(void);
guint           sat_cfg_save(void);
void            sat_cfg_close(void);
gboolean        sat_cfg_get_bool(sat_cfg_bool_e param);
gboolean        sat_cfg_get_bool_def(sat_cfg_bool_e param);
void            sat_cfg_set_bool(sat_cfg_bool_e param, gboolean value);
void            sat_cfg_reset_bool(sat_cfg_bool_e param);
gchar          *sat_cfg_get_str(sat_cfg_str_e param);
gchar          *sat_cfg_get_str_def(sat_cfg_str_e param);
void            sat_cfg_set_str(sat_cfg_str_e param, const gchar * value);
void            sat_cfg_reset_str(sat_cfg_str_e param);
gint            sat_cfg_get_int(sat_cfg_int_e param);
gint            sat_cfg_get_int_def(sat_cfg_int_e param);
void            sat_cfg_set_int(sat_cfg_int_e param, gint value);
void            sat_cfg_reset_int(sat_cfg_int_e param);

#endif
