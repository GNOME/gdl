/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __GDL_DOCK_TABLABEL_H__
#define __GDL_DOCK_TABLABEL_H__

#include <gtk/gtk.h>

/* Class macros */
#define GDL_TYPE_DOCK_TABLABEL            (gdl_dock_tablabel_get_type ())
#define GDL_DOCK_TABLABEL(obj)            (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK_TABLABEL, GdlDockTablabel))
#define GDL_DOCK_TABLABEL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_TABLABEL, GdlDockTablabelClass))
#define GDL_IS_DOCK_TABLABEL(obj)         (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK_TABLABEL))
#define GDL_IS_DOCK_TABLABEL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_TABLABEL))
#define GDL_DOCK_TABLABEL_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_TABLABEL, GdlDockTablabelClass))

typedef struct _GdlDockTablabel      GdlDockTablabel;
typedef struct _GdlDockTablabelClass GdlDockTablabelClass;

struct _GdlDockTablabel {
    GtkEventBox     event_box;

    guint           drag_handle_size;
    GtkOrientation  orientation;
    gint            active : 1;
    GtkWidget      *master;
};

struct _GdlDockTablabelClass {
    GtkEventBoxClass  parent_class;

    void            (*button_pressed_handle)  (GdlDockTablabel *tablabel,
                                               gint             button,
                                               guint            time);
};

GtkWidget     *gdl_dock_tablabel_new           (const gchar     *label);
GType          gdl_dock_tablabel_get_type      (void);
void           gdl_dock_tablabel_activate      (GdlDockTablabel *tablabel);
void           gdl_dock_tablabel_deactivate    (GdlDockTablabel *tablabel);

#endif
