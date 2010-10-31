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
#ifndef SAT_PREF_HELP_H
#define SAT_PREF_HELP_H 1



/** \brief Structure representing a boolean value */
typedef struct {
     gchar    *type;      /*!< The label shown on the selector */
     gchar    *cmd;     /*!< The configuration key */
} sat_help_t;


typedef enum {
     BROWSER_TYPE_NONE = 0,
#ifdef G_OS_UNIX
     BROWSER_TYPE_EPIPHANY,
     BROWSER_TYPE_GALEON,
     BROWSER_TYPE_KONQUEROR,
#endif
     BROWSER_TYPE_FIREFOX,
     BROWSER_TYPE_MOZILLA,
     BROWSER_TYPE_OPERA,
#ifdef G_OS_WIN32
     BROWSER_TYPE_IE,
#endif
     BROWSER_TYPE_OTHER,
     BROWSER_TYPE_NUM
} browser_type_t;



GtkWidget *sat_pref_help_create (void);
void       sat_pref_help_cancel (void);
void       sat_pref_help_ok     (void);


#endif
