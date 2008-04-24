/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>

  Comments, questions and bugreports should be submitted via
  http://sourceforge.net/projects/groundstation/
  More details can be found at the project home page:

  http://groundstation.sourceforge.net/
 
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "sat-log.h"

#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif

#include "rot-ctrl-window.h"


static gboolean window_is_open = FALSE;

/** \brief Open rotator control window */
void rot_ctrl_window_open ()
{
    if (window_is_open) {
        /* window may be hidden so bring it to front */
        
        return;
    }
    
    /* create the window */
    window_is_open = TRUE;
}

/** \brief Close rotator control window */
void rot_ctrl_window_close ()
{
    if (window_is_open) {
        window_is_open = FALSE;
    }
    else {
        sat_log_log (SAT_LOG_LEVEL_BUG,
                     _("%s: Rotor control window is not open!"),
                     __FUNCTION__);
    }
}


