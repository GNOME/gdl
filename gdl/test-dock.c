#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gdl-tools.h"

#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-notebook.h"
#include "gdl-dock-layout.h"
#include "gdl-dock-placeholder.h"

/* ---- this code is based on eel-debug by Darin Adler */

static void
log_handler (const char *domain,
             GLogLevelFlags level,
             const char *message,
             gpointer data)
{
    g_log_default_handler (domain, level, message, data);
    if ((level & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)) != 0) {
        RETSIGTYPE (* saved_handler) (int);
        
        saved_handler = signal (SIGINT, SIG_IGN);
        raise (SIGINT);
        signal (SIGINT, saved_handler);
    }
}

static void
set_log_handler (const char *domain)
{
    g_log_set_handler (domain, G_LOG_LEVEL_MASK, log_handler, NULL);
}

static void
setup_handlers ()
{
    set_log_handler ("");
    set_log_handler ("GLib");
    set_log_handler ("GLib-GObject");
    set_log_handler ("Gtk");
    set_log_handler ("Gdk");
}

/* ---- end of debugging code */


static GtkWidget *
create_item (const gchar *button_title)
{
	GtkWidget *vbox1;
	GtkWidget *button1;

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);

	button1 = gtk_button_new_with_label (button_title);
	gtk_widget_show (button1);
	gtk_box_pack_start (GTK_BOX (vbox1), button1, TRUE, TRUE, 0);

	return vbox1;
}

/* creates a simple widget with a textbox inside */
static GtkWidget *
create_text_item ()
{
	GtkWidget *vbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *text;

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                             GTK_SHADOW_ETCHED_IN);
	text = gtk_text_view_new ();
        g_object_set (text, "wrap-mode", GTK_WRAP_WORD, NULL);
	gtk_widget_show (text);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), text);

	return vbox1;
}

static void
button_dump_cb (GtkWidget *button, gpointer data)
{
        /* Dump XML tree. */
        gdl_dock_layout_save_to_file (GDL_DOCK_LAYOUT (data), "layout.xml");
	g_spawn_command_line_async ("cat layout.xml", NULL);
}

static void
run_layout_manager_cb (GtkWidget *w, gpointer data)
{
	GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);
	gdl_dock_layout_run_manager (layout);
}

int
main (int argc, char **argv)
{
	GtkWidget *item1, *item2, *item3;
	GtkWidget *items [4];
        GtkWidget *win, *table, *button, *box;
	int i;
	GdlDockLayout *layout;
	GtkWidget *dock;

	gtk_init (&argc, &argv);

        setup_handlers ();
        
	/* window creation */
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (win, "delete_event", 
			  G_CALLBACK (gtk_main_quit), NULL);
	gtk_window_set_title (GTK_WINDOW (win), "Docking widget test");
	gtk_window_set_default_size (GTK_WINDOW (win), 400, 400);

	/* table */
        table = gtk_vbox_new (FALSE, 5);
        gtk_container_add (GTK_CONTAINER (win), table);
	gtk_container_set_border_width (GTK_CONTAINER (table), 10);

	/* create the dock */
	dock = gdl_dock_new ();

	/* ... and the layout manager */
	layout = gdl_dock_layout_new (GDL_DOCK (dock));
        
        gtk_box_pack_start (GTK_BOX (table), dock, 
			    TRUE, TRUE, 0);
  
	/* create the dock items */
        item1 = gdl_dock_item_new ("item1", "Item #1", GDL_DOCK_ITEM_BEH_LOCKED);
        gtk_container_add (GTK_CONTAINER (item1), create_text_item ());
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item1), 
			   GDL_DOCK_TOP);
	gtk_widget_show (item1);

	item2 = gdl_dock_item_new ("item2", "Item #2", GDL_DOCK_ITEM_BEH_NORMAL);
	g_object_set (item2, "resize", FALSE, NULL);
	gtk_container_add (GTK_CONTAINER (item2), create_item ("Button 2"));
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item2), 
			   GDL_DOCK_RIGHT);
	gtk_widget_show (item2);

	item3 = gdl_dock_item_new ("item3", "Item #3", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item3), create_item ("Button 3"));
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item3), 
			   GDL_DOCK_BOTTOM);
	gtk_widget_show (item3);

	items [0] = gdl_dock_item_new ("Item #4", "Item #4", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (items [0]), create_text_item ());
	gtk_widget_show (items [0]);
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (items [0]), GDL_DOCK_BOTTOM);
	for (i = 1; i < 3; i++) {
	    gchar name[10];

	    snprintf (name, sizeof (name), "Item #%d", i + 4);
	    items [i] = gdl_dock_item_new (name, name, GDL_DOCK_ITEM_BEH_NORMAL);
	    gtk_container_add (GTK_CONTAINER (items [i]), create_text_item ());
	    gtk_widget_show (items [i]);
	    
	    gdl_dock_object_dock (GDL_DOCK_OBJECT (items [0]),
				  GDL_DOCK_OBJECT (items [i]),
				  GDL_DOCK_CENTER, NULL);
	};

        /* tests: manually dock and move around some of the items */
	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item3), GDL_DOCK_ITEM (item1),
			       GDL_DOCK_TOP, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), 
			       GDL_DOCK_RIGHT, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), 
			       GDL_DOCK_LEFT, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), NULL, 
			       GDL_DOCK_FLOATING, -1);
        
	box = gtk_hbox_new (TRUE, 5);
	gtk_box_pack_end (GTK_BOX (table), box, FALSE, FALSE, 0);

	button = gtk_button_new_with_label ("Layout Manager");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (run_layout_manager_cb), layout);
	gtk_box_pack_end (GTK_BOX (box), button, FALSE, TRUE, 0);
	
	button = gtk_button_new_with_label ("Dump XML");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (button_dump_cb), layout);
	gtk_box_pack_end (GTK_BOX (box), button, FALSE, TRUE, 0);
	
	gtk_widget_show_all (win);

	gdl_dock_placeholder_new ("ph1", GDL_DOCK_OBJECT (dock), GDL_DOCK_TOP, FALSE);
	gdl_dock_placeholder_new ("ph2", GDL_DOCK_OBJECT (dock), GDL_DOCK_BOTTOM, FALSE);
	gdl_dock_placeholder_new ("ph3", GDL_DOCK_OBJECT (dock), GDL_DOCK_LEFT, FALSE);
	gdl_dock_placeholder_new ("ph4", GDL_DOCK_OBJECT (dock), GDL_DOCK_RIGHT, FALSE);
	
	gtk_main ();

  	g_object_unref (layout);
	
	return 0;
}
