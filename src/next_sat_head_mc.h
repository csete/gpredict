/* here all the nicknames for the selected satellite get stored */
extern char **satlist_mc_nick;
/* holds the nickname of the next available satellite (next AOS) */
extern char next_sat_mc[256];
/* holds the nickname of the current target satellite */
extern char active_target_nick[256];
/* check if the target satellite gets tracked, and if it's between AOS and LOS */
extern int target_aquired;
/* check if -v, --verbose flags are set */
extern int verbose_mode;
/* check if -a, --automatic flags are set */
extern int auto_mode;


