/*
 *  Symbol browser component for GNOME Development Tools
 *
 *  Copyright (c) 2001 Naba Kumar <kh_naba@yahoo.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _SYMBOL_BROWSER_COBJECT_H_
#define _SYMBOL_BROWSER_COBJECT_H_

#include <bonobo/bonobo-object.h>
#include "symbol-browser.h"

G_BEGIN_DECLS

#define GNOME_SYMBOL_BROWSER_COBJECT_TYPE (gnome_symbol_browser_cobject_get_type())

#define GNOME_SYMBOL_BROWSER_COBJECT(k) (GTK_CHECK_CAST((k), \
				GNOME_SYMBOL_BROWSER_COBJECT_TYPE, GnomeSymbolBrowserCObject))

#define GNOME_SYMBOL_BROWSER_COBJECT_CLASS(o)	(GTK_CHECK_CLASS_CAST((o), \
				GNOME_SYMBOL_BROWSER_COBJECT_TYPE, GnomeSymbolBrowserCObjectClass))

#define GNOME_IS_SYMBOL_BROWSER_COBJECT(obj)	(GTK_CHECK_TYPE ((obj), \
				GNOME_SYMBOL_BROWSER_COBJECT_TYPE))

#define GNOME_IS_SYMBOL_BROWSER_COBJECT_CLASS(klass)	(GTK_CHECK_TYPE ((klass), \
				GNOME_SYMBOL_BROWSER_COBJECT_TYPE))

typedef struct _GnomeSymbolBrowserCObject	GnomeSymbolBrowserCObject;
typedef struct _GnomeSymbolBrowserCObjectClass	GnomeSymbolBrowserCObjectClass;

struct _GnomeSymbolBrowserCObject {
	BonoboObject parent;
	GtkWidget *symbol_browser;
};

struct _GnomeSymbolBrowserCObjectClass {
	BonoboObjectClass parent_class;

	POA_GNOME_Development_SymbolBrowser__epv epv;
};

GType
gnome_symbol_browser_cobject_get_type (void);

GnomeSymbolBrowserCObject *
gnome_symbol_browser_cobject_new (GnomeSymbolBrowser *symbol_browser);

G_END_DECLS

#endif /* _SYMBOL_BROWSER_COBJECT_H_ */
