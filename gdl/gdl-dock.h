/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __GDL_DOCK_H__
#define __GDL_DOCK_H__

#include <gtk/gtk.h>
#include <gdl/gdl-dock-item.h>
#include <libxml/tree.h>

#define GDL_TYPE_DOCK            (gdl_dock_get_type ())
#define GDL_DOCK(obj)            (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK, GdlDock))
#define GDL_DOCK_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK, GdlDockClass))
#define GDL_IS_DOCK(obj)         (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK))
#define GDL_IS_DOCK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK))


typedef struct _GdlDock       GdlDock;
typedef struct _GdlDockClass  GdlDockClass;

struct _GdlDock {
    GtkContainer container;

    GtkWidget          *root;
    GList              *floating;
    GdlDockRequestInfo  possible_target;
    GList              *items;

    /* auxiliary fields */
    gboolean            rect_drawn;
    GdkGC              *xor_gc;
    GdkGC              *root_xor_gc;
};

struct _GdlDockClass {
    GtkContainerClass parent_class;

    void (* layout_changed) (GdlDock *dock);
};


GtkWidget     *gdl_dock_new               (void);

GtkType        gdl_dock_get_type          (void);

void           gdl_dock_bind_item         (GdlDock          *dock,
                                           GdlDockItem      *item);
void           gdl_dock_unbind_item       (GdlDock          *dock,
                                           GdlDockItem      *item);

void           gdl_dock_add_item          (GdlDock          *dock,
                                           GdlDockItem      *item,
                                           GdlDockPlacement  place);

void           gdl_dock_add_floating_item (GdlDock        *dock,
                                           GdlDockItem    *item,
                                           gint            x,
                                           gint            y,
                                           GtkOrientation  orientatinon);

GdlDockItem   *gdl_dock_get_item_by_name  (GdlDock     *dock,
                                           const gchar *name);

void           gdl_dock_layout_load       (GdlDock    *dock,
                                           xmlNodePtr  node);
                                           
void           gdl_dock_layout_save       (GdlDock    *dock,
                                           xmlNodePtr  node);

#endif
