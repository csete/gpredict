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
/** \defgroup gui Graphical User Interface.
 *
 * This is the main section containing everything related to the graphical user
 * interface in gpredict.
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "compat.h"
#include "menubar.h"
#include "gui.h"
#include "mod-mgr.h"

void callback( GtkWidget *widget,
               gpointer   data );

/** \brief Create main GUI components.
 *  \return A container widget containing the GUI.
 *
 * This function creates the individual GUI component by calling the
 * corresponding sub functions and returns them packed within one single
 * container, a GtkVBox.
 *
 * The internal structure of the GtkVBox is opaque from the outside and the
 * layout between the individual subcomponents can be controled within this
 * function.
 *
 */
GtkWidget *
gui_create (GtkWidget *window)
{
     GtkWidget *vbox;


     vbox = gtk_vbox_new (FALSE, 0);

     /* add menu bar */
     gtk_box_pack_start (GTK_BOX (vbox),
                   menubar_create (window),
                   FALSE,
                   FALSE,
                   0);

     /* add tool bar */
/*      gtk_box_pack_start (GTK_BOX (vbox), */
/*                    gpredict_gui_toolbar_create (), */
/*                    FALSE, */
/*                    FALSE, */
/*                    0); */


     /* add module manager */
     gtk_box_pack_start (GTK_BOX (vbox), mod_mgr_create (), TRUE, TRUE, 0);

     
     /* add bottom info box */

     /* add status bar */

     return vbox;
}

