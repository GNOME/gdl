#include <config.h>
#include <bonobo.h>
#include <gnome.h>
#include <bonobo-activation/bonobo-activation.h>
#include <gdl/gdl.h>

enum _FileSelectionOperation {
	OP_NONE,
	OP_SAVE,
	OP_LOAD,
	OP_SET_FILE
};

typedef enum _FileSelectionOperation FileSelectionOperation;

struct _FileSelectionInfo {
	BonoboWidget *control;
	GtkWidget *widget;

	FileSelectionOperation operation;
};
typedef struct _FileSelectionInfo FileSelectionInfo;

static FileSelectionInfo file_selection_info = {
	NULL,
	NULL,
	OP_NONE
};

static gint
load_file (gchar* fname)
{
	CORBA_Environment ev;
	CORBA_Object interface;
	GNOME_Development_SymbolBrowser symbol_browser;
	gchar* dir;
	gchar *interface_name = "IDL:GNOME/Development/SymbolBrowser:1.0";
	BonoboControlFrame *frame;
	Bonobo_Control control;

	CORBA_exception_init (&ev);

	frame = bonobo_widget_get_control_frame (file_selection_info.control);
	control = bonobo_control_frame_get_control (frame);
	
	interface = Bonobo_Unknown_queryInterface (control,
						   interface_name,
						   &ev);

	if (BONOBO_EX (&ev)) {
		g_warning ("The Control does not seem to support `%s'.", interface_name);
		return FALSE;
	}

	symbol_browser = interface;

	dir = g_path_get_dirname (fname);

	g_print ("dirname = %s\n", dir);

	GNOME_Development_SymbolBrowser_openDirectory (symbol_browser,
						       dir,
						       &ev);
	g_free (dir);

	g_assert (!BONOBO_EX (&ev));

	CORBA_exception_free (&ev);

	return FALSE;
}

static gint
set_file (gchar* fname)
{
	CORBA_Environment ev;
	CORBA_Object interface;
	GNOME_Development_SymbolBrowser symbol_browser;
	gchar* dir;
	gchar *interface_name = "IDL:GNOME/Development/SymbolBrowser:1.0";
	BonoboControlFrame *frame;
	Bonobo_Control control;

	CORBA_exception_init (&ev);

	frame = bonobo_widget_get_control_frame (file_selection_info.control);
	control = bonobo_control_frame_get_control (frame);
	
	interface = Bonobo_Unknown_queryInterface (control,
						   interface_name,
						   &ev);

	if (BONOBO_EX (&ev)) {
		g_warning ("The Control does not seem to support `%s'.", interface_name);
		return FALSE;
	}

	symbol_browser = interface;

	g_print ("set file = %s\n", fname);

	GNOME_Development_SymbolBrowser_setFile (symbol_browser,
						       fname,
						       &ev);
	g_assert (!BONOBO_EX (&ev));

	CORBA_exception_free (&ev);

	return FALSE;
}

static void
file_selection_destroy_cb (GtkWidget *widget,
			   gpointer data)
{
	file_selection_info.widget = NULL;
}

static void
file_selection_cancel_cb (GtkWidget *widget,
		      gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void
file_selection_ok_cb (GtkWidget *widget,
		      gpointer data)
{
	const gchar *fname;

	fname = gtk_file_selection_get_filename
		(GTK_FILE_SELECTION (file_selection_info.widget));

	switch (file_selection_info.operation) {
	case OP_LOAD:
		load_file (fname);
		break;
	case OP_SET_FILE:
		set_file (fname);
		break;
	case OP_SAVE:
		// save_file (fname);
		break;
	default:
		g_assert_not_reached ();
	}
	
	gtk_widget_destroy (file_selection_info.widget);
}


static void
open_or_save_dialog (BonoboWindow          *app,
		     FileSelectionOperation op)
{
	GtkWidget    *widget;
	BonoboWidget *control;

	control = BONOBO_WIDGET (bonobo_window_get_contents (app));

	if (file_selection_info.widget != NULL) {
		gdk_window_show (GTK_WIDGET (file_selection_info.widget)->window);
		return;
	}

	if (op == OP_LOAD)
		widget = gtk_file_selection_new (_("Open file..."));
	else if (op == OP_SET_FILE)
		widget = gtk_file_selection_new (_("Set current file..."));
	else
		widget = gtk_file_selection_new (_("Save file..."));
		
	gtk_window_set_transient_for (GTK_WINDOW (widget),
				      GTK_WINDOW (app));

	file_selection_info.widget = widget;
	file_selection_info.control = control;
	file_selection_info.operation = op;

	g_signal_connect_object (G_OBJECT (GTK_FILE_SELECTION (widget)->cancel_button),
			  "clicked", G_CALLBACK (file_selection_cancel_cb),
			  widget, G_CONNECT_AFTER);

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
			  "clicked", G_CALLBACK (file_selection_ok_cb),
			  NULL);

	g_signal_connect (G_OBJECT (file_selection_info.widget), "destroy",
			  G_CALLBACK (file_selection_destroy_cb),
			  NULL);

	gtk_widget_show (file_selection_info.widget);
}

/* Open file dialog.  */
static void
open_file_cb (GtkWidget *widget,
	      gpointer   data)
{
	open_or_save_dialog (BONOBO_WINDOW (data), OP_LOAD);
}

/* Open file dialog.  */
static void
set_file_cb (GtkWidget *widget,
	      gpointer   data)
{
	open_or_save_dialog (BONOBO_WINDOW (data), OP_SET_FILE);
}

/* Save file.  */
static void
save_file_cb (GtkWidget *widget,
	      gpointer   data)
{
	open_or_save_dialog (BONOBO_WINDOW (data), OP_SAVE);
}

static void
exit_cb (GtkWidget *widget,
	 gpointer   data)
{
	bonobo_main_quit ();
}

static void
event_cb (BonoboListener    *listener, 
	  const char        *event,
	  CORBA_any         *any, 
	  CORBA_Environment *ev,
	  gpointer           user_data)
{
	GtkWidget *dialog;
	GtkWidget *window = user_data;

	if (!strcmp (event, "go-to")) {
		gchar *location = BONOBO_ARG_GET_STRING (any);

		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_CLOSE,
						 "You selected the file: \"%s\"",
						 location);
	}

	/* Close dialog on user response */
	g_signal_connect (G_OBJECT (dialog),
			  "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (dialog);
}


static BonoboUIVerb verbs [] = {
	BONOBO_UI_UNSAFE_VERB ("FileOpen", open_file_cb),
	BONOBO_UI_UNSAFE_VERB ("FileSetFile", set_file_cb),
	BONOBO_UI_UNSAFE_VERB ("FileSave", save_file_cb),
	BONOBO_UI_UNSAFE_VERB ("FileExit", exit_cb),

	BONOBO_UI_VERB_END
};

int
main (int argc, char *argv[])
{
	CORBA_Environment ev;
	BonoboWindow *win;
	GtkWidget *wid;
	BonoboUIContainer *container;
	BonoboUIComponent *component;
	BonoboControlFrame *control_frame;
	Bonobo_Control control;
	CORBA_Object source;
	BonoboListener *listener;

	CORBA_exception_init (&ev);

	gnome_program_init ("test-symbol-browser", VERSION, LIBGNOMEUI_MODULE,
			    argc, argv, GNOME_PARAM_POPT_TABLE,
			    bonobo_activation_popt_options, NULL);

	if (!bonobo_init (&argc, argv))
		g_error (_("Can't initialize bonobo!"));

	win = BONOBO_WINDOW (bonobo_window_new ("Gnome symbol browser",
						"Gnome Symbol Browser Test"));

	g_signal_connect (win, "destroy", bonobo_main_quit, NULL);
	gtk_window_set_default_size (GTK_WINDOW (win), 350, 600);
	
	container = bonobo_window_get_ui_container (win);

	component = bonobo_ui_component_new_default ();

	bonobo_ui_component_set_container (component, BONOBO_OBJREF (container), NULL);

	bonobo_ui_util_set_ui (component,
			       "",
			       SYMBOL_BROWSER_SRCDIR "test-symbol-browser-ui.xml",
			       "test",
			       &ev);

	bonobo_ui_component_add_verb_list_with_data (component, verbs, win);

	wid = bonobo_widget_new_control ("OAFIID:GNOME_Development_SymbolBrowser_Control",
					 BONOBO_OBJREF (container));

	control_frame = bonobo_widget_get_control_frame (wid);
	control = bonobo_control_frame_get_control (control_frame);

	source = Bonobo_Unknown_queryInterface (control,
						"IDL:Bonobo/EventSource:1.0",
						&ev);
						
	listener = bonobo_listener_new (NULL, NULL);
	g_signal_connect (listener, "event_notify", G_CALLBACK (event_cb), win);

	if (!BONOBO_EX (&ev)) {
		Bonobo_EventSource_addListener (source,
						bonobo_object_corba_objref (BONOBO_OBJECT (listener)),
						&ev);
	} else
		g_error ("Couldn't get event source for widget");
	
	Bonobo_Unknown_unref(control, &ev);
	
	g_object_set_data (G_OBJECT (win), "SymbolBrowser", wid);
	bonobo_window_set_contents (BONOBO_WINDOW (win), wid);
	gtk_widget_show_all (GTK_WIDGET (win));

	CORBA_exception_free (&ev);

	bonobo_main ();

	return 0;
}
