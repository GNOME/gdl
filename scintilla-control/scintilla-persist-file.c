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
#include "scintilla-persist-file.h"

#define BLOCK_SIZE 4096

static int
impl_save (BonoboPersistFile *pf,
	   const CORBA_char *filename,
	   CORBA_Environment *ev,
	   void *closure)
{
    ScintillaObject *sci = closure;
    FILE *fp;
    
    fp = fopen (filename, "w");
    
    if (fp) {
	char data[BLOCK_SIZE + 1];
	long length_doc = scintilla_send_message (sci, SCI_GETLENGTH, 0, 0);
	int i;

	for (i = 0; i < length_doc; i += BLOCK_SIZE) {
	    struct TextRange tr;
	    int grab_size = length_doc - i;
	    if (grab_size > BLOCK_SIZE) 
		grab_size = BLOCK_SIZE;
	    
	    tr.chrg.cpMin = i;
	    tr.chrg.cpMax = i + grab_size;
	    tr.lpstrText = data;
	    
	    scintilla_send_message (sci, SCI_GETTEXTRANGE, 0, (long)&tr);

	    fwrite (data, grab_size, 1, fp);
	}
	fclose (fp);
    }
    return 0;
}

static int
impl_load (BonoboPersistFile *pf,
	   const CORBA_char *filename,
	   CORBA_Environment *ev,
	   void *closure)
{
    ScintillaObject *sci = closure;
    FILE *fp;
    const char *mime_type;

    g_return_val_if_fail (IS_SCINTILLA (sci), -1);

    mime_type = gnome_vfs_get_file_mime_type (filename, NULL, FALSE);
    set_language_properties (sci, mime_type);

    scintilla_send_message (sci, SCI_CLEARALL, 0, 0);
    scintilla_send_message (sci, SCI_EMPTYUNDOBUFFER, 0, 0);
    scintilla_send_message (sci, SCI_SETSAVEPOINT, 0, 0);

    scintilla_send_message (sci, SCI_CANCEL, 0, 0);
    scintilla_send_message (sci, SCI_SETUNDOCOLLECTION, 0, 0);
    
    fp = fopen (filename, "r");
    if (fp) {
        char data[1024];
        int nread = fread (data, 1, sizeof (data), fp);
        while (nread > 0) {
            scintilla_send_message (sci, SCI_ADDTEXT, nread, (long)data);
            nread = fread (data, 1, sizeof (data), fp);
            fwrite (data, 1, nread, fp);
        }
        fclose (fp);
	
        scintilla_send_message (sci, SCI_SETUNDOCOLLECTION, 1, 0);
        
	scintilla_send_message (sci, SCI_EMPTYUNDOBUFFER, 0, 0);
	scintilla_send_message (sci, SCI_SETSAVEPOINT, 0, 0);
	scintilla_send_message (sci, SCI_GOTOPOS, 0, 0);

	return 0;
    } else {
	fprintf (stderr, "error\n");
	return -1;
    }
    return 0;
}

BonoboPersistFile *
scintilla_persist_file_new (GtkWidget *sci)
{
    g_return_val_if_fail (IS_SCINTILLA (sci), NULL);
    return bonobo_persist_file_new (impl_load, impl_save, sci);
}
