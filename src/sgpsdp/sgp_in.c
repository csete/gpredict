/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  This unit is:

  Copyright (C)  1992-1999  Dr TS Kelso.
  Copyright (C)  2001 by N. Kyriazis
  Copyright (C)  2006-2008 Alexandru Csete, OZ9AEC.


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
/* original header:               */
/* Unit SGP_In                    */
/*           Author:  Dr TS Kelso */
/* Original Version:  1992 Jun 25 */
/* Current Revision:  1999 Nov 27 */
/*          Version:  2.10 */
/*        Copyright:  1992-1999, All Rights Reserved */

/* Ported to C by N. Kyriazis  April 6  2001 */

#include <glib.h>
#include <glib/gprintf.h>
#include "sgp4sdp4.h"

/* Calculates the checksum mod 10 of a line from a TLE set and */
/* returns 1 if it compares with checksum in column 68, else 0.*/
/* tle_set is a character string holding the two lines read    */
/* from a text file containing NASA format Keplerian elements. */
/* NOTE!!! The stuff about two lines is not quite true.
   The function assumes that tle_set[0] is the begining
   of the line and that there are 68 elements - see the consumer
*/
int Checksum_Good(char *tle_set)
{
    int             i, check_digit, value, checksum = 0;

    if (tle_set == NULL)
    {
        return 0;
    }

    for (i = 0; i < 68; i++)
    {
        if ((tle_set[i] >= '0') && (tle_set[i] <= '9'))
            value = tle_set[i] - '0';
        else if (tle_set[i] == '-')
            value = 1;
        else
            value = 0;

        checksum += value;
    }                           /* End for(i = 0; i < 68; i++) */

    checksum %= 10;
    check_digit = tle_set[68] - '0';

    return (checksum == check_digit);
}

/* Carries out various checks on a TLE set to verify its validity */
/* tle_set is a character string holding the two lines read    */
/* from a text file containing NASA format Keplerian elements. */
int Good_Elements(char *tle_set)
{
    /* Verify checksum of both lines of a TLE set */
    if (!Checksum_Good(&tle_set[0]) || !Checksum_Good(&tle_set[69]))
        return (0);
    /* Check the line number of each line */
    if ((tle_set[0] != '1') || (tle_set[69] != '2'))
        return (0);
    /* Verify that Satellite Number is same in both lines */
    if (strncmp(&tle_set[2], &tle_set[71], 5) != 0)
        return (0);
    /* Check that various elements are in the right place */
    if ((tle_set[23] != '.') ||
        (tle_set[34] != '.') ||
        (tle_set[80] != '.') ||
        (tle_set[89] != '.') ||
        (tle_set[106] != '.') ||
        (tle_set[115] != '.') ||
        (tle_set[123] != '.') || (strncmp(&tle_set[61], " 0 ", 3) != 0))
        return (0);

    return (1);
}

/* Converts the strings in a raw two-line element set  */
/* to their intended numerical values. No processing   */
/* of these values is done, e.g. from deg to rads etc. */
/* This is done in the select_ephemeris() function.    */
void Convert_Satellite_Data(char *tle_set, tle_t * tle)
{
    char            buff[15];

    /* Satellite's catalogue number */
    strncpy(buff, &tle_set[2], 5);
    buff[5] = '\0';
    tle->catnr = atoi(buff);

    /* International Designator for satellite */
    strncpy(tle->idesg, &tle_set[9], 8);
    tle->idesg[8] = '\0';

    /* Epoch time; this is the complete,
       unconverted epoch.
     */
    strncpy(buff, &tle_set[18], 14);
    /* The DDD field may be padded with spaces instead of zeros,
       but we're about to interpret it as a single real number
       with g_ascii_strtod, so convert spaces to zeros if needed. */
    if (buff[2] == ' ')
        buff[2] = '0';
    if (buff[3] == ' ')
        buff[3] = '0';
    buff[14] = '\0';
    //        tle->epoch = atof (buff);
    tle->epoch = g_ascii_strtod(buff, NULL);

    /* Now, convert the epoch time into year, day
       and fraction of day, according to:

       YYDDD.FFFFFFFF
     */

    /* Epoch year; we assume > 2000 ... */
    strncpy(buff, &tle_set[18], 2);
    buff[2] = '\0';
    tle->epoch_year = 2000 + atoi(buff);

    /* Epoch day */
    strncpy(buff, &tle_set[20], 3);
    buff[3] = '\0';
    tle->epoch_day = atoi(buff);

    /* Epoch fraction of day */
    buff[0] = '0';
    strncpy(&buff[1], &tle_set[23], 9);
    buff[10] = '\0';
    tle->epoch_fod = g_ascii_strtod(buff, NULL);


    /* Satellite's First Time Derivative */
    strncpy(buff, &tle_set[33], 10);
    buff[10] = '\0';
    tle->xndt2o = g_ascii_strtod(buff, NULL);

    /* Satellite's Second Time Derivative */
    strncpy(buff, &tle_set[44], 1);
    buff[1] = '.';
    strncpy(&buff[2], &tle_set[45], 5);
    buff[7] = 'E';
    strncpy(&buff[8], &tle_set[50], 2);
    buff[10] = '\0';
    tle->xndd6o = g_ascii_strtod(buff, NULL);

    /* Satellite's bstar drag term
     * FIXME: How about buff[0] ????
     */
    strncpy(buff, &tle_set[53], 1);
    buff[1] = '.';
    strncpy(&buff[2], &tle_set[54], 5);
    buff[7] = 'E';
    strncpy(&buff[8], &tle_set[59], 2);
    buff[10] = '\0';
    tle->bstar = g_ascii_strtod(buff, NULL);

    /* Element Number */
    strncpy(buff, &tle_set[64], 4);
    buff[4] = '\0';
    tle->elset = atoi(buff);

    /* Satellite's Orbital Inclination (degrees) */
    strncpy(buff, &tle_set[77], 8);
    buff[8] = '\0';
    tle->xincl = g_ascii_strtod(buff, NULL);

    /* Satellite's RAAN (degrees) */
    strncpy(buff, &tle_set[86], 8);
    buff[8] = '\0';
    tle->xnodeo = g_ascii_strtod(buff, NULL);

    /* Satellite's Orbital Eccentricity */
    buff[0] = '.';
    strncpy(&buff[1], &tle_set[95], 7);
    buff[8] = '\0';
    tle->eo = g_ascii_strtod(buff, NULL);
    /* avoid division by 0 */
    if (tle->eo < 1.0e-6)
        tle->eo = 1.0e-6;

    /* Satellite's Argument of Perigee (degrees) */
    strncpy(buff, &tle_set[103], 8);
    buff[8] = '\0';
    tle->omegao = g_ascii_strtod(buff, NULL);

    /* Satellite's Mean Anomaly of Orbit (degrees) */
    strncpy(buff, &tle_set[112], 8);
    buff[8] = '\0';
    tle->xmo = g_ascii_strtod(buff, NULL);

    /* Satellite's Mean Motion (rev/day) */
    strncpy(buff, &tle_set[121], 10);
    buff[10] = '\0';
    tle->xno = g_ascii_strtod(buff, NULL);

    /* Satellite's Revolution number at epoch */
    strncpy(buff, &tle_set[132], 5);
    buff[5] = '\0';
    tle->revnum = g_ascii_strtod(buff, NULL);

}

int Get_Next_Tle_Set(char line[3][80], tle_t * tle)
{
    int             idx,        /* Index for loops and arrays    */
                    chr;        /* Used for inputting characters */

    char            tle_set[139];       /* Two lines of a TLE set */


    /* set status to unknown by sefault */
    tle->status = OP_STAT_UNKNOWN;

    /* Read the satellite's name */
    for (idx = 0; idx < 25; idx++)
    {
        chr = line[0][idx];
        if ((chr != CR) && (chr != LF) && (chr != '\0'))
        {
            /* avoid escaping & */
            if (chr == '&')
            {
                tle->sat_name[idx] = '/';
                continue;
            }

            /* check for operational status encoded in name */
            if ((idx < 23) && (chr == '[') && (line[0][idx + 2] == ']') &&
                ((line[0][idx + 1] == '+') ||
                 (line[0][idx + 1] == '-') ||
                 (line[0][idx + 1] == 'P') ||
                 (line[0][idx + 1] == 'B') ||
                 (line[0][idx + 1] == 'S') || (line[0][idx + 1] == 'X')))
            {
                /* get operational status */
                switch (line[0][idx + 1])
                {
                case '+':
                    tle->status = OP_STAT_OPERATIONAL;
                    break;

                case '-':
                    tle->status = OP_STAT_NONOP;
                    break;

                case 'P':
                    tle->status = OP_STAT_PARTIAL;
                    break;

                case 'B':
                    tle->status = OP_STAT_STDBY;
                    break;

                case 'S':
                    tle->status = OP_STAT_SPARE;
                    break;

                case 'X':
                    tle->status = OP_STAT_EXTENDED;
                    break;

                default:
                    tle->status = OP_STAT_UNKNOWN;
                    break;
                }

                /* terminate sat_name string */
                tle->sat_name[idx - 1] = '\0';

                /* force loop exit */
                idx = 25;
            }
            else
            {
                /* no operational status in name, go on */
                tle->sat_name[idx] = chr;
            }
        }
        else
        {
            /* strip off trailing spaces */
            while ((chr = line[0][--idx]) == ' ');
            tle->sat_name[++idx] = '\0';
            break;
        }
    }

    /* Read in first line of TLE set */
    strncpy(tle_set, line[1], 70);

    /* Read in second line of TLE set and terminate string */
    strncpy(&tle_set[69], line[2], 70);
    tle_set[138] = '\0';

    /* Check TLE set and abort if not valid */
    if (!Good_Elements(tle_set))
        return (-2);

    /* Convert the TLE set to orbital elements */
    Convert_Satellite_Data(tle_set, tle);

    return (1);
}

/* Selects the apropriate ephemeris type to be used */
/* for predictions according to the data in the TLE */
/* It also processes values in the tle set so that  */
/* they are apropriate for the sgp4/sdp4 routines   */
void select_ephemeris(sat_t * sat)
{
    double          ao, xnodp, dd1, dd2, delo, temp, a1, del1, r1;

    /* Preprocess tle set */
    sat->tle.xnodeo *= de2ra;
    sat->tle.omegao *= de2ra;
    sat->tle.xmo *= de2ra;
    sat->tle.xincl *= de2ra;
    temp = twopi / xmnpda / xmnpda;

    /* store mean motion before conversion */
    sat->meanmo = sat->tle.xno;
    sat->tle.xno = sat->tle.xno * temp * xmnpda;
    sat->tle.xndt2o *= temp;
    sat->tle.xndd6o = sat->tle.xndd6o * temp / xmnpda;
    sat->tle.bstar /= ae;

    /* Period > 225 minutes is deep space */
    dd1 = (xke / sat->tle.xno);
    dd2 = tothrd;
    a1 = pow(dd1, dd2);
    r1 = cos(sat->tle.xincl);
    dd1 = (1.0 - sat->tle.eo * sat->tle.eo);
    temp = ck2 * 1.5f * (r1 * r1 * 3.0 - 1.0) / pow(dd1, 1.5);
    del1 = temp / (a1 * a1);
    ao = a1 * (1.0 - del1 * (tothrd * 0.5 + del1 *
                             (del1 * 1.654320987654321 + 1.0)));
    delo = temp / (ao * ao);
    xnodp = sat->tle.xno / (delo + 1.0);

    /* Select a deep-space/near-earth ephemeris */
    if (twopi / xnodp / xmnpda >= .15625)
        sat->flags |= DEEP_SPACE_EPHEM_FLAG;
    else
        sat->flags &= ~DEEP_SPACE_EPHEM_FLAG;

    return;
}
