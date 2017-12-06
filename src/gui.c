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
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "compat.h"
#include "gui.h"
#include "menubar.h"
#include "mod-mgr.h"

GtkWidget *gui_create(GtkWidget *window)
{
     GtkWidget *vbox;

     vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
     gtk_box_pack_start(GTK_BOX(vbox), menubar_create(window), FALSE, FALSE, 1);
     gtk_box_pack_start(GTK_BOX(vbox), mod_mgr_create(), TRUE, TRUE, 0);

     return vbox;
}
