/*
 * gdl-file-selector-util.h - functions for getting files from a
 * selector
 *
 * Authors:
 *    Jacob Berkman  <jacob@ximian.com>
 *
 * Copyright 2001 Ximian, Inc.
 *
 */

#include <config.h>

#include "gdl-file-selector-util.h"

#include <bonobo/bonobo-event-source.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-widget.h>

#include <gtk/gtkmain.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtksignal.h>

#include <libgnome/libgnome.h>
#include <libgnome/gnome-i18n.h>

#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-window-icon.h>

#define GET_MODE(w) (GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (w), "GdlFileSelectorMode")))
#define SET_MODE(w, m) (gtk_object_set_data (GTK_OBJECT (w), "GdlFileSelectorMode", GINT_TO_POINTER (m)))

typedef enum {
	FILESEL_OPEN,
	FILESEL_OPEN_MULTI,
	FILESEL_SAVE
} FileselMode;

static gint
delete_file_selector (GtkWidget *d, GdkEventAny *e, gpointer data)
{
	gtk_widget_hide (d);
	gtk_main_quit ();
	return TRUE;
}

static void
listener_cb (BonoboListener *listener, 
	     gchar *event_name,
	     CORBA_any *any,
	     CORBA_Environment *ev,
	     gpointer data)
{
	GtkWidget *dialog;
	CORBA_sequence_CORBA_string *seq;
	char *subtype;

	dialog = data;
	gtk_widget_hide (dialog);

	subtype = bonobo_event_subtype (event_name);
	if (!strcmp (subtype, "Cancel"))
		goto cancel_clicked;

	seq = any->_value;
	if (seq->_length < 1)
		goto cancel_clicked;

	if (GET_MODE (dialog) == FILESEL_OPEN_MULTI) {
		char **strv;
		int i;

		if (seq->_length == 0)
			goto cancel_clicked;

		strv = g_new (char *, seq->_length + 1);
		for (i = 0; i < seq->_length; i++)
			strv[i] = g_strdup (seq->_buffer[i]);
		strv[i] = NULL;
		gtk_object_set_user_data (GTK_OBJECT (dialog), strv);
	} else {
		gtk_object_set_user_data (GTK_OBJECT (dialog),
					  g_strdup (seq->_buffer[0]));
	}

 cancel_clicked:
	g_free (subtype);
	gtk_main_quit ();
}

static BonoboWidget *
create_control (gboolean enable_vfs, FileselMode mode)
{
	GtkWidget *control;
	char *moniker;

	moniker = g_strdup_printf (
		"OAFIID:GNOME_FileSelector_Control!"
		"Application=%s;"
		"EnableVFS=%d;"
		"MultipleSelection=%d;"
		"SaveMode=%d",
		gnome_app_id,
		enable_vfs,
		mode == FILESEL_OPEN_MULTI, 
		mode == FILESEL_SAVE);

	control = bonobo_widget_new_control (moniker, CORBA_OBJECT_NIL);
	g_free (moniker);

	return control ? BONOBO_WIDGET (control) : NULL;
}

static GtkWindow *
create_bonobo_selector (gboolean    enable_vfs,
			FileselMode mode, 
			const char *mime_types,
			const char *default_path, 
			const char *default_filename)

{
	GtkWidget    *dialog;
	BonoboWidget *control;

	control = create_control (enable_vfs, mode);
	if (!control)
		return NULL;

	dialog = gtk_window_new (GTK_WINDOW_DIALOG);
	gtk_container_add (GTK_CONTAINER (dialog), GTK_WIDGET (control));
	gtk_widget_set_usize (dialog, 560, 450);

	bonobo_event_source_client_add_listener (
		bonobo_widget_get_objref (control), 
		listener_cb, 
		"GNOME/FileSelector/Control:ButtonClicked",
		NULL, dialog);

	if (mime_types)
		bonobo_widget_set_property (
			control, "MimeTypes", mime_types, NULL);

	if (default_path)
		bonobo_widget_set_property (
			control, "DefaultLocation", default_path, NULL);

	if (default_filename)
		bonobo_widget_set_property (
			control, "DefaultFileName", default_filename, NULL);
	
	return GTK_WINDOW (dialog);
}

static void
ok_clicked_cb (GtkWidget *widget, gpointer data)
{
	GtkFileSelection *fsel;
	gchar *file_name;

	fsel = data;

	file_name = gtk_file_selection_get_filename (fsel);

	if (!strlen (file_name))
		return;
	
	/* Change into directory if that's what user selected */
	if (g_file_test (file_name, G_FILE_TEST_ISDIR)) {
		gint name_len;
		gchar *dir_name;

		name_len = strlen (file_name);
		if (name_len < 1 || file_name [name_len - 1] != '/') {
			/* The file selector needs a '/' at the end of a directory name */
			dir_name = g_strconcat (file_name, "/", NULL);
		} else {
			dir_name = g_strdup (file_name);
		}
		gtk_file_selection_set_filename (fsel, dir_name);
		g_free (dir_name);
	} else if (GET_MODE (fsel) == FILESEL_OPEN_MULTI) {
		GtkCList *clist;
		GList  *row;
		char **strv, *filedirname, *temp;
		int i, j, rows, rownum;

		gtk_widget_hide (GTK_WIDGET (fsel));
		
		clist = GTK_CLIST (fsel->file_list);
		rows = g_list_length (clist->selection);
		strv = g_new (char *, rows + 2);
		strv[0] = g_strdup (file_name);

		/* i *heart* gtkfilesel's api. 
		 *
		 * we iterate twice since setting "" as the file name
		 * (to get the directory) clears the selection. this
		 * is based on some stuff from the gimp.
		 *
		 */

		for (rownum = 0, i = 1, row = clist->row_list; row; row = g_list_next (row), rownum++) {
			if (GTK_CLIST_ROW (row)->state != GTK_STATE_SELECTED)
				continue;

			if (gtk_clist_get_cell_type (clist, rownum, 0) != GTK_CELL_TEXT)
				continue;

			gtk_clist_get_text (clist, rownum, 0, &strv[i++]);
		}

		strv[i] = NULL;

		gtk_file_selection_set_filename (fsel, "");
		filedirname = gtk_file_selection_get_filename (fsel);

		for (i = j = 1; strv[i]; i++) {
			temp = g_concat_dir_and_file (filedirname, strv[i]);

			/* avoid duplicates */
			if (strcmp (temp, strv[0]))
				strv[j++] = temp;
			else
				g_free (temp);
		}

		strv[j] = NULL;

		gtk_object_set_user_data (GTK_OBJECT (fsel), strv);
		gtk_main_quit ();
	} else {
		gtk_widget_hide (GTK_WIDGET (fsel));

		gtk_object_set_user_data (GTK_OBJECT (fsel),
					  g_strdup (file_name));
		gtk_main_quit ();
	}
}

static void
cancel_clicked_cb (GtkWidget *widget, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (data));
	gtk_main_quit ();
}


static GtkWindow *
create_gtk_selector (FileselMode mode,
		     const char *default_path,
		     const char *default_filename)
{
	GtkWidget *filesel;

	filesel = gtk_file_selection_new (NULL);

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (ok_clicked_cb),
			    filesel);

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			    "clicked", GTK_SIGNAL_FUNC (cancel_clicked_cb),
			    filesel);

	if (default_path) {
		char *tmp;

		tmp = g_strconcat (default_path, 
				   default_path[strlen (default_path) - 1] == '/'
				   ? NULL : "/", NULL);

		gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), tmp);
		g_free (tmp);
	}

	if (mode == FILESEL_OPEN_MULTI) {
		gtk_clist_set_selection_mode (GTK_CLIST (GTK_FILE_SELECTION (filesel)->file_list),
					      GTK_SELECTION_MULTIPLE);
	}

	return GTK_WINDOW (filesel);
}

/* FIXME: break up into smaller functions */
static gpointer
run_file_slector (GtkWindow  *parent,
		  gboolean    enable_vfs,
		  FileselMode mode, 
		  const char *title,
		  const char *mime_types,
		  const char *default_path, 
		  const char *default_filename)

{
	GtkWindow *dialog = NULL;
	gpointer   retval;

	if (!getenv ("GNOME_FILESEL_DISABLE_BONOBO"))
		dialog = create_bonobo_selector (enable_vfs, mode, mime_types, 
						 default_path, default_filename);
	if (!dialog)
		dialog = create_gtk_selector (mode, default_path, default_filename);

	SET_MODE (dialog, mode);

	gnome_window_icon_set_from_default (dialog);
	gtk_window_set_title (dialog, title);
	gtk_window_set_modal (dialog, TRUE);

	if (parent)
		gtk_window_set_transient_for (dialog, parent);
	
	gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			    GTK_SIGNAL_FUNC (delete_file_selector),
			    dialog);

	gtk_widget_show_all (GTK_WIDGET (dialog));

	gtk_main ();

	retval = gtk_object_get_user_data (GTK_OBJECT (dialog));

	gtk_widget_destroy (GTK_WIDGET (dialog));

	return retval;
}

/**
 * gdl_file_selector_open:
 * @parent: optional window the dialog should be a transient for.
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 *
 * Creates and shows a modal open file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: the URI (or plain file path if @enable_vfs is FALSE)
 * of the file selected, or NULL if cancel was pressed.
 **/
char *
gdl_file_selector_open (GtkWindow  *parent,
			  gboolean    enable_vfs,
			  const char *title,
			  const char *mime_types,
			  const char *default_path)
{
	return run_file_slector (parent, enable_vfs, FILESEL_OPEN, 
				 title ? title : _("Select a file to open"),
				 mime_types, default_path, NULL);
}

/**
 * gdl_file_selector_open_multi:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 *
 * Creates and shows a modal open file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: a NULL terminated string array of the selected URIs
 * (or local file paths if @enable_vfs is FALSE), or NULL if cancel
 * was pressed.
 **/
char **
gdl_file_selector_open_multi (GtkWindow  *parent,
				gboolean    enable_vfs,
				const char *title,
				const char *mime_types,
				const char *default_path)
{
	return run_file_slector (parent, enable_vfs, FILESEL_OPEN_MULTI,
				 title ? title : _("Select files to open"),
				 mime_types, default_path, NULL);
}

/**
 * gdl_file_selector_save:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 * @default_filename: optional file name to default to
 *
 * Creates and shows a modal save file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: the URI (or plain file path if @enable_vfs is FALSE)
 * of the file selected, or NULL if cancel was pressed.
 **/
char *
gdl_file_selector_save (GtkWindow  *parent,
			  gboolean    enable_vfs,
			  const char *title,
			  const char *mime_types,
			  const char *default_path, 
			  const char *default_filename)
{
	return run_file_slector (parent, enable_vfs, FILESEL_SAVE,
				 title ? title : _("Select a filename to save"),
				 mime_types, default_path, default_filename);
}
