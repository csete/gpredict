/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <gtk/gtk.h>
#include "sat-debugger.h"


static GtkWidget *window;
static GtkWidget *lonsp,*latsp;
static gboolean active = FALSE;

static gint debugger_delete     (GtkWidget *, GdkEvent *, gpointer);


void
sat_debugger_run (void)
{
     GtkWidget *hbox;
     GtkObject *adj1,*adj2;


     if (active)
          return;

     

     window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

     adj1 = gtk_adjustment_new (0.0, -180.0, 180.0, 0.1, 1.0, 1);
     lonsp = gtk_spin_button_new (GTK_ADJUSTMENT (adj1), 0.1, 1);

     adj2 = gtk_adjustment_new (0.0, -90.0, 90.0, 0.1, 1.0, 1);
     latsp = gtk_spin_button_new (GTK_ADJUSTMENT (adj2), 0.1, 1);

     hbox = gtk_hbox_new (TRUE,5);
     
     gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new ("LON:"), TRUE, TRUE, 0);
     gtk_box_pack_start (GTK_BOX (hbox), lonsp, TRUE, TRUE, 0);
     gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new ("LAT:"), TRUE, TRUE, 0);
     gtk_box_pack_start (GTK_BOX (hbox), latsp, TRUE, TRUE, 0);

     gtk_container_add (GTK_CONTAINER (window), hbox);
     gtk_container_set_border_width (GTK_CONTAINER (window), 20);

     g_signal_connect (G_OBJECT (window), "delete_event",
                 G_CALLBACK (debugger_delete), NULL);    

     gtk_widget_show_all (window);

     active = TRUE;
}


void
sat_debugger_get_ssp (gdouble *lon, gdouble *lat)
{
     if (active) {
          *lon = gtk_spin_button_get_value (GTK_SPIN_BUTTON (lonsp));
          *lat = gtk_spin_button_get_value (GTK_SPIN_BUTTON (latsp));
     }
     else {
          *lon = 0.0;
          *lat = 0.0;
     }
}



void
sat_debugger_close (void)
{
     if (active) {
          gtk_widget_destroy (window);
          active = FALSE;
     }
}


static gint
debugger_delete      (GtkWidget *widget,
                GdkEvent  *event,
                gpointer   data)
{
    (void) widget; /* avoid unused parameter compiler warning */
    (void) event; /* avoid unused parameter compiler warning */ 
    (void) data; /* avoid unused parameter compiler warning */ 

    active = FALSE;
    return FALSE;
}
