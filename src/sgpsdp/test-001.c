/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2008  Alexandru Csete.

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
    along with this program; if not, write to the
          Free Software Foundation, Inc.,
      59 Temple Place, Suite 330,
      Boston, MA  02111-1307
      USA
*/
/* Unit test for SGP4 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sgp4sdp4.h"

#define TEST_STEPS 5

/* structure to hold a set of data */
typedef struct {
    double          t;
    double          x;
    double          y;
    double          z;
    double          vx;
    double          vy;
    double          vz;
} dataset_t;


const dataset_t expected[TEST_STEPS] = {
    {0.0,
     2328.97048951, -5995.22076416, 1719.97067261,
     2.91207230, -0.98341546, -7.09081703},
    {360.0,
     2456.10705566, -6071.93853760, 1222.89727783,
     2.67938992, -0.44829041, -7.22879231},
    {720.0,
     2567.56195068, -6112.50384522, 713.96397400,
     2.44024599, 0.09810869, -7.31995916},
    {1080.0,
     2663.09078980, -6115.48229980, 196.39640427,
     2.19611958, 0.65241995, -7.36282432},
    {1440.0,
     2742.55133057, -6079.67144775, -326.38095856,
     1.94850229, 1.21106251, -7.35619372}
};


char            tle_str[3][80];
sat_t           sat;

int main(void)
{
    FILE           *fp;
    int             i;

    /* read tle file */
    fp = fopen("test-001.tle", "r");
    if (fp != NULL)
    {
        if (fgets(tle_str[0], 80, fp) == NULL)
        {
            printf("Error reading TLE line 1\n");
            fclose(fp);
            return 1;
        }
        if (fgets(tle_str[1], 80, fp) == NULL)
        {
            printf("Error reading TLE line 2\n");
            fclose(fp);
            return 1;
        }
        if (fgets(tle_str[2], 80, fp) == NULL)
        {
            printf("Error reading TLE line 3\n");
            fclose(fp);
            return 1;
        }
        fclose(fp);

        if (Get_Next_Tle_Set(tle_str, &sat.tle) == 1)
        {
            printf("TEST DATA:\n");
        }
        else
        {
            printf("Could not read TLE data 1\n");
            return 1;
        }
    }
    else
    {
        printf("Could not open test-001.tle\n");
        return 1;
    }

    printf("%s", tle_str[0]);
    printf("%s", tle_str[1]);
    printf("%s", tle_str[2]);

    select_ephemeris(&sat);

    printf("\nDEEP_SPACE_EPHEM: %d (expected 0)\n\n",
           (sat.flags & DEEP_SPACE_EPHEM_FLAG));


    printf("                         RESULT          EXPECTED          "
           "  DELTA\n");
    printf("-----------------------------------------------------------"
           "-------------------\n");

    for (i = 0; i < TEST_STEPS; i++)
    {

        SGP4(&sat, expected[i].t);
        Convert_Sat_State(&sat.pos, &sat.vel);

        printf("STEP %d  t: %6.1f  X: %14.8f   %14.8f   %.8f (%.5f%%)\n",
               i + 1, expected[i].t, sat.pos.x, expected[i].x,
               fabs(sat.pos.x - expected[i].x),
               100.0 * fabs(sat.pos.x - expected[i].x) / fabs(expected[i].x));
        printf("                   Y: %14.8f   %14.8f   %.8f (%.5f%%)\n",
               sat.pos.y, expected[i].y,
               fabs(sat.pos.y - expected[i].y),
               100.0 * fabs(sat.pos.y - expected[i].y) / fabs(expected[i].y));
        printf("                   Z: %14.8f   %14.8f   %.8f (%.5f%%)\n",
               sat.pos.z, expected[i].z,
               fabs(sat.pos.z - expected[i].z),
               100.0 * fabs(sat.pos.z - expected[i].z) / fabs(expected[i].z));
        printf("                  VX: %14.8f   %14.8f   %.8f (%.5f%%)\n",
               sat.vel.x, expected[i].vx,
               fabs(sat.vel.x - expected[i].vx),
               100.0 * fabs(sat.vel.x -
                            expected[i].vx) / fabs(expected[i].vx));
        printf("                  VY: %14.8f   %14.8f   %.8f (%.5f%%)\n",
               sat.vel.y, expected[i].vy, fabs(sat.vel.y - expected[i].vy),
               100.0 * fabs(sat.vel.y -
                            expected[i].vy) / fabs(expected[i].vy));
        printf("                  VZ: %14.8f   %14.8f   %.8f (%.5f%%)\n",
               sat.vel.z, expected[i].vz, fabs(sat.vel.z - expected[i].vz),
               100.0 * fabs(sat.vel.z -
                            expected[i].vz) / fabs(expected[i].vz));
    }

    return 0;
}
