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
#ifndef TLE_LOOKUP_H
#define TLE_LOOKUP_H 1

#include <glib.h>


/** \brief TLE lookup error codes
    \ingroup tle_lookup_if
 */
typedef enum {
	TLE_LOOKUP_INIT_OK = 0,     /*!< No errors. */
	TLE_LOOKUP_EMPTY_DIR = 1,   /*!< TLE directory empty, nothing loaded. */
	TLE_LOOKUP_NO_MEM = 2       /*!< Insufficient memory (fatal). */
} tle_lookup_err_t;

gint   tle_lookup_init  (const gchar *tledir);
void   tle_lookup_close (void);
gchar *tle_lookup       (gint catnum);
guint  tle_lookup_count (void);


#endif
