#include <gtk/gtk.h>

#include "gdl-tools.h"

#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-notebook.h"


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


GtkWidget *item1, *item2, *item3, *item4, *item5;


int
main (int argc, char **argv)
{
	GtkWidget *win, *dock, *box;
	int i;

	gtk_init (&argc, &argv);

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect (GTK_OBJECT (win), "delete_event", 
			    gtk_main_quit, NULL);

	gtk_window_set_title (GTK_WINDOW (win), "Test!");
	gtk_window_set_default_size (GTK_WINDOW (win), 400, 400);

	box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (win), box);

	gtk_container_add (GTK_CONTAINER (box), gtk_button_new_with_label ("Test"));

	/* create the dock */
	dock = gdl_dock_new ();
	gtk_container_add (GTK_CONTAINER (box), dock);
	gtk_container_set_border_width (GTK_CONTAINER (dock), 10);

	/* create the dock items */
  	item1 = gdl_dock_item_new ("item1", GDL_DOCK_ITEM_BEH_LOCKED);
  	gtk_container_add (GTK_CONTAINER (item1), create_text_item ());
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item1), 
			   GDL_DOCK_TOP);
	gtk_widget_show (item1);

	item2 = gdl_dock_item_new ("item2", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_object_set (GTK_OBJECT (item2), 
			"resize", FALSE, 
			"shrink", FALSE, 
			NULL);
	gtk_container_add (GTK_CONTAINER (item2), create_item ("Button 2"));
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item2), 
			   GDL_DOCK_RIGHT);
	gtk_widget_show (item2);

#if 1
	item3 = gdl_dock_item_new ("item3", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item3), create_item ("Button 3"));
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item3), 
			   GDL_DOCK_BOTTOM);
	gtk_widget_show (item3);
#endif

#if 1
	item4 = gdl_dock_notebook_new ();
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item4),
			   GDL_DOCK_BOTTOM);
	for (i = 0; i < 3; i++) {
		GtkWidget *item;

		item = gdl_dock_item_new ("Item", GDL_DOCK_ITEM_BEH_NORMAL);
		gtk_container_add (GTK_CONTAINER (item), create_text_item ());
		gtk_widget_show (item);
		gtk_container_add (GTK_CONTAINER (item4), item);
	};
	gtk_widget_show (item4);

#endif

#if 0
	item5 = gdl_dock_item_new ("item5", GDL_DOCK_ITEM_BEH_NORMAL);
	{
		GtkWidget *nb;

		nb = gtk_notebook_new ();
		gtk_container_add (GTK_CONTAINER (item5), nb);

		for (i = 0; i < 3; i++) {
			GtkWidget *item;

			item = gdl_dock_item_new ("Item", 
						  GDL_DOCK_ITEM_BEH_NORMAL);
			gtk_container_add (GTK_CONTAINER (item), 
					   create_text_item ());
			gtk_widget_show (item);
			gtk_container_add (GTK_CONTAINER (nb), item);
		};

	};
	gdl_dock_add_item (GDL_DOCK (dock), GDL_DOCK_ITEM (item5), 
			   GDL_DOCK_BOTTOM);
	gtk_widget_show (item5);
#endif
	
	/* tests: manually dock and move around some of the items */
	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item3), GDL_DOCK_ITEM (item1),
			       GDL_DOCK_TOP);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), NULL, GDL_DOCK_FLOATING);

	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), 
			       GDL_DOCK_RIGHT);

	/* this one causes a gdl_dock_paned_reorder */
	gdl_dock_item_dock_to (GDL_DOCK_ITEM (item2), GDL_DOCK_ITEM (item3), 
			       GDL_DOCK_LEFT);

	gtk_widget_show_all (win);

	gtk_main ();

	return 0;
}
