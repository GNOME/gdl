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

#include "scintilla/ScintillaWidget.h"
#include "scintilla/Scintilla.h"

#include "scintilla-persist-file.h"
#include "scintilla-editor-buffer.h"

#include <gdl/gdl-server-manager.h>

enum {
    PROP_POSITION,
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
		      gpointer user_data);
static void get_prop (BonoboPropertyBag *bag, 
		      BonoboArg *arg,
		      guint arg_id,
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

    scintilla_send_message (sci, SCI_SETMARGINWIDTHN, 1, 30);
    scintilla_send_message (sci, SCI_SETMARGINTYPEN, 1, SC_MARGIN_NUMBER);

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
