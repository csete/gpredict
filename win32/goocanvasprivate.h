/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasprivate.h - private types & utility functions.
 */
#ifndef __GOO_CANVAS_PRIVATE_H__
#define __GOO_CANVAS_PRIVATE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


/*
 * GPtrArray extensions.
 */
void goo_canvas_util_ptr_array_insert     (GPtrArray *ptr_array,
					   gpointer   data,
					   gint       index);

void goo_canvas_util_ptr_array_move       (GPtrArray *ptr_array,
					   gint       old_index,
					   gint       new_index);

gint goo_canvas_util_ptr_array_find_index (GPtrArray *ptr_array,
					   gpointer   data);


#define GOO_TYPE_CAIRO_FILL_RULE   (goo_cairo_fill_rule_get_type ())
#define GOO_TYPE_CAIRO_OPERATOR    (goo_cairo_operator_get_type())
#define GOO_TYPE_CAIRO_ANTIALIAS   (goo_cairo_antialias_get_type())
#define GOO_TYPE_CAIRO_LINE_CAP    (goo_cairo_line_cap_get_type ())
#define GOO_TYPE_CAIRO_LINE_JOIN   (goo_cairo_line_join_get_type ())

GType goo_cairo_fill_rule_get_type (void) G_GNUC_CONST;
GType goo_cairo_operator_get_type  (void) G_GNUC_CONST;
GType goo_cairo_antialias_get_type (void) G_GNUC_CONST;
GType goo_cairo_line_cap_get_type  (void) G_GNUC_CONST;
GType goo_cairo_line_join_get_type (void) G_GNUC_CONST;

cairo_pattern_t* goo_canvas_cairo_pattern_from_pixbuf (GdkPixbuf *pixbuf);
cairo_surface_t* goo_canvas_cairo_surface_from_pixbuf (GdkPixbuf *pixbuf);


G_END_DECLS

#endif /* __GOO_CANVAS_PRIVATE_H__ */
