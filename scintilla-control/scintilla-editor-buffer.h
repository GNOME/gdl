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

#ifndef __SCINTILLA_EDITOR_BUFFER_H__
#define __SCINTILLA_EDITOR_BUFFER_H__

typedef struct _ScintillaEditorBuffer ScintillaEditorBuffer;

#include <bonobo.h>
#include <gtk/gtk.h>

#include "scintilla/ScintillaWidget.h"

BEGIN_GNOME_DECLS

#define SCINTILLA_EDITOR_BUFFER_TYPE (scintilla_editor_buffer_get_type ())
#define SCINTILLA_EDITOR_BUFFER(o) (GTK_CHECK_CAST ((o), SCINTILLA_EDITOR_BUFFER_TYPE, ScintillaEditorBuffer))
#define SCINTILLA_EDITOR_BUFFER_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SCINTILLA_EDITOR_BUFFER_CLASS_TYPE, ScintillaEditorBufferClass))
#define SCINTILLA_IS_EDITOR_BUFFER(o) (GTK_CHECK_TYPE ((o), SCINTILLA_EDITOR_BUFFER_TYPE))
#define SCINTILLA_IS_EDITOR_BUFFER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SCINTILLA_EDITOR_BUFFER_TYPE))

struct _ScintillaEditorBuffer {
    BonoboObject base;

    ScintillaObject *sci;
};

typedef struct 
{
    BonoboObjectClass parent_class;
} ScintillaEditorBufferClass;

ScintillaEditorBuffer *scintilla_editor_buffer_new (ScintillaObject *sci);
GtkType scintilla_editor_buffer_get_type (void);

END_GNOME_DECLS

#endif

