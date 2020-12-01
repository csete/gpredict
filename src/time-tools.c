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

#include <glib.h>
#include <glib/gi18n.h>
//#include <sys/time.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"
#include "sat-cfg.h"
//#ifdef G_OS_WIN32
//#  include "libc_internal.h"
//#  include "libc_interface.h"
//#endif



/** \brief Get the current time.
 *
 * Read the system clock and return the current Julian day.
 */
gdouble
get_current_daynum ()
{
    struct tm utc;
    GDateTime *now;
    double daynum;

    UTC_Calendar_Now (&utc);
    now = g_date_time_new_now_local();
    daynum = Julian_Date (&utc);
    daynum = daynum + (double)g_date_time_get_microsecond(now)/8.64e+10;
    g_date_time_unref(now);

    return daynum;
}

int
daynum_to_str(char *s, size_t max, const char *format, gdouble jultime){
    //    printf("Someone called me\n");
    time_t tim;
    size_t size=0;
    tim = (jultime - 2440587.5)*86400.0;
    if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_LOCAL_TIME))
        size = strftime (s, max, format, localtime (&tim));
    else
        size = strftime (s, max, format, gmtime (&tim));

    if (size<max) 
        s[size] = '\0';
    else
        s[max-1] = '\0';
    return size;
}

/* This function calculates the day number from m/d/y. */
/* Legacy code no longer in use
long
get_daynum_from_dmy (int d, int m, int y)
{

    long dn;
    double mm, yy;

    if (m<3)
    { 
        y--; 
        m+=12; 
    }

    if (y<57)
        y+=100;

    yy=(double)y;
    mm=(double)m;
    dn=(long)(floor(365.25*(yy-80.0))-floor(19.0+yy/100.0)+floor(4.75+yy/400.0)-16.0);
    dn+=d+30*m+(long)floor(0.6*mm-0.3);

    return dn;
}
*/
