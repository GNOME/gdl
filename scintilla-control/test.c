#include <config.h>
#include <bonobo.h>
#include <gnome.h>
#include <liboaf/liboaf.h>
#include <gdl/GDL.h>
#include "scintilla/Scintilla.h"
#include "scintilla/ScintillaWidget.h"

char *filename;

static void
save_clicked (GtkWidget *widget, void *closure)
{
    CORBA_Environment ev;
    Bonobo_PersistFile *ppersist_file = closure;

    CORBA_exception_init (&ev);

    Bonobo_PersistFile_save (*ppersist_file, filename, &ev);

    CORBA_exception_free (&ev);			     
}

static GNOME_Development_EditorBuffer_iobuf *
allocate_iobuf (char *str)
{
    GNOME_Development_EditorBuffer_iobuf *ret;
    ret = GNOME_Development_EditorBuffer_iobuf__alloc ();
    ret->_length = strlen (str);
    ret->_buffer = CORBA_sequence_CORBA_octet_allocbuf (ret->_length);
    strcpy (ret->_buffer, str);

    return ret;
}

int
main (int argc, char *argv[])
{
    CORBA_Environment ev;
    GtkWidget *win;
    GtkWidget *sci;
    BonoboObjectClient *cli;
    Bonobo_PersistFile file;
    BonoboUIContainer *container;
    Bonobo_UIContainer corba_container;
    BonoboUIComponent *component;
    GNOME_Development_EditorBuffer buffer;
    GNOME_Development_EditorBuffer_iobuf *iobuf;
    GNOME_Development_EditorGutter gutter;
    GtkWidget *hbox;
    GtkWidget *save_btn;
    BonoboControlFrame *control_frame;
    
    if (argc != 2) {
	g_print ("usage: test [filename]\n");
	exit (0);
    }
    
    filename = argv[1];
    
    CORBA_exception_init (&ev);

    gnome_init_with_popt_table ("scintilla test", VERSION, argc, argv, 
				oaf_popt_options, 0, NULL);

    oaf_init (argc, argv);
    if (!bonobo_init (oaf_orb_get (), NULL, NULL))
	g_error (_("Can't initialize bonobo!"));

    bonobo_activate ();
    
    win = bonobo_window_new ("scitest", "Scintilla Component Test");
    gtk_signal_connect (GTK_OBJECT (win), "destroy", gtk_main_quit, NULL);

    container = bonobo_ui_container_new ();
    bonobo_ui_container_set_win (container, BONOBO_WINDOW (win));
    corba_container = bonobo_object_corba_objref (BONOBO_OBJECT (container));
    component = bonobo_ui_component_new ("test");
    bonobo_ui_component_set_container (component, corba_container);

    bonobo_ui_util_set_ui (component, "/home/dave/cvs/scintilla-control", "test-ui.xml", "test");

    sci = bonobo_widget_new_control ("OAFIID:Bonobo_Control_Scintilla", corba_container);
    save_btn = gtk_button_new_with_label ("save");
    gtk_signal_connect (GTK_OBJECT (save_btn), "clicked",
			GTK_SIGNAL_FUNC (save_clicked), &file);

    hbox = gtk_vbox_new (FALSE, 5);

    gtk_box_pack_start (GTK_BOX (hbox), save_btn, FALSE, 0, 0);
    gtk_box_pack_start (GTK_BOX (hbox), sci, TRUE, 0, 0);
    bonobo_window_set_contents (BONOBO_WINDOW (win), hbox);
    gtk_widget_show_all (GTK_WIDGET (win));

    control_frame = bonobo_widget_get_control_frame (BONOBO_WIDGET (sci));
    bonobo_control_frame_set_autoactivate (control_frame, FALSE);
    bonobo_control_frame_control_activate (control_frame);

    cli = bonobo_widget_get_server (BONOBO_WIDGET (sci));
    
    file = bonobo_object_client_query_interface (cli, "IDL:Bonobo/PersistFile:1.0", NULL);
    Bonobo_PersistFile_load (file, filename, &ev);
    
    gutter = bonobo_object_client_query_interface (cli, "IDL:GNOME/Development/EditorGutter:1.0", NULL);
    
    GNOME_Development_EditorGutter_addMarker (gutter, 1, "Breakpoint", &ev);
    GNOME_Development_EditorGutter_addMarker (gutter, 10, "DisabledBreakpoint", &ev);
    GNOME_Development_EditorGutter_addMarker (gutter, 4, "CurrentLine", &ev);
    GNOME_Development_EditorGutter_removeMarker (gutter, 4, "CurrentLine", &ev);
    GNOME_Development_EditorGutter_addMarker (gutter, 6, "CurrentLine", &ev);

    bonobo_main ();

    return 0;
}
