/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gdl-tools.c
 *
 * Copyright (C) 2002 Jeroen Zwartepoorte
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <bonobo/bonobo-ui-util.h>
#include "gdl-tools.h"

static GSList *inited_arrays = NULL;

static void
gdl_pixmaps_free (gpointer data)
{
	GdlPixmap *pixcache = data;
	int i;

	g_return_if_fail (g_slist_find (inited_arrays, pixcache) == NULL);

	for (i = 0; pixcache[i].path; i++)
		g_free (pixcache[i].pixbuf);

	inited_arrays = g_slist_remove (inited_arrays, pixcache);
	if (g_slist_length (inited_arrays) == 0) {
		g_slist_free (inited_arrays);
		inited_arrays = NULL;
	}
}

/* Copied from evolution/shell/evolution-shell-component-utils.c */
void gdl_pixmaps_update (BonoboUIComponent *uic,
			 const char *pixmap_dir,
			 GdlPixmap *pixcache)
{
	int i;

	g_return_if_fail (uic != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (uic));
	g_return_if_fail (pixmap_dir != NULL);
	g_return_if_fail (pixcache != NULL);

	if (g_slist_find (inited_arrays, pixcache) == NULL) {
		inited_arrays = g_slist_prepend (inited_arrays, pixcache);
		g_object_set_data_full (G_OBJECT (uic), "GdlPixmaps",
					pixcache, gdl_pixmaps_free);
	}

	for (i = 0; pixcache[i].path; i++) {
		if (!pixcache[i].pixbuf) {
			char *path;
			GdkPixbuf *pixbuf;

			path = g_build_filename (pixmap_dir, pixcache[i].fname, NULL);

			pixbuf = gdk_pixbuf_new_from_file (path, NULL);
			if (pixbuf == NULL) {
				g_warning ("Cannot load image -- %s", path);
			} else {
				pixcache[i].pixbuf = bonobo_ui_util_pixbuf_to_xml (pixbuf);
				g_object_unref (pixbuf);
				bonobo_ui_component_set_prop (uic,
					pixcache[i].path, "pixname",
					pixcache[i].pixbuf, NULL);
			}

			g_free (path);
		} else {
			bonobo_ui_component_set_prop (uic, pixcache[i].path,
						      "pixname",
						      pixcache[i].pixbuf,
						      NULL);
		}
	}
}

GtkWidget *
gdl_button_new_with_stock_image (const char *text,
				 const char *stock_id)
{
	GtkWidget *button;
	GtkStockItem item;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;
	GtkWidget *align;

	button = gtk_button_new ();

	if (GTK_BIN (button)->child)
		gtk_container_remove (GTK_CONTAINER (button),
				      GTK_BIN (button)->child);

	if (gtk_stock_lookup (stock_id, &item)) {
		label = gtk_label_new_with_mnemonic (text);

		gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));

		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
		hbox = gtk_hbox_new (FALSE, 2);

		align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
		gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);

		gtk_container_add (GTK_CONTAINER (button), align);
		gtk_container_add (GTK_CONTAINER (align), hbox);
		gtk_widget_show_all (align);

		return button;
	}

	label = gtk_label_new_with_mnemonic (text);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));

	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

	gtk_widget_show (label);
	gtk_container_add (GTK_CONTAINER (button), label);

	return button;
}
