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
#ifndef FIRST_TIME_H
#define FIRST_TIME_H 1


/** \brief Bit fields in the returned error code */
enum {
    FTC_ERROR_STEP_01 = 1 << 1,
    FTC_ERROR_STEP_02 = 1 << 2,
    FTC_ERROR_STEP_03 = 1 << 3,
    FTC_ERROR_STEP_04 = 1 << 4,
    FTC_ERROR_STEP_05 = 1 << 5,
    FTC_ERROR_STEP_06 = 1 << 6,
    FTC_ERROR_STEP_07 = 1 << 7,
    FTC_ERROR_STEP_08 = 1 << 8,
    FTC_ERROR_STEP_09 = 1 << 9
};

guint first_time_check_run (void);

#endif
