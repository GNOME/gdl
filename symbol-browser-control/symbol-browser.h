/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* symbol-browser widget
 *
 * Copyright (C) 2001 Naba Kumar <kh_naba@yahoo.com>
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
 */

#ifndef _GNOME_SYMBOL_BROWSER_H_
#define _GNOME_SYMBOL_BROWSER_H_

#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define GNOME_TYPE_SYMBOL_BROWSER			(gnome_symbol_browser_get_type ())
#define GNOME_SYMBOL_BROWSER(obj)			(GTK_CHECK_CAST ((obj), GNOME_TYPE_SYMBOL_BROWSER, GnomeSymbolBrowser))
#define GNOME_SYMBOL_BROWSER_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SYMBOL_BROWSER, GnomeSymbolBrowserClass))
#define GNOME_IS_SYMBOL_BROWSER(obj)		(GTK_CHECK_TYPE ((obj), GNOME_TYPE_SYMBOL_BROWSER))
#define GNOME_IS_SYMBOL_BROWSER_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), GNOME_TYPE_SYMBOL_BROWSER))

typedef struct _GnomeSymbolBrowser        GnomeSymbolBrowser;
typedef struct _GnomeSymbolBrowserPriv GnomeSymbolBrowserPriv;
typedef struct _GnomeSymbolBrowserClass   GnomeSymbolBrowserClass;

struct _GnomeSymbolBrowser {
	GtkVBox parent;

	GnomeSymbolBrowserPriv *priv;
};

struct _GnomeSymbolBrowserClass {
	GtkVBoxClass parent_class;
	
	void (*go_to) (GnomeSymbolBrowser* symbol_browser,
			gchar* file,
			glong line);
};

GtkType
gnome_symbol_browser_get_type (void);

GtkWidget*
gnome_symbol_browser_new (void);

gboolean
gnome_symbol_browser_open_dir(GnomeSymbolBrowser* sb, const gchar* dir);

void
gnome_symbol_browser_clear(GnomeSymbolBrowser* gsb);

void
gnome_symbol_browser_reset(GnomeSymbolBrowser* gsb);

void
gnome_symbol_browser_destroy (GtkObject *obj);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GNOME_SYMBOL_BROWSER_H_ */
