/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * gdl-dock-tablabel.h
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Gir�ldez <gustavo.giraldez@gmx.net>
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

#ifndef __GDL_DOCK_TABLABEL_H__
#define __GDL_DOCK_TABLABEL_H__

#include <gtk/gtk.h>
#include <gdl/gdl-dock-item.h>


G_BEGIN_DECLS

/* standard macros */
#define GDL_TYPE_DOCK_TABLABEL            (gdl_dock_tablabel_get_type ())
#define GDL_DOCK_TABLABEL(obj)            (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK_TABLABEL, GdlDockTablabel))
#define GDL_DOCK_TABLABEL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_TABLABEL, GdlDockTablabelClass))
#define GDL_IS_DOCK_TABLABEL(obj)         (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK_TABLABEL))
#define GDL_IS_DOCK_TABLABEL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_TABLABEL))
#define GDL_DOCK_TABLABEL_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_TABLABEL, GdlDockTablabelClass))

/* data types & structures */
typedef struct _GdlDockTablabel      GdlDockTablabel;
typedef struct _GdlDockTablabelClass GdlDockTablabelClass;

struct _GdlDockTablabel {
    GtkBin          parent;

    guint           drag_handle_size;
    GtkWidget      *item;
    GdkWindow      *event_window;
    gboolean        active;

    GdkEventButton  drag_start_event;
    gboolean        pre_drag;
};

struct _GdlDockTablabelClass {
    GtkBinClass      parent_class;

    void            (*button_pressed_handle)  (GdlDockTablabel *tablabel,
                                               GdkEventButton  *event);
};

/* public interface */
 
GtkWidget     *gdl_dock_tablabel_new           (GdlDockItem *item);
GType          gdl_dock_tablabel_get_type      (void);

void           gdl_dock_tablabel_activate      (GdlDockTablabel *tablabel);
void           gdl_dock_tablabel_deactivate    (GdlDockTablabel *tablabel);

G_END_DECLS

#endif
