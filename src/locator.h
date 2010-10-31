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
#ifndef LOCATOR_H
#define LOCATOR_H 1

#define RIG_OK     0
#define RIG_EINVAL 1


int    qrb                (double lon1, double lat1, double lon2, double lat2,
                           double *distance, double *azimuth);

double distance_long_path (double distance);

double azimuth_long_path  (double azimuth);

int    longlat2locator    (double longitude, double latitude,
                           char *locator, int pair_count);

int    locator2longlat    (double *longitude, double *latitude,
                           const char *locator);

double dms2dec            (int degrees, int minutes, double seconds, int sw);

int    dec2dms            (double dec,
                           int *degrees, int *minutes, double *seconds, int *sw);

int    dec2dmmm           (double dec, int *degrees, double *minutes, int *sw);

double dmmm2dec           (int degrees, double minutes, int sw);


#endif
