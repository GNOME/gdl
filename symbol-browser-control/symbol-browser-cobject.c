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
#include "symbol-browser-cobject.h"
#include <gdl/gdl.h>

static BonoboObjectClass *parent_class;
static POA_GNOME_Development_SymbolBrowser__epv epv;
static POA_GNOME_Development_SymbolBrowser__vepv vepv;

/* Forward declarations */
static void
gnome_symbol_browser_cobject_class_init(GnomeSymbolBrowserCObject* cobj_class);

static void
gnome_symbol_browser_cobject_init (GnomeSymbolBrowserCObject* cobj);

/* Public functions */
GtkType
gnome_symbol_browser_cobject_get_type (void)
{
	static GtkType type = 0;
	if (!type)
	{
		GtkTypeInfo info = {
			"GnomeSymbolBrowserCObject",
			sizeof (GnomeSymbolBrowserCObject),
			sizeof (GnomeSymbolBrowserCObjectClass),
			(GtkClassInitFunc) gnome_symbol_browser_cobject_class_init,
			(GtkObjectInitFunc) gnome_symbol_browser_cobject_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type(), &info);
	}
	return type;
}

GnomeSymbolBrowserCObject*
gnome_symbol_browser_cobject_new (GnomeSymbolBrowser *symbol_browser)
{
	POA_GNOME_Development_SymbolBrowser *servant;
	GNOME_Development_SymbolBrowser objref;
	GnomeSymbolBrowserCObject* symbol_browser_cobject;
	CORBA_Environment ev;
 
	g_return_val_if_fail (symbol_browser != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SYMBOL_BROWSER(symbol_browser), NULL);
	
	symbol_browser_cobject =
		gtk_type_new (gnome_symbol_browser_cobject_get_type());
	
	servant = (POA_GNOME_Development_SymbolBrowser*) g_new0(BonoboObjectServant, 1);
	servant->vepv = &vepv;

	CORBA_exception_init (&ev);
	POA_GNOME_Development_SymbolBrowser__init((PortableServer_Servant) servant, &ev);
	
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_free (servant);
		CORBA_exception_free (&ev);
		gtk_object_destroy (GTK_OBJECT (symbol_browser_cobject));
		return NULL;
	}
	
	CORBA_exception_free (&ev);
	objref = bonobo_object_activate_servant (
			BONOBO_OBJECT(symbol_browser_cobject), servant);

	gtk_object_ref (GTK_OBJECT (symbol_browser));
	symbol_browser_cobject->symbol_browser = GTK_WIDGET(symbol_browser);
	
	bonobo_object_construct (BONOBO_OBJECT (symbol_browser_cobject), objref);
	
	return symbol_browser_cobject;
};

/* Private Functions */
static void
gnome_symbol_browser_cobject_init (GnomeSymbolBrowserCObject* cobj)
{
	/* Doesn't seem to have anything to do right now */
}

static void
cobject_destroy (GtkObject* obj)
{
	GnomeSymbolBrowserCObject* cobj;
	
	g_return_if_fail (obj != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER_COBJECT(obj));
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT(obj);
	gtk_object_unref (GTK_OBJECT(cobj->symbol_browser));
}

static void
impl_open_directory(PortableServer_Servant servant,
		const CORBA_char *dir, CORBA_Environment* en)
{
	GnomeSymbolBrowserCObject *cobj;
	printf ("In function impl_open_directory()\n");
	cobj = GNOME_SYMBOL_BROWSER_COBJECT(bonobo_object_from_servant (servant));
	gnome_symbol_browser_open_dir(GNOME_SYMBOL_BROWSER(cobj->symbol_browser), (gchar*) dir);
}

static void
impl_clear(PortableServer_Servant servant,
		CORBA_Environment* en)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT(bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_clear(GNOME_SYMBOL_BROWSER(cobj->symbol_browser)); */
}

static void
impl_update(PortableServer_Servant servant,
		CORBA_Environment* en)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT(bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_update(GNOME_SYMBOL_BROWSER(cobj->symbol_browser)); */
}

static void
impl_update_file(PortableServer_Servant servant,
		const CORBA_char* filename, CORBA_Environment* en)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT(bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_update_file(GNOME_SYMBOL_BROWSER(cobj->symbol_browser),
			(gchar*)filename); */
}

static void
impl_remove_file(PortableServer_Servant servant,
		const CORBA_char* filename, CORBA_Environment* en)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT(bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_remove_file(GNOME_SYMBOL_BROWSER(cobj->symbol_browser),
		(gchar*)filename); */
}

static void
impl_save(PortableServer_Servant servant,
		CORBA_Environment* en)
{
	GnomeSymbolBrowserCObject *cobj;
	
	cobj = GNOME_SYMBOL_BROWSER_COBJECT(bonobo_object_from_servant (servant));
	/* gnome_symbol_browser_save(GNOME_SYMBOL_BROWSER(cobj->symbol_browser)); */
}

static void
gnome_symbol_browser_cobject_class_init(GnomeSymbolBrowserCObject* cobj_class)
{
	GtkObjectClass *object_class = (GtkObjectClass*) cobj_class;
	parent_class = gtk_type_class (bonobo_object_get_type ());
    
	object_class->destroy = cobject_destroy;
    	
	/* EPV initialization */
	epv.openDirectory = impl_open_directory;
	epv.clear = impl_clear;
	epv.update = impl_update;
	epv.save = impl_save;
	
	epv.updateFile = impl_update_file;
	epv.removeFile = impl_remove_file;

	/* VEPV initialization */
	vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	vepv.GNOME_Development_SymbolBrowser_epv = &epv;
}
