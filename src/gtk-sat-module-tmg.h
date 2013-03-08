/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

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

/****

     NOTE: This file is an internal part of gtk-sat-module and should not
     be used by other files than gtk-sat-module.c and gtk-sat-module-popup.c

*****/

#ifndef __GTK_SAT_MODULE_TMG_H__
#define __GTK_SAT_MODULE_TMG_H__ 1

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



void tmg_create (GtkSatModule *mod);
void tmg_update_widgets (GtkSatModule *mod);
void tmg_update_state (GtkSatModule *mod);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_SAT_MODULE_H__ */
