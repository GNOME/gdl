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

#include <bonobo.h>
#include <gdl/gdl.h>
#include "symbol-browser-cobject.h"

static BonoboXObjectClass *parent_class;

/* Forward declarations */
static void gnome_symbol_browser_cobject_class_init (GnomeSymbolBrowserCObjectClass *klass);
static void gnome_symbol_browser_cobject_init       (GnomeSymbolBrowserCObject *cobj);
static void gnome_symbol_browser_cobject_destroy    (GtkObject *object);

static void impl_open_directory (PortableServer_Servant servant,
				 const CORBA_char      *dir,
				 CORBA_Environment     *ev);
static void impl_clear		(PortableServer_Servant servant,
				 CORBA_Environment     *ev);
static void impl_update		(PortableServer_Servant servant,
				 CORBA_Environment     *ev);
static void impl_update_file	(PortableServer_Servant servant,
				 const CORBA_char      *filename,
				 CORBA_Environment     *ev);
static void impl_remove_file	(PortableServer_Servant servant,
				 const CORBA_char      *filename,
				 CORBA_Environment     *ev);
static void impl_save		(PortableServer_Servant servant,
				 CORBA_Environment     *ev);


/* Private Functions */
static void
gnome_symbol_browser_cobject_class_init (GnomeSymbolBrowserCObjectClass *klass)
{
	GtkObjectClass *object_class;
	POA_GNOME_Development_SymbolBrowser__epv *epv = &klass->epv;

	object_class = GTK_OBJECT_CLASS (klass);
	parent_class = gtk_type_class (BONOBO_X_OBJECT_TYPE);

	object_class->destroy = gnome_symbol_browser_cobject_destroy;

	/* EPV initialization */
	epv->openDirectory = impl_open_directory;
	epv->clear = impl_clear;
	epv->update = impl_update;
	epv->save = impl_save;
	epv->updateFile = impl_update_file;
	epv->removeFile = impl_remove_file;
}

static void
gnome_symbol_browser_cobject_init (GnomeSymbolBrowserCObject *cobj)
{
}

static void
gnome_symbol_browser_cobject_destroy (GtkObject *object)
{
	GnomeSymbolBrowserCObject *cobj;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER_COBJECT (object));
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT (object);
	g_object_unref (cobj->symbol_browser);
}

static void
impl_open_directory (PortableServer_Servant servant,
		     const CORBA_char      *dir,
		     CORBA_Environment     *ev)
{
	GnomeSymbolBrowserCObject *cobj;

	cobj = GNOME_SYMBOL_BROWSER_COBJECT (bonobo_object_from_servant (servant));
	gnome_symbol_browser_open_dir (GNOME_SYMBOL_BROWSER (cobj->symbol_browser),
				       (gchar*) dir);
}

static void
impl_clear (PortableServer_Servant servant,
	    CORBA_Environment     *ev)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT (bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_clear(GNOME_SYMBOL_BROWSER(cobj->symbol_browser)); */
}

static void
impl_update (PortableServer_Servant servant,
	     CORBA_Environment     *ev)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT (bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_update(GNOME_SYMBOL_BROWSER(cobj->symbol_browser)); */
}

static void
impl_update_file (PortableServer_Servant servant,
		  const CORBA_char      *filename,
		  CORBA_Environment     *ev)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT (bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_update_file(GNOME_SYMBOL_BROWSER(cobj->symbol_browser),
			(gchar*)filename); */
}

static void
impl_remove_file (PortableServer_Servant servant,
		  const CORBA_char      *filename,
		  CORBA_Environment     *ev)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT (bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_remove_file(GNOME_SYMBOL_BROWSER(cobj->symbol_browser),
		(gchar*)filename); */
}

static void
impl_save (PortableServer_Servant servant,
	   CORBA_Environment     *ev)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT (bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_save(GNOME_SYMBOL_BROWSER(cobj->symbol_browser)); */
}

/* Public functions. */
BONOBO_X_TYPE_FUNC_FULL (GnomeSymbolBrowserCObject, 
			 GNOME_Development_SymbolBrowser, 
			 BONOBO_X_OBJECT_TYPE, 
			 gnome_symbol_browser_cobject);

GnomeSymbolBrowserCObject *
gnome_symbol_browser_cobject_new (GnomeSymbolBrowser *symbol_browser)
{
	GnomeSymbolBrowserCObject *cobject;

	g_return_val_if_fail (symbol_browser != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SYMBOL_BROWSER (symbol_browser), NULL);

	cobject = GNOME_SYMBOL_BROWSER_COBJECT (g_object_new (GNOME_SYMBOL_BROWSER_COBJECT_TYPE, NULL));

	g_object_ref (symbol_browser);
	cobject->symbol_browser = GTK_WIDGET (symbol_browser);

	return cobject;
}
