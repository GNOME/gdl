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

#include <config.h>

#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>
#include <bonobo/bonobo-shlib-factory.h>
#include <gdl/gdl.h>
#include "symbol-browser.h"
#include "symbol-browser-cobject.h"

/* #define USE_SHARED_LIBRARY_COMPONENT */

enum
{
	ARG_FILE,
	ARG_LINE,
	ARG_CLASS
};

#ifndef USE_SHARED_LIBRARY_COMPONENT

static BonoboGenericFactory *factory;
static gint active_controls = 0;

#endif

static void on_format_update(GtkWidget* w, BonoboControl* control)
{
	/*
	GnomeSymbolBrowser* symbol_browser = GNOME_SYMBOL_BROWSER(
		gtk_object_get_data (GTK_OBJECT(control), "SymbolBrowser"));
	gnome_symbol_browser_update (symbol_browser);
	*/
}

static BonoboUIVerb verbs[] = 
{
	BONOBO_UI_UNSAFE_VERB ("FormatUpdate", on_format_update),
	BONOBO_UI_VERB_END
};

typedef struct {
	gchar* file;
	glong  line;
} ControlData;

static ControlData*
control_data_new (gchar* file, glong line)
{
	ControlData* cd;
	
	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (line > -1, NULL);
	
	cd = g_malloc (sizeof(ControlData));
	cd->file = g_strdup(file);
	cd->line = line;
	
	return cd;
}

static void
control_data_destroy (ControlData* cd)
{
	g_return_if_fail (cd != NULL);
	if (cd->file) g_free (cd->file);
	g_free (cd);
}

static void
get_prop (BonoboPropertyBag * bag, BonoboArg * arg, guint arg_id, 
	  CORBA_Environment *ev, gpointer data)
{
	ControlData* cd;
	GtkObject* obj = data;
	
	cd = gtk_object_get_data (obj, "ControlData");
	
	switch (arg_id) {
		case ARG_FILE:
			if (cd && cd->file) {
				BONOBO_ARG_SET_STRING (arg, cd->file);
			} else {
				BONOBO_ARG_SET_STRING (arg, "");
			}
			break;
		case ARG_LINE:
			if (cd) {
				BONOBO_ARG_SET_LONG (arg, cd->line);
			} else {
				BONOBO_ARG_SET_LONG (arg, -1);
			}
			break;
		default:
			g_warning ("Unknow property queried with the control");
			break;
	}
}

static void
set_prop (BonoboPropertyBag * bag, const BonoboArg * arg, guint arg_id, 
	  CORBA_Environment *ev, gpointer data)
{
	switch (arg_id) {
		case ARG_FILE:
			break;
		case ARG_LINE:
			break;
		default:
			g_warning ("Unknow property set in the control");
	}
}

static void
on_control_set_frame (BonoboControl *control,
	      gpointer data)
{
	Bonobo_UIContainer uic;
	BonoboUIComponent *component;
	
	if (bonobo_control_get_control_frame (control) == CORBA_OBJECT_NIL)
		return;
	
	uic = bonobo_control_get_remote_ui_container (control);
	component = bonobo_control_get_ui_component (control);
	bonobo_ui_component_set_container (component, uic);

	/* FIXME: Merge UI */
	bonobo_ui_component_add_verb_list_with_data (component, verbs, control);

	bonobo_ui_util_set_ui (component, GNOME_DATADIR,
			       "gnome-symbol-browser.xml",
			       "Gnome Symbol Browser");
}

static void
on_control_destroy (GtkObject *control, GnomeSymbolBrowser *symbol_browser)
{

#ifndef USE_SHARED_LIBRARY_COMPONENT

	active_controls --;

	if (active_controls)
		return;

	g_print ("Gnome symbol browser: factory now shutting down\n");
	bonobo_object_unref (BONOBO_OBJECT (factory));
	gtk_main_quit ();
	
#endif
	
	gtk_object_destroy (GTK_OBJECT(symbol_browser));
}

static void
on_symbol_browser_go_to (GnomeSymbolBrowser* symbol_browser,
		gchar* file, glong line, gpointer data)
{
	GtkObject* obj; 
	BonoboPropertyBag* prop_bag;
	BonoboArg *arg;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (GTK_IS_OBJECT(data));
	
	obj = data;
	
	prop_bag = gtk_object_get_data (obj, "PropertyBag");
	
	g_return_if_fail (prop_bag != NULL);
	g_return_if_fail (BONOBO_IS_PROPERTY_BAG(prop_bag));
	
	if (file != NULL && line > -1) {
		gtk_object_set_data_full (obj, "ControlData",
			control_data_new(file, line),
			(GtkDestroyNotify)control_data_destroy);
		g_print ("Go to: %s:%ld\n", file, line);
		
		arg = bonobo_arg_new (BONOBO_ARG_LONG);
		BONOBO_ARG_SET_LONG(arg, line);
		
		/* Notify listeners */
		bonobo_property_bag_notify_listeners(prop_bag, "Line", arg, NULL);
	}
}

/* PropertyBag */
static void
property_bag_init(BonoboControl* control)
{
	BonoboPropertyBag  *pb;
	BonoboArg  *arg;
	GtkWidget* symbol_browser;
	
	GDL_TRACE_EXTRA ("Initializing property bag.");
	
	symbol_browser = gtk_object_get_data(GTK_OBJECT(control), "SymbolBrowser");
	
	pb = bonobo_property_bag_new (get_prop, set_prop, control);
	bonobo_control_set_properties (control, pb);
	gtk_object_set_data (GTK_OBJECT(control), "PropertyBag", pb);
	
	arg = bonobo_arg_new (BONOBO_ARG_STRING);
	BONOBO_ARG_SET_STRING (arg, "");

	bonobo_property_bag_add (pb, "File", ARG_FILE,
				 BONOBO_ARG_STRING, arg,
				 "Current active file", 0);

	CORBA_free (arg);
	
	arg = bonobo_arg_new (BONOBO_ARG_LONG);
	BONOBO_ARG_SET_LONG (arg, -1);

	bonobo_property_bag_add (pb, "Line", ARG_LINE,
				 BONOBO_ARG_LONG, arg,
				 "Current Line number", 0);
	CORBA_free (arg);

	bonobo_object_unref (BONOBO_OBJECT (pb));
}

static BonoboObject *
control_factory (BonoboGenericFactory *factory, gpointer closure)
{
	BonoboControl *control;
	GtkWidget *symbol_browser;
	GnomeSymbolBrowserCObject *cobj;
	
	GDL_TRACE_EXTRA("Creating a control");
	
#ifndef USE_SHARED_LIBRARY_COMPONENT
	
	active_controls++;

#endif
	
	/* We don't have container now. We will set it later.
	 * when the control frame is set
	 */
	symbol_browser = gnome_symbol_browser_new();
	gtk_widget_show (symbol_browser);
	
	control = bonobo_control_new (symbol_browser);

	if (!control){
		GDL_TRACE_EXTRA ("Unable to create Bonobo Control.");
		gtk_widget_unref (symbol_browser);
		return NULL;
	} else {
		cobj = gnome_symbol_browser_cobject_new (
				GNOME_SYMBOL_BROWSER(symbol_browser));
		
		gtk_object_set_data (GTK_OBJECT (control), "SymbolBrowser", symbol_browser);
		gtk_object_set_data (GTK_OBJECT (control), "SymbolBrowserCObject", cobj);
		
		bonobo_object_add_interface (BONOBO_OBJECT (control), 
						 BONOBO_OBJECT (cobj));
		
		/* Initialize property bag */
		property_bag_init(control);
		
		/* UI initialization takes place when the control frame is set */
		gtk_signal_connect (GTK_OBJECT (control), "set_frame",
					GTK_SIGNAL_FUNC (on_control_set_frame), symbol_browser);
	
		gtk_signal_connect (GTK_OBJECT (control), "destroy",
					GTK_SIGNAL_FUNC (on_control_destroy), symbol_browser);
		
		gtk_signal_connect (GTK_OBJECT (symbol_browser), "go_to",
					GTK_SIGNAL_FUNC (on_symbol_browser_go_to), control);
		
		GDL_TRACE_EXTRA ("Bonobo Control created successfully.");
		
		return BONOBO_OBJECT (control);
	}
}

#ifndef USE_SHARED_LIBRARY_COMPONENT

int
main (int argc, char **argv)
{
	CORBA_ORB orb;

	/* Initialize the i18n support */
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain(PACKAGE);
	
	/* intialize gnome */
	gnome_init_with_popt_table ("GNOME_Development_SymbolBrowser_Control",  "0.1",
				     argc, argv, oaf_popt_options, 0, NULL);

	/* initialize CORBA, OAF  and bonobo */
	orb = oaf_init (argc,argv);
	if (!orb)
		g_error ("initializing orb failed");
	if (!bonobo_init (orb, NULL, NULL))
		g_error ("could not initialize Bonobo");

	factory = bonobo_generic_factory_new ("OAFIID:GNOME_Development_SymbolBrowser_ControlFactory",
					      control_factory,
					      NULL);
	if (factory == NULL)
		g_error ("I could not register the Gnome Symbol Brwoser Factory.");

	bonobo_main ();

	return 0;
}

#else

BONOBO_OAF_SHLIB_FACTORY ("OAFIID:GNOME_Development_SymbolBrowser_ControlFactory",
	"Gnome Symbol Browser Factory", control_factory, NULL);

#endif /* USE_SHARED_LIBRARY_COMPONENT */
