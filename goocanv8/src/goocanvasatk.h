/*
 * GooCanvas. Copyright (C) 2005 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvasatk.h - the accessibility code.
 */
#ifndef __GOO_CANVAS_ATK_H__
#define __GOO_CANVAS_ATK_H__

#include <gtk/gtk.h>


G_BEGIN_DECLS

GType    goo_canvas_accessible_factory_get_type  (void) G_GNUC_CONST;
GType    goo_canvas_item_accessible_factory_get_type  (void) G_GNUC_CONST;
GType    goo_canvas_widget_accessible_factory_get_type  (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GOO_CANVAS_ATK_H__ */
