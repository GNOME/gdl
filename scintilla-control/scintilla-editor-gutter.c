/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * This file is part of the GNOME Devtool Libraries..
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

#define PARENT_TYPE BONOBO_X_OBJECT_TYPE

static GtkObjectClass *parent_class = NULL;

enum {
    MARKER_BREAKPOINT, 
    MARKER_DISABLED_BREAKPOINT,
    MARKER_CURRENT_LINE,
    LAST_MARKER
};

struct _ScintillaEditorGutterPriv {
    ScintillaObject *sci;

    int marker_refcounts[LAST_MARKER];
};

ScintillaEditorGutter *
scintilla_editor_gutter_construct (ScintillaEditorGutter *bs, 
                                   ScintillaObject *sci)
{
    bs->priv->sci = sci;
    
    scintilla_send_message (sci, SCI_MARKERDEFINE, MARKER_BREAKPOINT, 
                            SC_MARK_CIRCLE);
    scintilla_send_message (sci, SCI_MARKERSETBACK, MARKER_BREAKPOINT, 
                            0x0000FF);
    scintilla_send_message (sci, SCI_MARKERDEFINE, MARKER_DISABLED_BREAKPOINT, 
                            SC_MARK_CIRCLE);
    scintilla_send_message (sci, SCI_MARKERSETBACK, 
                            MARKER_DISABLED_BREAKPOINT, 
                            0x808080);
    scintilla_send_message (sci, SCI_MARKERDEFINE, MARKER_CURRENT_LINE, 
                            SC_MARK_SHORTARROW);
    
    return bs;
}

ScintillaEditorGutter *
scintilla_editor_gutter_new (ScintillaObject *sci)
{
    ScintillaEditorGutter *bs = gtk_type_new (SCINTILLA_EDITOR_GUTTER_TYPE);

    return scintilla_editor_gutter_construct (bs, sci);
}

/* implementation functions */

static void
impl_add_marker (PortableServer_Servant servant,
                 CORBA_long line,
                 const CORBA_char *type,
                 CORBA_Environment *ev)
{
    ScintillaEditorGutter *gutter = 
        SCINTILLA_EDITOR_GUTTER (bonobo_object_from_servant (servant));
    int marker;
    
    if (!strcmp (type, "Breakpoint")) {
        marker = MARKER_BREAKPOINT;
    } else if (!strcmp (type, "DisabledBreakpoint")) {
        marker = MARKER_DISABLED_BREAKPOINT;
    } else if (!strcmp (type, "CurrentLine")) {
        marker = MARKER_CURRENT_LINE;
    } else {
        g_warning ("unknown marker type: %s\n", type);
    }

    scintilla_send_message (gutter->priv->sci, SCI_MARKERADD,
                            line - 1, marker);
}

static void
impl_remove_marker (PortableServer_Servant servant,
                    CORBA_long line,
                    const CORBA_char *type,
                    CORBA_Environment *ev)
{
    ScintillaEditorGutter *gutter = 
        SCINTILLA_EDITOR_GUTTER (bonobo_object_from_servant (servant));
    int marker;
    
    if (!strcmp (type, "Breakpoint")) {
        marker = MARKER_BREAKPOINT;
    } else if (!strcmp (type, "DisabledBreakpoint")) {
        marker = MARKER_DISABLED_BREAKPOINT;
    } else if (!strcmp (type, "CurrentLine")) {
        marker = MARKER_CURRENT_LINE;
    } else {
        g_warning ("unknown marker type: %s\n", type);
    }

    scintilla_send_message (gutter->priv->sci, SCI_MARKERDELETE,
                            line - 1, marker);
}

static GNOME_Development_EditorGutter_MarkerList *
impl_get_markers (PortableServer_Servant servant,
                  CORBA_Environment *ev)
{
    g_warning ("getMarkers unimplemented\n");
    
}

static void 
scintilla_editor_gutter_destroy (GtkObject *object)
{
    ScintillaEditorGutter *gutter = SCINTILLA_EDITOR_GUTTER (object);
    g_free (gutter->priv);
    parent_class->destroy (object);
}

static void
scintilla_editor_gutter_class_init (ScintillaEditorGutterClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;
	POA_GNOME_Development_EditorGutter__epv *epv = &klass->epv;
	parent_class = gtk_type_class (PARENT_TYPE);
	object_class->destroy = scintilla_editor_gutter_destroy;

    epv->addMarker = impl_add_marker;    
    epv->removeMarker = impl_remove_marker;    
    epv->getMarkers = impl_get_markers;
}

static void 
scintilla_editor_gutter_init (ScintillaEditorGutter *bs)
{
	bs->priv = g_new0 (ScintillaEditorGutterPriv, 1);
}

BONOBO_X_TYPE_FUNC_FULL (ScintillaEditorGutter,
                         GNOME_Development_EditorGutter,
                         PARENT_TYPE,
                         scintilla_editor_gutter);


			

