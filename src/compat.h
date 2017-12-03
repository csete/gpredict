/* Utilities to ensure compatibility across multiple platforms. */
#ifndef COMPAT_H
#define COMPAT_H 1

gchar          *get_data_dir(void);
gchar          *get_maps_dir(void);
gchar          *get_logo_dir(void);
gchar          *get_icon_dir(void);
gchar          *get_user_conf_dir(void);
gchar          *get_modules_dir(void);
gchar          *get_satdata_dir(void);
gchar          *get_trsp_dir(void);
gchar          *get_hwconf_dir(void);
gchar          *get_old_conf_dir(void);
gchar          *map_file_name(const gchar * map);
gchar          *logo_file_name(const gchar * logo);
gchar          *icon_file_name(const gchar * icon);
gchar          *data_file_name(const gchar * data);
gchar          *sat_file_name(const gchar * satfile);
gchar          *trsp_file_name(const gchar * trspfile);
gchar          *hw_file_name(const gchar * hwfile);

gchar          *sat_file_name_from_catnum(guint catnum);
gchar          *sat_file_name_from_catnum_s(gchar * catnum);

#endif
