/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2013  Alexandru Csete, OZ9AEC.

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
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif


/*! \brief Rotate map to be centered around a specific longitude.
 *  \param in Input map centered around 0 degrees longitude.
 *  \param out Output map that will be centered around clon. Must have the same
 *             dimension and color space as the input map.
 *  \param clon The longitude that should be at center between -180 and 180 degrees (East positive).
 * 
 * This function will rotate the input map to be centered around the specified longitude.
 * The output map must be pre-allocated and have the sae width, height and colorspace as the input
 * map. The longitude parameter must be between -180 and 180 degrees with negative values meaning
 * West of Greenwich,positive values meaning East of Greenwich.
 */
void map_tools_shift_center(GdkPixbuf *in, GdkPixbuf *out, float clon)
{
    if (clon > 180.0)
        clon = 180.0;
    else if (clon < -180.0)
        clon = -180.0;

    int lon_x = round((clon+180.0) * gdk_pixbuf_get_width(in)/360.0);
    int img_w = gdk_pixbuf_get_width(in);
    int img_h = gdk_pixbuf_get_height(in);
    
    if (clon < 0.0) {
        /* copy left side to right */
        gdk_pixbuf_copy_area(in,
                             0,
                             0,
                             lon_x + img_w/2,
                             img_h,
                             out,
                             img_w - (lon_x + img_w/2),
                             0);
        /* copy right side to left */
        gdk_pixbuf_copy_area(in,
                             lon_x + img_w/2,
                             0,
                             img_w - (lon_x + img_w/2),
                             img_h,
                             out,
                             0,
                             0);
    }
    else if (clon > 0.0) {
        /* copy left side to right */
        gdk_pixbuf_copy_area(in,
                             0,
                             0,
                             lon_x - img_w/2,
                             img_h,
                             out,
                             img_w - (lon_x - img_w/2),
                             0);
        /* copy right side to left */
        gdk_pixbuf_copy_area(in,
                             lon_x - img_w/2,
                             0,
                             img_w - (lon_x - img_w/2),
                             img_h,
                             out,
                             0,
                             0);
    }
    else {
        /* no shifting of center */
        gdk_pixbuf_copy_area(in,
                             0,
                             0,
                             img_w,
                             img_h,
                             out,
                             0,
                             0);
    }   
}

