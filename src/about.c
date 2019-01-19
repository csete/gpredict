/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2019  Alexandru Csete, OZ9AEC.

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
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "about.h"
#include "compat.h"


const gchar *authors[] = {
    "Alexandru Csete, OZ9AEC, with contributions from:",
    "",
    "A. Maitland Bottoms, AA4HS",
    "Alan Moffet, KE7IJZ",
    "Baris Dinc, TA7W",
    "Charles Suprin, AA1VS",
    "Dale Mellor",
    "Daniel Estevez, EA4GPZ",
    "Dave Hines",
    "David Cureton",
    "David VK5DG",
    "Fabian P. Schmidt",
    "Gisle Vanem",
    "Henry Hallam",
    "Ilias Daradimos",
    "Jan Simon, DL2ZXA",
    "Jason Uher",
    "John Magliacane, KD2BD",
    "Libre Space Foundation",
    "Lloyd Brown",
    "LongnoseRob, JI1MNC",
    "Marcel Cimander",
    "Mario Haustein",
    "Martin Pool",
    "Matthew Alberti, KM4EXS",
    "Michael Tatarinov",
    "Mirko Caserta",
    "Nate Bargmann, N0NB",
    "Neoklis Kyriazis, 5B4AZ",
    "Patrick Dohmen, DL4PD",
    "Patrick Strasser, OE6PSE",
    "Paul Schulz, VK5FPAW",
    "Phil Ashby, 2E0IPX",
    "S. R. Sampson",
    "Stefan Lobas",
    "Stephane Fillod",
    "Tom Jones",
    "T.S. Kelso",
    "Valentin Yakovenkov",
    "William J Beksi, KC2EXL",
    "Xavier Crehueras, EB3CZS",
    "Yaroslav Stavnichiy",
    "",
    "Imagery:",
    "Most of the maps originate from NASA Visible Earth",
    "see website http://visibleearth.nasa.gov/",
    NULL
};

extern GtkWidget *app;

void about_dialog_create()
{
    GtkWidget      *dialog;
    GdkPixbuf      *icon;
    gchar          *iconfile;

    dialog = gtk_about_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app));
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), _("Gpredict"));
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),
                                   _("Copyright (C) 2001-2019 Alexandru Csete OZ9AEC and contributors"));
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
                                 "http://gpredict.oz9aec.net/");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog),
                                      GTK_LICENSE_GPL_2_0);
    iconfile = logo_file_name("gpredict_icon_color.svg");
    icon = gdk_pixbuf_new_from_file_at_size(iconfile, 128, 128, NULL);
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), icon);
    gtk_window_set_icon_from_file(GTK_WINDOW(dialog), iconfile, NULL);
    g_free(iconfile);
    g_object_unref(icon);
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
    gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog),
                                            _("translator-credits"));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
