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
#ifndef MOD_CFG_H
#define MOD_CFG_H 1

#include <glib.h>


/** \brief Status codes returned by functions. */
typedef enum {
     MOD_CFG_OK = 0,   /*!< Operation performed ok, changes have been made */
     MOD_CFG_CANCEL,   /*!< Operation has been cancelled */
     MOD_CFG_ERROR     /*!< There was an error, see log messages */
} mod_cfg_status_t;


gchar            *mod_cfg_new    (void);
mod_cfg_status_t  mod_cfg_edit   (gchar *modname, GKeyFile *cfgdata, GtkWidget *toplevel);
mod_cfg_status_t  mod_cfg_save   (gchar *modname, GKeyFile *cfgdata);
mod_cfg_status_t  mod_cfg_delete (gchar *modname, gboolean needcfm);


#endif
