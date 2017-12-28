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
     7473.37066650, 428.95261765, 5828.74786377,
     5.1071513, 6.44468284, -0.18613096},
    {360.0,
     -3305.22537232, 32410.86328125, -24697.17675781,
     -1.30113538, -1.15131518, -0.28333528},
    {720.0,
     14271.28759766, 24110.46411133, -4725.76837158,
     -0.32050445, 2.67984074, -2.08405289},
    {1080.0,
     -9990.05883789, 22717.35522461, -23616.890662501,
     -1.01667246, -2.29026759, 0.72892364},
    {1440.0,
     9787.86975097, 33753.34667969, -15030.81176758,
     -1.09425966, 0.92358845, -1.52230928}
};


char            tle_str[3][80];
sat_t           sat;

int main(void)
{
    FILE           *fp;
    int             i;

    /* read tle file */
    fp = fopen("test-002.tle", "r");
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
            printf("Could not read TLE data 2\n");
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

    printf("\nDEEP_SPACE_EPHEM: %d (expected %d)\n\n",
           (sat.flags & DEEP_SPACE_EPHEM_FLAG), DEEP_SPACE_EPHEM_FLAG);


    printf("                          RESULT            EXPECTED       "
           "     DELTA\n");
    printf("-----------------------------------------------------------"
           "----------------------\n");

    for (i = 0; i < TEST_STEPS; i++)
    {
        SDP4(&sat, expected[i].t);
        Convert_Sat_State(&sat.pos, &sat.vel);

        printf("STEP %d  t: %6.1f  X: %15.8f   %15.8f   %.8f (%.5f%%)\n",
               i + 1, expected[i].t, sat.pos.x, expected[i].x,
               fabs(sat.pos.x - expected[i].x),
               100.0 * fabs(sat.pos.x - expected[i].x) / fabs(expected[i].x));
        printf("                   Y: %15.8f   %15.8f   %.8f (%.5f%%)\n",
               sat.pos.y, expected[i].y,
               fabs(sat.pos.y - expected[i].y),
               100.0 * fabs(sat.pos.y - expected[i].y) / fabs(expected[i].y));
        printf("                   Z: %15.8f   %15.8f   %.8f (%.5f%%)\n",
               sat.pos.z, expected[i].z,
               fabs(sat.pos.z - expected[i].z),
               100.0 * fabs(sat.pos.z - expected[i].z) / fabs(expected[i].z));
        printf("                  VX: %15.8f   %15.8f   %.8f (%.5f%%)\n",
               sat.vel.x, expected[i].vx,
               fabs(sat.vel.x - expected[i].vx),
               100.0 * fabs(sat.vel.x -
                            expected[i].vx) / fabs(expected[i].vx));
        printf("                  VY: %15.8f   %15.8f   %.8f (%.5f%%)\n",
               sat.vel.y, expected[i].vy, fabs(sat.vel.y - expected[i].vy),
               100.0 * fabs(sat.vel.y -
                            expected[i].vy) / fabs(expected[i].vy));
        printf("                  VZ: %15.8f   %15.8f   %.8f (%.5f%%)\n",
               sat.vel.z, expected[i].vz, fabs(sat.vel.z - expected[i].vz),
               100.0 * fabs(sat.vel.z -
                            expected[i].vz) / fabs(expected[i].vz));
    }

    return 0;
}
