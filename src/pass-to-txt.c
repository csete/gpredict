/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "locator.h"
#include "pass-to-txt.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-vis.h"
#include "sat-pass-dialogs.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"


#define NUMCOL 19
#define COL_WIDTH 8


const gchar    *SPCT[] = {
    N_(" Time"),
    N_("  Az  "),               /* 6 */
    N_("   El "),
    N_("  Ra  "),
    N_("  Dec "),
    N_("Range"),
    N_(" Rate "),
    N_("  Lat "),
    N_("  Lon  "),
    N_(" SSP  "),
    N_("Footp"),
    N_(" Alt "),
    N_(" Vel "),
    N_(" Dop "),
    N_(" Loss "),
    N_(" Del "),
    N_("  MA  "),
    N_(" Pha  "),
    N_("Vis")
};


const guint     COLW[] = {
    0,
    6,
    6,
    6,
    6,
    5,
    6,
    6,
    7,
    6,
    5,
    5,
    5,
    5,
    6,
    5,
    6,
    6,
    3
};


const gchar    *MPCT[] = {
    N_(" AOS"),
    N_("  TCA"),                /* 6 */
    N_("  LOS"),
    N_("Duration"),
    N_("Max El"),
    N_("AOS Az"),
    N_("Max El Az"),
    N_("LOS Az"),
    N_("Orbit"),
    N_("Vis")
};


const guint     MCW[] = {
    0,
    0,
    0,
    8,
    6,
    6,
    9,
    6,
    5,
    3
};


static void     Calc_RADec(gdouble jul_utc, gdouble saz, gdouble sel,
                           qth_t * qth, obs_astro_t * obs_set);

gchar          *pass_to_txt_pgheader(pass_t * pass, qth_t * qth, gint fields)
{
    gboolean        loc;
    gchar          *utc;
    gchar          *header;
    gchar           aosbuff[TIME_FORMAT_MAX_LENGTH];
    gchar           losbuff[TIME_FORMAT_MAX_LENGTH];
    gchar          *fmtstr;
    time_t          aos, los;
    guint           size;

    (void)fields;               /* avoid unused parameter compiler warning */

    fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
    loc = sat_cfg_get_bool(SAT_CFG_BOOL_USE_LOCAL_TIME);
    aos = (pass->aos - 2440587.5) * 86400.;
    los = (pass->los - 2440587.5) * 86400.;

    if (loc)
    {
        utc = g_strdup(_("Local"));

        /* AOS */
        size =
            strftime(aosbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, localtime(&aos));
        if (size == 0)
            /* size > MAX_LENGTH */
            aosbuff[TIME_FORMAT_MAX_LENGTH - 1] = '\0';

        /* LOS */
        size =
            strftime(losbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, localtime(&los));
        if (size == 0)
            /* size > MAX_LENGTH */
            losbuff[TIME_FORMAT_MAX_LENGTH - 1] = '\0';

    }
    else
    {
        utc = g_strdup(_("UTC"));

        /* AOS */
        size = strftime(aosbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, gmtime(&aos));
        if (size == 0)
            /* size > MAX_LENGTH */
            aosbuff[TIME_FORMAT_MAX_LENGTH - 1] = '\0';

        /* LOS */
        size = strftime(losbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, gmtime(&los));
        if (size == 0)
            /* size > MAX_LENGTH */
            losbuff[TIME_FORMAT_MAX_LENGTH - 1] = '\0';
    }

    header = g_strdup_printf(_("Pass details for %s (orbit %d)\n"
                               "Observer: %s, %s\n"
                               "LAT:%.2f LON:%.2f\n"
                               "AOS: %s %s\n"
                               "LOS: %s %s\n"),
                             pass->satname, pass->orbit,
                             qth->name, qth->loc, qth->lat, qth->lon,
                             aosbuff, utc, losbuff, utc);

    g_free(utc);

    return header;
}

gchar          *pass_to_txt_tblheader(pass_t * pass, qth_t * qth, gint fields)
{
    gchar          *fmtstr;
    guint           size;
    gchar           tbuff[TIME_FORMAT_MAX_LENGTH];
    guint           i;
    guint           linelength = 0;
    gchar          *line;
    gchar          *sep;
    gchar          *buff;

    (void)qth;                  /* avoid unused parameter compiler warning */

    /* first, get the length of the time field */
    fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
    size = daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, pass->aos);

    g_free(fmtstr);

    /* add time column */
    buff = g_strnfill(size - 4, ' ');
    line = g_strconcat(_(SPCT[0]), buff, NULL);
    linelength = size + 1;
    g_free(buff);

    for (i = 1; i < NUMCOL; i++)
    {

        if (fields & (1 << i))
        {

            /* add column to line */
            buff = g_strconcat(line, " ", _(SPCT[i]), NULL);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);

            /* update line length */
            linelength += COLW[i] + 1;
        }
    }

    /* add separator line */
    sep = g_strnfill(linelength, '-');
    buff = g_strdup_printf("%s\n%s\n%s\n", sep, line, sep);
    g_free(line);
    g_free(sep);

    return buff;
}

gchar          *pass_to_txt_tblcontents(pass_t * pass, qth_t * qth,
                                        gint fields)
{
    gchar          *fmtstr;
    gchar           tbuff[TIME_FORMAT_MAX_LENGTH];
    guint           i, num;
    gchar          *line;
    gchar          *data = NULL;
    gchar          *buff;
    pass_detail_t  *detail;
    obs_astro_t     astro;
    gdouble         ra, dec, numf;
    gchar          *ssp;

    /* first, get the length of the time field */
    fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
    daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, pass->aos);

    /* get number of rows */
    num = g_slist_length(pass->details);

    for (i = 0; i < num; i++)
    {

        /* get detail */
        detail = PASS_DETAIL(g_slist_nth_data(pass->details, i));

        /* time */
        daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, detail->time);

        line = g_strdup_printf(" %s", tbuff);

        /* Az */
        if (fields & SINGLE_PASS_FLAG_AZ)
        {
            buff = g_strdup_printf("%s %6.2f", line, detail->az);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* El */
        if (fields & SINGLE_PASS_FLAG_EL)
        {
            buff = g_strdup_printf("%s %6.2f", line, detail->el);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Ra */
        if (fields & SINGLE_PASS_FLAG_RA)
        {
            Calc_RADec(detail->time, detail->az, detail->el, qth, &astro);
            ra = Degrees(astro.ra);
            buff = g_strdup_printf("%s %6.2f", line, ra);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Dec */
        if (fields & SINGLE_PASS_FLAG_DEC)
        {
            Calc_RADec(detail->time, detail->az, detail->el, qth, &astro);
            dec = Degrees(astro.dec);
            buff = g_strdup_printf("%s %6.2f", line, dec);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Range */
        if (fields & SINGLE_PASS_FLAG_RANGE)
        {
            buff = g_strdup_printf("%s %5.0f", line, detail->range);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Range Rate */
        if (fields & SINGLE_PASS_FLAG_RANGE_RATE)
        {
            buff = g_strdup_printf("%s %6.3f", line, detail->range_rate);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Lat */
        if (fields & SINGLE_PASS_FLAG_LAT)
        {
            buff = g_strdup_printf("%s %6.2f", line, detail->lat);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Lon */
        if (fields & SINGLE_PASS_FLAG_LON)
        {
            buff = g_strdup_printf("%s %7.2f", line, detail->lon);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* SSP */
        if (fields & SINGLE_PASS_FLAG_SSP)
        {
            ssp = g_try_malloc(7);
            if (longlat2locator(detail->lon, detail->lat, ssp, 3) == RIG_OK)
            {
                buff = g_strdup_printf("%s %s", line, ssp);
                g_free(line);
                line = g_strdup(buff);
                g_free(buff);
            }
            g_free(ssp);
        }

        /* Footprint */
        if (fields & SINGLE_PASS_FLAG_FOOTPRINT)
        {
            buff = g_strdup_printf("%s %5.0f", line, detail->footprint);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Alt */
        if (fields & SINGLE_PASS_FLAG_ALT)
        {
            buff = g_strdup_printf("%s %5.0f", line, detail->alt);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Vel */
        if (fields & SINGLE_PASS_FLAG_VEL)
        {
            buff = g_strdup_printf("%s %5.3f", line, detail->velo);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Doppler */
        if (fields & SINGLE_PASS_FLAG_DOPPLER)
        {
            numf = -100.0e06 * (detail->range_rate / 299792.4580);
            buff = g_strdup_printf("%s %5.0f", line, numf);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Loss */
        if (fields & SINGLE_PASS_FLAG_LOSS)
        {
            numf = 72.4 + 20.0 * log10(detail->range);  // dB
            buff = g_strdup_printf("%s %6.2f", line, numf);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Delay */
        if (fields & SINGLE_PASS_FLAG_DELAY)
        {
            numf = detail->range / 299.7924580; // msec 
            buff = g_strdup_printf("%s %5.2f", line, numf);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* MA */
        if (fields & SINGLE_PASS_FLAG_MA)
        {
            buff = g_strdup_printf("%s %6.2f", line, detail->ma);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Phase */
        if (fields & SINGLE_PASS_FLAG_PHASE)
        {
            buff = g_strdup_printf("%s %6.2f", line, detail->phase);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Visibility */
        if (fields & SINGLE_PASS_FLAG_VIS)
        {
            buff = g_strdup_printf("%s  %c", line, vis_to_chr(detail->vis));
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* append line to return string */
        if (i == 0)
        {
            data = g_strdup_printf("%s\n", line);
            g_free(line);
        }
        else
        {
            buff = g_strconcat(data, line, "\n", NULL);
            g_free(data);
            data = g_strdup(buff);
            g_free(buff);
        }
    }

    g_free(fmtstr);

    return data;
}

gchar          *passes_to_txt_pgheader(GSList * passes, qth_t * qth,
                                       gint fields)
{
    gchar          *header;
    pass_t         *pass;

    (void)fields;

    pass = PASS(g_slist_nth_data(passes, 0));

    header = g_strdup_printf(_("Upcoming passes for %s\n"
                               "Observer: %s, %s\n"
                               "LAT:%.2f LON:%.2f\n"),
                             pass->satname, qth->name, qth->loc,
                             qth->lat, qth->lon);

    return header;
}

gchar          *passes_to_txt_tblheader(GSList * passes, qth_t * qth,
                                        gint fields)
{
    gchar          *fmtstr;
    guint           size;
    gchar           tbuff[TIME_FORMAT_MAX_LENGTH];
    guint           i;
    guint           linelength = 0;
    gchar          *line;
    gchar          *sep;
    gchar          *buff;
    pass_t         *pass;

    (void)qth;

    /* first, get the length of the time field */
    pass = PASS(g_slist_nth_data(passes, 0));
    fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
    size = daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, pass->aos);

    g_free(fmtstr);

    /* add AOS, TCA, and LOS columns */
    buff = g_strnfill(size - 3, ' ');
    line =
        g_strconcat(_(MPCT[0]), buff, _(MPCT[1]), buff, _(MPCT[2]), buff,
                    NULL);
    linelength = 3 * (size + 2);
    g_free(buff);

    for (i = 3; i < 10; i++)
    {
        if (fields & (1 << i))
        {

            /* add column to line */
            buff = g_strconcat(line, "  ", _(MPCT[i]), NULL);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);

            /* update line length */
            linelength += MCW[i] + 2;
        }
    }

    /* add separator line */
    sep = g_strnfill(linelength, '-');
    buff = g_strdup_printf("%s\n%s\n%s\n", sep, line, sep);
    g_free(line);
    g_free(sep);

    return buff;
}

gchar          *passes_to_txt_tblcontents(GSList * passes, qth_t * qth,
                                          gint fields)
{
    gchar          *fmtstr;
    gchar           tbuff[TIME_FORMAT_MAX_LENGTH];
    guint           i, num;
    gchar          *line = NULL;
    gchar          *data = NULL;
    gchar          *buff;
    pass_t         *pass;

    (void)qth;                  /* avoid unused parameter compiler warning */

    pass = PASS(g_slist_nth_data(passes, 0));

    /* first, get the length of the time field */
    fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
    daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, pass->aos);

    /* get number of rows */
    num = g_slist_length(passes);

    for (i = 0; i < num; i++)
    {

        pass = PASS(g_slist_nth_data(passes, i));

        /* AOS */
        daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, pass->aos);

        line = g_strdup_printf(" %s", tbuff);

        /* TCA */
        daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, pass->tca);

        buff = g_strdup(line);
        g_free(line);
        line = g_strdup_printf("%s  %s", buff, tbuff);
        g_free(buff);

        /* LOS */
        daynum_to_str(tbuff, TIME_FORMAT_MAX_LENGTH, fmtstr, pass->los);

        buff = g_strdup(line);
        g_free(line);
        line = g_strdup_printf("%s  %s", buff, tbuff);
        g_free(buff);

        /* Duration */
        if (fields & (1 << MULTI_PASS_COL_DURATION))
        {
            guint           h, m, s;

            /* convert julian date to seconds */
            s = (guint) ((pass->los - pass->aos) * 86400);

            /* extract hours */
            h = (guint) floor(s / 3600);
            s -= 3600 * h;

            /* extract minutes */
            m = (guint) floor(s / 60);
            s -= 60 * m;

            buff = g_strdup_printf("%s  %02d:%02d:%02d", line, h, m, s);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Max El */
        if (fields & (1 << MULTI_PASS_COL_MAX_EL))
        {
            buff = g_strdup_printf("%s  %6.2f", line, pass->max_el);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* AOS Az */
        if (fields & (1 << MULTI_PASS_COL_AOS_AZ))
        {
            buff = g_strdup_printf("%s  %6.2f", line, pass->aos_az);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Max El Az */
        if (fields & (1 << MULTI_PASS_COL_MAX_EL_AZ))
        {
            buff = g_strdup_printf("%s  %9.2f", line, pass->maxel_az);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* LOS Az */
        if (fields & (1 << MULTI_PASS_COL_LOS_AZ))
        {
            buff = g_strdup_printf("%s  %6.2f", line, pass->los_az);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Orbit */
        if (fields & (1 << MULTI_PASS_COL_ORBIT))
        {
            buff = g_strdup_printf("%s  %5d", line, pass->orbit);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* Visibility */
        if (fields & (1 << MULTI_PASS_COL_VIS))
        {
            buff = g_strdup_printf("%s  %s", line, pass->vis);
            g_free(line);
            line = g_strdup(buff);
            g_free(buff);
        }

        /* append line to return string */
        if (i == 0)
        {
            data = g_strdup_printf("%s\n", line);
            g_free(line);
        }
        else
        {
            buff = g_strconcat(data, line, "\n", NULL);
            g_free(data);
            data = g_strdup(buff);
            g_free(buff);
        }

    }

    g_free(fmtstr);

    return data;
}

static void Calc_RADec(gdouble jul_utc, gdouble saz, gdouble sel,
                       qth_t * qth, obs_astro_t * obs_set)
{
    double          phi, theta, sin_theta, cos_theta, sin_phi, cos_phi,
        az, el, Lxh, Lyh, Lzh, Sx, Ex, Zx, Sy, Ey, Zy, Sz, Ez, Zz,
        Lx, Ly, Lz, cos_delta, sin_alpha, cos_alpha;
    geodetic_t      geodetic;


    geodetic.lon = qth->lon * de2ra;
    geodetic.lat = qth->lat * de2ra;
    geodetic.alt = qth->alt / 1000.0;
    geodetic.theta = 0;

    az = saz * de2ra;
    el = sel * de2ra;
    phi = geodetic.lat;
    theta = FMod2p(ThetaG_JD(jul_utc) + geodetic.lon);
    sin_theta = sin(theta);
    cos_theta = cos(theta);
    sin_phi = sin(phi);
    cos_phi = cos(phi);
    Lxh = -cos(az) * cos(el);
    Lyh = sin(az) * cos(el);
    Lzh = sin(el);
    Sx = sin_phi * cos_theta;
    Ex = -sin_theta;
    Zx = cos_theta * cos_phi;
    Sy = sin_phi * sin_theta;
    Ey = cos_theta;
    Zy = sin_theta * cos_phi;
    Sz = -cos_phi;
    Ez = 0;
    Zz = sin_phi;
    Lx = Sx * Lxh + Ex * Lyh + Zx * Lzh;
    Ly = Sy * Lxh + Ey * Lyh + Zy * Lzh;
    Lz = Sz * Lxh + Ez * Lyh + Zz * Lzh;
    obs_set->dec = ArcSin(Lz);  /* Declination (radians) */
    cos_delta = sqrt(1 - Sqr(Lz));
    sin_alpha = Ly / cos_delta;
    cos_alpha = Lx / cos_delta;
    obs_set->ra = AcTan(sin_alpha, cos_alpha);  /* Right Ascension (radians) */
    obs_set->ra = FMod2p(obs_set->ra);
}
