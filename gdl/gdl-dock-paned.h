/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * gdl-dock-paned.h
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

#ifndef __GDL_DOCK_PANED_H__
#define __GDL_DOCK_PANED_H__

#include <gdl/gdl-dock-item.h>

G_BEGIN_DECLS

/* standard macros */
#define GDL_TYPE_DOCK_PANED                  (gdl_dock_paned_get_type ())
#define GDL_DOCK_PANED(obj)                  (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK_PANED, GdlDockPaned))
#define GDL_DOCK_PANED_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_PANED, GdlDockPanedClass))
#define GDL_IS_DOCK_PANED(obj)               (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK_PANED))
#define GDL_IS_DOCK_PANED_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_PANED))
#define GDL_DOCK_PANED_GET_CLASS(obj)        (GTK_CHECK_GET_CLASS ((obj), GDL_TYE_DOCK_PANED, GdlDockPanedClass))

/* data types & structures */
typedef struct _GdlDockPaned      GdlDockPaned;
typedef struct _GdlDockPanedClass GdlDockPanedClass;

struct _GdlDockPaned {
    GdlDockItem  dock_item;

    gboolean     position_changed;
};

struct _GdlDockPanedClass {
    GdlDockItemClass parent_class;
};


/* public interface */
 
GType      gdl_dock_paned_get_type        (void);

GtkWidget *gdl_dock_paned_new             (GtkOrientation orientation);


G_END_DECLS

#endif /* __GDL_DOCK_PANED_H__ */

