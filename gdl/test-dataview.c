#include <config.h>
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>

#include "gdl-data-view.h"
#include "gdl-data-model-test.h"

int
main (int argc, char *argv[])
{
	GtkWidget *win;
	GtkWidget *view;
	GtkWidget *scrolled;
	GdlDataModel *model;
	int cx, cy;

	gtk_init (&argc, &argv);

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (win), 500, 200);

	view = gdl_data_view_new ();

	gtk_widget_grab_focus (GTK_WIDGET (view));

	model = GDL_DATA_MODEL (gdl_data_model_test_new ());
	gdl_data_view_set_model (GDL_DATA_VIEW (view), 
				 model);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled), view);	
	

	gtk_container_add (GTK_CONTAINER (win), scrolled);


	gtk_widget_show_all (win);

	gtk_main ();
}
