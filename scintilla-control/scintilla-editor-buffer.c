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

#include "scintilla/Scintilla.h"
#include "scintilla-editor-buffer.h"

#include <gnome.h>
#include <bonobo.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static void scintilla_editor_buffer_destroy (GtkObject *object);
static void scintilla_editor_buffer_class_init (ScintillaEditorBufferClass *class);
static void scintilla_editor_buffer_init (BonoboObject *object);

GtkObjectClass *parent_class;

#define PARENT_TYPE BONOBO_X_OBJECT_TYPE

#define CORBA_boolean__alloc() (CORBA_boolean*) CORBA_octet_allocbuf (sizeof (CORBA_boolean))

BONOBO_X_TYPE_FUNC_FULL (ScintillaEditorBuffer, 
                         GNOME_Development_EditorBuffer,
                         PARENT_TYPE,
                         scintilla_editor_buffer);

/* public routines */

ScintillaEditorBuffer *
scintilla_editor_buffer_new (ScintillaObject *sci)
{
    ScintillaEditorBuffer *eb;
    eb = gtk_type_new (scintilla_editor_buffer_get_type ());    
    eb->sci = sci;
    
    return eb;
}

static CORBA_long
impl_get_length (PortableServer_Servant servant,
		 CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = SCINTILLA_EDITOR_BUFFER (bonobo_object_from_servant (servant));

    return scintilla_send_message (eb->sci, SCI_GETTEXTLENGTH, 0, 0);
}
 
static void
impl_get_chars (PortableServer_Servant servant,
                CORBA_long offset,
                CORBA_long count,
                GNOME_Development_EditorBuffer_iobuf **buffer,
                CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = SCINTILLA_EDITOR_BUFFER (bonobo_object_from_servant (servant));
    struct TextRange tr;
    long pos = offset;
    long len = scintilla_send_message (eb->sci, SCI_GETTEXTLENGTH, 0, 0);

    if (len - pos < count) {
	count = len - pos;
    }

    *buffer = GNOME_Development_EditorBuffer_iobuf__alloc ();
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
	     const CORBA_char *str,
	     CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = SCINTILLA_EDITOR_BUFFER (bonobo_object_from_servant (servant));
    scintilla_send_message (eb->sci, SCI_INSERTTEXT, offset, (long)str);
}

static void 
impl_delete (PortableServer_Servant servant,
	     CORBA_long offset,
	     CORBA_long count,
	     CORBA_Environment *ev)
{
    ScintillaEditorBuffer *eb = SCINTILLA_EDITOR_BUFFER (bonobo_object_from_servant (servant));
    long len = scintilla_send_message (eb->sci, SCI_GETTEXTLENGTH, 0, 0);

    if (len - offset < count) {
	count = len - offset;
    }
    
    scintilla_send_message (eb->sci, SCI_SETSEL, offset, offset + count);
    scintilla_send_message (eb->sci, SCI_DELETEBACK, 0, 0);
}
       
static void
scintilla_editor_buffer_class_init (ScintillaEditorBufferClass *klass) 
{
    POA_GNOME_Development_EditorBuffer__epv *epv = &klass->epv;
    
    parent_class = gtk_type_class (PARENT_TYPE);
    
    epv->getLength = impl_get_length;
    epv->getChars = impl_get_chars;
    epv->insert = impl_insert;
    epv->delete = impl_delete;
}

static void
scintilla_editor_buffer_init (BonoboObject *object)
{
    ScintillaEditorBuffer *eb = SCINTILLA_EDITOR_BUFFER (object);
    eb->sci = NULL;
}
