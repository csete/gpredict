/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>
           Charles Suprin <hamaa1vs@gmail.com>

  Comments, questions and bugreports should be submitted via
  http://sourceforge.net/projects/gpredict/
  More details can be found at the project home page:

  http://gpredict.oz9aec.net/
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, visit http://www.fsf.org/
*/

/** \brief Edit ground station details.
 *
 * The functions in this unit are used to edit the details of a given
 * ground station. The editor consists of a simple dialog window containing
 * widget for the individual setting.
 *
 * The functions can be used for editing the details of an existing ground
 * station or for adding a new location. In the first case, the widgets
 * in dialog will be preloaded with the existing data, while in the second
 * case the widgets will be empty and, upon successful completion a new
 * entry will be added to the GtkTreeView which contains the locations.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "gpredict-utils.h"
#include "loc-tree.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sat-pref-qth-data.h"
#include "sat-pref-qth-editor.h"
#include "locator.h"



/** \brief Symbolic refs to be used when calling select_location in order
 *         to determine which mode the selection should run in, ie.
 *         select location or select weather station.
 */
enum {
     SELECTION_MODE_LOC = 1,
     SELECTION_MODE_WX  = 2
};


extern GtkWidget *window; /* dialog window defined in sat-pref.c */



/* private widgets */
static GtkWidget *dialog;         /* dialog window */
static GtkWidget *name;           /* QTH name */
static GtkWidget *location;       /* QTH location */
static GtkWidget *desc;           /* QTH description */
static GtkWidget *lat,*lon,*alt;  /* LAT, LON and ALT */
static GtkWidget *ns,*ew;

static GtkWidget *qra;            /* QRA locator */
#ifdef HAS_LIBGPS
static GtkWidget *type;         /* GPSD type */
static GtkWidget *server;         /* GPSD Server */
static GtkWidget *port;           /* GPSD Port */
#endif
static gulong latsigid,lonsigid,nssigid,ewsigid,qrasigid;

static GtkWidget *wx;             /* weather station */



static GtkWidget *create_editor_widgets (GtkTreeView *treeview, gboolean new);
static void       update_widgets        (GtkTreeView *treeview);
static void       clear_widgets         (void);
static void       name_changed          (GtkWidget *widget, gpointer data);
static void       select_location       (GtkWidget *widget, gpointer data);
static gboolean   apply_changes         (GtkTreeView *treeview, gboolean new);

static void       latlon_changed  (GtkWidget *widget, gpointer data);
static void       qra_changed     (GtkEntry *entry, gpointer data);


/** \brief Add or edit a QTH entry.
 *
 * The parameter new is used to indicate whether a new entry should be
 * created or just edit the one selected in the treeview.
 */
void
sat_pref_qth_editor_run (GtkTreeView *treeview, gboolean new)
{
     gint       response;
     gboolean   finished = FALSE;


     /* crate dialog and add contents */
     dialog = gtk_dialog_new_with_buttons (_("Edit ground station data"),
                                                    GTK_WINDOW (window),
                                                    GTK_DIALOG_MODAL |
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CLEAR,
                                                    GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OK,
                                                    GTK_RESPONSE_OK,
                                                    NULL);

     /* disable OK button to begin with */
     gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                                GTK_RESPONSE_OK,
                                                FALSE);

     gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG (dialog))),
                            create_editor_widgets (treeview, new));

     /* this hacky-thing is to keep the dialog running in case the
        CLEAR button is plressed. OK and CANCEL will exit the loop
     */
     while (!finished) {

          response = gtk_dialog_run (GTK_DIALOG (dialog));

          switch (response) {

               /* OK */
          case GTK_RESPONSE_OK:
               if (apply_changes (treeview, new)) {
                    finished = TRUE;
               }
               else {
                    finished = FALSE;
               }
               break;

               /* CLEAR */
          case GTK_RESPONSE_REJECT:
               clear_widgets ();
               break;

               /* Everything else is considered CANCEL */
          default:
               finished = TRUE;
               break;
          }
     }

     gtk_widget_destroy (dialog);
}


/** \brief Create and initialise widgets */
static GtkWidget *
create_editor_widgets (GtkTreeView *treeview, gboolean new)
{
     GtkWidget   *table;
     GtkWidget   *label;
     GtkWidget   *locbut;
     GtkWidget   *wxbut;


     table = gtk_table_new (9, 4, FALSE);
     gtk_container_set_border_width (GTK_CONTAINER (table), 5);
     gtk_table_set_col_spacings (GTK_TABLE (table), 5);
     gtk_table_set_row_spacings (GTK_TABLE (table), 5);

     /* QTH name */
     label = gtk_label_new (_("Name"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
     
     name = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (name), 25);
     gtk_widget_set_tooltip_text (name,
                           _("Enter a short name for this ground station, e.g. callsign.\n" \
                             "Allowed characters: 0..9, a..z, A..Z, - and _"));
     /* new api does not allow private tip 
                                _("The name will be used to identify the ground station when "\
                                   "it is presented to the user. Maximum allowed length "\
                                   "is 25 characters."));
     */
     gtk_table_attach_defaults (GTK_TABLE (table), name, 1, 4, 0, 1);

     /* attach changed signal so that we can enable OK button when
        a proper name has been entered
     */
     g_signal_connect (name, "changed", G_CALLBACK (name_changed), NULL);

     /* QTH description */
     label = gtk_label_new (_("Description"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
     
     desc = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (desc), 256);
     gtk_widget_set_tooltip_text (desc,
                                  _("Enter an optional description for this ground station."));
     /* new api does not have private tip 
                                _("The description can be used as additional "\
                                   "information. It may be included when generating reports. "\
                                   "The maximum length for the description is 256 characters."));
     */
     gtk_table_attach_defaults (GTK_TABLE (table), desc, 1, 4, 1, 2);

     /* location */
     label = gtk_label_new (_("Location"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
     
     location = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (location), 50);
     gtk_widget_set_tooltip_text (location,
                                _("Optional location of the ground station, fx. Copenhagen, Denmark."));
     gtk_table_attach_defaults (GTK_TABLE (table), location, 1, 3, 2, 3);

     locbut = gpredict_hstock_button (GTK_STOCK_INDEX, _("Select"),
                                              _("Select a predefined location from a list."));
     g_signal_connect (locbut, "clicked",
                           G_CALLBACK (select_location),
                           GUINT_TO_POINTER (SELECTION_MODE_LOC));
     gtk_table_attach_defaults (GTK_TABLE (table), locbut, 3, 4, 2, 3);
     

     /* latitude */
     label = gtk_label_new (_("Latitude (\302\260)"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);

     lat = gtk_spin_button_new_with_range (0.00, 90.00, 0.0001);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (lat), 0.0001, 1.0);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (lat), TRUE);
     gtk_spin_button_set_digits (GTK_SPIN_BUTTON (lat), 4);
     gtk_widget_set_tooltip_text (lat,
                                  _("Select the latitude of the ground station in decimal degrees."));
     gtk_table_attach_defaults (GTK_TABLE (table), lat, 1, 2, 3, 4);

     ns = gtk_combo_box_new_text ();
     gtk_combo_box_append_text (GTK_COMBO_BOX (ns), _("North"));
     gtk_combo_box_append_text (GTK_COMBO_BOX (ns), _("South"));
     gtk_combo_box_set_active (GTK_COMBO_BOX (ns), 0);
     /*** FIXME tooltips */
     gtk_table_attach_defaults (GTK_TABLE (table), ns, 2, 3, 3, 4);

     /* longitude */
     label = gtk_label_new (_("Longitude (\302\260)"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);

     lon = gtk_spin_button_new_with_range (0.00, 180.00, 0.0001);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (lon), 0.0001, 1.0);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (lon), TRUE);
     gtk_spin_button_set_digits (GTK_SPIN_BUTTON (lon), 4);
     gtk_widget_set_tooltip_text (lon,
                                  _("Select the longitude of the ground station in decimal degrees."));
     gtk_table_attach_defaults (GTK_TABLE (table), lon, 1, 2, 4, 5);

     ew = gtk_combo_box_new_text ();
     gtk_combo_box_append_text (GTK_COMBO_BOX (ew), _("East"));
     gtk_combo_box_append_text (GTK_COMBO_BOX (ew), _("West"));
     gtk_combo_box_set_active (GTK_COMBO_BOX (ew), 0);
     /*** FIXME tooltips */
     gtk_table_attach_defaults (GTK_TABLE (table), ew, 2, 3, 4, 5);


     /* connect lat/lon spinners and combos to callback
        remember signal id so that we can block signals
        while doing automatic cross-updates
     */
     latsigid = g_signal_connect (lat, "value-changed",
                                         G_CALLBACK (latlon_changed),
                                         NULL);
     lonsigid = g_signal_connect (lon, "value-changed",
                                         G_CALLBACK (latlon_changed),
                                         NULL);
     nssigid = g_signal_connect (ns, "changed",
                                        G_CALLBACK (latlon_changed),
                                        NULL);
     ewsigid = g_signal_connect (ew, "changed",
                                        G_CALLBACK (latlon_changed),
                                        NULL);
     
     /* QRA locator */
     label = gtk_label_new (_("Locator"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 5, 6);

     qra = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (qra), 6);
     gtk_widget_set_tooltip_text (qra,
                                  _("Maidenhead locator grid."));
     gtk_table_attach_defaults (GTK_TABLE (table), qra, 1, 2, 5, 6);
     qrasigid = g_signal_connect (qra, "changed",
                                         G_CALLBACK (qra_changed),
                                         NULL);

     /* altitude */
     label = gtk_label_new (_("Altitude"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 6, 7);

     alt = gtk_spin_button_new_with_range (0, 5000, 1);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (alt), 1, 100);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (alt), TRUE);
     gtk_widget_set_tooltip_text (alt,
                                  _("Select the altitude of the ground station in meters or feet " \
                                    "depending on your settings"));
     gtk_table_attach_defaults (GTK_TABLE (table), alt, 1, 2, 6, 7);

     if (sat_cfg_get_bool (SAT_CFG_BOOL_USE_IMPERIAL)) {
          label = gtk_label_new (_("ft above sea level"));
     }
     else {
          label = gtk_label_new (_("m above sea level"));
     }
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 6, 7);

     /* weather station */
     label = gtk_label_new (_("Weather St"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 7, 8);

     wx = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (wx), 4);
     gtk_widget_set_tooltip_text (wx,
                                  _("Four letter code for weather station"));
     gtk_table_attach_defaults (GTK_TABLE (table), wx, 1, 3, 7, 8);

     wxbut = gpredict_hstock_button (GTK_STOCK_INDEX, _("Select"),
                                             _("Select a predefined weather station from a list."));
     g_signal_connect (wxbut, "clicked",
                           G_CALLBACK (select_location),
                           GUINT_TO_POINTER (SELECTION_MODE_WX));
     gtk_table_attach_defaults (GTK_TABLE (table), wxbut, 3, 4, 7, 8);

#  ifdef HAS_LIBGPS
     /* GPSD enabled*/
     label = gtk_label_new (_("QTH Type"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 8, 9);

     type = gtk_combo_box_new_text();
     gtk_combo_box_append_text (GTK_COMBO_BOX (type), "Static");
     gtk_combo_box_append_text (GTK_COMBO_BOX (type), "GPSD");
     gtk_combo_box_set_active (GTK_COMBO_BOX (type), 0);
     gtk_widget_set_tooltip_text (type,
                                  _("A qth can be static, ie. it does not change, or gpsd based for computers with gps attached."));
     gtk_table_attach_defaults (GTK_TABLE (table), type, 1, 2, 8, 9);
     /* GPSD Server */
     label = gtk_label_new (_("GPSD Server"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 9, 10);

     server = gtk_entry_new ();
     gtk_entry_set_max_length (GTK_ENTRY (server), 6000);
     gtk_widget_set_tooltip_text (server,
                                  _("GPSD Server."));
     gtk_table_attach_defaults (GTK_TABLE (table), server, 1, 2, 9, 10);
     
     /* GPSD Port*/
     label = gtk_label_new (_("GPSD Port"));
     gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
     gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 10, 11);

     port = gtk_spin_button_new_with_range (0, 32768, 1);
     gtk_spin_button_set_increments (GTK_SPIN_BUTTON (port), 1, 100);
     gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (port), TRUE);
     gtk_widget_set_tooltip_text (port,
                                  _("Set the port for GPSD to use. Default for gpsd is 2947."));
     gtk_table_attach_defaults (GTK_TABLE (table), port, 1, 2, 10, 11);
#  endif

     if (!new)
          update_widgets (treeview);

     gtk_widget_show_all (table);

     return table;
}


/** \brief Update widgets from the currently selected row in the treeview
 */
static void
update_widgets (GtkTreeView *treeview)
{
     GtkTreeSelection *selection; /* the selection in the tree view */
     GtkTreeModel     *model;     /* the tree model corresponding to the selection */
     GtkTreeIter       iter;      /* the iter of the selection */
     gchar            *qthname;   /* location name */
     gchar            *qthdesc;   /* location description */
     gchar            *qthloc;    /* location */
     gdouble           qthlat;    /* latitude */
     gdouble           qthlon;    /* longitude */
     guint             qthalt;    /* altitude */
     gchar            *qthwx;     /* weather station */
     gchar            *qthgpsdserver;     /* gpsdserver */
     guint             qthtype;    /* type */
     guint             qthgpsdport;    /* gpsdport */


     selection = gtk_tree_view_get_selection (treeview);

     if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

          /* get values */
          gtk_tree_model_get (model, &iter,
                                   QTH_LIST_COL_NAME, &qthname,
                                   QTH_LIST_COL_LOC,  &qthloc,
                                   QTH_LIST_COL_DESC, &qthdesc,
                                   QTH_LIST_COL_LAT, &qthlat,
                                   QTH_LIST_COL_LON, &qthlon,
                                   QTH_LIST_COL_ALT, &qthalt,
                                   QTH_LIST_COL_WX, &qthwx,
                              QTH_LIST_COL_GPSD_SERVER, &qthgpsdserver,
                              QTH_LIST_COL_GPSD_PORT, &qthgpsdport,
                              QTH_LIST_COL_TYPE, &qthtype,
                                   -1);

          /* update widgets and free memory afterwards */
          if (qthname) {
               gtk_entry_set_text (GTK_ENTRY (name), qthname);
          }

          if (qthloc) {
               gtk_entry_set_text (GTK_ENTRY (location), qthloc);
               g_free (qthloc);
          }

          if (qthdesc) {
               gtk_entry_set_text (GTK_ENTRY (desc), qthdesc);
               g_free (qthdesc);
          }

          if (qthwx) {
               gtk_entry_set_text (GTK_ENTRY (wx), qthwx);
               g_free (qthwx);
          }
#ifdef HAS_LIBGPS
          if (qthgpsdserver) {
               gtk_entry_set_text (GTK_ENTRY (server), qthgpsdserver);
               g_free (qthgpsdserver);
          }
#endif
          if (qthlat < 0.00)
               gtk_combo_box_set_active (GTK_COMBO_BOX (ns), 1);
          else
               gtk_combo_box_set_active (GTK_COMBO_BOX (ns), 0);
               
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (lat), fabs (qthlat));

          if (qthlon < 0.00)
               gtk_combo_box_set_active (GTK_COMBO_BOX (ew), 1);
          else
               gtk_combo_box_set_active (GTK_COMBO_BOX (ew), 0);

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (lon), fabs (qthlon));

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (alt), qthalt);
#ifdef HAS_LIBGPS
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (port), qthgpsdport);
          gtk_combo_box_set_active (GTK_COMBO_BOX (type), qthtype);
#endif

          sat_log_log (SAT_LOG_LEVEL_DEBUG,
                          _("%s:%d: Loaded %s for editing:\n"\
                            "LAT:%.4f LON:%.4f ALT:%d"),
                          __FILE__, __LINE__,
                          (qthname != NULL) ? qthname : "???",
                          qthlat, qthlon, qthalt);

          g_free (qthname);

     }
     else {
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s:%d: No ground station selected!"),
                          __FILE__, __LINE__);
     }
}



/** \brief Clear the contents of all widgets.
 *
 * This function is usually called when the user clicks on the CLEAR button
 *
 */
static void
clear_widgets () 
{
     gtk_entry_set_text (GTK_ENTRY (name), "");
     gtk_entry_set_text (GTK_ENTRY (location), "");
     gtk_entry_set_text (GTK_ENTRY (desc), "");
     gtk_entry_set_text (GTK_ENTRY (wx), "");
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (lat), 0.0);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (lon), 0.0);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (alt), 0);
     gtk_entry_set_text (GTK_ENTRY (qra), "");
}


/** \brief Apply changes.
 *  \return TRUE if things are ok, FALSE otherwise.
 *
 * This function is usually called when the user clicks the OK button.
 */
static gboolean
apply_changes         (GtkTreeView *treeview, gboolean new)
{
     GtkTreeSelection *selection; /* selection in the treeview */
     GtkTreeModel     *model;     /* the tree model corresponding to the selection */
     GtkListStore     *liststore; /* the list store corresponding to the model */
     GtkTreeIter       iter;      /* iter used to add and modify row data */
     const gchar      *qthname;
     const gchar      *qthloc;
     const gchar      *qthdesc;
     const gchar      *qthwx;
     const gchar      *qthgpsdserver;
     gdouble           qthlat;
     gdouble           qthlon;
     guint             qthalt;
     guint             qthtype;
     guint             qthgpsdport;
     const gchar      *qthqra;



     /* get values from dialog box */
     qthname = gtk_entry_get_text (GTK_ENTRY (name));
     qthloc  = gtk_entry_get_text (GTK_ENTRY (location));
     qthdesc = gtk_entry_get_text (GTK_ENTRY (desc));
     qthwx   = gtk_entry_get_text (GTK_ENTRY (wx));

#ifdef HAS_LIBGPS
     qthgpsdserver   = gtk_entry_get_text (GTK_ENTRY (server));
#endif

     qthlat  = gtk_spin_button_get_value (GTK_SPIN_BUTTON (lat));
     if (gtk_combo_box_get_active (GTK_COMBO_BOX (ns)))
          qthlat = -qthlat;

     qthlon  = gtk_spin_button_get_value (GTK_SPIN_BUTTON (lon));
     if (gtk_combo_box_get_active (GTK_COMBO_BOX (ew)))
          qthlon = -qthlon;

     qthalt = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (alt));     

#ifdef HAS_LIBGPS
     qthgpsdport = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (port));     
     qthtype = gtk_combo_box_get_active ( GTK_COMBO_BOX (type));
#endif

     /* get liststore */
     liststore = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

     /* if this is a new entry, insert row into model */
     if (new == TRUE) {
          gtk_list_store_append (liststore, &iter);
     }
     /* otherwise get current selection */
     else {
          selection = gtk_tree_view_get_selection (treeview);

          if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
               liststore = GTK_LIST_STORE (model);
          }
          else {
               /* no selection; internal error */
               sat_log_log (SAT_LOG_LEVEL_ERROR,
                               _("%s:%d: Oooops, gpredict encountered an internal error "\
                                 "(no selection in qth list)"),
                               __FILE__, __LINE__);

               return FALSE;
          }
     }

     /* update values */
     gtk_list_store_set (liststore, &iter,
                              QTH_LIST_COL_NAME, qthname,
                              QTH_LIST_COL_LOC, qthloc,
                              QTH_LIST_COL_DESC, qthdesc,
                              QTH_LIST_COL_LAT, qthlat,
                              QTH_LIST_COL_LON, qthlon,
                              QTH_LIST_COL_ALT, qthalt,
                              QTH_LIST_COL_WX, qthwx,
                              QTH_LIST_COL_GPSD_SERVER, qthgpsdserver,
                              QTH_LIST_COL_GPSD_PORT, qthgpsdport,
                              QTH_LIST_COL_TYPE, qthtype,
                              -1);

     qthqra  = gtk_entry_get_text (GTK_ENTRY (qra));
     gtk_list_store_set (liststore, &iter,
                              QTH_LIST_COL_QRA, qthqra,
                              -1);

     return TRUE;
}



/** \brief Manage name changes.
 *
 * This function is called when the contents of the name entry changes.
 * The primary purpose of this function is to check whether the char length
 * of the name is greater than zero, if yes enable the OK button of the dialog.
 */
static void
name_changed          (GtkWidget *widget, gpointer data)
{
     const gchar *text;
     gchar *entry, *end, *j;
     gint len, pos;

     (void) data; /* avoid unused parameter compiler warning */

     /* step 1: ensure that only valid characters are entered
        (stolen from xlog, tnx pg4i)
     */
     entry = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
     if ((len = g_utf8_strlen (entry, -1)) > 0)
          {
               end = entry + g_utf8_strlen (entry, -1);
               for (j = entry; j < end; ++j)
                    {
                         switch (*j)
                              {
                              case '0' ... '9':
                              case 'a' ... 'z':
                              case 'A' ... 'Z':
                              case '-':
                              case '_':
                                   break;
                              default:
                                   gdk_beep ();
                                   pos = gtk_editable_get_position (GTK_EDITABLE (widget));
                                   gtk_editable_delete_text (GTK_EDITABLE (widget),
                                                                   pos, pos+1);
                                   break;
                              }
                    }
          }


     /* step 2: if name seems all right, enable OK button */
     text = gtk_entry_get_text (GTK_ENTRY (widget));

     if (g_utf8_strlen (text, -1) > 0) {
          gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                                     GTK_RESPONSE_OK,
                                                     TRUE);
     }
     else {
          gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                                     GTK_RESPONSE_OK,
                                                     FALSE);
     }
}



/** \brief Manage SELECT button clicks.
 *
 * This function is called when the user clicks on one of the SELECT buttons.
 * the data parameter contains information about which button has been clicked.
 */
static void
select_location       (GtkWidget *widget, gpointer data)
{
     guint    mode = GPOINTER_TO_UINT (data);
     guint    flags;
     gchar   *qthloc;
     gchar   *qthwx;
     gfloat   qthlat;
     gfloat   qthlon;
     guint    qthalt;
     gboolean selected = FALSE;

     (void) widget; /* avoid unused parameter compiler warning */

     switch (mode) {

          /* We distinguish only between WX mode and "everything else".
             Although a value != 1 or 2 is definitely a bug, we need to
             have some sensible fall-back.
          */
     case SELECTION_MODE_WX:
          flags = TREE_COL_FLAG_NAME | TREE_COL_FLAG_WX;
          break;

     default:
          flags = TREE_COL_FLAG_NAME |
               TREE_COL_FLAG_LAT  |
               TREE_COL_FLAG_LON  |
               TREE_COL_FLAG_ALT  |
               TREE_COL_FLAG_WX;

          mode = SELECTION_MODE_LOC;
          break;
     }

     selected = loc_tree_create (NULL, flags, &qthloc, &qthlat, &qthlon, &qthalt, &qthwx);

     if (selected) {
          /* update widgets */
          switch (mode) {

          case SELECTION_MODE_WX:
               gtk_entry_set_text (GTK_ENTRY (wx), qthwx);
               break;

          case SELECTION_MODE_LOC:
               gtk_entry_set_text (GTK_ENTRY (location), qthloc);
               gtk_entry_set_text (GTK_ENTRY (wx), qthwx);

               gtk_spin_button_set_value (GTK_SPIN_BUTTON (lat), (gdouble) fabs (qthlat));
               if (qthlat < 0.00)
                    gtk_combo_box_set_active (GTK_COMBO_BOX (ns), 1);
               else
                    gtk_combo_box_set_active (GTK_COMBO_BOX (ns), 0);

               gtk_spin_button_set_value (GTK_SPIN_BUTTON (lon), (gdouble) fabs (qthlon));
               if (qthlon < 0.00)
                    gtk_combo_box_set_active (GTK_COMBO_BOX (ew), 1);
               else
                    gtk_combo_box_set_active (GTK_COMBO_BOX (ew), 0);
               
               gtk_spin_button_set_value (GTK_SPIN_BUTTON (alt), qthalt);

               break;

          default:
               /*** FIXME: add some error reporting */
               break;
          }

          /* free some memory */
          g_free (qthloc);
          g_free (qthwx);
     }

     /* else do nothing; we are finished */
}





/** \brief Manage coordinate changes.
 *
 * This function is called when the qth coordinates change. The change can
 * be either one of the spin buttons or the combo boxes. It reads the
 * coordinates and the calculates the new Maidenhead locator square.
 */
static void
latlon_changed        (GtkWidget *widget, gpointer data)
{
     gchar *locator;
     gint   retcode;
     gdouble latf,lonf;
     
     (void) widget; /* avoid unused parameter compiler warning */
     (void) data; /* avoid unused parameter compiler warning */
     
     locator = g_try_malloc (7);
     
     /* no need to check locator != NULL, since hamlib func will do it for us
        and return RIGEINVAL
     */
     lonf = gtk_spin_button_get_value (GTK_SPIN_BUTTON (lon));
     latf = gtk_spin_button_get_value (GTK_SPIN_BUTTON (lat));

     /* set the correct sign */
     if (gtk_combo_box_get_active (GTK_COMBO_BOX (ns))) {
          /* index 1 => South */
          latf = -latf;
     }

     if (gtk_combo_box_get_active (GTK_COMBO_BOX (ew))) {
          /* index 1 => Wesr */
          lonf = -lonf;
     }

     retcode = longlat2locator (lonf, latf, locator, 3);

     if (retcode == RIG_OK) {
          /* debug message */
          sat_log_log (SAT_LOG_LEVEL_DEBUG,
                          _("%s:%s: %.2f %.2f => %s"),
                          __FILE__, __func__,
                          gtk_spin_button_get_value (GTK_SPIN_BUTTON (lon)),
                          gtk_spin_button_get_value (GTK_SPIN_BUTTON (lat)),
                          locator);

          g_signal_handler_block (qra, qrasigid);

          gtk_entry_set_text (GTK_ENTRY (qra), locator);

          g_signal_handler_unblock (qra, qrasigid);
     }
     else {
          /* send an error message and don't update */
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s:%d: Error converting lon/lat to locator"),
                          __FILE__, __LINE__);
     }


     if (locator)
          g_free (locator);
}

/** \brief Manage locator changes.
 *
 * This function is called when the Maidenhead locator is changed.
 * It will calculate the new coordinates and update the spin butrtons and
 * the combo boxes.
 */
static void
qra_changed     (GtkEntry *entry, gpointer data)
{
     gint retcode;
     gdouble latf,lonf;
     gchar *msg;
     
     (void) entry; /* avoid unused parameter compiler warning */
     (void) data; /* avoid unused parameter compiler warning */

     retcode = locator2longlat (&lonf, &latf, gtk_entry_get_text (GTK_ENTRY (qra)));

     if (retcode == RIG_OK) {

          /* debug message */
          sat_log_log (SAT_LOG_LEVEL_DEBUG,
                          _("%s:%s: %s => %.2f %.2f"),
                          __FILE__, __func__,
                          gtk_entry_get_text (GTK_ENTRY (qra)),
                          lonf, latf);

          /* block signal emissions for lat/lon widgets */
          g_signal_handler_block (lat, latsigid);
          g_signal_handler_block (lon, lonsigid);
          g_signal_handler_block (ns, nssigid);
          g_signal_handler_block (ew, ewsigid);
          g_signal_handler_block (qra, qrasigid);

          /* update widgets */
          if (latf < 0.00)
               gtk_combo_box_set_active (GTK_COMBO_BOX (ns), 1);
          else
               gtk_combo_box_set_active (GTK_COMBO_BOX (ns), 0);

          if (lonf < 0.00)
               gtk_combo_box_set_active (GTK_COMBO_BOX (ew), 1);
          else
               gtk_combo_box_set_active (GTK_COMBO_BOX (ew), 0);

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (lat), fabs (latf));
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (lon), fabs (lonf));

          /* make sure text is upper case */
          msg = g_ascii_strup (gtk_entry_get_text (GTK_ENTRY (qra)), -1);
          gtk_entry_set_text (GTK_ENTRY (qra), msg);
          g_free (msg);

          /* unblock signal emissions */
          g_signal_handler_unblock (lat, latsigid);
          g_signal_handler_unblock (lon, lonsigid);
          g_signal_handler_unblock (ns, nssigid);
          g_signal_handler_unblock (ew, ewsigid);
          g_signal_handler_unblock (qra, qrasigid);

     }
     else {
          /* send an error message and don't update */
          sat_log_log (SAT_LOG_LEVEL_ERROR,
                          _("%s:%d: Invalid locator: %s"),
                          __FILE__, __LINE__,
                          gtk_entry_get_text (GTK_ENTRY (qra)));
     }

}

