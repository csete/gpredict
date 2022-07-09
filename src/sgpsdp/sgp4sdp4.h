
#ifndef SGP4SDP4_H
#define SGP4SDP4_H 1

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
/* #include <unistd.h> */

/* from David Kaelbling <drk@sgi.com> */
#define select duplicate_select
#include <unistd.h>
#undef select


typedef enum {
    ORBIT_TYPE_UNKNOWN = 0,
    ORBIT_TYPE_LEO,             /*!< Low Earth orbit, up to 1200 km. */
    ORBIT_TYPE_ICO,             /*!< Intermediate Circular Orbit, up to 1400 km. */
    ORBIT_TYPE_GEO,             /*!< Geostationary. */
    ORBIT_TYPE_GSO,             /*!< Geosynchronuous. */
    ORBIT_TYPE_MOLNIYA,
    ORBIT_TYPE_TUNDRA,
    ORBIT_TYPE_POLAR,
    ORBIT_TYPE_SUNSYNC,
    ORBIT_TYPE_DECAYED
} orbit_type_t;

/** \brief Operational status of satellite. */
typedef enum {
    OP_STAT_UNKNOWN = 0,
    OP_STAT_OPERATIONAL,        /*!< Operational           [+] */
    OP_STAT_NONOP,              /*!< Nonoperational        [-] */
    OP_STAT_PARTIAL,            /*!< Partially operational [P] */
    OP_STAT_STDBY,              /*!< Backup/Standby        [B] */
    OP_STAT_SPARE,              /*!< Spare                 [S] */
    OP_STAT_EXTENDED            /*!< Extended Mission      [X] */
} op_stat_t;


/** \brief Two-line-element satellite orbital data.
 *  \ingroup sgpsdpif
 *  \bug doc incomplete.
 */
typedef struct {
    double          epoch;      /*!< Epoch Time in NORAD TLE format YYDDD.FFFFFFFF */
    unsigned int    epoch_year; /*!< Epoch: year */
    unsigned int    epoch_day;  /*!< Epoch: day of year */
    double          epoch_fod;  /*!< Epoch: Fraction of day. */
    double          xndt2o;     /*!< 1. time derivative of mean motion */
    double          xndd6o;     /*!< 2. time derivative of mean motion */
    double          bstar;      /*!< Bstar drag coefficient. */
    double          xincl;      /*!< Inclination */
    double          xnodeo;     /*!< R.A.A.N. */
    double          eo;         /*!< Eccentricity */
    double          omegao;     /*!< argument of perigee */
    double          xmo;        /*!< mean anomaly */
    double          xno;        /*!< mean motion */

    int             catnr;      /*!< Catalogue Number.  */
    int             elset;      /*!< Element Set number. */
    int             revnum;     /*!< Revolution Number at epoch. */

    char            sat_name[25];       /*!< Satellite name string. */
    char            idesg[9];   /*!< International Designator. */
    op_stat_t       status;     /*!< Operational status. */

    /* values needed for squint calculations */
    double          xincl1;
    double          xnodeo1;
    double          omegao1;
} tle_t;


/** \brief Geodetic position data structure.
 *  \ingroup sgpsdpif
 *
 * \bug It is uncertain whether the units are uniform across all functions.
 */
typedef struct {
    double          lat;        /*!< Latitude [rad] */
    double          lon;        /*!< Longitude [rad] */
    double          alt;        /*!< Altitude [km]? */
    double          theta;
} geodetic_t;

/** \brief General three-dimensional vector structure.
 *  \ingroup sgpsdpif
 */
typedef struct {
    double          x;          /*!< X component */
    double          y;          /*!< Y component */
    double          z;          /*!< Z component */
    double          w;          /*!< Magnitude */
} vector_t;


/** \brief Bearing to satellite from observer
 *  \ingroup sgpsdpif
 */
typedef struct {
    double          az;         /*!< Azimuth [deg] */
    double          el;         /*!< Elevation [deg] */
    double          range;      /*!< Range [km] */
    double          range_rate; /*!< Velocity [km/sec] */
} obs_set_t;

typedef struct {
    double          ra;         /*!< Right Ascension [dec] */
    double          dec;        /*!< Declination [dec] */
} obs_astro_t;


/* Common arguments between deep-space functions */
typedef struct {
    /* Used by dpinit part of Deep() */
    double          eosq, sinio, cosio, betao, aodp, theta2, sing, cosg;
    double          betao2, xmdot, omgdot, xnodot, xnodp;

    /* Used by dpsec and dpper parts of Deep() */
    double          xll, omgadf, xnode, em, xinc, xn, t;

    /* Used by thetg and Deep() */
    double          ds50;
} deep_arg_t;

/* static data for SGP4 and SDP4 */
typedef struct {
    double          aodp, aycof, c1, c4, c5, cosio, d2, d3, d4, delmo, omgcof;
    double          eta, omgdot, sinio, xnodp, sinmo, t2cof, t3cof, t4cof,
        t5cof;
    double          x1mth2, x3thm1, x7thm1, xmcof, xmdot, xnodcf, xnodot,
        xlcof;
} sgpsdp_static_t;

/* static data for DEEP */
typedef struct {
    double          thgr, xnq, xqncl, omegaq, zmol, zmos, savtsn, ee2, e3, xi2;
    double          xl2, xl3, xl4, xgh2, xgh3, xgh4, xh2, xh3, sse, ssi, ssg,
        xi3;
    double          se2, si2, sl2, sgh2, sh2, se3, si3, sl3, sgh3, sh3, sl4,
        sgh4;
    double          ssl, ssh, d3210, d3222, d4410, d4422, d5220, d5232, d5421;
    double          d5433, del1, del2, del3, fasx2, fasx4, fasx6, xlamo, xfact;
    double          xni, atime, stepp, stepn, step2, preep, pl, sghs, xli;
    double          d2201, d2211, sghl, sh1, pinc, pe, shs, zsingl, zcosgl;
    double          zsinhl, zcoshl, zsinil, zcosil;
} deep_static_t;

/**
 * \brief Satellite data structure
 * \ingroup sgpsdpif
 *
 */
typedef struct {
    char           *name;
    char           *nickname;
    char           *website;
    tle_t           tle;        /*!< Keplerian elements */
    int             flags;      /*!< Flags for algo ctrl */
    sgpsdp_static_t sgps;
    deep_static_t   dps;
    deep_arg_t      deep_arg;
    vector_t        pos;        /*!< Raw position and range */
    vector_t        vel;        /*!< Raw velocity */

    /* time keeping fields */
    double          jul_epoch;
    double          jul_utc;
    double          tsince;
    double          aos;        /*!< Next AOS. */
    double          los;        /*!< Next LOS */

    double          az;         /*!< Azimuth [deg] */
    double          el;         /*!< Elevation [deg] */
    double          range;      /*!< Range [km] */
    double          range_rate; /*!< Range Rate [km/sec] */
    double          ra;         /*!< Right Ascension [deg] */
    double          dec;        /*!< Declination [deg] */
    double          ssplat;     /*!< SSP latitude [deg] */
    double          ssplon;     /*!< SSP longitude [deg] */
    double          alt;        /*!< altitude [km] */
    double          velo;       /*!< velocity [km/s] */
    double          ma;         /*!< mean anomaly */
    double          footprint;  /*!< footprint */
    double          phase;      /*!< orbit phase */
    double          meanmo;     /*!< mean motion kept in rev/day */
    long            orbit;      /*!< orbit number */
    orbit_type_t    otype;      /*!< orbit type. */
} sat_t;


/** \brief Type casting macro */
#define SAT(sat)  ((sat_t *) sat)


/** Table of constant values **/
#define de2ra    1.74532925E-2  /* Degrees to Radians */
#define pi       3.1415926535898        /* Pi */
#define pio2     1.5707963267949        /* Pi/2 */
#define x3pio2   4.71238898     /* 3*Pi/2 */
#define twopi    6.2831853071796        /* 2*Pi  */
#define e6a      1.0E-6
#define tothrd   6.6666667E-1   /* 2/3 */
#define xj2      1.0826158E-3   /* J2 Harmonic */
#define xj3     -2.53881E-6     /* J3 Harmonic */
#define xj4     -1.65597E-6     /* J4 Harmonic */
#define xke      7.43669161E-2
#define xkmper   6.378135E3     /* Earth radius km */
#define xmnpda   1.44E3         /* Minutes per day */
#define ae       1.0
#define ck2      5.413079E-4
#define ck4      6.209887E-7
#define __f      3.352779E-3
#define ge       3.986008E5
#define __s__    1.012229
#define qoms2t   1.880279E-09
#define secday   8.6400E4       /* Seconds per day */
#define omega_E  1.0027379
#define omega_ER 6.3003879
#define zns      1.19459E-5
#define c1ss     2.9864797E-6
#define zes      1.675E-2
#define znl      1.5835218E-4
#define c1l      4.7968065E-7
#define zel      5.490E-2
#define zcosis   9.1744867E-1
#define zsinis   3.9785416E-1
#define zsings  -9.8088458E-1
#define zcosgs   1.945905E-1
#define zcoshs   1
#define zsinhs   0
#define q22      1.7891679E-6
#define q31      2.1460748E-6
#define q33      2.2123015E-7
#define g22      5.7686396
#define g32      9.5240898E-1
#define g44      1.8014998
#define g52      1.0508330
#define g54      4.4108898
#define root22   1.7891679E-6
#define root32   3.7393792E-7
#define root44   7.3636953E-9
#define root52   1.1428639E-7
#define root54   2.1765803E-9
#define thdt     4.3752691E-3
#define rho      1.5696615E-1
#define mfactor  7.292115E-5
#define __sr__   6.96000E5      /*Solar radius - kilometers (IAU 76) */
#define AU       1.49597870E8   /*Astronomical unit - kilometers (IAU 76) */

/* Entry points of Deep() 
FIXME: Change to enu */
#define dpinit   1              /* Deep-space initialization code */
#define dpsec    2              /* Deep-space secular code        */
#define dpper    3              /* Deep-space periodic code       */

/* Carriage return and line feed */
#define CR  0x0A
#define LF  0x0D

/* Flow control flag definitions */
#define ALL_FLAGS              -1
#define SGP_INITIALIZED_FLAG   0x000001
#define SGP4_INITIALIZED_FLAG  0x000002
#define SDP4_INITIALIZED_FLAG  0x000004
#define SGP8_INITIALIZED_FLAG  0x000008
#define SDP8_INITIALIZED_FLAG  0x000010
#define SIMPLE_FLAG            0x000020
#define DEEP_SPACE_EPHEM_FLAG  0x000040
#define LUNAR_TERMS_DONE_FLAG  0x000080
#define NEW_EPHEMERIS_FLAG     0x000100
#define DO_LOOP_FLAG           0x000200
#define RESONANCE_FLAG         0x000400
#define SYNCHRONOUS_FLAG       0x000800
#define EPOCH_RESTART_FLAG     0x001000
#define VISIBLE_FLAG           0x002000
#define SAT_ECLIPSED_FLAG      0x004000


/** Function prototypes **/


/* sgp4sdp4.c */
void            SGP4(sat_t * sat, double tsince);
void            SDP4(sat_t * sat, double tsince);
void            Deep(int ientry, sat_t * sat);
int             isFlagSet(int flag);
int             isFlagClear(int flag);
void            SetFlag(int flag);
void            ClearFlag(int flag);

/* sgp_in.c */
int             Checksum_Good(char *tle_set);
int             Good_Elements(char *tle_set);
void            Convert_Satellite_Data(char *tle_set, tle_t * tle);
int             Get_Next_Tle_Set(char lines[3][80], tle_t * tle);
void            select_ephemeris(sat_t * sat);

/* sgp_math.c */
int             Sign(double arg);
double          Sqr(double arg);
double          Cube(double arg);
double          Radians(double arg);
double          Degrees(double arg);
double          ArcSin(double arg);
double          ArcCos(double arg);
void            Magnitude(vector_t * v);
void            Vec_Add(vector_t * v1, vector_t * v2, vector_t * v3);
void            Vec_Sub(vector_t * v1, vector_t * v2, vector_t * v3);
void            Scalar_Multiply(double k, vector_t * v1, vector_t * v2);
void            Scale_Vector(double k, vector_t * v);
double          Dot(vector_t * v1, vector_t * v2);
double          Angle(vector_t * v1, vector_t * v2);
void            Cross(vector_t * v1, vector_t * v2, vector_t * v3);
void            Normalize(vector_t * v);
double          AcTan(double sinx, double cosx);
double          FMod2p(double x);
double          Modulus(double arg1, double arg2);
double          Frac(double arg);
int             Round(double arg);
double          Int(double arg);
void            Convert_Sat_State(vector_t * pos, vector_t * vel);

/* sgp_obs.c */
void            Calculate_User_PosVel(double _time, geodetic_t * geodetic,
                                      vector_t * obs_pos, vector_t * obs_vel);
void            Calculate_LatLonAlt(double _time, vector_t * pos,
                                    geodetic_t * geodetic);
void            Calculate_Obs(double _time, vector_t * pos, vector_t * vel,
                              geodetic_t * geodetic, obs_set_t * obs_set);
void            Calculate_RADec_and_Obs(double _time, vector_t * pos,
                                        vector_t * vel, geodetic_t * geodetic,
                                        obs_astro_t * obs_set);

/* sgp_time.c */
double          Julian_Date_of_Epoch(double epoch);
double          Epoch_Time(double jd);
int             DOY(int yr, int mo, int dy);
double          Fraction_of_Day(int hr, int mi, int se);
void            Calendar_Date(double jd, struct tm *cdate);
void            Time_of_Day(double jd, struct tm *cdate);
double          Julian_Date(struct tm *cdate);
void            Date_Time(double jd, struct tm *cdate);
int             Check_Date(struct tm *cdate);
void            Time_to_UTC(struct tm *cdate, struct tm *odate);
struct tm       Time_from_UTC(struct tm *cdate);
double          JD_to_UTC(double jt);
double          JD_from_UTC(double jt);
double          Delta_ET(double year);
double          Julian_Date_of_Year(double year);
double          ThetaG(double epoch, deep_arg_t * deep_arg);
double          ThetaG_JD(double jd);
void            UTC_Calendar_Now(struct tm *cdate);

/* solar.c */
void            Calculate_Solar_Position(double _time,
                                         vector_t * solar_vector);
int             Sat_Eclipsed(vector_t * pos, vector_t * sol, double *depth);


#endif
