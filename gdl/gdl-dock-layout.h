/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __GDL_DOCK_LAYOUT_H__
#define __GDL_DOCK_LAYOUT_H__

#include <glib.h>
#include <libxml/tree.h>
#include <glade/glade.h>
#include <gdl/gdl-dock.h>


#define	GDL_TYPE_DOCK_LAYOUT		  (gdl_dock_layout_get_type ())
#define GDL_DOCK_LAYOUT(object)		  (GTK_CHECK_CAST ((object), GDL_TYPE_DOCK_LAYOUT, GdlDockLayout))
#define GDL_DOCK_LAYOUT_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_LAYOUT, GdlDockLayoutClass))
#define GDL_IS_DOCK_LAYOUT(object)	  (GTK_CHECK_TYPE ((object), GDL_TYPE_DOCK_LAYOUT))
#define GDL_IS_DOCK_LAYOUT_CLASS(klass)	  (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_LAYOUT))
#define	GDL_DOCK_LAYOUT_GET_CLASS(object) (GTK_CHECK_GET_CLASS ((object), GDL_TYPE_DOCK_LAYOUT, GdlDockLayoutClass))


typedef struct _GdlDockLayout GdlDockLayout;
typedef struct _GdlDockLayoutClass GdlDockLayoutClass;

struct _GdlDockLayout {
    GObject    g_object;

    GdlDock   *dock;

    xmlDocPtr  doc;
    gboolean   dirty;

    /* dialog section */
    GtkWidget        *dialog;
    GtkWidget        *layout_entry;
    GtkTreeSelection *selection;
    GtkListStore     *items_model;
    GtkListStore     *layouts_model;
    gboolean          changed_by_user;
};

struct _GdlDockLayoutClass {
    GObjectClass  g_object_class;
};


GType            gdl_dock_layout_get_type       (void);

GdlDockLayout   *gdl_dock_layout_new            (GdlDock       *dock);

void             gdl_dock_layout_attach         (GdlDockLayout *layout,
                                                 GdlDock       *dock);

gboolean         gdl_dock_layout_load_layout    (GdlDockLayout *layout,
                                                 const gchar   *name);

void             gdl_dock_layout_save_layout    (GdlDockLayout *layout,
                                                 const gchar   *name);

void             gdl_dock_layout_delete_layout  (GdlDockLayout *layout,
                                                 const gchar   *name);

GList           *gdl_dock_layout_get_layouts    (GdlDockLayout *layout,
                                                 gboolean       include_default);

void             gdl_dock_layout_run_manager    (GdlDockLayout *layout);

gboolean         gdl_dock_layout_load_from_file (GdlDockLayout *layout,
                                                 const gchar   *filename);

gboolean         gdl_dock_layout_save_to_file   (GdlDockLayout *layout,
                                                 const gchar   *filename);

gboolean         gdl_dock_layout_is_dirty       (GdlDockLayout *layout);

#endif


