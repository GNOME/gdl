/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- 
 * gdl-stock.c
 * 
 * Copyright (C) 2003 Jeroen Zwartepoorte
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gtk/gtkiconfactory.h>
#include "gdl-stock.h"

static GtkIconFactory *gdl_stock_factory = NULL;

static struct {
	const gchar *stock_id;
	const gchar *filename;
}
gdl_icons[] = 
{
	{ GDL_STOCK_CLOSE, "stock-close-12.png" },
	{ GDL_STOCK_MENU_LEFT, "stock-menu-left-12.png" },
	{ GDL_STOCK_MENU_RIGHT, "stock-menu-right-12.png" }
};

static void
icon_set_from_file (GtkIconSet       *set,
		    const gchar      *filename,
		    GtkIconSize       size,
		    gboolean          fallback)
{
	GtkIconSource *source;
	GdkPixbuf     *pixbuf;
	gchar         *path;

	source = gtk_icon_source_new ();

	gtk_icon_source_set_size (source, size);
	gtk_icon_source_set_size_wildcarded (source, FALSE);
	
	path = g_build_filename (GDL_IMAGESDIR, filename, NULL);
	pixbuf = gdk_pixbuf_new_from_file (path, NULL);
	if (!pixbuf) {
	    g_warning ("Unable to load stock icon %s", path);
	    g_object_unref (source);
	    g_free (path);
	    return;
	}
	
	g_free (path);

	gtk_icon_source_set_pixbuf (source, pixbuf);
	
	g_object_unref (pixbuf);
	
	gtk_icon_set_add_source (set, source);
	
	if (fallback) {
		gtk_icon_source_set_size_wildcarded (source, TRUE);
		gtk_icon_set_add_source (set, source);
	}
	
	gtk_icon_source_free (source);
}

static void
add_icon (GtkIconFactory *factory,
	  const gchar    *stock_id,
	  const gchar    *filename)
{
	GtkIconSet *set;
	gboolean    fallback = FALSE;

	set = gtk_icon_factory_lookup (factory, stock_id);

	if (!set) {
		set = gtk_icon_set_new ();
		gtk_icon_factory_add (factory, stock_id, set);
		gtk_icon_set_unref (set);

		fallback = TRUE;
	}
	
	icon_set_from_file (set, filename, GTK_ICON_SIZE_MENU, fallback);
}

void
gdl_stock_init (void)
{
	static gboolean initialized = FALSE;
	gint i;

	if (initialized)
		return;

	gdl_stock_factory = gtk_icon_factory_new ();

	for (i = 0; i < G_N_ELEMENTS (gdl_icons); i++) {
		add_icon (gdl_stock_factory,
			  gdl_icons[i].stock_id,
			  gdl_icons[i].filename);
	}

	gtk_icon_factory_add_default (gdl_stock_factory);

	initialized = TRUE;
}
