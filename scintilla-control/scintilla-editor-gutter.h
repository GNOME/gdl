/*  -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * This file is part of the GNOME Debugging Framework.
 * 
 * Copyright (C) 2001 Dave Camp <dave@ximian.com>
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


#ifndef __SCINTILLA_EDITOR_GUTTER_H__
#define __SCINTILLA_EDITOR_GUTTER_H__

#include <config.h>
#include "scintilla-editor-gutter.h"

#include <gnome.h>
#include <bonobo.h>
#include <gtk/gtk.h>
#include <liboaf/liboaf.h>
#include <limits.h>

#include "scintilla/Scintilla.h"
#include "scintilla/ScintillaWidget.h"

#include "scintilla-persist-file.h"
#include "scintilla-editor-buffer.h"

#include <bonobo/bonobo-xobject.h>
#include <bonobo/bonobo-event-source.h>
#include <gdl/gdl.h>

BEGIN_GNOME_DECLS

#define SCINTILLA_EDITOR_GUTTER_TYPE                (scintilla_editor_gutter_get_gtk_type ())
#define SCINTILLA_EDITOR_GUTTER(o)                  (GTK_CHECK_CAST ((o), SCINTILLA_EDITOR_GUTTER_TYPE, ScintillaEditorGutter))
#define SCINTILLA_EDITOR_GUTTER_CLASS(k)            (GTK_CHECK_CLASS_CAST((k), SCINTILLA_EDITOR_GUTTER_TYPE, ScintillaEditorGutterClass))
#define GDF_IS_BREAKPOINT_SET(o)               (GTK_CHECK_TYPE ((o), SCINTILLA_EDITOR_GUTTER_TYPE))
#define GDF_IS_BREAKPOINT_SET_CLASS(k)         (GTK_CHECK_CLASS_TYPE ((k), SCINTILLA_EDITOR_GUTTER_TYPE))

typedef struct _ScintillaEditorGutterPriv  ScintillaEditorGutterPriv;
typedef struct _ScintillaEditorGutter      ScintillaEditorGutter;

struct _ScintillaEditorGutter {
    BonoboXObject parent;
    ScintillaEditorGutterPriv *priv;
    BonoboEventSource *event_source;
};

typedef struct {
	BonoboXObjectClass parent;
	POA_GNOME_Development_EditorGutter__epv epv;
} ScintillaEditorGutterClass;

GtkType                scintilla_editor_gutter_get_gtk_type (void);
ScintillaEditorGutter *scintilla_editor_gutter_new          (ScintillaObject *sci);
ScintillaEditorGutter *scintilla_editor_gutter_construct    (ScintillaEditorGutter *bs,
                                                             ScintillaObject *sci);
END_GNOME_DECLS

#endif
