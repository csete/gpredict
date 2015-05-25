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
#ifndef MOD_CFG_GET_PARAM_H
#define MOD_CFG_GET_PARAM_H 1

#include "sat-cfg.h"

gboolean        mod_cfg_get_bool(GKeyFile * f, const gchar * sec,
                                 const gchar * key, sat_cfg_bool_e p);
gint            mod_cfg_get_int(GKeyFile * f, const gchar * sec,
                                const gchar * key, sat_cfg_int_e p);
gchar          *mod_cfg_get_str(GKeyFile * f, const gchar * sec,
                                const gchar * key, sat_cfg_str_e p);
void            mod_cfg_get_integer_list_boolean(GKeyFile * cfgdata,
                                                 const gchar * section,
                                                 const gchar * key,
                                                 GHashTable * dest);
void            mod_cfg_set_integer_list_boolean(GKeyFile * cfgdata,
                                                 GHashTable * hash,
                                                 const gchar * cfgsection,
                                                 const gchar * cfgkey);

#endif
