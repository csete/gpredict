/*
 * GooCanvas. Copyright (C) 2005-6 Damon Chaplin.
 * Released under the GNU LGPL license. See COPYING for details.
 *
 * goocanvaswidget.h - wrapper item for an embedded GtkWidget.
 */
#ifndef __GOO_CANVAS_WIDGET_H__
#define __GOO_CANVAS_WIDGET_H__

#include <gtk/gtk.h>
#include "goocanvasitemsimple.h"

G_BEGIN_DECLS


#define GOO_TYPE_CANVAS_WIDGET            (goo_canvas_widget_get_type ())
#define GOO_CANVAS_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CANVAS_WIDGET, GooCanvasWidget))
#define GOO_CANVAS_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CANVAS_WIDGET, GooCanvasWidgetClass))
#define GOO_IS_CANVAS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CANVAS_WIDGET))
#define GOO_IS_CANVAS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CANVAS_WIDGET))
#define GOO_CANVAS_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_CANVAS_WIDGET, GooCanvasWidgetClass))


typedef struct _GooCanvasWidget       GooCanvasWidget;
typedef struct _GooCanvasWidgetClass  GooCanvasWidgetClass;

/**
 * GooCanvasWidget
 *
 * The #GooCanvasWidget-struct struct contains private data only.
 */
struct _GooCanvasWidget
{
  GooCanvasItemSimple parent_object;

  GtkWidget *widget;
  gdouble x, y, width, height;
  GtkAnchorType anchor;
};

struct _GooCanvasWidgetClass
{
  GooCanvasItemSimpleClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_goo_canvas_reserved1) (void);
  void (*_goo_canvas_reserved2) (void);
  void (*_goo_canvas_reserved3) (void);
  void (*_goo_canvas_reserved4) (void);
};


GType          goo_canvas_widget_get_type          (void) G_GNUC_CONST;
GooCanvasItem* goo_canvas_widget_new               (GooCanvasItem    *parent,
						    GtkWidget        *widget,
						    gdouble           x,
						    gdouble           y,
						    gdouble           width,
						    gdouble           height,
						    ...);

G_END_DECLS

#endif /* __GOO_CANVAS_WIDGET_H__ */
