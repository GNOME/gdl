/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "gdl-tools.h"
#include "gdl-dock-notebook.h"


/* Private prototypes */

static void  gdl_dock_notebook_class_init    (GdlDockNotebookClass *klass);
static void  gdl_dock_notebook_init          (GdlDockNotebook *notebook);

static void  gdl_dock_notebook_add           (GtkContainer *container,
					      GtkWidget    *widget);
static void  gdl_dock_notebook_remove        (GtkContainer *container,
					      GtkWidget    *widget);
static void  gdl_dock_notebook_forall        (GtkContainer *container,
					      gboolean      include_internals,
					      GtkCallback   callback,
					      gpointer      callback_data);

static void  gdl_dock_notebook_auto_reduce   (GdlDockItem *item);

static void gdl_dock_notebook_set_orientation (GdlDockItem    *item,
					       GtkOrientation  orientation);


/* Class variables and definitions */

static GdlDockItemClass *parent_class = NULL;


/* Private functions */

static void
gdl_dock_notebook_class_init (GdlDockNotebookClass *klass)
{
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;
    GdlDockItemClass  *item_class;

    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;
    item_class = (GdlDockItemClass *) klass;

    parent_class = gtk_type_class (gdl_dock_item_get_type ());

    container_class->add = gdl_dock_notebook_add;
    container_class->remove = gdl_dock_notebook_remove;
    container_class->forall = gdl_dock_notebook_forall;

    item_class->auto_reduce = gdl_dock_notebook_auto_reduce;
    item_class->set_orientation = gdl_dock_notebook_set_orientation;
}

static void
gdl_dock_notebook_init (GdlDockNotebook *notebook)
{
    notebook->notebook = NULL;
}

static void
gdl_dock_notebook_add (GtkContainer *container,
		       GtkWidget    *widget)
{
    GdlDockNotebook *notebook;
    GdlDockItem *item;

    g_return_if_fail (container != NULL);
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (container));
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    notebook = GDL_DOCK_NOTEBOOK (container);
    item = GDL_DOCK_ITEM (widget);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook->notebook), 
    			      widget, gtk_label_new (item->name));
}

static void
gdl_dock_notebook_remove (GtkContainer *container,
			  GtkWidget    *widget)
{
    GdlDockNotebook *notebook;
    GdlDockItem *item;
    guint pagenr;

    g_return_if_fail (container != NULL);
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (container));
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget) || 
                      widget == GDL_DOCK_NOTEBOOK (container)->notebook);
    
    notebook = GDL_DOCK_NOTEBOOK (container);
    if (widget == notebook->notebook)
        gtk_widget_unparent (widget);
    else {
        item = GDL_DOCK_ITEM (widget);

        pagenr = gtk_notebook_page_num (GTK_NOTEBOOK (notebook->notebook), 
                                        widget);
        gtk_notebook_remove_page (GTK_NOTEBOOK (notebook->notebook), pagenr);
    };
}

static void
gdl_dock_notebook_forall (GtkContainer *container,
			  gboolean      include_internals,
			  GtkCallback   callback,
			  gpointer      callback_data)
{
    GdlDockNotebook *notebook;

    g_return_if_fail (container != NULL);
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (container));
    g_return_if_fail (callback != NULL);

    notebook = GDL_DOCK_NOTEBOOK (container);

    (*callback) (notebook->notebook, callback_data);
}

static void
gdl_dock_notebook_auto_reduce (GdlDockItem *item)
{
    GtkWidget       *child, *parent;
    GdlDockNotebook *nb;
    GList           *children;

    g_return_if_fail (item != NULL);
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (item));

    parent = GTK_WIDGET (item)->parent;
    nb = GDL_DOCK_NOTEBOOK (item);

    GDL_TRACE ();
    
    children = gtk_container_children (GTK_CONTAINER (nb->notebook));
    /* Check if only one item remains in the notebook. */
    if (g_list_length (children) == 1) {
        child = gtk_notebook_get_nth_page (
            GTK_NOTEBOOK (GDL_DOCK_NOTEBOOK (item)->notebook), 0);
            
        /* Remove widget from notebook, remove the notebook from it's parent
           and add the widget directly to the notebook's parent. */
        gtk_widget_ref (child);
        gtk_notebook_remove_page (GTK_NOTEBOOK (nb->notebook), 0);
        gtk_container_remove (GTK_CONTAINER (parent), GTK_WIDGET (item));
        gtk_container_add (GTK_CONTAINER (parent), child);
        gtk_widget_unref (child);
    }
    g_list_free (children);
}

static void
gdl_dock_notebook_set_orientation (GdlDockItem    *item,
				   GtkOrientation  orientation)
{
    /* FIXME */
}

/* Public interface */

GtkWidget *
gdl_dock_notebook_new (void)
{
    GdlDockNotebook *notebook;
    GtkBin          *bin;

    notebook = GDL_DOCK_NOTEBOOK 
	(gtk_type_new (gdl_dock_notebook_get_type ()));
    bin = GTK_BIN (notebook);

    bin->child = notebook->notebook = gtk_notebook_new ();

    /* create the container notebook */
    gtk_widget_set_parent (notebook->notebook, GTK_WIDGET (notebook));
    
    GDL_DOCK_ITEM (notebook)->name = g_strdup ("Notebook");

    gtk_widget_show (notebook->notebook);

    return GTK_WIDGET (notebook);
}

guint
gdl_dock_notebook_get_type (void)
{
    static GtkType dock_notebook_type = 0;

    if (dock_notebook_type == 0) {
        GtkTypeInfo dock_notebook_info = {
            "GdlDockNotebook",
            sizeof (GdlDockNotebook),
            sizeof (GdlDockNotebookClass),
            (GtkClassInitFunc) gdl_dock_notebook_class_init,
            (GtkObjectInitFunc) gdl_dock_notebook_init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL,
        };

        dock_notebook_type = gtk_type_unique (gdl_dock_item_get_type (), 
					      &dock_notebook_info);
    }

    return dock_notebook_type;
}
