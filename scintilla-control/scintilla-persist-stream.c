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

#include "scintilla/Scintilla.h"
#include "scintilla/ScintillaWidget.h"
#include "scintilla/SciLexer.h"
#include "scintilla-persist-stream.h"

#define BLOCK_SIZE 4096

static int
impl_save (BonoboPersistStream *ps,
           const Bonobo_Stream stream,
           Bonobo_Persist_ContentType type,
           void *closure, 
	   CORBA_Environment *ev)
{
    ScintillaObject *sci = closure;

    char data[BLOCK_SIZE + 1];
    long length_doc = scintilla_send_message (sci, SCI_GETLENGTH, 0, 0);
    int i;
    
    for (i = 0; i < length_doc; i += BLOCK_SIZE) {
        Bonobo_Stream_iobuf buf;

        struct TextRange tr;
        int grab_size = length_doc - i;
        if (grab_size > BLOCK_SIZE) 
            grab_size = BLOCK_SIZE;
        
        tr.chrg.cpMin = i;
        tr.chrg.cpMax = i + grab_size;
        tr.lpstrText = data;
        
        scintilla_send_message (sci, SCI_GETTEXTRANGE, 0, (long)&tr);
        

        buf._maximum = grab_size;
        buf._length = grab_size;
        buf._buffer = (CORBA_char*)data;
        Bonobo_Stream_write (stream, &buf, ev);
    }

    return 0;
}
static int
impl_load (BonoboPersistStream *ps,
           Bonobo_Stream stream,
           Bonobo_Persist_ContentType type,
	   void *closure,
           CORBA_Environment *ev)
{
    ScintillaObject *sci = closure;
    char data[1024];

    g_return_val_if_fail (IS_SCINTILLA (sci), -1);

    set_language_properties (sci, type);

    scintilla_send_message (sci, SCI_CLEARALL, 0, 0);
    scintilla_send_message (sci, SCI_EMPTYUNDOBUFFER, 0, 0);
    scintilla_send_message (sci, SCI_SETSAVEPOINT, 0, 0);

    scintilla_send_message (sci, SCI_CANCEL, 0, 0);
    scintilla_send_message (sci, SCI_SETUNDOCOLLECTION, 0, 0);
    
    do {
        Bonobo_Stream_iobuf *buf;
        Bonobo_Stream_read (stream, BLOCK_SIZE, &buf, ev);

        if (ev->_major != CORBA_NO_EXCEPTION)
            break;

        scintilla_send_message (sci, SCI_ADDTEXT, (long)buf->_length,
                                (long)buf->_buffer);
        if (buf->_length <= 0)
            break;
        CORBA_free (buf);
    } while (1);
    scintilla_send_message (sci, SCI_SETUNDOCOLLECTION, 1, 0);
    
    scintilla_send_message (sci, SCI_EMPTYUNDOBUFFER, 0, 0);
    scintilla_send_message (sci, SCI_SETSAVEPOINT, 0, 0);
    scintilla_send_message (sci, SCI_GOTOPOS, 0, 0);
    
    return 0;
}

static Bonobo_Persist_ContentTypeList *
impl_get_content_types (BonoboPersistStream *ps, gpointer data,
                        CORBA_Environment *ev)
{
    return bonobo_persist_generate_content_types (10, 
                                                  "text/plain",
                                                  "text/x-idl",
                                                  "text/x-c",
                                                  "text/x-c++",
                                                  "text/html",
                                                  "text/xml",
                                                  "text/x-perl",
                                                  "text/x-sql",
                                                  "text/x-python",
                                                  "text/x-makefile");
}

BonoboPersistFile *
scintilla_persist_stream_new (GtkWidget *sci)
{
    g_return_val_if_fail (IS_SCINTILLA (sci), NULL);
    return bonobo_persist_stream_new (impl_load, impl_save, NULL,
                                      impl_get_content_types, sci);
}
