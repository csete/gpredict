/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2017  Alexandru Csete, OZ9AEC.

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
    "Jan Simon, DL2ZXA",
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
    "S. R. Sampson",
    "Stefan Lobas",
    "Stephane Fillod",
    "Tom Jones",
    "T.S. Kelso",
    "Valentin Yakovenkov",
    "William J Beksi, KC2EXL"
    "",
    "Imagery:",
    "Most of the maps originate from NASA Visible Earth",
    "see website http://visibleearth.nasa.gov/",
    NULL
};

const gchar license[] = N_("Copyright (C) 2001-2017 Alexandru Csete OZ9AEC and contributors.\n"\
                           "Contact: oz9aec at gmail.com\n\n"\
                           "Gpredict is free software; you can redistribute it and "\
                           "modify it under the terms of the GNU General Public License "\
                           "as published by the Free Software Foundation; either version 2 "\
                           "of the License, or (at your option) any later version.\n\n"\
                           "This program is distributed free of charge in the hope that it will "\
                           "be useful, but WITHOUT ANY WARRANTY; without even the implied "\
                           "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "\
                           "See the GNU Library General Public License for more details.\n\n"\
                           "You should have received a copy of the GNU General Public License "\
                           "along with this program (see Help->License). Otherwise you can find "\
                           "a copy on the FSF "\
                           "website http://www.fsf.org/licensing/licenses/gpl.html or you can "\
                           "write to the\n\n"
                           "Free Software Foundation, Inc.\n"\
                           "59 Temple Place - Suite 330\n"
                           "Boston\n"\
                           "MA 02111-1307\n"
                           "USA.\n");

void about_dialog_create()
{
    GtkWidget      *dialog;
    GdkPixbuf      *icon;
    gchar          *iconfile;

    dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), _("Gpredict"));
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),
                                   _("Copyright (C) 2001-2017 Alexandru Csete OZ9AEC and contributors"));
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
                                 "http://gpredict.oz9aec.net/");
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), _(license));
    gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(dialog), TRUE);
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
