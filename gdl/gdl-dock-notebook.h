/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __GDL_DOCK_NOTEBOOK_H__
#define __GDL_DOCK_NOTEBOOK_H__

#include <gdl/gdl-dock-item.h>


#define GDL_TYPE_DOCK_NOTEBOOK            (gdl_dock_notebook_get_type ())
#define GDL_DOCK_NOTEBOOK(obj)            (GTK_CHECK_CAST ((obj), GDL_TYPE_DOCK_NOTEBOOK, GdlDockNotebook))
#define GDL_DOCK_NOTEBOOK_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_NOTEBOOK, GdlDockNotebookClass))
#define GDL_IS_DOCK_NOTEBOOK(obj)         (GTK_CHECK_TYPE ((obj), GDL_TYPE_DOCK_NOTEBOOK))
#define GDL_IS_DOCK_NOTEBOOK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_NOTEBOOK))
#define GDL_DOCK_NOTEBOOK_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_NOTEBOOK, GdlDockNotebookClass))

typedef struct _GdlDockNotebook        GdlDockNotebook;
typedef struct _GdlDockNotebookClass   GdlDockNotebookClass;

struct _GdlDockNotebook {
    GdlDockItem  item;

    GtkWidget   *notebook;
};

struct _GdlDockNotebookClass {
    GdlDockItemClass  parent_class;
};

GtkWidget     *gdl_dock_notebook_new               (void);

GType          gdl_dock_notebook_get_type          (void);

#endif

