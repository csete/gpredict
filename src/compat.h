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
#ifndef COMPAT_H
#define COMPAT_H 1


gchar *get_data_dir   (void);
gchar *get_maps_dir   (void);
gchar *get_icon_dir   (void);
gchar *get_user_conf_dir (void);
gchar *get_modules_dir (void);
gchar *get_satdata_dir (void);
gchar *get_trsp_dir    (void);
gchar *get_hwconf_dir  (void);
gchar *get_old_conf_dir (void);
gchar *map_file_name  (const gchar *map);
gchar *icon_file_name (const gchar *icon);
gchar *data_file_name (const gchar *data);
gchar *sat_file_name  (const gchar *satfile);
gchar *trsp_file_name (const gchar *trspfile);
gchar *hw_file_name   (const gchar *hwfile);

gchar *sat_file_name_from_catnum (guint catnum);
gchar *sat_file_name_from_catnum_s (gchar *catnum);

#endif
