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
#include "scintilla-editor-buffer.h"

#include <gdl/gdl-server-manager.h>

enum {
    PROP_POSITION,
    PROP_LINE_NUM,
    PROP_SELECTION_START,
    PROP_SELECTION_END
};

static void init_scintilla_control_factory (void);
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

static void notify_cb (ScintillaObject *sci, int wparam, void *lparam, 
                       gpointer user_data);

static void cut_cb (GtkWidget *widget, ScintillaObject *sci);
static void copy_cb (GtkWidget *widget, ScintillaObject *sci);
static void paste_cb (GtkWidget *widget, ScintillaObject *sci);

BonoboUIVerb verbs[] = {
    BONOBO_UI_UNSAFE_VERB ("EditCut", cut_cb),
    BONOBO_UI_UNSAFE_VERB ("EditCopy", copy_cb),
    BONOBO_UI_UNSAFE_VERB ("EditPaste", paste_cb),
    BONOBO_UI_VERB_END
};

void
init_scintilla_control_factory (void)
{
    static BonoboGenericFactory *fact = NULL;
    
    if (fact != NULL)
	return;
    
    fact = bonobo_generic_factory_new ("OAFIID:control-factory:scintilla:851a1008-ca21-498d-8188-30a7543b137b", scintilla_factory, NULL);

    if (fact == NULL) 
	g_error (_("Could not initialize factory"));
}

BonoboObject *
scintilla_factory (BonoboGenericFactory *fact, void *closure)
{
    GtkWidget *sci;
    BonoboControl *control;
    BonoboPropertyBag *pb;
    BonoboPersistFile *file_impl;
    BonoboPersistStream *stream_impl;
    ScintillaEditorBuffer *buffer_impl;
    
    sci = scintilla_new ();

    gtk_widget_show_all (GTK_WIDGET (sci));
    
    control = bonobo_control_new (GTK_WIDGET (sci));

    gtk_signal_connect (GTK_OBJECT (control), "activate",
			GTK_SIGNAL_FUNC (scintilla_activate_cb), sci);

    /* Create the properties */
    pb = bonobo_property_bag_new (get_prop, set_prop, sci);

    bonobo_property_bag_add (pb, "position", PROP_POSITION,
			     BONOBO_ARG_LONG, NULL,
			     "Position in the buffer", 
			     BONOBO_PROPERTY_UNSTORED);
    bonobo_property_bag_add (pb, "line_num", PROP_LINE_NUM,
			     BONOBO_ARG_LONG, NULL,
			     "Current line number", 
			     BONOBO_PROPERTY_UNSTORED);
    bonobo_property_bag_add (pb, "selection_start", PROP_SELECTION_START,
			     BONOBO_ARG_LONG, NULL,
			     "Beginning of the selection", 
			     BONOBO_PROPERTY_UNSTORED);
    bonobo_property_bag_add (pb, "selection_end", PROP_SELECTION_END,
			     BONOBO_ARG_LONG, NULL,
			     "End of the selection", 
			     BONOBO_PROPERTY_UNSTORED);

    bonobo_control_set_properties (control, pb);
			  
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

    gtk_signal_connect (GTK_OBJECT (sci), "notify",
                        GTK_SIGNAL_FUNC (notify_cb), NULL);

    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINWIDTHN, 1, 30);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINTYPEN, 1, SC_MARGIN_NUMBER);
    scintilla_send_message (SCINTILLA(sci), SCI_SETFOLDFLAGS, 16, 0);
    
    scintilla_send_message (SCINTILLA(sci), SCI_SETPROPERTY, (long)"fold", (long)"1");
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINWIDTHN, 2, 25);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
    scintilla_send_message (SCINTILLA (sci), SCI_SETMODEVENTMASK, SC_MOD_CHANGEFOLD, 0);
    scintilla_send_message (SCINTILLA(sci), SCI_SETMARGINSENSITIVEN, 2, 1);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPEN, LONG_MAX);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPEN, 0);
    scintilla_send_message (SCINTILLA(sci), SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_PLUS);
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
    } else {
#if 0
	bonobo_ui_component_rm (ui_component, "/", NULL);
#endif
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
        BONOBO_ARG_SET_LONG (arg, pos);
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

int 
main (int argc, char *argv[])
{
    CORBA_Environment ev;
    
    CORBA_exception_init (&ev);

    gnome_init_with_popt_table ("scintilla test", VERSION, argc, argv, 
				oaf_popt_options, 0, NULL);

    oaf_init (argc, argv);
    if (!bonobo_init (oaf_orb_get (), NULL, NULL))
	g_error (_("Can't initialize bonobo!"));

    bonobo_activate ();

    init_scintilla_control_factory ();
    
    bonobo_main ();

    return 0;
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
            g_print ("fold changed\n");
            
            fold_changed (sci,
                          notification->line, 
                          notification->foldLevelNow,
                          notification->foldLevelPrev);
        }
        break;
    }
}

