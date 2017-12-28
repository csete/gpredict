#ifndef GPREDICT_UTILS_H
#define GPREDICT_UTILS_H 1

#include <gtk/gtk.h>

#define M_TO_FT(x) (3.2808399*x)
#define FT_TO_M(x) (x/3.2808399)
#define KM_TO_MI(x) (x/1.609344)
#define MI_TO_KM(x) (1.609344*x)

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif


GtkWidget      *gpredict_hpixmap_button(const gchar * file,
                                        const gchar * text,
                                        const gchar * tooltip);

GtkWidget      *gpredict_vpixmap_button(const gchar * file,
                                        const gchar * text,
                                        const gchar * tooltip);

GtkWidget      *gpredict_hstock_button(const gchar * stock_id,
                                       const gchar * text,
                                       const gchar * tooltip);

GtkWidget      *gpredict_mini_mod_button(const gchar * pixmapfile,
                                         const gchar * tooltip);

void            gpredict_set_combo_tooltips(GtkWidget * combo, gpointer text);

gint            gpredict_file_copy(const gchar * in, const gchar * out);

void            rgba_from_cfg(guint cfg_rgba, GdkRGBA * gdk_rgba);
guint           rgba_to_cfg(const GdkRGBA * gdk_rgba);
void            rgb_from_cfg(guint cfg_rgb, GdkRGBA * gdk_rgba);
guint           rgb_to_cfg(const GdkRGBA * gdk_rgba);

void            rgb2gdk(guint rgb, GdkColor * color);
void            rgba2gdk(guint rgba, GdkColor * color, guint16 * alpha);
void            gdk2rgb(const GdkColor * color, guint * rgb);
void            gdk2rgba(const GdkColor * color, guint16 alpha, guint * rgba);
gchar          *rgba2html(guint rgba);
int             gpredict_strcmp(const char *s1, const char *s2);
char           *gpredict_strcasestr(const char *s1, const char *s2);
gboolean        gpredict_save_key_file(GKeyFile * cfgdata,
                                       const char *filename);
gboolean        gpredict_legal_char(int ch);
#endif
