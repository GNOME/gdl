/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include <gdl/GDL.h>
#include "scintilla-editor-buffer.h"
#include "scintilla/Scintilla.h"

#include <gnome.h>
#include <bonobo.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static CORBA_Object create_editor_buffer (BonoboObject *object);
static void scintilla_editor_buffer_destroy (GtkObject *object);
static void scintilla_editor_buffer_class_init (ScintillaEditorBufferClass *class);
static void scintilla_editor_buffer_init (BonoboObject *object);

static BonoboObjectClass *parent_class;
static POA_Development_EditorBuffer__epv editor_buffer_epv;
static POA_Development_EditorBuffer__vepv editor_buffer_vepv;

#define CORBA_boolean__alloc() (CORBA_boolean*) CORBA_octet_allocbuf (sizeof (CORBA_boolean))

static inline ScintillaEditorBuffer*
editor_buffer_from_servant (PortableServer_Servant servant)
{
    return SCINTILLA_EDITOR_BUFFER (bonobo_object_from_servant (servant));
}

/* public routines */

ScintillaEditorBuffer *
scintilla_editor_buffer_new (ScintillaObject *sci)
{
    ScintillaEditorBuffer *eb;
    Development_EditorBuffer objref;

    eb = 
	gtk_type_new (scintilla_editor_buffer_get_type ());
    objref = 
	create_editor_buffer (BONOBO_OBJECT (eb));
    if (objref == CORBA_OBJECT_NIL) {
	gtk_object_destroy (GTK_OBJECT (eb));
	return NULL;
    }
    
    bonobo_object_construct (BONOBO_OBJECT (eb),
			     objref);
    
    eb->sci = sci;
    
    return eb;
}

GtkType 
scintilla_editor_buffer_get_type (void)
{
    static GtkType type = 0;
        
    if (!type) {
	GtkTypeInfo info = {
	    "IDL:Development/EditorBuffer:1.0",
	    sizeof (ScintillaEditorBuffer),
	    sizeof (ScintillaEditorBufferClass),
	    (GtkClassInitFunc) scintilla_editor_buffer_class_init,
	    (GtkObjectInitFunc) scintilla_editor_buffer_init,
	    NULL,
	    NULL,
	    (GtkClassInitFunc) NULL
	};

	type = gtk_type_unique (bonobo_object_get_type (), &info);
    }

    return type;
}

/* private routines */
CORBA_Object
create_editor_buffer (BonoboObject *object) 
{
    POA_Development_EditorBuffer *servant;
    CORBA_Environment ev;
    CORBA_exception_init (&ev);
    
    servant = 
        (POA_Development_EditorBuffer*)g_new0(BonoboObjectServant, 1);
    servant->vepv = &editor_buffer_vepv;
    
    POA_Development_EditorBuffer__init((PortableServer_Servant) servant, 
				       &ev);
    if (ev._major != CORBA_NO_EXCEPTION) {
        g_free (servant);
        CORBA_exception_free (&ev);
        return CORBA_OBJECT_NIL;
    }
    CORBA_exception_free (&ev);
    return bonobo_object_activate_servant (object, servant);
}

static void
scintilla_editor_buffer_destroy (GtkObject *object) 
{
    ScintillaEditorBuffer *eb = SCINTILLA_EDITOR_BUFFER (object);
}

static CORBA_long
impl_get_length (PortableServer_Servant servant,
		 CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = editor_buffer_from_servant (servant);

    return scintilla_send_message (eb->sci, SCI_GETTEXTLENGTH, 0, 0);
}
 
static void
impl_read (PortableServer_Servant servant,
	   CORBA_long offset,
	   CORBA_long count,
	   Development_EditorBuffer_iobuf **buffer,
	   CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = editor_buffer_from_servant (servant);
    TextRange tr;
    long pos = offset;
    long len = scintilla_send_message (eb->sci, SCI_GETTEXTLENGTH, 0, 0);

    if (len - pos > count) {
	count = len - pos;
    }

    *buffer = Development_EditorBuffer_iobuf__alloc ();
    CORBA_sequence_set_release (*buffer, TRUE);
    (*buffer)->_buffer = CORBA_sequence_CORBA_octet_allocbuf (count);
    (*buffer)->_length = count;
    
    tr.chrg.cpMin = pos;
    tr.chrg.cpMax = pos + count;
    tr.lpstrText = (char *)(*buffer)->_buffer;

    scintilla_send_message (eb->sci, SCI_GETTEXTRANGE, 0, (long)&tr);    
}

static void
impl_insert (PortableServer_Servant servant,
	     CORBA_long offset,
	     const Development_EditorBuffer_iobuf *buffer,
	     CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = editor_buffer_from_servant (servant);
    char *string = g_malloc (buffer->_length + 1);
    memcpy (string, buffer->_buffer, buffer->_length);
    string[buffer->_length] = '\0';
    
    scintilla_send_message (eb->sci, SCI_INSERTTEXT, offset, (long)string);
    g_free (string);
}

static void 
impl_delete (PortableServer_Servant servant,
	     CORBA_long offset,
	     CORBA_long count,
	     CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = editor_buffer_from_servant (servant);
    long len = scintilla_send_message (eb->sci, SCI_GETTEXTLENGTH, 0, 0);

    if (len - offset > count) {
	count = len - offset;
    }
    
    scintilla_send_message (eb->sci, SCI_SETSEL, offset, offset + count);
    scintilla_send_message (eb->sci, SCI_DELETEBACK, 0, 0);
}

static void
init_editor_buffer_corba_class (void) 
{
    /* EPV */
    editor_buffer_epv.get_length = impl_get_length;
    editor_buffer_epv.read = impl_read;
    editor_buffer_epv.insert = impl_insert;
    editor_buffer_epv.delete = impl_delete;

    /* VEPV */
    editor_buffer_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
    editor_buffer_vepv.Development_EditorBuffer_epv = &editor_buffer_epv;
}
        
static void
scintilla_editor_buffer_class_init (ScintillaEditorBufferClass *class) 
{
    GtkObjectClass *object_class = (GtkObjectClass*) class;
    parent_class = gtk_type_class (bonobo_object_get_type ());
    
    object_class->destroy = scintilla_editor_buffer_destroy;
    
    init_editor_buffer_corba_class ();
}

static void
scintilla_editor_buffer_init (BonoboObject *object)
{
    ScintillaEditorBuffer *eb = SCINTILLA_EDITOR_BUFFER (object);
    eb->sci = NULL;
}
