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
#ifndef SAT_LOG_H
#define SAT_LOG_H 1

#include <gtk/gtk.h>

#define SAT_LOG_MSG_SEPARATOR "|"

/** \brief Debug message sources. */
typedef enum {
     SAT_LOG_SRC_NONE     = 0,     /*!< No source, unknown source. */
     SAT_LOG_SRC_HAMLIB   = 1,     /*!< Debug message comes from hamlib */
     SAT_LOG_SRC_GPREDICT = 2      /*!< Debug message comes from grig itself */
} sat_log_src_t;


typedef enum {
     SAT_LOG_LEVEL_NONE  = 0,
     SAT_LOG_LEVEL_BUG   = 1,
     SAT_LOG_LEVEL_ERROR = 2,
     SAT_LOG_LEVEL_WARN  = 3,
     SAT_LOG_LEVEL_MSG   = 4,
     SAT_LOG_LEVEL_DEBUG = 5
} sat_log_level_t;


void sat_log_init        (void);
void sat_log_close       (void);
void sat_log_log         (sat_log_level_t level, const char *fmt, ...);
void sat_log_set_visible (gboolean visible);
void sat_log_set_level   (sat_log_level_t level);


#endif
