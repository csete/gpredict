
#ifndef __goo_canvas_marshal_MARSHAL_H__
#define __goo_canvas_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:VOID (./goocanvasmarshal.list:1) */
#define goo_canvas_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* VOID:INT (./goocanvasmarshal.list:2) */
#define goo_canvas_marshal_VOID__INT	g_cclosure_marshal_VOID__INT

/* VOID:INT,INT (./goocanvasmarshal.list:3) */
extern void goo_canvas_marshal_VOID__INT_INT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:BOOLEAN (./goocanvasmarshal.list:4) */
#define goo_canvas_marshal_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN

/* VOID:OBJECT,OBJECT (./goocanvasmarshal.list:5) */
extern void goo_canvas_marshal_VOID__OBJECT_OBJECT (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

/* BOOLEAN:BOXED (./goocanvasmarshal.list:6) */
extern void goo_canvas_marshal_BOOLEAN__BOXED (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* BOOLEAN:OBJECT,BOXED (./goocanvasmarshal.list:7) */
extern void goo_canvas_marshal_BOOLEAN__OBJECT_BOXED (GClosure     *closure,
                                                      GValue       *return_value,
                                                      guint         n_param_values,
                                                      const GValue *param_values,
                                                      gpointer      invocation_hint,
                                                      gpointer      marshal_data);

G_END_DECLS

#endif /* __goo_canvas_marshal_MARSHAL_H__ */

