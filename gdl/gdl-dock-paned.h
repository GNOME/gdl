/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* GdlDockPaned - A slightly more advanced paned widget.
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Author: Christopher James Lahey <clahey@helixcode.com>
 *
 * based on GtkPaned from Gtk+.  Gtk+ Copyright notice follows.
 */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GDL_DOCK_PANED_H__
#define __GDL_DOCK_PANED_H__

#include <gtk/gtkcontainer.h>
#include <gdl/gdl-dock-item.h>

#define GDL_TYPE_DOCK_PANED                  (gdl_dock_paned_get_type ())
#define GDL_DOCK_PANED(obj)                  (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK_PANED, GdlDockPaned))
#define GDL_DOCK_PANED_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_PANED, GdlDockPanedClass))
#define GDL_IS_DOCK_PANED(obj)               (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK_PANED))
#define GDL_IS_DOCK_PANED_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_PANED))
#define GDL_DOCK_PANED_GET_CLASS(obj)        (GTK_CHECK_GET_CLASS ((obj), GDL_TYE_DOCK_PANED, GdlDockPanedClass))



typedef struct _GdlDockPaned      GdlDockPaned;
typedef struct _GdlDockPanedClass GdlDockPanedClass;

struct _GdlDockPaned {
    GdlDockItem dock_item;
    
    GtkWidget     *child1;
    GtkWidget     *child2;
  
    GdkWindow     *handle;
    GdkGC         *xor_gc;
    GdkCursorType  cursor_type;
  
    /*< public >*/
    guint16  handle_size;
  
    /*< private >*/
    guint16  handle_width;
    guint16  handle_height;

    gint     child1_real_size;
    gint     child1_size;
    gint     last_allocation;
    gint     min_position;
    gint     max_position;

    gint     old_child1_size;
    gint     quantum;
    
    guint    position_set : 1;
    guint    in_drag : 1;
    
    gint16   handle_xpos;
    gint16   handle_ypos;
};

struct _GdlDockPanedClass {
    GdlDockItemClass parent_class;
};


GType      gdl_dock_paned_get_type        (void);

GtkWidget *gdl_dock_paned_new             (GtkOrientation orientation);

void       gdl_dock_paned_add1            (GdlDockPaned *paned,
                                           GtkWidget    *child);

void       gdl_dock_paned_add2            (GdlDockPaned *paned,
                                           GtkWidget    *child);

gint       gdl_dock_paned_get_position    (GdlDockPaned *paned);

void       gdl_dock_paned_set_position    (GdlDockPaned *paned,
                                           gint          position);

void       gdl_dock_paned_set_handle_size (GdlDockPaned *paned,
                                           guint16       size);

/* FIXME: make it virtual of DockItem? */
void       gdl_dock_paned_reorder         (GdlDockPaned     *paned,
                                           GtkWidget        *child1,
                                           GdlDockPlacement  position);

#endif /* __GDL_DOCK_PANED_H__ */
