/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * This file is part of the GNOME Devtool Libraries.
 * 
 * Copyright (C) 2000 Dave Camp <dave@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 */

#include <config.h>
#include <gnome.h>
#include <bonobo.h>
#include <gtk/gtk.h>
#include <liboaf/liboaf.h>
#include <limits.h>

#include "scintilla/Scintilla.h"
#include "scintilla/ScintillaWidget.h"

#include "scintilla-persist-file.h"
#include "scintilla-persist-stream.h"
#include "scintilla-editor-buffer.h"
#include "scintilla-editor-gutter.h"
#include "scintilla-find.h"

#include <gdl/gdl-server-manager.h>

enum {
    PROP_POSITION,
    PROP_LINE_NUM,
    PROP_SELECTION_START,
    PROP_SELECTION_END
};

static BonoboObject *scintilla_factory (BonoboGenericFactory *fact, 
                                        void *closure);
static void scintilla_activate_cb (BonoboControl *control, 
				   gboolean activate,
				   ScintillaObject *sci);
static void set_prop (BonoboPropertyBag *bag, 
		      const BonoboArg *arg,
		      guint arg_id,
                      CORBA_Environment *ev,
		      gpointer user_data);
static void get_prop (BonoboPropertyBag *bag, 
		      BonoboArg *arg,
		      guint arg_id,
                      CORBA_Environment *ev,
		      gpointer user_data);

static void scintilla_update_statusbar (ScintillaObject *sci);

static void notify_cb (ScintillaObject *sci, int wparam, void *lparam, 
                       gpointer user_data);
static void destroy_cb (ScintillaObject *sci, gpointer data);

static void cut_cb (GtkWidget *widget, ScintillaObject *sci);
static void copy_cb (GtkWidget *widget, ScintillaObject *sci);
static void paste_cb (GtkWidget *widget, ScintillaObject *sci);
static void find_cb (GtkWidget *widget, ScintillaObject *sci);
static void find_again_cb (GtkWidget *widget, ScintillaObject *sci);
static void replace_cb (GtkWidget *widget, ScintillaObject *sci);

BonoboUIVerb verbs[] = {
    BONOBO_UI_UNSAFE_VERB ("EditCut", cut_cb),
    BONOBO_UI_UNSAFE_VERB ("EditCopy", copy_cb),
    BONOBO_UI_UNSAFE_VERB ("EditPaste", paste_cb),
    BONOBO_UI_UNSAFE_VERB ("EditFind", find_cb),
    BONOBO_UI_UNSAFE_VERB ("EditFindAgain", find_again_cb),
    BONOBO_UI_UNSAFE_VERB ("EditReplace", replace_cb),
    BONOBO_UI_VERB_END
};


BonoboObject *
scintilla_factory (BonoboGenericFactory *fact, void *closure)
{
    GtkWidget *sci;
    BonoboControl *control;
    BonoboPropertyBag *pb;
    BonoboPersistFile *file_impl;
    BonoboPersistStream *stream_impl;
    ScintillaEditorBuffer *buffer_impl;
    ScintillaEditorGutter *gutter_impl;
    
    sci = scintilla_new ();

    gtk_widget_show_all (GTK_WIDGET (sci));
    
    control = bonobo_control_new (GTK_WIDGET (sci));

    gtk_signal_connect (GTK_OBJECT (control), "activate",
			GTK_SIGNAL_FUNC (scintilla_activate_cb), sci);

    /* Create the properties */
    pb = bonobo_property_bag_new (get_prop, set_prop, sci);

    bonobo_property_bag_add (pb, "position", PROP_POSITION,
			     BONOBO_ARG_LONG, NULL,
			     _("Position in the buffer"), 
			     BONOBO_PROPERTY_UNSTORED);
    bonobo_property_bag_add (pb, "line_num", PROP_LINE_NUM,
			     BONOBO_ARG_LONG, NULL,
			     _("Current line number"), 
			     BONOBO_PROPERTY_UNSTORED);
    bonobo_property_bag_add (pb, "selection_start", PROP_SELECTION_START,
			     BONOBO_ARG_LONG, NULL,
			     _("Beginning of the selection"), 
			     BONOBO_PROPERTY_UNSTORED);
    bonobo_property_bag_add (pb, "selection_end", PROP_SELECTION_END,
			     BONOBO_ARG_LONG, NULL,
			     _("End of the selection"), 
			     BONOBO_PROPERTY_UNSTORED);

    bonobo_control_set_properties (control, pb);
    bonobo_object_unref (BONOBO_OBJECT (pb));
			  
    /* Add other interfaces */
    file_impl = scintilla_persist_file_new (sci);
    bonobo_object_add_interface (BONOBO_OBJECT (control), 
				 BONOBO_OBJECT (file_impl));

    stream_impl = scintilla_persist_stream_new (sci);
    bonobo_object_add_interface (BONOBO_OBJECT (control),
                                 BONOBO_OBJECT (stream_impl));

    buffer_impl = scintilla_editor_buffer_new (SCINTILLA (sci));
    bonobo_object_add_interface (BONOBO_OBJECT (control),
				 BONOBO_OBJECT (buffer_impl));

    gutter_impl = scintilla_editor_gutter_new (SCINTILLA (sci));
    bonobo_object_add_interface (BONOBO_OBJECT (control),
                                 BONOBO_OBJECT (gutter_impl));
    
    gtk_signal_connect (GTK_OBJECT (sci), "notify",
                        GTK_SIGNAL_FUNC (notify_cb), NULL);

    gtk_signal_connect (GTK_OBJECT (sci), "destroy",
                        GTK_SIGNAL_FUNC (destroy_cb), NULL);

    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINWIDTHN, 1, 30);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINTYPEN, 1, SC_MARGIN_NUMBER);
    scintilla_send_message (SCINTILLA(sci), SCI_SETFOLDFLAGS, 16, 0);
    
    scintilla_send_message (SCINTILLA(sci), SCI_SETPROPERTY, (long)"fold", (long)"1");
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINWIDTHN, 2, 25);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
    scintilla_send_message (SCINTILLA (sci), SCI_SETMODEVENTMASK, SC_MOD_CHANGEFOLD, 0);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINSENSITIVEN, 2, 1);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_ARROWDOWN);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPEN, LONG_MAX);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPEN, 0);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_ARROW);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERSETFORE, SC_MARKNUM_FOLDER, LONG_MAX);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERSETBACK, SC_MARKNUM_FOLDER, 0);
    scintilla_send_message (SCINTILLA(sci), SCI_SETINDENTATIONGUIDES, 1, 0);
    
    return BONOBO_OBJECT (control);
}

static void
scintilla_activate_cb (BonoboControl *control, 
		       gboolean activate, 
		       ScintillaObject *sci)
{
    BonoboUIComponent *ui_component;
    GtkWidget *fixed, *lines, *colon, *cols, *frame;
    GtkRequisition req;
    int pos;
    BonoboControl *status;
    
    ui_component = bonobo_control_get_ui_component (control);

    if (activate) {
	Bonobo_UIContainer remote_uic;

	remote_uic = bonobo_control_get_remote_ui_container (control);
	bonobo_ui_component_set_container (ui_component, remote_uic);

        bonobo_ui_component_freeze (ui_component, NULL);

	/* Hook up the user interface */
	bonobo_ui_component_add_verb_list_with_data (ui_component, verbs, sci);
	
	bonobo_object_release_unref (remote_uic, NULL);
	bonobo_ui_util_set_ui (ui_component, GDL_DATADIR,
			       "scintilla-ui.xml", "scintilla-control");
        bonobo_ui_component_thaw (ui_component, NULL);
	
	/* Sadly, we need to recreate the statusbar widget everytime 
	 * scintilla activates: the widget can't be reparented atm */
	
	/* Create widgets */
	lines = gtk_label_new ("WWW");
	colon = gtk_label_new (":");
	cols = gtk_label_new ("WWW");
	fixed = gtk_fixed_new ();
	
	/* Add widgets and set size */
	gtk_misc_set_alignment (GTK_MISC (lines), 1.0, 0.5);
	gtk_fixed_put (GTK_FIXED (fixed), lines, 0, 0);	
	gtk_widget_size_request (lines, &req);
	gtk_fixed_put (GTK_FIXED (fixed), colon, req.width, 0);
	pos = req.width;
	gtk_widget_size_request (colon, &req);
	pos += req.width;
	gtk_fixed_put (GTK_FIXED (fixed), cols, pos, 0);
	gtk_widget_size_request (cols, &req);	
	gtk_widget_set_usize (fixed, pos + req.width, req.height);

	/* Create frame for labels */	
	frame = gtk_frame_new (NULL);
	status = bonobo_control_new (frame);

	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), fixed);
	gtk_widget_show_all (frame);

	/* Store lines, colon and cols labels as data (needed for updating) */		
	gtk_object_set_data (GTK_OBJECT (sci), "lines", lines);
	gtk_object_set_data (GTK_OBJECT (sci), "colon", colon);
	gtk_object_set_data (GTK_OBJECT (sci), "cols", cols);
	
	/* Add control to statusbar */
	bonobo_ui_component_object_set (ui_component, "/status/EditorStats",
					BONOBO_OBJREF (status),
					NULL);
	
	scintilla_update_statusbar (sci);
    } else {
#if 0
	bonobo_ui_component_rm (ui_component, "/", NULL);
#endif
	gtk_object_set_data (GTK_OBJECT (sci), "lines", NULL);
	gtk_object_set_data (GTK_OBJECT (sci), "colon", NULL);
	gtk_object_set_data (GTK_OBJECT (sci), "cols", NULL);
	
	bonobo_ui_component_unset_container (ui_component);
    }
}

/* 
 * Properties 
 */

void 
set_prop (BonoboPropertyBag *bag,
	  const BonoboArg *arg,
	  guint arg_id,
          CORBA_Environment *ev,
	  gpointer user_data)
{
    ScintillaObject *sci = user_data;

    switch (arg_id) {
    case PROP_POSITION :
    {
	long pos = BONOBO_ARG_GET_LONG (arg);
	scintilla_send_message (sci, SCI_GOTOPOS, pos, 0);
	break;
    } 
    case PROP_LINE_NUM :
    {
        long line = BONOBO_ARG_GET_LONG (arg);
	/* scintilla line numbers start at 0 */
	line--;
        scintilla_send_message (sci, SCI_GOTOLINE, line, 0);
        scintilla_send_message (sci, SCI_MOVECARETINSIDEVIEW, 0, 0);
        break;
    }
    case PROP_SELECTION_START :
    {
	long pos = BONOBO_ARG_GET_LONG (arg);
	scintilla_send_message (sci, SCI_SETSELECTIONSTART, pos, 0);
	break;
    } 
    case PROP_SELECTION_END :
    {
	long pos = BONOBO_ARG_GET_LONG (arg);
	scintilla_send_message (sci, SCI_SETSELECTIONEND, pos, 0);
	break;
    }
    }    
}

void 
get_prop (BonoboPropertyBag *bag,
	  BonoboArg *arg,
	  guint arg_id,
          CORBA_Environment *ev,
	  gpointer user_data)
{
    ScintillaObject *sci = user_data;

    switch (arg_id) {
    case PROP_POSITION :
    {
	long pos = scintilla_send_message (sci, SCI_GETCURRENTPOS, 0, 0);
	BONOBO_ARG_SET_LONG (arg, pos);
	break;
    }
    case PROP_LINE_NUM :
    {
        long pos = scintilla_send_message (sci, SCI_GETCURRENTPOS, 0, 0);
        long line = scintilla_send_message (sci, SCI_LINEFROMPOSITION, pos, 0);
	/* scintilla line numbers start at 0 */
	line++;
        BONOBO_ARG_SET_LONG (arg, line);
        break;
    }
    case PROP_SELECTION_START :
    {
	long pos = scintilla_send_message (sci, SCI_GETSELECTIONSTART, 0, 0);
	BONOBO_ARG_SET_LONG (arg, pos);
	break;
    }
    case PROP_SELECTION_END :
    {
	long pos = scintilla_send_message (sci, SCI_GETSELECTIONEND, 0, 0);
	BONOBO_ARG_SET_LONG (arg, pos);
	break;
    }
    }
}

static void
scintilla_update_statusbar (ScintillaObject *sci)
{
	GtkWidget *lines, *colon, *cols;
	GtkRequisition req;
	int caret_pos, line_pos, col_pos;
	gchar text[10];

	lines = gtk_object_get_data (GTK_OBJECT (sci), "lines");
	colon = gtk_object_get_data (GTK_OBJECT (sci), "colon");
	cols = gtk_object_get_data (GTK_OBJECT (sci), "cols");
	
	if (lines) {
		/* Get data */
		caret_pos = scintilla_send_message (sci, SCI_GETCURRENTPOS, 0, 0);
		line_pos = scintilla_send_message (sci, SCI_LINEFROMPOSITION, caret_pos, 0);
		col_pos = scintilla_send_message (sci, SCI_GETCOLUMN, caret_pos, 0);
		
		/* Set label text */
		snprintf (text, 10, "%i", line_pos + 1);
		gtk_label_set_text (GTK_LABEL (lines), text);
		snprintf (text, 10, "%i", col_pos);
		gtk_label_set_text (GTK_LABEL (cols), text);
		
		/* Reposition lines label */
		gtk_widget_size_request (lines, &req);
		gtk_widget_set_uposition (lines, colon->allocation.x - req.width, 
					  colon->allocation.y);
	}	
}

/*
 * Verb Implementations 
 */

void 
cut_cb (GtkWidget *widget, ScintillaObject *sci)
{
    scintilla_send_message (sci, SCI_CUT, 0, 0);
}

void 
copy_cb (GtkWidget *widget, ScintillaObject *sci)
{
    scintilla_send_message (sci, SCI_COPY, 0, 0);
}

void 
paste_cb (GtkWidget *widget, ScintillaObject *sci)
{
    scintilla_send_message (sci, SCI_PASTE, 0, 0);
}

void
find_cb (GtkWidget *widget, ScintillaObject *sci)
{
    run_find_dialog (sci);
}

void
find_again_cb (GtkWidget *widget, ScintillaObject *sci)
{
    find_again (sci);
}

void
replace_cb (GtkWidget *widget, ScintillaObject *sci)
{
    run_replace_dialog (sci);
}

static void
destroy_cb (ScintillaObject *sci, gpointer data)
{
    ScintillaFindDialog *fd;
    ScintillaReplaceDialog *rd;

    fd = gtk_object_get_data (GTK_OBJECT (sci), "find_dialog");
    if (fd)
        scintilla_find_dialog_destroy (fd);

    rd = gtk_object_get_data (GTK_OBJECT (sci), "replace_dialog");
    if (rd)
        scintilla_replace_dialog_destroy (rd);
}


/* Most of what is below here is taken from SciTE */

static void 
expand (ScintillaObject *sci, 
        int *line, gboolean do_expand, gboolean force, int vis_levels,
        int level)
{
    int level_line;
    int line_max_subord = scintilla_send_message (sci, SCI_GETLASTCHILD, 
                                                  *line, level);
    (*line)++;
    while (*line <= line_max_subord) {
        if (force) {
            if (vis_levels > 0) 
                scintilla_send_message (sci, SCI_SHOWLINES, *line, *line);
            else 
                scintilla_send_message (sci, SCI_HIDELINES, *line, *line);
        } else {
            if (do_expand)
                scintilla_send_message (sci, SCI_SHOWLINES, *line, *line);
        }
        level_line = level;
        if (level_line == -1) 
            level_line = scintilla_send_message (sci, SCI_GETFOLDLEVEL, *line, 0);
        if (level_line & SC_FOLDLEVELHEADERFLAG) {
            if (force) {
                if (vis_levels > 1) 
                    scintilla_send_message (sci, SCI_SETFOLDEXPANDED, 
                                            *line, 1);
                else
                    scintilla_send_message (sci, SCI_SETFOLDEXPANDED, *line, 0);
                expand (sci, line, do_expand, force, vis_levels - 1, -1);
            } else {
                if (do_expand && scintilla_send_message (sci, SCI_GETFOLDEXPANDED, *line, 0)) {
                    expand (sci, line, TRUE, force, vis_levels - 1, -1);
                } else {
                    expand (sci, line, FALSE, force, vis_levels - 1, -1);
                }
            }
        } else {
            (*line)++;
        }
    }            
}

static void
fold_changed (ScintillaObject *sci, int line, int level_now, int level_prev)
{
    if (level_now & SC_FOLDLEVELHEADERFLAG) {
        scintilla_send_message (sci, SCI_SETFOLDEXPANDED, line, 1);
    } else if (level_prev & SC_FOLDLEVELHEADERFLAG) {
        if (!scintilla_send_message (sci, SCI_GETFOLDEXPANDED, line, 0)) {
            expand (sci, &line, TRUE, FALSE, 0, level_prev);
        }
    }
}

static void 
margin_click (ScintillaObject *sci, int position, int modifiers) 
{
    int line_click = scintilla_send_message (sci, SCI_LINEFROMPOSITION,
                                             position, 0);
    if (scintilla_send_message (sci, SCI_GETFOLDLEVEL, line_click, 0) & SC_FOLDLEVELHEADERFLAG) {
        if (modifiers & SCMOD_SHIFT) {
            scintilla_send_message (sci, SCI_SETFOLDEXPANDED, line_click, 1);
            expand (sci, &line_click, TRUE, TRUE, 100, -1);
        } else if (modifiers & SCMOD_CTRL) {
            if (scintilla_send_message (sci, SCI_GETFOLDEXPANDED, 
                                        line_click, 0)) {
                scintilla_send_message (sci, SCI_SETFOLDEXPANDED, 
                                        line_click, 0);
                expand (sci, &line_click, FALSE, TRUE, 0, -1);
            } else {
                scintilla_send_message (sci, SCI_SETFOLDEXPANDED, 
                                        line_click, 1);
                expand (sci, &line_click, TRUE, TRUE, 100, -1);
            }
        } else {
            scintilla_send_message (sci, SCI_TOGGLEFOLD, line_click, 0);
        }
        
    }           
}

static void
notify_cb (ScintillaObject *sci, int wparam, void *lparam, 
           gpointer user_data)
{
    struct SCNotification *notification = lparam;
    int id = notification->nmhdr.code;

    switch (id) {
    case SCN_MARGINCLICK :
        margin_click (sci, notification->position, notification->modifiers);
        break;
    case SCN_MODIFIED :
        if (notification->modificationType == SC_MOD_CHANGEFOLD) {
            fold_changed (sci,
                          notification->line, 
                          notification->foldLevelNow,
                          notification->foldLevelPrev);
        }
        break;
    case SCN_UPDATEUI :
    	scintilla_update_statusbar (sci);
        break;
    }
}

BONOBO_OAF_FACTORY ("OAFIID:Bonobo_Control_ScintillaFactory", 
                    "Scintilla Factory", VERSION, scintilla_factory, NULL);
