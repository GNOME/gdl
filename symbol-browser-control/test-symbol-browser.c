#include <config.h>
#include <bonobo.h>
#include <gnome.h>
#include <liboaf/liboaf.h>
#include <gdl/gdl.h>

enum _FileSelectionOperation {
	OP_NONE,
	OP_SAVE,
	OP_LOAD,
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
	BonoboObjectClient *object_client;
	BonoboWidget* wid;
	GNOME_Development_SymbolBrowser symbol_browser;
	gchar* dir;
	gchar *interface_name = "IDL:GNOME/Development/SymbolBrowser:1.0";


	
	printf ("In function: load_file()\n");
    CORBA_exception_init (&ev);
	
	wid = BONOBO_WIDGET (file_selection_info.control);
	object_client = bonobo_widget_get_server (wid);
	
	interface = bonobo_object_client_query_interface (
			object_client, interface_name, NULL);
	
	if (interface == CORBA_OBJECT_NIL) {
		g_warning ("The Control does not seem to support `%s'.", interface_name);
		return FALSE;
	}
	symbol_browser = interface;
	
	dir = g_dirname(fname);
	
	GNOME_Development_SymbolBrowser_openDirectory(symbol_browser,
		dir, &ev);
	
	g_free (dir);
	
	CORBA_exception_free(&ev);
	return FALSE;
}

static void
file_selection_destroy_cb (GtkWidget *widget,
			   gpointer data)
{
	file_selection_info.widget = NULL;
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
	case OP_SAVE:
		// save_file (fname);
		break;
	default:
		g_assert_not_reached ();
	}
	
	gtk_widget_destroy (file_selection_info.widget);
}


static void
open_or_save_dialog (BonoboWindow *app,
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
	else
		widget = gtk_file_selection_new (_("Save file..."));

	gtk_window_set_transient_for (GTK_WINDOW (widget),
				      GTK_WINDOW (app));

	file_selection_info.widget = widget;
	file_selection_info.control = control;
	file_selection_info.operation = op;

	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (widget)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (widget));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (file_selection_ok_cb),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (file_selection_info.widget), "destroy",
			    GTK_SIGNAL_FUNC (file_selection_destroy_cb),
			    NULL);

	gtk_widget_show (file_selection_info.widget);
}

/* Open file dialog.  */
static void
open_file_cb (GtkWidget *widget,
			      gpointer data)
{
	open_or_save_dialog (BONOBO_WINDOW (data), OP_LOAD);
}

/* Save file.  */
static void
save_file_cb (GtkWidget *widget,
			      gpointer data)
{
	open_or_save_dialog (BONOBO_WINDOW (data), OP_SAVE);
}

static void
exit_cb (GtkWidget *widget,
	 gpointer data)
{
	gtk_main_quit ();
}

static BonoboUIVerb verbs [] = {
	BONOBO_UI_UNSAFE_VERB ("FileOpen", open_file_cb),
	BONOBO_UI_UNSAFE_VERB ("FileSave", save_file_cb),
	BONOBO_UI_UNSAFE_VERB ("FileExit", exit_cb),

	BONOBO_UI_VERB_END
};

int
main (int argc, char *argv[])
{
    CORBA_Environment ev;
    GtkWidget *win;
    GtkWidget *wid;
    BonoboObjectClient *cli;
    BonoboUIContainer *container;
    Bonobo_UIContainer corba_container;
    BonoboUIComponent *component;
    BonoboControlFrame *control_frame;
	
    
    CORBA_exception_init (&ev);

    gnome_init_with_popt_table ("Gnome Symbol Browser Test",
				VERSION, argc, argv, 
				oaf_popt_options, 0, NULL);

    oaf_init (argc, argv);
    if (!bonobo_init (oaf_orb_get (), NULL, NULL))
	g_error (_("Can't initialize bonobo!"));

    bonobo_activate ();
    
    win = bonobo_window_new ("Gnome symbol browser", "Gnome Symbol Browser Test");
    gtk_signal_connect (GTK_OBJECT (win), "destroy", gtk_main_quit, NULL);

    container = bonobo_ui_container_new ();
    bonobo_ui_container_set_win (container, BONOBO_WINDOW (win));
    corba_container = bonobo_object_corba_objref (BONOBO_OBJECT (container));
    component = bonobo_ui_component_new ("test");
    bonobo_ui_component_set_container (component, corba_container);
	bonobo_ui_component_add_verb_list_with_data (component, verbs, win);

    bonobo_ui_util_set_ui (component, "/home/naba/Projects/gdl/symbol-browser-control",
		"test-symbol-browser-ui.xml", "test");

    wid = bonobo_widget_new_control ("OAFIID:GNOME_Development_SymbolBrowser_Control",
			corba_container);
	gtk_object_set_data(GTK_OBJECT(win), "SymbolBrowser", wid);
    bonobo_window_set_contents (BONOBO_WINDOW (win), wid);
    gtk_widget_show_all (GTK_WIDGET (win));

	CORBA_exception_free(&ev);

    bonobo_main ();

    return 0;
}
