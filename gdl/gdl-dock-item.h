/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __GDL_DOCK_ITEM_H__
#define __GDL_DOCK_ITEM_H__

#include <gtk/gtk.h>
#include <gnome-xml/tree.h>


#define GDL_TYPE_DOCK_ITEM            (gdl_dock_item_get_type ())
#define GDL_DOCK_ITEM(obj)            (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK_ITEM, GdlDockItem))
#define GDL_DOCK_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_ITEM, GdlDockItemClass))
#define GDL_IS_DOCK_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK_ITEM))
#define GDL_IS_DOCK_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_ITEM))

#define GDL_DOCK_ITEM_IS_FLOATING(item) ((item)->is_floating)

#define GDL_DOCK_ITEM_IS_BOUND(item)  ((item)->dock != NULL)

#define GDL_DOCK_ITEM_CHECK_BOND(item,d)  ((item)->dock == GTK_WIDGET (d))

#define GDL_DOCK_ITEM_CHECK_AND_BIND(item,other) G_STMT_START {                 \
    if (!GDL_DOCK_ITEM_IS_BOUND (item))                                         \
        gdl_dock_bind_item (GDL_DOCK (GDL_DOCK_ITEM (other)->dock), item);      \
    g_return_if_fail (item->dock == GDL_DOCK_ITEM (other)->dock); } G_STMT_END

#define GDL_DOCK_ITEM_GET_PARENT(item,p) G_STMT_START {         \
    p = GTK_WIDGET (item)->parent;                              \
    while (p && !GDL_IS_DOCK_ITEM (p) && !GDL_IS_DOCK (p))      \
        p = p->parent; } G_STMT_END


typedef enum {
    GDL_DOCK_TOP,
    GDL_DOCK_BOTTOM,
    GDL_DOCK_RIGHT,
    GDL_DOCK_LEFT,
    GDL_DOCK_CENTER,
    GDL_DOCK_FLOATING
} GdlDockPlacement;

typedef enum {
    GDL_DOCK_ITEM_BEH_NORMAL = 0,
    GDL_DOCK_ITEM_BEH_EXCLUSIVE = 1 << 0,
    GDL_DOCK_ITEM_BEH_NEVER_FLOATING = 1 << 1,
    GDL_DOCK_ITEM_BEH_NEVER_VERTICAL = 1 << 2,
    GDL_DOCK_ITEM_BEH_NEVER_HORIZONTAL = 1 << 3,
    GDL_DOCK_ITEM_BEH_LOCKED = 1 << 4
} GdlDockItemBehavior;


typedef struct _GdlDockItem        GdlDockItem;
typedef struct _GdlDockItemClass   GdlDockItemClass;
typedef struct _GdlDockRequestInfo GdlDockRequestInfo;


struct _GdlDockItem {
    GtkBin bin;

    GdkWindow           *bin_window;
    GdkWindow           *float_window;

    gchar               *name;
    gchar               *long_name;

    GdlDockItemBehavior  behavior;
    /* FIXME: this should go away and be replaced using layout managment
       functions */
    GdlDockPlacement     placement;
    GtkOrientation       orientation;
    GtkWidget           *dock;
    GtkWidget           *tab_label;
    GtkWidget           *menu;
    guint                drag_handle_size;

    guint      is_floating : 1;
    guint      float_window_mapped : 1;
    guint      in_drag : 1;
    guint      in_resize : 1;
    guint      disable_auto_reduce : 1;

    guint      resize : 1;
    guint      shrink : 1;
    guint      handle_shown : 1;
    gint       dragoff_x, dragoff_y;
    gint       float_x, float_y;

    /* these should be gint and not guint... trust me */
    gint       float_width, float_height;
};

/* structure for drag_request return information */
struct _GdlDockRequestInfo {
    /* target is a GtkWidget because the dock can be a target */
    GdlDockItem      *requestor;
    GtkWidget        *target;
    GdlDockPlacement  position;
    GdkRectangle      rect;  /* where will the item dock */
};

struct _GdlDockItemClass {
    GtkBinClass parent_class;

    /* virtuals */
    void     (* auto_reduce)      (GdlDockItem *item);

    gboolean (* dock_request)     (GdlDockItem        *item, 
                                   gint                x, 
                                   gint                y, 
                                   GdlDockRequestInfo *target);

    void     (* dock_drag_begin)  (GdlDockItem    *item);
    void     (* dock_drag_motion) (GdlDockItem    *item,
                                   gint            x,
                                   gint            y);
    void     (* dock_drag_end)    (GdlDockItem    *item);
                                   
    void     (* set_orientation)  (GdlDockItem    *item,
                                   GtkOrientation  orientation);
                                   
    void     (* save_layout)      (GdlDockItem *item,
                                   xmlNodePtr   node);

    void     (* item_hide)        (GdlDockItem *item);
};



GtkWidget     *gdl_dock_item_new               (const gchar         *name,
                                                const gchar         *long_name,
                                                GdlDockItemBehavior  behavior);

GtkType        gdl_dock_item_get_type          (void);

void           gdl_dock_item_dock_to           (GdlDockItem      *item,
                                                GdlDockItem      *target,
                                                GdlDockPlacement  position,
                                                gint              docking_param);

void           gdl_dock_item_set_orientation   (GdlDockItem    *item,
                                                GtkOrientation  orientation);

void           gdl_dock_item_auto_reduce       (GdlDockItem *item);

gboolean       gdl_dock_item_dock_request      (GdlDockItem        *item, 
                                                gint                x,
                                                gint                y, 
                                                GdlDockRequestInfo *target);

GtkWidget     *gdl_dock_item_get_tablabel      (GdlDockItem *item);
void           gdl_dock_item_set_tablabel      (GdlDockItem *item,
                                                GtkWidget   *tablabel);
void           gdl_dock_item_hide_handle       (GdlDockItem *item);
void           gdl_dock_item_show_handle       (GdlDockItem *item);

/* GdkWindow setup and stuff */
void           gdl_dock_item_window_sink       (GdlDockItem *item);

void           gdl_dock_item_window_float      (GdlDockItem *item);

void           gdl_dock_item_drag_floating     (GdlDockItem *item, 
                                                gint         x, 
                                                gint         y);

void           gdl_dock_item_unbind            (GdlDockItem *item);

void           gdl_dock_item_hide              (GdlDockItem *item);

void           gdl_dock_item_save_layout       (GdlDockItem *item,
                                                xmlNodePtr   node);


#endif
