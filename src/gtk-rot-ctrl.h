#ifndef __GTK_ROT_CTRL_H__
#define __GTK_ROT_CTRL_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtk-sat-module.h"
#include "predict-tools.h"
#include "rotor-conf.h"
#include "sgpsdp/sgp4sdp4.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TYPE_ROT_CTRL          (gtk_rot_ctrl_get_type ())
#define GTK_ROT_CTRL(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                   gtk_rot_ctrl_get_type (),\
                                   GtkRotCtrl)

#define GTK_ROT_CTRL_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                             gtk_rot_ctrl_get_type (),\
                             GtkRotCtrlClass)

#define IS_GTK_ROT_CTRL(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_rot_ctrl_get_type ())

typedef struct _gtk_rot_ctrl GtkRotCtrl;
typedef struct _GtkRotCtrlClass GtkRotCtrlClass;

struct _gtk_rot_ctrl {
    GtkBox          box;

    /* Azimuth widgets */
    GtkWidget      *AzSat, *AzSet, *AzRead, *AzDevSel;

    /* Elevation widgets */
    GtkWidget      *ElSat, *ElSet, *ElRead, *ElDevSel, *ElDev;

    /* other widgets */
    GtkWidget      *SatSel;
    GtkWidget      *SatCnt;
    GtkWidget      *DevSel;
    GtkWidget      *plot;       /*!< Polar plot widget */
    GtkWidget      *LockBut;
    GtkWidget      *MonitorCheckBox;
    GtkWidget      *track;
    GtkWidget      *cycle_spin;      /*!< Update timer cycle */
    GtkWidget      *thld_spin;       /*!< Threshold spin */

    rotor_conf_t   *conf;
    gdouble         t;          /*!< Time when sat data last has been updated. */

    /* satellites */
    GSList         *sats;       /*!< List of sats in parent module */
    sat_t          *target;     /*!< Target satellite */
    pass_t         *pass;       /*!< Next pass of target satellite */
    qth_t          *qth;        /*!< The QTH for this module */
    gboolean        flipped;    /*!< Whether the current pass loaded is a flip pass or not */

    guint           delay;      /*!< Timeout delay. */
    guint           timerid;    /*!< Timer ID */
    gdouble         threshold;  /*!< Motion threshold */

    gboolean        tracking;   /*!< Flag set when we are tracking a target. */
    gboolean        monitor;    /*!< Flag indicating that rig is in monitor mode. */
    gboolean        engaged;    /*!< Flag indicating that rotor device is engaged. */

    gint            errcnt;     /*!< Error counter. */

    /* TCP client to rotctld */
    struct {
        GThread    *thread;
        GTimer     *timer;
        GMutex      mutex;
        gint        socket;     /* network socket to rotctld */
        gfloat      azi_in;     /* last AZI angle read from rotctld */
        gfloat      ele_in;     /* last ELE angle read from rotctld */
        gfloat      azi_out;    /* AZI target */
        gfloat      ele_out;    /* ELE target */
        gboolean    new_trg;    /* new target position set */
        gboolean    running;
        gboolean    io_error;
    } client;
};

struct _GtkRotCtrlClass {
    GtkBoxClass     parent_class;
};

GType           gtk_rot_ctrl_get_type(void);
GtkWidget      *gtk_rot_ctrl_new(GtkSatModule * module);
void            gtk_rot_ctrl_update(GtkRotCtrl * ctrl, gdouble t);
void            gtk_rot_ctrl_select_sat(GtkRotCtrl * ctrl, gint catnum);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_ROT_CTRL_H__ */
