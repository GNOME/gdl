/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 8 -*- */
/**
 * gdl-dock-item-grip.h
 * 
 * Based on bonobo-dock-item-grip.  Original copyright notice follows.
 *
 * Author:
 *    Michael Meeks
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 */

#ifndef _GDL_DOCK_ITEM_GRIP_H_
#define _GDL_DOCK_ITEM_GRIP_H_

#include <gtk/gtkwidget.h>
#include <gdl/gdl-dock-item.h>

G_BEGIN_DECLS

#define GDL_TYPE_DOCK_ITEM_GRIP            (gdl_dock_item_grip_get_type())
#define GDL_DOCK_ITEM_GRIP(obj)            \
    (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK_ITEM_GRIP, GdlDockItemGrip))
#define GDL_DOCK_ITEM_GRIP_CLASS(klass)    \
    (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_ITEM_GRIP, GdlDockItemGripClass))
#define GDL_IS_DOCK_ITEM_GRIP(obj)         \
    (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK_ITEM_GRIP))
#define GDL_IS_DOCK_ITEM_GRIP_CLASS(klass) \
    (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_ITEM_GRIP))
#define GDL_DOCK_ITEM_GRIP_GET_CLASS(obj)  \
    (GTK_CHECK_GET_CLASS ((obj), GDL_TYPE_DOCK_ITEM_GRIP, GdlDockItemGripClass))

typedef struct {
    GtkWidget parent;
	
    GdlDockItem *item;
} GdlDockItemGrip;

typedef struct {
    GtkWidgetClass parent_class;

    void (*activate) (GdlDockItemGrip *grip);
} GdlDockItemGripClass;

GType      gdl_dock_item_grip_get_type (void);
GtkWidget *gdl_dock_item_grip_new      (GdlDockItem *item);

G_END_DECLS

#endif /* _GDL_DOCK_ITEM_GRIP_H_ */
