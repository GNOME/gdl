#include <gtk/gtk.h>
#include <gnome-xml/tree.h>

#include "gdl-tools.h"

#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-notebook.h"


GtkWidget *edit, *options, *dock;
xmlDocPtr doc;

/* creates a simple widget with a list inside */
static GtkWidget *
create_item (const gchar *button_title)
{
	GtkWidget *vbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *viewport1;
	GtkWidget *list1;
	GtkWidget *button1;
	GList *l = NULL;
	int i;

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

	viewport1 = gtk_viewport_new (NULL, NULL);
	gtk_widget_show (viewport1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);

	for (i = 1; i < 5; i++) {
		gchar *s;
		s = g_strdup_printf ("List item %d", i);
		l = g_list_append (l, gtk_list_item_new_with_label (s));
		g_free (s);
	};
	list1 = gtk_list_new ();
	gtk_list_append_items (GTK_LIST (list1), l);
	gtk_widget_show (list1);
	gtk_container_add (GTK_CONTAINER (viewport1), list1);

	button1 = gtk_button_new_with_label (button_title);
	gtk_widget_show (button1);
	gtk_box_pack_start (GTK_BOX (vbox1), button1, FALSE, FALSE, 0);

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

	text = gtk_text_new (NULL,
			     gtk_scrolled_window_get_vadjustment 
			     (GTK_SCROLLED_WINDOW (scrolledwindow1)));
	gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, 
			 "This is a text widget", -1);
	gtk_text_set_word_wrap (GTK_TEXT (text), TRUE);
	gtk_text_set_editable (GTK_TEXT (text), TRUE);
	gtk_widget_show (text);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), text);

	return vbox1;
}

static void
options_select_cb (GtkWidget *options, xmlNodePtr layout)
{
        /* Load layout. */
        gdl_dock_layout_load (GDL_DOCK (dock), layout);
}

static void
button_save_cb (GtkWidget *button, gpointer data)
{
        gchar *name;
        xmlNodePtr layout;
        GtkWidget *menu, *menuitem;
        
        /* Check if a name has been entered. */
        name = gtk_entry_get_text (GTK_ENTRY (edit));
        if (!name || strlen (name) == 0) {
                g_warning ("please enter layout name before saving");
                return;
        }

        /* Save layout. */
        layout = xmlNewChild (doc->root, NULL, "layout", NULL);
        xmlSetProp (layout, "name", name);
        gdl_dock_layout_save (GDL_DOCK (dock), layout);

        /* Add item to options. */
        menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (options));
        menuitem = gtk_menu_item_new_with_label (name);
        gtk_signal_connect (GTK_OBJECT (menuitem), 
                            "activate", 
                            GTK_SIGNAL_FUNC (options_select_cb), 
                            layout);
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_widget_show (menuitem);
        gtk_option_menu_set_history (GTK_OPTION_MENU (options), 
                g_list_length (gtk_container_children (GTK_CONTAINER (menu))) - 1);
}

static void
button_dump_cb (GtkWidget *button, gpointer data)
{
        /* Dump XML tree. */
        xmlDocDump (stdout, doc);
}

static GtkWidget *
create_layout_frame ()
{
        GtkWidget *box, *box2, *button, *optionmenu;

        box = gtk_vbox_new (FALSE, 5);
        box2 = gtk_hbox_new (FALSE, 5);
        gtk_container_add (GTK_CONTAINER (box), box2);

        edit = gtk_entry_new ();
        gtk_container_add (GTK_CONTAINER (box2), edit);

        button = gtk_button_new_with_label (_("Save layout"));
        gtk_container_add (GTK_CONTAINER (box2), button);                            
        gtk_signal_connect (GTK_OBJECT (button), 
                            "clicked", 
                            GTK_SIGNAL_FUNC (button_save_cb), 
                            NULL);

        box2 = gtk_hbox_new (FALSE, 5);
        gtk_container_add (GTK_CONTAINER (box), box2);
        
        button = gtk_button_new_with_label (_("Dump XML"));
        gtk_container_add (GTK_CONTAINER (box2), button);
        gtk_signal_connect (GTK_OBJECT (button),
                            "clicked",
                            GTK_SIGNAL_FUNC (button_dump_cb),
                            NULL);

        options = gtk_option_menu_new ();
        gtk_container_add (GTK_CONTAINER (box2), options);
        optionmenu = gtk_menu_new ();
        gtk_option_menu_set_menu (GTK_OPTION_MENU (options), optionmenu);

        return box;
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
        GtkWidget *win, *table, *menuitem;
	int i;
        xmlNodePtr default_layout;

	gtk_init (&argc, &argv);

	/* window creation */
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect (GTK_OBJECT (win), "delete_event", 
			    gtk_main_quit, NULL);
	gtk_window_set_title (GTK_WINDOW (win), "Test!");
	gtk_window_set_default_size (GTK_WINDOW (win), 400, 400);

	/* table */
        table = gtk_table_new (3, 1, FALSE);
        gtk_container_add (GTK_CONTAINER (win), table);
	gtk_container_set_border_width (GTK_CONTAINER (table), 10);

	/* layout managment frame */
        gtk_table_attach (GTK_TABLE (table),
                          create_layout_frame (),
                          0, 1,
                          0, 1,
                          GTK_FILL,
                          GTK_FILL,
                          0,
                          5);

	/* create the dock */
	dock = gdl_dock_new ();
	gtk_signal_connect (GTK_OBJECT (dock), "layout_changed",
			    layout_changed_cb, NULL);

	gtk_table_attach (GTK_TABLE (table), 
	                  dock,
	                  0, 1,
	                  1, 2, 
	                  GTK_FILL | GTK_EXPAND | GTK_SHRINK,
	                  GTK_FILL | GTK_EXPAND | GTK_SHRINK,
	                  0,
	                  0);
  
	/* create the dock items */
        item1 = gdl_dock_item_new ("item1", "Item #1", GDL_DOCK_ITEM_BEH_LOCKED);
        gtk_container_add (GTK_CONTAINER (item1), create_text_item ());
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item1), 
			   GDL_DOCK_TOP);
	gtk_widget_show (item1);

	item2 = gdl_dock_item_new ("item2", "Item #2", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_object_set (GTK_OBJECT (item2), 
			"resize", FALSE, 
			"shrink", FALSE, 
			NULL);
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
		GtkWidget *item;

		snprintf (name, sizeof (name), "Item #%d", i + 4);
		item = gdl_dock_item_new (name, name, GDL_DOCK_ITEM_BEH_NORMAL);
		gtk_container_add (GTK_CONTAINER (item), create_text_item ());
		gtk_widget_show (item);
		gtk_container_add (GTK_CONTAINER (item4), item);
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

	gtk_widget_show_all (win);

        /* Create XML tree. */
        doc = xmlNewDoc ("1.0");
        doc->root = xmlNewDocNode (doc, NULL, "dock-layout", NULL);
        
        /* Set default layout. */
        default_layout = xmlNewChild (doc->root, NULL, "layout", NULL);
        xmlSetProp (default_layout, "name", "Default");
        gdl_dock_layout_save (GDL_DOCK (dock), default_layout);

        /* Create menuitem for default layout. */
        menuitem = gtk_menu_item_new_with_label (_("Default"));
        gtk_signal_connect (GTK_OBJECT (menuitem), 
                            "activate", 
                            GTK_SIGNAL_FUNC (options_select_cb), 
                            default_layout);
        gtk_widget_show (menuitem);
        gtk_menu_append (GTK_MENU (
                gtk_option_menu_get_menu (GTK_OPTION_MENU (options))), menuitem);
        gtk_option_menu_set_history (GTK_OPTION_MENU (options), 0);
        
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

	return 0;
}
