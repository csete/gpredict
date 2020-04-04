/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC
                      2011  Charles Suprin, AA1VS
 
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-data.h"
#include "gtk-sat-popup-common.h"
#include "gtk-single-sat.h"
#include "locator.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-info.h"
#include "sat-log.h"
#include "sat-vis.h"
#include "sgpsdp/sgp4sdp4.h"
#include "sat-pass-dialogs.h"
#include "time-tools.h"

/* Column titles indexed with column symb. refs. */
const gchar    *SINGLE_SAT_FIELD_TITLE[SINGLE_SAT_FIELD_NUMBER] = {
    N_("Azimuth"),
    N_("Elevation"),
    N_("Direction"),
    N_("Right Asc."),
    N_("Declination"),
    N_("Slant Range"),
    N_("Range Rate"),
    N_("Next Event"),
    N_("Next AOS"),
    N_("Next LOS"),
    N_("SSP Lat."),
    N_("SSP Lon."),
    N_("SSP Loc."),
    N_("Footprint"),
    N_("Altitude"),
    N_("Velocity"),
    N_("Doppler@100M"),
    N_("Sig. Loss"),
    N_("Sig. Delay"),
    N_("Mean Anom."),
    N_("Orbit Phase"),
    N_("Orbit Num."),
    N_("Visibility")
};

/* Column title hints indexed with column symb. refs. */
const gchar    *SINGLE_SAT_FIELD_HINT[SINGLE_SAT_FIELD_NUMBER] = {
    N_("Azimuth of the satellite"),
    N_("Elevation of the satellite"),
    N_("Direction of the satellite"),
    N_("Right Ascension of the satellite"),
    N_("Declination of the satellite"),
    N_("The range between satellite and observer"),
    N_("The rate at which the Slant Range changes"),
    N_("The time of next AOS or LOS"),
    N_("The time of next AOS"),
    N_("The time of next LOS"),
    N_("Latitude of the sub-satellite point"),
    N_("Longitude of the sub-satellite point"),
    N_("Sub-Satellite Point as Maidenhead grid square"),
    N_("Diameter of the satellite footprint"),
    N_("Altitude of the satellite"),
    N_("Tangential velocity of the satellite"),
    N_("Doppler Shift @ 100MHz"),
    N_("Signal loss @ 100MHz"),
    N_("Signal Delay"),
    N_("Mean Anomaly"),
    N_("Orbit Phase"),
    N_("Orbit Number"),
    N_("Visibility of the satellite")
};

static GtkBoxClass *parent_class = NULL;


static void gtk_single_sat_destroy(GtkWidget * widget)
{
    GtkSingleSat   *ssat = GTK_SINGLE_SAT(widget);
    sat_t          *sat = SAT(g_slist_nth_data(ssat->sats, ssat->selected));

    if (sat != NULL)
        g_key_file_set_integer(ssat->cfgdata, MOD_CFG_SINGLE_SAT_SECTION,
                               MOD_CFG_SINGLE_SAT_SELECT, sat->tle.catnr);

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_single_sat_class_init(GtkSingleSatClass * class)
{
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) class;
    widget_class->destroy = gtk_single_sat_destroy;
    parent_class = g_type_class_peek_parent(class);
}

static void gtk_single_sat_init(GtkSingleSat * list)
{
    (void)list;
}

/* Update a field in the GtkSingleSat view. */
static void update_field(GtkSingleSat * ssat, guint i)
{
    sat_t          *sat;
    gchar          *buff = NULL;
    gchar           tbuf[TIME_FORMAT_MAX_LENGTH];
    gchar           hmf = ' ';
    gdouble         number;
    gint            retcode;
    gchar          *fmtstr;
    gchar          *alstr;
    sat_vis_t       vis;

    /* make some sanity checks */
    if (ssat->labels[i] == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Can not update invisible field (I:%d F:%d)"),
                    __FILE__, __LINE__, i, ssat->flags);
        return;
    }

    /* get selected satellite */
    sat = SAT(g_slist_nth_data(ssat->sats, ssat->selected));
    if (!sat)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Can not update non-existing sat"),
                    __FILE__, __LINE__);
        return;
    }

    /* update requested field */
    switch (i)
    {
    case SINGLE_SAT_FIELD_AZ:
        buff = g_strdup_printf("%6.2f\302\260", sat->az);
        break;
    case SINGLE_SAT_FIELD_EL:
        buff = g_strdup_printf("%6.2f\302\260", sat->el);
        break;
    case SINGLE_SAT_FIELD_DIR:
        if (sat->otype == ORBIT_TYPE_GEO)
        {
            buff = g_strdup("Geostationary");
        }
        else if (decayed(sat))
        {
            buff = g_strdup("Decayed");
        }
        else if (sat->range_rate > 0.0)
        {
            /* Receeding */
            buff = g_strdup("Receeding");
        }
        else if (sat->range_rate < 0.0)
        {
            /* Approaching */
            buff = g_strdup("Approaching");
        }
        else
        {
            buff = g_strdup("N/A");
        }
        break;
    case SINGLE_SAT_FIELD_RA:
        buff = g_strdup_printf("%6.2f\302\260", sat->ra);
        break;
    case SINGLE_SAT_FIELD_DEC:
        buff = g_strdup_printf("%6.2f\302\260", sat->dec);
        break;
    case SINGLE_SAT_FIELD_RANGE:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->range));
        else
            buff = g_strdup_printf("%.0f km", sat->range);
        break;
    case SINGLE_SAT_FIELD_RANGE_RATE:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->range_rate));
        else
            buff = g_strdup_printf("%.3f km/sec", sat->range_rate);
        break;
    case SINGLE_SAT_FIELD_NEXT_EVENT:
        if (sat->aos > sat->los)
        {
            /* next event is LOS */
            number = sat->los;
            alstr = g_strdup("LOS: ");
        }
        else
        {
            /* next event is AOS */
            number = sat->aos;
            alstr = g_strdup("AOS: ");
        }
        if (number > 0.0)
        {

            /* format the number */
            fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
            daynum_to_str(tbuf, TIME_FORMAT_MAX_LENGTH, fmtstr, number);

            g_free(fmtstr);

            buff = g_strconcat(alstr, tbuf, NULL);

        }
        else
        {
            buff = g_strdup(_("N/A"));
        }
        g_free(alstr);
        break;
    case SINGLE_SAT_FIELD_AOS:
        if (sat->aos > 0.0)
        {
            /* format the number */
            fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
            daynum_to_str(tbuf, TIME_FORMAT_MAX_LENGTH, fmtstr, sat->aos);
            g_free(fmtstr);
            buff = g_strdup(tbuf);
        }
        else
        {
            buff = g_strdup(_("N/A"));
        }
        break;
    case SINGLE_SAT_FIELD_LOS:
        if (sat->los > 0.0)
        {
            fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
            daynum_to_str(tbuf, TIME_FORMAT_MAX_LENGTH, fmtstr, sat->los);
            g_free(fmtstr);
            buff = g_strdup(tbuf);
        }
        else
        {
            buff = g_strdup(_("N/A"));
        }
        break;
    case SINGLE_SAT_FIELD_LAT:
        number = sat->ssplat;
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW))
        {
            if (number < 0.00)
            {
                number = -number;
                hmf = 'S';
            }
            else
            {
                hmf = 'N';
            }
        }
        buff = g_strdup_printf("%.2f\302\260%c", number, hmf);
        break;
    case SINGLE_SAT_FIELD_LON:
        number = sat->ssplon;
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW))
        {
            if (number < 0.00)
            {
                number = -number;
                hmf = 'W';
            }
            else
            {
                hmf = 'E';
            }
        }
        buff = g_strdup_printf("%.2f\302\260%c", number, hmf);
        break;
    case SINGLE_SAT_FIELD_SSP:
        /* SSP locator */
        buff = g_try_malloc(7);
        retcode = longlat2locator(sat->ssplon, sat->ssplat, buff, 3);
        if (retcode == RIG_OK)
        {
            buff[6] = '\0';
        }
        else
        {
            g_free(buff);
            buff = NULL;
        }

        break;
    case SINGLE_SAT_FIELD_FOOTPRINT:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->footprint));
        }
        else
        {
            buff = g_strdup_printf("%.0f km", sat->footprint);
        }
        break;
    case SINGLE_SAT_FIELD_ALT:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->alt));
        else
            buff = g_strdup_printf("%.0f km", sat->alt);
        break;
    case SINGLE_SAT_FIELD_VEL:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->velo));
        else
            buff = g_strdup_printf("%.3f km/sec", sat->velo);
        break;
    case SINGLE_SAT_FIELD_DOPPLER:
        number = -100.0e06 * (sat->range_rate / 299792.4580);   // Hz
        buff = g_strdup_printf("%.0f Hz", number);
        break;
    case SINGLE_SAT_FIELD_LOSS:
        number = 72.4 + 20.0 * log10(sat->range);       // dB
        buff = g_strdup_printf("%.2f dB", number);
        break;
    case SINGLE_SAT_FIELD_DELAY:
        number = sat->range / 299.7924580;      // msec 
        buff = g_strdup_printf("%.2f msec", number);
        break;
    case SINGLE_SAT_FIELD_MA:
        buff = g_strdup_printf("%.2f\302\260", sat->ma);
        break;
    case SINGLE_SAT_FIELD_PHASE:
        buff = g_strdup_printf("%.2f\302\260", sat->phase);
        break;
    case SINGLE_SAT_FIELD_ORBIT:
        buff = g_strdup_printf("%ld", sat->orbit);
        break;
    case SINGLE_SAT_FIELD_VISIBILITY:
        vis = get_sat_vis(sat, ssat->qth, sat->jul_utc);
        buff = vis_to_str(vis);
        break;
    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Invalid field number (%d)"),
                    __FILE__, __LINE__, i);
        break;
    }

    if (buff != NULL)
    {
        gtk_label_set_text(GTK_LABEL(ssat->labels[i]), buff);
        g_free(buff);
    }
}

static gint sat_name_compare(sat_t * a, sat_t * b)
{
    return gpredict_strcmp(a->nickname, b->nickname);
}

/* Copy satellite from hash table to singly linked list. */
static void store_sats(gpointer key, gpointer value, gpointer user_data)
{
    GtkSingleSat   *single_sat = GTK_SINGLE_SAT(user_data);
    sat_t          *sat = SAT(value);

    (void)key;

    single_sat->sats = g_slist_insert_sorted(single_sat->sats, sat,
                                             (GCompareFunc) sat_name_compare);
}

static void Calculate_RADec(sat_t * sat, qth_t * qth, obs_astro_t * obs_set)
{
    /* Reference:  Methods of Orbit Determination by  */
    /*                Pedro Ramon Escobal, pp. 401-402 */

    double          phi, theta, sin_theta, cos_theta, sin_phi, cos_phi,
        az, el, Lxh, Lyh, Lzh, Sx, Ex, Zx, Sy, Ey, Zy, Sz, Ez, Zz,
        Lx, Ly, Lz, cos_delta, sin_alpha, cos_alpha;
    geodetic_t      geodetic;

    geodetic.lon = qth->lon * de2ra;
    geodetic.lat = qth->lat * de2ra;
    geodetic.alt = qth->alt / 1000.0;
    geodetic.theta = 0;

    az = sat->az * de2ra;
    el = sat->el * de2ra;
    phi = geodetic.lat;
    theta = FMod2p(ThetaG_JD(sat->jul_utc) + geodetic.lon);
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

static void select_satellite(GtkWidget * menuitem, gpointer data)
{
    GtkSingleSat   *ssat = GTK_SINGLE_SAT(data);
    guint           i =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "index"));
    gchar          *title;
    sat_t          *sat;

    /* there are many "ghost"-trigging of this signal, but we only need to make
       a new selection when the received menuitem is selected
     */
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
    {
        ssat->selected = i;

        sat = SAT(g_slist_nth_data(ssat->sats, i));

        title = g_markup_printf_escaped("<b>%s</b>", sat->nickname);
        gtk_label_set_markup(GTK_LABEL(ssat->header), title);
        g_free(title);
    }
}

/* Single sat options menu */
static void gtk_single_sat_popup_cb(GtkWidget * button, gpointer data)
{
    GtkSingleSat   *single_sat = GTK_SINGLE_SAT(data);
    GtkWidget      *menu;
    GtkWidget      *menuitem;
    GtkWidget      *label;
    GSList         *group = NULL;
    gchar          *buff;
    sat_t          *sat;
    sat_t          *sati;       /* used to create list of satellites */
    guint           i, n;

    sat = SAT(g_slist_nth_data(single_sat->sats, single_sat->selected));
    if (sat == NULL)
        return;

    n = g_slist_length(single_sat->sats);

    menu = gtk_menu_new();

    /* satellite name/info */
    menuitem = gtk_menu_item_new();
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    buff = g_markup_printf_escaped("<b>%s</b>", sat->nickname);
    gtk_label_set_markup(GTK_LABEL(label), buff);
    g_free(buff);
    gtk_container_add(GTK_CONTAINER(menuitem), label);

    /* attach data to menuitem and connect callback */
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", single_sat->qth);
    g_signal_connect(menuitem, "activate",
                     G_CALLBACK(show_sat_info_menu_cb),
                     gtk_widget_get_toplevel(button));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* add the menu items for current,next, and future passes. */
    add_pass_menu_items(menu, sat, single_sat->qth, &single_sat->tstamp, data);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* select sat */
    for (i = 0; i < n; i++)
    {
        sati = SAT(g_slist_nth_data(single_sat->sats, i));

        menuitem = gtk_radio_menu_item_new_with_label(group, sati->nickname);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));

        if (i == single_sat->selected)
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                           TRUE);

        /* store item index so that it is available in the callback */
        g_object_set_data(G_OBJECT(menuitem), "index", GUINT_TO_POINTER(i));
        g_signal_connect_after(menuitem, "activate",
                               G_CALLBACK(select_satellite), single_sat);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    gtk_widget_show_all(menu);

    /* gtk_menu_popup got deprecated in 3.22, first available in Ubuntu 18.04 */
#if GTK_MINOR_VERSION < 22
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   0, gdk_event_get_time((GdkEvent *) NULL));
#else
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
#endif
}

/* Refresh internal references to the satellites. */
void gtk_single_sat_reload_sats(GtkWidget * single_sat, GHashTable * sats)
{
    /* free GSlists */
    g_slist_free(GTK_SINGLE_SAT(single_sat)->sats);
    GTK_SINGLE_SAT(single_sat)->sats = NULL;

    /* reload satellites */
    g_hash_table_foreach(sats, store_sats, single_sat);
}

/*
 * Reload configuration.
 * 
 * @param widget The GtkSingleSat widget.
 * @param newcfg The new configuration data for the module
 * @param sats   The satellites.
 * @param local  Flag indicating whether reconfiguration is requested from 
 *               local configuration dialog.
 */
void gtk_single_sat_reconf(GtkWidget * widget,
                           GKeyFile * newcfg,
                           GHashTable * sats, qth_t * qth, gboolean local)
{
    guint32         fields;

    /* store pointer to new cfg data */
    GTK_SINGLE_SAT(widget)->cfgdata = newcfg;

    /* get visible fields from new configuration */
    fields = mod_cfg_get_int(newcfg,
                             MOD_CFG_SINGLE_SAT_SECTION,
                             MOD_CFG_SINGLE_SAT_FIELDS,
                             SAT_CFG_INT_SINGLE_SAT_FIELDS);

    if (fields != GTK_SINGLE_SAT(widget)->flags)
        GTK_SINGLE_SAT(widget)->flags = fields;

    /* if this is a local reconfiguration sats may have changed */
    if (local)
        gtk_single_sat_reload_sats(widget, sats);

    /* QTH may have changed too since we have a default QTH */
    GTK_SINGLE_SAT(widget)->qth = qth;

    /* get refresh rate and cycle counter */
    GTK_SINGLE_SAT(widget)->refresh = mod_cfg_get_int(newcfg,
                                                      MOD_CFG_SINGLE_SAT_SECTION,
                                                      MOD_CFG_SINGLE_SAT_REFRESH,
                                                      SAT_CFG_INT_SINGLE_SAT_REFRESH);
    GTK_SINGLE_SAT(widget)->counter = 1;
}

/* Select new satellite */
void gtk_single_sat_select_sat(GtkWidget * single_sat, gint catnum)
{
    GtkSingleSat   *ssat = GTK_SINGLE_SAT(single_sat);
    sat_t          *sat = NULL;
    gchar          *title;
    gboolean        foundsat = FALSE;
    gint            i, n;

    /* find satellite with catnum */
    n = g_slist_length(ssat->sats);
    for (i = 0; i < n; i++)
    {
        sat = SAT(g_slist_nth_data(ssat->sats, i));
        if (sat->tle.catnr == catnum)
        {
            /* found satellite */
            ssat->selected = i;
            foundsat = TRUE;

            /* exit loop */
            i = n;
        }
    }

    if (!foundsat)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not find satellite with catalog number %d"),
                    __func__, catnum);
        return;
    }
    title = g_markup_printf_escaped("<b>%s</b>", sat->nickname);
    gtk_label_set_markup(GTK_LABEL(ssat->header), title);
    g_free(title);
}

/* Update satellites */
void gtk_single_sat_update(GtkWidget * widget)
{
    GtkSingleSat   *ssat = GTK_SINGLE_SAT(widget);
    guint           i;

    /* first, do some sanity checks */
    if ((ssat == NULL) || !IS_GTK_SINGLE_SAT(ssat))
    {

        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Invalid GtkSingleSat!"), __func__);
        return;
    }


    /* check refresh rate */
    if (ssat->counter < ssat->refresh)
    {
        ssat->counter++;
    }
    else
    {
        /* we calculate here to avoid double calc */
        if ((ssat->flags & SINGLE_SAT_FLAG_RA) ||
            (ssat->flags & SINGLE_SAT_FLAG_DEC))
        {
            obs_astro_t     astro;
            sat_t          *sat =
                SAT(g_slist_nth_data(ssat->sats, ssat->selected));

            Calculate_RADec(sat, ssat->qth, &astro);
            sat->ra = Degrees(astro.ra);
            sat->dec = Degrees(astro.dec);
        }

        /* update visible fields one by one */
        for (i = 0; i < SINGLE_SAT_FIELD_NUMBER; i++)
        {
            if (ssat->flags & (1 << i))
                update_field(ssat, i);

        }
        ssat->counter = 1;
    }
}

GType gtk_single_sat_get_type()
{
    static GType    gtk_single_sat_type = 0;

    if (!gtk_single_sat_type)
    {
        static const GTypeInfo gtk_single_sat_info = {
            sizeof(GtkSingleSatClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) gtk_single_sat_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(GtkSingleSat),
            5,                  /* n_preallocs */
            (GInstanceInitFunc) gtk_single_sat_init,
            NULL
        };

        gtk_single_sat_type = g_type_register_static(GTK_TYPE_BOX,
                                                     "GtkSingleSat",
                                                     &gtk_single_sat_info, 0);
    }

    return gtk_single_sat_type;
}

GtkWidget      *gtk_single_sat_new(GKeyFile * cfgdata, GHashTable * sats,
                                   qth_t * qth, guint32 fields)
{
    GtkWidget      *widget;
    GtkSingleSat   *single_sat;
    GtkWidget      *hbox;       /* horizontal box for header */
    GtkWidget      *label1;
    GtkWidget      *label2;
    sat_t          *sat;
    gchar          *title;
    guint           i;
    gint            selectedcatnum;

    widget = g_object_new(GTK_TYPE_SINGLE_SAT, NULL);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(widget),
                                   GTK_ORIENTATION_VERTICAL);
    single_sat = GTK_SINGLE_SAT(widget);

    single_sat->update = gtk_single_sat_update;

    /* Read configuration data. */
    /* ... */

    g_hash_table_foreach(sats, store_sats, widget);
    single_sat->selected = 0;
    single_sat->qth = qth;
    single_sat->cfgdata = cfgdata;

    /* initialise column flags */
    if (fields > 0)
        single_sat->flags = fields;
    else
        single_sat->flags = mod_cfg_get_int(cfgdata,
                                            MOD_CFG_SINGLE_SAT_SECTION,
                                            MOD_CFG_SINGLE_SAT_FIELDS,
                                            SAT_CFG_INT_SINGLE_SAT_FIELDS);


    /* get refresh rate and cycle counter */
    single_sat->refresh = mod_cfg_get_int(cfgdata,
                                          MOD_CFG_SINGLE_SAT_SECTION,
                                          MOD_CFG_SINGLE_SAT_REFRESH,
                                          SAT_CFG_INT_SINGLE_SAT_REFRESH);
    single_sat->counter = 1;

    /* get selected catnum if available */
    selectedcatnum = mod_cfg_get_int(cfgdata,
                                     MOD_CFG_SINGLE_SAT_SECTION,
                                     MOD_CFG_SINGLE_SAT_SELECT,
                                     SAT_CFG_INT_SINGLE_SAT_SELECT);
    /* popup button */
    single_sat->popup_button =
        gpredict_mini_mod_button("gpredict-mod-popup.png",
                                 _("Satellite options / shortcuts"));
    g_signal_connect(single_sat->popup_button, "clicked",
                     G_CALLBACK(gtk_single_sat_popup_cb), widget);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), single_sat->popup_button,
                       FALSE, FALSE, 0);


    /* create header */
    sat = SAT(g_slist_nth_data(single_sat->sats, 0));
    title = g_markup_printf_escaped("<b>%s</b>",
                                    sat ? sat->nickname : "noname");
    single_sat->header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(single_sat->header), title);
    g_free(title);
    g_object_set(single_sat->header, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), single_sat->header, TRUE, TRUE, 10);

    gtk_box_pack_start(GTK_BOX(widget), hbox, FALSE, FALSE, 0);

    /* create and initialise table  */
//    single_sat->table = gtk_table_new(SINGLE_SAT_FIELD_NUMBER, 3, FALSE);
    single_sat->table = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(single_sat->table), 5);
    gtk_grid_set_row_spacing(GTK_GRID(single_sat->table), 0);
    gtk_grid_set_column_spacing(GTK_GRID(single_sat->table), 5);

    /* create and add label widgets */
    for (i = 0; i < SINGLE_SAT_FIELD_NUMBER; i++)
    {
        if (single_sat->flags & (1 << i))
        {
            label1 = gtk_label_new(_(SINGLE_SAT_FIELD_TITLE[i]));
            g_object_set(label1, "xalign", 1.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(single_sat->table), label1, 0, i, 1, 1);

            label2 = gtk_label_new("-");
            g_object_set(label2, "xalign", 0.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(single_sat->table), label2, 2, i, 1, 1);
            single_sat->labels[i] = label2;

            /* add tooltips */
            gtk_widget_set_tooltip_text(label1, _(SINGLE_SAT_FIELD_HINT[i]));
            gtk_widget_set_tooltip_text(label2, _(SINGLE_SAT_FIELD_HINT[i]));

            label1 = gtk_label_new(":");
            gtk_grid_attach(GTK_GRID(single_sat->table), label1, 1, i, 1, 1);
        }
        else
        {
            single_sat->labels[i] = NULL;
        }
    }

    /* create and initialise scrolled window */
    single_sat->swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(single_sat->swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(single_sat->swin), single_sat->table);
    gtk_box_pack_end(GTK_BOX(widget), single_sat->swin, TRUE, TRUE, 0);
    gtk_widget_show_all(widget);

    if (selectedcatnum)
        gtk_single_sat_select_sat(widget, selectedcatnum);

    return widget;
}
