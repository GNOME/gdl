#include <string.h>
#include <gtk/gtk.h>

#include "gdl-tools.h"

#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-notebook.h"
#include "gdl-dock-layout.h"


/* creates a simple widget with a list inside */
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
					GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

	text = gtk_text_view_new ();
	gtk_widget_show (text);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), text);

	return vbox1;
}

static void
button_dump_cb (GtkWidget *button, gpointer data)
{
        /* Dump XML tree. */
        gdl_dock_layout_save_to_file (GDL_DOCK_LAYOUT (data), "layout.xml");
}

static void
run_layout_manager_cb (GtkWidget *w, gpointer data)
{
	GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);
	gdl_dock_layout_run_manager (layout);
}

static void
layout_changed_cb (GtkWidget *widget, gpointer data)
{
	g_message ("Dock emitted layout_changed signal");
}
  
int
main (int argc, char **argv)
{
	GtkWidget *item1, *item2, *item3, *item4;
	GtkWidget *items [3];
        GtkWidget *win, *table, *button, *box;
	int i;
	GdlDockLayout *layout;
	GtkWidget *dock;

	gtk_init (&argc, &argv);

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
	g_signal_connect (dock, "layout_changed",
			  G_CALLBACK (layout_changed_cb), NULL);

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
	g_object_set (item2, "resize", FALSE, "shrink", FALSE, NULL);
	gtk_container_add (GTK_CONTAINER (item2), create_item ("Button 2"));
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item2), 
			   GDL_DOCK_RIGHT);
	gtk_widget_show (item2);

	item3 = gdl_dock_item_new ("item3", "Item #3", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item3), create_item ("Button 3"));
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item3), 
			   GDL_DOCK_BOTTOM);
	gtk_widget_show (item3);

	item4 = gdl_dock_notebook_new ();
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item4),
			   GDL_DOCK_BOTTOM);
	for (i = 0; i < 3; i++) {
		gchar name[10];

		snprintf (name, sizeof (name), "Item #%d", i + 4);
		items [i] = gdl_dock_item_new (name, name, GDL_DOCK_ITEM_BEH_NORMAL);
		gtk_container_add (GTK_CONTAINER (items [i]), create_text_item ());
		gtk_widget_show (items [i]);
		gtk_container_add (GTK_CONTAINER (item4), items [i]);
	};
	gtk_widget_show (item4);

	/* tests: manually dock and move around some of the items */
	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item3), GDL_DOCK_ITEM (item1),
			       GDL_DOCK_TOP, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), NULL, 
			       GDL_DOCK_FLOATING, -1);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), 
			       GDL_DOCK_RIGHT, -1);

	/* this one causes a gdl_dock_paned_reorder */
	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), 
			       GDL_DOCK_LEFT, -1);

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

	/* save default layout */
	gdl_dock_layout_save_layout (layout, NULL);

	/* test gdl_dock_get_item_by_name */
	{
		GdlDockItem *i;
		i = gdl_dock_get_item_by_name (GDL_DOCK (dock), "item2");
		if (GTK_WIDGET (i) == item2)
			g_message ("gdl_dock_get_item_by_name succeded");
		else
			g_message ("ERROR: gdl_dock_get_item_by_name failed!");
	};

	gtk_main ();

#if 0      
	gdl_dock_item_hide (GDL_DOCK_ITEM (items [2]));
	gdl_dock_item_hide (GDL_DOCK_ITEM (items [1]));
	gdl_dock_item_hide (GDL_DOCK_ITEM (items [0]));
	gdl_dock_item_hide (GDL_DOCK_ITEM (item3));
	gdl_dock_item_hide (GDL_DOCK_ITEM (item2));
	gdl_dock_item_hide (GDL_DOCK_ITEM (item1));
	gdl_dock_unbind_item (GDL_DOCK (dock), GDL_DOCK_ITEM (items [2]));
	gdl_dock_unbind_item (GDL_DOCK (dock), GDL_DOCK_ITEM (items [1]));
	gdl_dock_unbind_item (GDL_DOCK (dock), GDL_DOCK_ITEM (items [0]));
	gdl_dock_unbind_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item3));
	gdl_dock_unbind_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item2));
	gdl_dock_unbind_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item1));
#endif

  	g_object_unref (layout);
    	gtk_widget_destroy (win);
	
	return 0;
}
