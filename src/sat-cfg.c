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
/**
 * @defgroup satcfg Read, manage and store gpredict configuration.
 *
 * The purpose with this module is to centralise the access to the gpredict.cfg
 * configuration file and also to have a central place where the min, max and
 * default values are defined.
 */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-polar-view.h"
#include "gtk-sat-module.h"
#include "gtk-sat-list.h"
#include "gtk-single-sat.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pass-dialogs.h"

/* *INDENT-OFF* */
#define LIST_COLUMNS_DEFAULTS (SAT_LIST_FLAG_NAME |\
    SAT_LIST_FLAG_AZ   |\
    SAT_LIST_FLAG_EL   |\
    SAT_LIST_FLAG_RANGE |\
    SAT_LIST_FLAG_DIR |\
    SAT_LIST_FLAG_NEXT_EVENT |\
    SAT_LIST_FLAG_ALT |\
    SAT_LIST_FLAG_ORBIT)

#define SINGLE_PASS_COL_DEFAULTS (SINGLE_PASS_FLAG_TIME |\
    SINGLE_PASS_FLAG_AZ |\
    SINGLE_PASS_FLAG_EL |\
    SINGLE_PASS_FLAG_RANGE |\
    SINGLE_PASS_FLAG_FOOTPRINT)

#define MULTI_PASS_COL_DEFAULTS (MULTI_PASS_FLAG_AOS_TIME |\
    MULTI_PASS_FLAG_LOS_TIME |\
    MULTI_PASS_FLAG_DURATION |\
    MULTI_PASS_FLAG_AOS_AZ |\
    MULTI_PASS_FLAG_MAX_EL |\
    MULTI_PASS_FLAG_LOS_AZ)

#define SINGLE_SAT_FIELD_DEF (SINGLE_SAT_FLAG_AZ |\
    SINGLE_SAT_FLAG_EL |\
    SINGLE_SAT_FLAG_RANGE |\
    SINGLE_SAT_FLAG_RANGE_RATE |\
    SINGLE_SAT_FLAG_NEXT_EVENT |\
    SINGLE_SAT_FLAG_SSP |\
    SINGLE_SAT_FLAG_FOOTPRINT |\
    SINGLE_SAT_FLAG_ALT |\
    SINGLE_SAT_FLAG_VEL |\
    SINGLE_SAT_FLAG_DOPPLER |\
    SINGLE_SAT_FLAG_LOSS |\
    SINGLE_SAT_FLAG_DELAY |\
    SINGLE_SAT_FLAG_MA |\
    SINGLE_SAT_FLAG_PHASE |\
    SINGLE_SAT_FLAG_ORBIT |\
    SINGLE_SAT_FLAG_VISIBILITY)
/* *INDENT-ON* */

/** Structure representing a boolean value */
typedef struct {
    gchar          *group;      /*!< The configuration group */
    gchar          *key;        /*!< The configuration key */
    gboolean        defval;     /*!< The default value */
} sat_cfg_bool_t;

/** Structure representing an integer value */
typedef struct {
    gchar          *group;      /*!< The configuration group */
    gchar          *key;        /*!< The configuration key */
    gint            defval;     /*!< The default value */
} sat_cfg_int_t;

/** Structure representing a string value */
typedef struct {
    gchar          *group;      /*!< The configuration group */
    gchar          *key;        /*!< The configuration key */
    gchar          *defval;     /*!< The default value */
} sat_cfg_str_t;

/** Array containing the boolean configuration values */
sat_cfg_bool_t  sat_cfg_bool[SAT_CFG_BOOL_NUM] = {
    {"GLOBAL", "USE_LOCAL_TIME", FALSE},
    {"GLOBAL", "USE_NSEW", FALSE},
    {"GLOBAL", "USE_IMPERIAL", FALSE},
    {"GLOBAL", "MAIN_WIN_POS", FALSE},
    {"GLOBAL", "MOD_WIN_POS", FALSE},
    {"GLOBAL", "MOD_STATE", FALSE},
    {"MODULES", "RULES_HINT", FALSE},
    {"MODULES", "MAP_QTH_INFO", TRUE},
    {"MODULES", "MAP_NEXT_EVENT", TRUE},
    {"MODULES", "MAP_CURSOR_TRACK", FALSE},
    {"MODULES", "MAP_SHOW_GRID", TRUE},
    {"MODULES", "MAP_SHOW_TERMINATOR", TRUE},
    {"MODULES", "MAP_KEEP_RATIO", FALSE},
    {"MODULES", "POLAR_QTH_INFO", TRUE},
    {"MODULES", "POLAR_NEXT_EVENT", TRUE},
    {"MODULES", "POLAR_CURSOR_TRACK", TRUE},
    {"MODULES", "POLAR_EXTRA_AZ_TICKS", FALSE},
    {"MODULES", "POLAR_SHOW_TRACK_AUTO", FALSE},
    {"TLE", "SERVER_AUTH", FALSE},
    {"TLE", "PROXY_AUTH", FALSE},
    {"TLE", "ADD_NEW_SATS", TRUE},
    {"LOG", "KEEP_LOG_FILES", FALSE},
    {"PREDICT", "USE_REAL_T0", FALSE}
};

/** Array containing the integer configuration parameters */
sat_cfg_int_t   sat_cfg_int[SAT_CFG_INT_NUM] = {
    {"VERSION", "MAJOR", 1},
    {"VERSION", "MINOR", 4},
    {"MODULES", "DATA_TIMEOUT", 300},
    {"MODULES", "LAYOUT", 2},   /* FIXME */
    {"MODULES", "VIEW_1", GTK_SAT_MOD_VIEW_MAP},        /* FIXME */
    {"MODULES", "VIEW_2", GTK_SAT_MOD_VIEW_POLAR},      /* FIXME */
    {"MODULES", "VIEW_3", GTK_SAT_MOD_VIEW_SINGLE},     /* FIXME */
    {"GLOBAL", "CURRENT_PAGE", -1},     /* FIXME */
    {"GLOBAL", "WARP", 1},
    {"MODULES", "LIST_REFRESH", 1},
    {"MODULES", "LIST_COLUMNS", LIST_COLUMNS_DEFAULTS},
    {"MODULES", "MAP_CENTER", 0},
    {"MODULES", "MAP_REFRESH", 10},
    {"MODULES", "MAP_INFO_COLOUR", 0x00FF00FF},
    {"MODULES", "MAP_INFO_BGD_COLOUR", 0x000000FF},
    {"MODULES", "MAP_QTH_COLOUR", 0x00FFFFFF},
    {"MODULES", "MAP_SAT_COLOUR", 0xF0F000FF},
    {"MODULES", "MAP_SELECTED_SAT_COLOUR", 0xFFFFFFFF},
    {"MODULES", "MAP_COV_AREA_COLOUR", 0xFFFFFF1F},
    {"MODULES", "MAP_GRID_COLOUR", 0x7F7F7FC8},
    {"MODULES", "MAP_TERMINATOR_COLOUR", 0xFFFF0080},
    {"MODULES", "MAP_EARTH_SHADOW_COLOUR", 0x00000060},
    {"MODULES", "MAP_TICK_COLOUR", 0x7F7F7FC8},
    {"MODULES", "MAP_TRACK_COLOUR", 0xFF1200BB},
    {"MODULES", "MAP_TRACK_NUM", 3},
    {"MODULES", "MAP_SHADOW_ALPHA", 0xDD},
    {"MODULES", "POLAR_REFRESH", 3},
    {"MODULES", "POLAR_CHART_ORIENT", POLAR_VIEW_NESW},
    {"MODULES", "POLAR_BGD_COLOUR", 0xFFFFFFFF},
    {"MODULES", "POLAR_AXIS_COLOUR", 0x0F0F0F7F},
    {"MODULES", "POLAR_TICK_COLOUR", 0x007F00FF},
    {"MODULES", "POLAR_SAT_COLOUR", 0x8F0000FF},
    {"MODULES", "POLAR_SELECTED_SAT_COL", 0xFF0D0BFF},
    {"MODULES", "POLAR_TRACK_COLOUR", 0x0000FFFF},
    {"MODULES", "POLAR_INFO_COLOUR", 0x00007FFF},
    {"MODULES", "SINGLE_SAT_REFRESH", 1},
    {"MODULES", "SINGLE_SAT_FIELDS", SINGLE_SAT_FIELD_DEF},
    {"MODULES", "SINGLE_SAT_SELECTED", 0},
    {"MODULES", "EVENT_LIST_REFRESH", 1},
    {"PREDICT", "MINIMUM_ELEV", 5},
    {"PREDICT", "NUMBER_OF_PASSES", 10},
    {"PREDICT", "LOOK_AHEAD", 3},
    {"PREDICT", "TIME_RESOLUTION", 10},
    {"PREDICT", "NUMBER_OF_ENTRIES", 20},
    {"PREDICT", "SINGLE_PASS_COL", SINGLE_PASS_COL_DEFAULTS},
    {"PREDICT", "MULTI_PASS_COL", MULTI_PASS_COL_DEFAULTS},
    {"PREDICT", "SAVE_FORMAT", 0},
    {"PREDICT", "SAVE_CONTENTS", 0},
    {"PREDICT", "TWILIGHT_THRESHOLD", -6},
    {"SKY_AT_GLANCE", "TIME_SPAN_HOURS", 8},
    {"SKY_AT_GLANCE", "COLOUR_01", 0x3c46c8},
    {"SKY_AT_GLANCE", "COLOUR_02", 0x00500a},
    {"SKY_AT_GLANCE", "COLOUR_03", 0xd5472b},
    {"SKY_AT_GLANCE", "COLOUR_04", 0xd06b38},
    {"SKY_AT_GLANCE", "COLOUR_05", 0xcf477a},
    {"SKY_AT_GLANCE", "COLOUR_06", 0xbf041f},
    {"SKY_AT_GLANCE", "COLOUR_07", 0x688522},
    {"SKY_AT_GLANCE", "COLOUR_08", 0x0420bf},
    {"SKY_AT_GLANCE", "COLOUR_09", 0xa304bf},
    {"SKY_AT_GLANCE", "COLOUR_10", 0x04bdbf},
    {"GLOBAL", "WINDOW_POS_X", 0},
    {"GLOBAL", "WINDOW_POS_Y", 0},
    {"GLOBAL", "WINDOW_WIDTH", 700},
    {"GLOBAL", "WINDOW_HEIGHT", 700},
    {"GLOBAL", "HTML_BROWSER_TYPE", 0},
    {"TLE", "AUTO_UPDATE_FREQ", 2},     /* weekly, see tle_auto_upd_freq_t */
    {"TLE", "AUTO_UPDATE_ACTION", 1},   /* notify, see tle_auto_upd_action_t */
    {"TLE", "LAST_UPDATE", 0},
    {"LOG", "CLEAN_AGE", 0},    /* 0 = Never clean */
    {"LOG", "LEVEL", 2}
};

/** Array containing the string configuration values */
sat_cfg_str_t   sat_cfg_str[SAT_CFG_STR_NUM] = {
    {"GLOBAL", "TIME_FORMAT", "%Y/%m/%d %H:%M:%S"},
    {"GLOBAL", "DEFAULT_QTH", "sample.qth"},
    {"GLOBAL", "OPEN_MODULES", "Amateur"},
    {"GLOBAL", "HTML_BROWSER", NULL},
    {"MODULES", "GRID", "1;0;2;0;1;2;0;1;1;2;3;1;2;1;2"},
    {"MODULES", "MAP_FILE", "nasa-bmng-07_1024.jpg"},
    {"MODULES", "MAP_FONT", "Sans 8"},
    {"MODULES", "POLAR_FONT", "Sans 10"},
    {"TRSP", "SERVER", "https://db.satnogs.org/api/"},
    {"TRSP", "FREQ_FILE", "transmitters/?format=json"},
    {"TRSP", "MODE_FILE", "modes/?format=json"},
    {"TRSP", "PROXY", NULL},
    {"TLE", "SERVER", "https://celestrak.org/NORAD/elements/"},
    {"TLE", "FILES", "amateur.txt;cubesat.txt;visual.txt;weather.txt"},
    {"TLE", "PROXY", NULL},
    {"TLE", "URLS",
     "https://www.amsat.org/amsat/ftp/keps/current/nasabare.txt;"
     "https://celestrak.org/NORAD/elements/amateur.txt;"
     "https://celestrak.org/NORAD/elements/cubesat.txt;"
     "https://celestrak.org/NORAD/elements/galileo.txt;"
     "https://celestrak.org/NORAD/elements/glo-ops.txt;"
     "https://celestrak.org/NORAD/elements/gps-ops.txt;"
     "https://celestrak.org/NORAD/elements/iridium.txt;"
     "https://celestrak.org/NORAD/elements/iridium-NEXT.txt;"
     "https://celestrak.org/NORAD/elements/molniya.txt;"
     "https://celestrak.org/NORAD/elements/noaa.txt;"
     "https://celestrak.org/NORAD/elements/science.txt;"
     "https://celestrak.org/NORAD/elements/tle-new.txt;"
     "https://celestrak.org/NORAD/elements/visual.txt;"
     "https://celestrak.org/NORAD/elements/weather.txt"},
    {"TLE", "FILE_DIR", NULL},
    {"PREDICT", "SAVE_DIR", NULL}
};

/* The configuration data buffer */
static GKeyFile *config = NULL;

/**
 * Load configuration data.
 * @return 0 if everything OK, 1 otherwise.
 *
 * This function reads the configuration data from gpredict.cfg into
 * memory. This function must be called very early at program start.
 *
 * The the configuration data in memory is already "loaded" the data will
 * be ereased first.
 */
guint sat_cfg_load()
{
    gchar          *keyfile, *confdir;
    GError         *error = NULL;

    if (config != NULL)
        sat_cfg_close();

    /* load the configuration file */
    config = g_key_file_new();
    confdir = get_user_conf_dir();
    keyfile = g_strconcat(confdir, G_DIR_SEPARATOR_S, "gpredict.cfg", NULL);
    g_free(confdir);
    g_key_file_load_from_file(config, keyfile, G_KEY_FILE_KEEP_COMMENTS,
                              &error);
    g_free(keyfile);

    if (error != NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_WARN,
                    _("%s: Error reading config file (%s)"),
                    __func__, error->message);
        sat_log_log(SAT_LOG_LEVEL_WARN,
                    _("%s: Using built-in defaults"), __func__);
        g_clear_error(&error);
        return 1;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, _("%s: Everything OK."), __func__);
    }

    /* config version compatibility */
    guint16         cfg_ver;

    cfg_ver = sat_cfg_get_int(SAT_CFG_INT_VERSION_MAJOR) << 8 |
        sat_cfg_get_int(SAT_CFG_INT_VERSION_MINOR);

    /* if config version is < 1.1; reset SAT_CFG_STR_TLE_FILES */
    if (cfg_ver < 0x0101)
    {
        sat_cfg_reset_str(SAT_CFG_STR_TLE_FILES);
        sat_cfg_set_int(SAT_CFG_INT_VERSION_MAJOR, 1);
        sat_cfg_set_int(SAT_CFG_INT_VERSION_MINOR, 4);
    }
    else if (cfg_ver < 0x0104)
    {
        /* Version 1.4 replaces TLE_SERVER and TLE_FILES with TLE_URLS,
         * so import TLE_SERVER and FILES if they exist.
         */
        gchar          *urls = NULL;
        gchar          *buff;
        gchar          *tle_srv = sat_cfg_get_str(SAT_CFG_STR_TLE_SERVER);
        gchar          *tle_fstr = sat_cfg_get_str(SAT_CFG_STR_TLE_FILES);
        gchar         **tle_fvec = g_strsplit(tle_fstr, ";", 0);
        unsigned int    i;

        for (i = 0; i < g_strv_length(tle_fvec); i++)
        {
            if (i > 0)
            {
                buff = g_strdup_printf("%s;%s/%s", urls, tle_srv, tle_fvec[i]);
                g_free(urls);
            }
            else
            {
                buff = g_strdup_printf("%s/%s", tle_srv, tle_fvec[i]);
            }
            urls = g_strdup(buff);
            g_free(buff);
        }
        if (urls)
            sat_cfg_set_str(SAT_CFG_STR_TLE_URLS, urls);
        //sat_cfg_reset_str(SAT_CFG_STR_TLE_SERVER);
        //sat_cfg_reset_str(SAT_CFG_STR_TLE_FILES);
        g_free(urls);
        g_free(tle_srv);
        g_free(tle_fstr);
        g_strfreev(tle_fvec);

        sat_cfg_set_int(SAT_CFG_INT_VERSION_MAJOR, 1);
        sat_cfg_set_int(SAT_CFG_INT_VERSION_MINOR, 4);
    }

    return 0;
}

/**
 * Save configuration data.
 * @return 0 on success, 1 if an error occurred.
 *
 * This function saves the configuration data currently stored in
 * memory to the gpredict.cfg file.
 */
guint sat_cfg_save()
{
    gchar          *keyfile;
    gchar          *confdir;
    guint           err = 0;

    confdir = get_user_conf_dir();
    keyfile = g_strconcat(confdir, G_DIR_SEPARATOR_S, "gpredict.cfg", NULL);
    err = gpredict_save_key_file(config, keyfile);
    g_free(confdir);

    return err;
}

/**
 * Close configuration module.
 *
 * This function cleans up the memory allocated to the storage and
 * management of configuration data. Note: configuration data will
 * no be accessible after call to this function, unless sat_cfg_load
 * is called again. This function should only be called when the
 * program exits.
 */
void sat_cfg_close()
{
    if (config != NULL)
    {
        g_key_file_free(config);
        config = NULL;
    }
}

/** Get boolean value */
gboolean sat_cfg_get_bool(sat_cfg_bool_e param)
{
    gboolean        value = FALSE;
    GError         *error = NULL;

    if (param < SAT_CFG_BOOL_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);

            /* return default value */
            value = sat_cfg_bool[param].defval;
        }
        else
        {
            /* fetch value */
            value = g_key_file_get_boolean(config,
                                           sat_cfg_bool[param].group,
                                           sat_cfg_bool[param].key, &error);

            if (error != NULL)
            {
                g_clear_error(&error);
                value = sat_cfg_bool[param].defval;
            }
        }

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown BOOL param index (%d)\n"), __func__, param);
    }

    return value;
}

/** Get default value of boolean parameter */
gboolean sat_cfg_get_bool_def(sat_cfg_bool_e param)
{
    gboolean        value = FALSE;

    if (param < SAT_CFG_BOOL_NUM)
    {
        value = sat_cfg_bool[param].defval;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown BOOL param index (%d)\n"), __func__, param);
    }

    return value;
}

/**
 * Store a boolean configuration value.
 * @param param The parameter to store.
 * @param value The value of the parameter.
 *
 * This function stores a boolean configuration value in the configuration
 * table.
 */
void sat_cfg_set_bool(sat_cfg_bool_e param, gboolean value)
{
    if (param < SAT_CFG_BOOL_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);
        }
        else
        {
            g_key_file_set_boolean(config,
                                   sat_cfg_bool[param].group,
                                   sat_cfg_bool[param].key, value);
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown BOOL param index (%d)\n"), __func__, param);
    }
}

void sat_cfg_reset_bool(sat_cfg_bool_e param)
{
    if (param < SAT_CFG_BOOL_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);
        }
        else
        {
            g_key_file_remove_key(config,
                                  sat_cfg_bool[param].group,
                                  sat_cfg_bool[param].key, NULL);
        }

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown BOOL param index (%d)\n"), __func__, param);
    }
}

/**
 * Get string value
 *
 * Return a newly allocated gchar * which must be freed when no longer needed.
 */
gchar          *sat_cfg_get_str(sat_cfg_str_e param)
{
    gchar          *value;
    GError         *error = NULL;

    if (param < SAT_CFG_STR_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);

            /* return default value */
            value = g_strdup(sat_cfg_str[param].defval);
        }
        else
        {
            /* fetch value */
            value = g_key_file_get_string(config,
                                          sat_cfg_str[param].group,
                                          sat_cfg_str[param].key, &error);
            if (error != NULL)
            {
                g_clear_error(&error);
                value = g_strdup(sat_cfg_str[param].defval);
            }
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown STR param index (%d)\n"), __func__, param);

        value = g_strdup("ERROR");
    }

    return value;
}

/**
 * Get default value of string parameter
 *
 * Returns a newly allocated gchar * which must be freed when no longer needed.
 */
gchar          *sat_cfg_get_str_def(sat_cfg_str_e param)
{
    gchar          *value;

    if (param < SAT_CFG_STR_NUM)
    {
        value = g_strdup(sat_cfg_str[param].defval);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown STR param index (%d)\n"), __func__, param);

        value = g_strdup("ERROR");
    }

    return value;
}

/** Store a str configuration value. */
void sat_cfg_set_str(sat_cfg_str_e param, const gchar * value)
{
    if (param < SAT_CFG_STR_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);
        }
        else
        {
            if (value)
            {
                g_key_file_set_string(config,
                                      sat_cfg_str[param].group,
                                      sat_cfg_str[param].key, value);
            }
            else
            {
                /* remove key from config */
                g_key_file_remove_key(config,
                                      sat_cfg_str[param].group,
                                      sat_cfg_str[param].key, NULL);
            }
        }
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown STR param index (%d)\n"), __func__, param);
    }
}

void sat_cfg_reset_str(sat_cfg_str_e param)
{

    if (param < SAT_CFG_STR_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);
        }
        else
        {
            g_key_file_remove_key(config,
                                  sat_cfg_str[param].group,
                                  sat_cfg_str[param].key, NULL);
        }

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown STR param index (%d)\n"), __func__, param);
    }
}

gint sat_cfg_get_int(sat_cfg_int_e param)
{
    gint            value = 0;
    GError         *error = NULL;

    if (param < SAT_CFG_INT_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);

            /* return default value */
            value = sat_cfg_int[param].defval;
        }
        else
        {
            /* fetch value */
            value = g_key_file_get_integer(config,
                                           sat_cfg_int[param].group,
                                           sat_cfg_int[param].key, &error);

            if (error != NULL)
            {
                g_clear_error(&error);
                value = sat_cfg_int[param].defval;
            }
        }

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown INT param index (%d)\n"), __func__, param);
    }

    return value;
}

gint sat_cfg_get_int_def(sat_cfg_int_e param)
{
    gint            value = 0;

    if (param < SAT_CFG_INT_NUM)
    {
        value = sat_cfg_int[param].defval;
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown INT param index (%d)\n"), __func__, param);
    }

    return value;
}

void sat_cfg_set_int(sat_cfg_int_e param, gint value)
{
    if (param < SAT_CFG_INT_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);
        }
        else
        {
            g_key_file_set_integer(config,
                                   sat_cfg_int[param].group,
                                   sat_cfg_int[param].key, value);
        }

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown INT param index (%d)\n"), __func__, param);
    }
}

void sat_cfg_reset_int(sat_cfg_int_e param)
{
    if (param < SAT_CFG_INT_NUM)
    {
        if (config == NULL)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Module not initialised\n"), __func__);
        }
        else
        {
            g_key_file_remove_key(config,
                                  sat_cfg_int[param].group,
                                  sat_cfg_int[param].key, NULL);
        }

    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Unknown INT param index (%d)\n"), __func__, param);
    }
}
