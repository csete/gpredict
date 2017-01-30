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
#ifndef SAT_LOG_H
#define SAT_LOG_H 1

#include <gtk/gtk.h>

#define SAT_LOG_MSG_SEPARATOR "|"

typedef enum {
    SAT_LOG_LEVEL_NONE = 0,
    SAT_LOG_LEVEL_ERROR = 1,
    SAT_LOG_LEVEL_WARN = 2,
    SAT_LOG_LEVEL_INFO = 3,
    SAT_LOG_LEVEL_DEBUG = 4
} sat_log_level_t;

void            sat_log_init(void);
void            sat_log_close(void);
void            sat_log_log(sat_log_level_t level, const char *fmt, ...);
void            sat_log_set_visible(gboolean visible);
void            sat_log_set_level(sat_log_level_t level);

#endif
