/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2009  Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>

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
#include <gtk/gtk.h>
#include "sgpsdp/sgp4sdp4.h"
#include "predict-tools.h"
#include "gtk-sat-data.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "pass-to-txt.h"
#include "print-pass.h"
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif


/* In points */
#define HEADER_HEIGHT (10*72/25.4)
#define HEADER_GAP (3*72/25.4)

typedef struct
{
    //gchar *filename;
    gdouble font_size;

    gint lines_per_page;

    gchar  *pgheader;
    gchar **lines;
    gint num_lines;
    gint num_pages;
    gint fields;
} PrintData;



static void begin_print (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gpointer           user_data);
static void draw_page (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr,
                       gpointer           user_data);
static void end_print (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gpointer           user_data);


/** \brief Print a satellite pass.
  * \param pass Pointer to the pass_t data
  * \param qth Pointer to the qth_t structure
  * \param parent Transient parent of the dialog, or NULL
  *
  * This function prints a satellite pass to the printer (or a file) using the
  * Gtk+ printing API. The function takes the user configuration into account
  * and only prints the selected columns. The font size will be adjusted so that
  * one row can fit on one line. The function will also try to reduce the number
  * of rows so that the whole pass can fit on one page:
  *
  *    +-------------------------+
  *    |         header          |
  *    |------------+------------|
  *    |            |            |
  *    |            |            |
  *    |  polar     |   az/el    |
  *    |            |            |
  *    |------------+------------|
  *    |                         |
  *    |    Table with data      |
  *    |                         |
  *    |  - - - - - - - - - - -  |
  *    |  - - - - - - - - - - -  |
  *    |  - - - - - - - - - - -  |
  *    |  - - - - - - - - - - -  |
  *    |  - - - - - - - - - - -  |
  *    |  - - - - - - - - - - -  |
  *    |  - - - - - - - - - - -  |
  *    |                         |
  *    +-------------------------+
  *
  */
void print_pass   (pass_t *pass, qth_t *qth, GtkWindow *parent)
{
    gchar *text,*header,*buff;

    GtkPrintOperation *operation;
    PrintData *data;
    GError *error = NULL;


    /* TODO check pass and qth */


    operation = gtk_print_operation_new ();
    data = g_new0 (PrintData, 1);
    data->font_size = 12.0;  // FIXME

    /* page header */
    data->pgheader = g_strdup_printf (_("Pass details for %s (orbit %d)"), pass->satname, pass->orbit);

    /* convert data to printable strings; we use existing pass_to_txt functions */
    data->fields = sat_cfg_get_int (SAT_CFG_INT_PRED_SINGLE_COL);
    header = pass_to_txt_tblheader (pass, qth, data->fields);
    text = pass_to_txt_tblcontents (pass, qth, data->fields);
    buff = g_strconcat (header, text, NULL);
    data->lines = g_strsplit (buff, "\n", 0);
    g_free (text);
    g_free (header);
    g_free (buff);

    g_signal_connect (G_OBJECT (operation), "begin-print", G_CALLBACK (begin_print), data);
    g_signal_connect (G_OBJECT (operation), "draw-page", G_CALLBACK (draw_page), data);
    g_signal_connect (G_OBJECT (operation), "end-print", G_CALLBACK (end_print), data);

    gtk_print_operation_set_use_full_page (operation, FALSE);
    gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);

    gtk_print_operation_run (operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                             parent, &error);

    g_object_unref (operation);


    if (error) {
        sat_log_log (SAT_LOG_LEVEL_ERROR,
                     "%s: %s", __func__, error->message);

        GtkWidget *dialog;

        dialog = gtk_message_dialog_new (parent,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         "%s", error->message);
        g_error_free (error);

        g_signal_connect (dialog, "response",
                          G_CALLBACK (gtk_widget_destroy), NULL);

        gtk_widget_show (dialog);
    }


}


/** \brief Print passes
  * \param passes A GSList containing pointers to pass_t data.
  *
  * Create a summary page, then for each pass_t call print_pass()
  */
void print_passes (GSList *passes)
{
    (void) passes; /* avoid unused parameter compiler warning */

    sat_log_log (SAT_LOG_LEVEL_ERROR, _("%s: Not implemented!"), __func__);
}







static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             gpointer           user_data)
{
    PrintData *data = (PrintData *)user_data;
    //char *contents;
    int i;
    double height;

    height = gtk_print_context_get_height (context) - HEADER_HEIGHT - HEADER_GAP;

    data->lines_per_page = floor (height / data->font_size);

    //g_file_get_contents (data->filename, &contents, NULL, NULL);

    //data->lines = g_strsplit (contents, "\n", 0);
    //g_free (contents);

    i = 0;
    while (data->lines[i] != NULL)
        i++;

    data->num_lines = i;
    data->num_pages = (data->num_lines - 1) / data->lines_per_page + 1;

    gtk_print_operation_set_n_pages (operation, data->num_pages);
}



static void
draw_page (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gint               page_nr,
           gpointer           user_data)
{
    PrintData *data = (PrintData *)user_data;
    cairo_t *cr;
    PangoLayout *layout;
    gint text_width, text_height;
    gdouble width;
    gint line, i;
    PangoFontDescription *desc;
    gchar *page_str;

    (void) operation; /* avoid unused parameter compiler warning */

    cr = gtk_print_context_get_cairo_context (context);

    width = gtk_print_context_get_width (context);

    cairo_rectangle (cr, 0, 0, width, HEADER_HEIGHT);

    cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
    cairo_fill_preserve (cr);

    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_set_line_width (cr, 1);
    cairo_stroke (cr);


    layout = gtk_print_context_create_pango_layout (context);

    desc = pango_font_description_from_string ("sans 14");
    pango_layout_set_font_description (layout, desc);
    pango_font_description_free (desc);

    pango_layout_set_text (layout, data->pgheader, -1);
    pango_layout_get_pixel_size (layout, &text_width, &text_height);

    if (text_width > width) {
        pango_layout_set_width (layout, width);
        pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_START);
        pango_layout_get_pixel_size (layout, &text_width, &text_height);
    }

    cairo_move_to (cr, (width - text_width) / 2,  (HEADER_HEIGHT - text_height) / 2);
    pango_cairo_show_layout (cr, layout);

    page_str = g_strdup_printf ("%d/%d", page_nr + 1, data->num_pages);
    pango_layout_set_text (layout, page_str, -1);
    g_free (page_str);

    pango_layout_set_width (layout, -1);
    pango_layout_get_pixel_size (layout, &text_width, &text_height);
    cairo_move_to (cr, width - text_width - 4, (HEADER_HEIGHT - text_height) / 2);
    pango_cairo_show_layout (cr, layout);

    g_object_unref (layout);

    layout = gtk_print_context_create_pango_layout (context);

    desc = pango_font_description_from_string ("monospace");
    pango_font_description_set_size (desc, data->font_size * PANGO_SCALE);
    pango_layout_set_font_description (layout, desc);
    pango_font_description_free (desc);

    cairo_move_to (cr, 0, HEADER_HEIGHT + HEADER_GAP);
    line = page_nr * data->lines_per_page;
    for (i = 0; i < data->lines_per_page && line < data->num_lines; i++) {
        pango_layout_set_text (layout, data->lines[line], -1);
        pango_cairo_show_layout (cr, layout);
        cairo_rel_move_to (cr, 0, data->font_size);
        line++;
    }

    g_object_unref (layout);
}


/**
  *
  */
static void end_print (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gpointer           user_data)
{
    PrintData *data = (PrintData *)user_data;
    
    (void) operation; /* avoid unused parameter compiler warning */
    (void) context; /* avoid unused parameter compiler warning */
    
    g_free (data->pgheader);
    g_strfreev (data->lines);
    g_free (data);
}
