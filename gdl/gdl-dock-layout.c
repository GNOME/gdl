/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>

#include "gdl-dock-layout.h"
#include "gdl-tools.h"


/* ----- Private variables ----- */

enum {
    PROP_0,
    PROP_DOCK
};

#define ROOT_ELEMENT         "dock-layout"
#define DEFAULT_LAYOUT       "__default__"
#define LAYOUT_ELEMENT_NAME  "layout"
#define NAME_ATTRIBUTE_NAME  "name"

#define LAYOUT_GLADE_FILE    "layout.glade"

enum {
    COLUMN_NAME,
    COLUMN_SHOW,
    COLUMN_ITEM
};


/* ----- Private prototypes ----- */

static void     gdl_dock_layout_class_init      (GdlDockLayoutClass *klass);

static void     gdl_dock_layout_init            (GdlDockLayout      *layout);

static void     gdl_dock_layout_set_property    (GObject            *object,
                                                 guint               prop_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);

static void     gdl_dock_layout_get_property    (GObject            *object,
                                                 guint               prop_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);

static void     gdl_dock_layout_dispose         (GObject            *object);

static void     gdl_dock_layout_build_doc       (GdlDockLayout      *layout);

static xmlNodePtr gdl_dock_layout_find_layout   (GdlDockLayout      *layout, 
                                                 const gchar        *name);

static gboolean gdl_dock_layout_construct_dialog (GdlDockLayout     *layout);

static void     gdl_dock_layout_populate_models (GdlDockLayout      *layout);


/* ----- Private implementation ----- */

static void
gdl_dock_layout_class_init (GdlDockLayoutClass *klass)
{
    GObjectClass *g_object_class = (GObjectClass *) klass;

    g_object_class->set_property = gdl_dock_layout_set_property;
    g_object_class->get_property = gdl_dock_layout_get_property;
    g_object_class->dispose = gdl_dock_layout_dispose;

    g_object_class_install_property (
        g_object_class, PROP_DOCK,
        g_param_spec_object ("dock", _("Dock"),
                             _("GdlDock object which the layout object "
                               "is attached to"),
                             GDL_TYPE_DOCK, 
                             G_PARAM_READWRITE));
}

static void
gdl_dock_layout_init (GdlDockLayout *layout)
{
    layout->dock = NULL;
    layout->doc = NULL;
    layout->dirty = FALSE;
    layout->dialog = NULL;
    layout->selection = NULL;
    layout->layout_entry = NULL;
    layout->items_model = NULL;
    layout->layouts_model = NULL;
}

static void
gdl_dock_layout_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
    GdlDockLayout *layout = GDL_DOCK_LAYOUT (object);

    switch (prop_id) {
    case PROP_DOCK:
        gdl_dock_layout_attach (layout, g_value_get_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    };
}

static void
gdl_dock_layout_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
    GdlDockLayout *layout = GDL_DOCK_LAYOUT (object);

    switch (prop_id) {
    case PROP_DOCK:
        g_value_set_object (value, layout->dock);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    };
}

static void
gdl_dock_layout_dispose (GObject *object)
{
    GdlDockLayout *layout;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GDL_IS_DOCK_LAYOUT (object));

    layout = GDL_DOCK_LAYOUT (object);
    
    if (layout->dock)
        gdl_dock_layout_attach (layout, NULL);

    if (layout->doc) {
        xmlFreeDoc (layout->doc);
        layout->doc = NULL;
    };

    if (layout->dialog) {
        gtk_widget_destroy (layout->dialog);
        g_object_unref (layout->items_model);
        g_object_unref (layout->layouts_model);
        layout->dialog = NULL;
        layout->layout_entry = NULL;
        layout->selection = NULL;
        layout->items_model = NULL;
        layout->layouts_model = NULL;
    };
}

static void
gdl_dock_layout_build_doc (GdlDockLayout *layout)
{
    g_return_if_fail (layout->doc == NULL);

    layout->doc = xmlNewDoc ("1.0");
    layout->doc->children = xmlNewDocNode (layout->doc, NULL, 
                                           ROOT_ELEMENT, NULL);
}

static xmlNodePtr
gdl_dock_layout_find_layout (GdlDockLayout *layout, 
                             const gchar   *name)
{
    xmlNodePtr node;
    gboolean found = FALSE;

    g_return_val_if_fail (layout != NULL, NULL);
    
    if (!layout->doc)
        return NULL;

    /* get document root */
    node = layout->doc->children;
    for (node = node->children; node; node = node->next) {
        gchar *layout_name;

        if (strcmp (node->name, LAYOUT_ELEMENT_NAME))
            /* skip non-layout element */
            continue;

        /* we want the first layout */
        if (!name)
            break;

        layout_name = xmlGetProp (node, NAME_ATTRIBUTE_NAME);
        if (!strcmp (name, layout_name))
            found = TRUE;
        free (layout_name);

        if (found)
            break;
    };
    return node;
}

static void
save_layout_cb (GtkWidget *w, gpointer data)
{
    GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);
    gchar         *name;

    /* get the entry text and use it as a name to save the layout */
    name = g_strdup (gtk_entry_get_text (GTK_ENTRY (layout->layout_entry)));
    g_strstrip (name);
    if (strlen (name) > 0) {
        gboolean exists;

        exists = (gdl_dock_layout_find_layout (layout, name) != NULL);
        if (!exists && strcmp (name, DEFAULT_LAYOUT)) {
            GtkTreeIter iter;

            /* add the name to the model */
            gtk_list_store_append (layout->layouts_model, &iter);
            gtk_list_store_set (layout->layouts_model, &iter,
                                COLUMN_NAME, name,
                                -1);
        };
        gdl_dock_layout_save_layout (layout, name);

    } else {
        GtkWidget *error_dialog;
        error_dialog = gtk_message_dialog_new (
            GTK_WINDOW (layout->dialog),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("You must provide a name for the layout"));
        gtk_dialog_run (GTK_DIALOG (error_dialog));
        gtk_widget_destroy (error_dialog);
    };
    g_free (name);
}

static void
load_layout_cb (GtkWidget *w, gpointer data)
{
    GtkTreeModel  *model;
    GtkTreeIter    iter;
    GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);
    gchar         *name;

    if (gtk_tree_selection_get_selected (layout->selection, &model, &iter)) {
        gtk_tree_model_get (model, &iter,
                            COLUMN_NAME, &name,
                            -1);
        gdl_dock_layout_load_layout (layout, name);
        layout->changed_by_user = TRUE;
        g_free (name);
    };
}

static void
delete_layout_cb (GtkWidget *w, gpointer data)
{
    GtkTreeModel  *model;
    GtkTreeIter    iter;
    GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);
    gchar         *name;

    if (gtk_tree_selection_get_selected (layout->selection, &model, &iter)) {
        gtk_tree_model_get (model, &iter,
                            COLUMN_NAME, &name,
                            -1);
        gdl_dock_layout_delete_layout (layout, name);
        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        g_free (name);
    };
}

static void
show_toggled_cb (GtkCellRendererToggle *renderer,
                 gchar                 *path_str,
                 gpointer               data)
{
    GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);
    GtkTreeModel  *model = GTK_TREE_MODEL (layout->items_model);
    GtkTreeIter    iter;
    GtkTreePath   *path = gtk_tree_path_new_from_string (path_str);
    gboolean       value;
    GdlDockItem   *item;

    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 
                        COLUMN_SHOW, &value, 
                        COLUMN_ITEM, &item, 
                        -1);

    value = !value;
    gtk_list_store_set (layout->items_model, &iter, COLUMN_SHOW, value, -1);
    if (value)
        gdl_dock_item_show (item);
    else
        gdl_dock_item_hide (item);

    layout->changed_by_user = TRUE;
    gtk_tree_path_free (path);
}

static void
selection_changed_cb (GtkTreeSelection *selection,
                      gpointer          data)
{
    GtkTreeModel  *model;
    GtkTreeIter    iter;
    GdlDockLayout *layout = GDL_DOCK_LAYOUT (data);
    gchar         *name;

    /* set the entry widget to the selected layout name */
    if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
        gtk_tree_model_get (model, &iter,
                            COLUMN_NAME, &name,
                            -1);
        gtk_entry_set_text (GTK_ENTRY (layout->layout_entry), name);
        g_free (name);
    };
}

static gboolean
gdl_dock_layout_construct_dialog (GdlDockLayout *layout)
{
    GladeXML          *gui;
    GtkWidget         *items_list, *layouts_list;
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;

    if (layout->dialog)
        return TRUE;

    /* load ui */
    gui = glade_xml_new (g_build_filename (
        GDL_GLADEDIR, LAYOUT_GLADE_FILE, NULL), NULL, NULL);
    if (!gui) {
        g_warning (_("Could not load layout user interface file '%s'"), 
                   LAYOUT_GLADE_FILE);
        return FALSE;
    };

    /* customize dialog */
    layout->dialog = glade_xml_get_widget (gui, "layout_dialog");
    if (!layout->dialog) {
        g_warning (_("Incorrect layout interface file '%s'"),
                   LAYOUT_GLADE_FILE);
        g_object_unref (gui);
        return FALSE;
    };
        
    gtk_dialog_add_button (GTK_DIALOG (layout->dialog), 
                           "gtk-close", GTK_RESPONSE_CLOSE);
    gtk_dialog_set_has_separator (GTK_DIALOG (layout->dialog), FALSE);
    gtk_window_set_default_size (GTK_WINDOW (layout->dialog), -1, 300);

    /* build list models */
    layout->items_model = gtk_list_store_new (3, 
                                              G_TYPE_STRING, 
                                              G_TYPE_BOOLEAN,
                                              G_TYPE_POINTER);
    gtk_tree_sortable_set_sort_column_id (
        GTK_TREE_SORTABLE (layout->items_model), 
        COLUMN_NAME, GTK_SORT_ASCENDING);

    layout->layouts_model = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_sortable_set_sort_column_id (
        GTK_TREE_SORTABLE (layout->layouts_model),
        COLUMN_NAME, GTK_SORT_ASCENDING);
    
    /* get ui widget references */
    layout->layout_entry = glade_xml_get_widget (gui, "newlayout_entry");
    items_list = glade_xml_get_widget (gui, "items_list");
    layouts_list = glade_xml_get_widget (gui, "layouts_list");

    gtk_tree_view_set_model (GTK_TREE_VIEW (items_list),
                             GTK_TREE_MODEL (layout->items_model));
    gtk_tree_view_set_model (GTK_TREE_VIEW (layouts_list),
                             GTK_TREE_MODEL (layout->layouts_model));

    /* add lists columns */
    renderer = gtk_cell_renderer_toggle_new ();
    g_signal_connect (renderer, "toggled", 
                      G_CALLBACK (show_toggled_cb), layout);
    column = gtk_tree_view_column_new_with_attributes (_("Show?"),
                                                       renderer,
                                                       "active", COLUMN_SHOW,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (items_list), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Item"),
                                                       renderer,
                                                       "text", COLUMN_NAME,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (items_list), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                       renderer,
                                                       "text", COLUMN_NAME,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (layouts_list), column);

    /* connect signals */
    layout->selection = gtk_tree_view_get_selection (
        GTK_TREE_VIEW (layouts_list));
    g_signal_connect (layout->selection, "changed",
                      G_CALLBACK (selection_changed_cb), layout);

    glade_xml_signal_connect_data (gui, "on_newlayout_entry_activate",
                                   GTK_SIGNAL_FUNC (save_layout_cb), layout);
    glade_xml_signal_connect_data (gui, "on_save_button_clicked",
                                   GTK_SIGNAL_FUNC (save_layout_cb), layout);
    glade_xml_signal_connect_data (gui, "on_load_button_clicked",
                                   GTK_SIGNAL_FUNC (load_layout_cb), layout);
    glade_xml_signal_connect_data (gui, "on_delete_button_clicked",
                                   GTK_SIGNAL_FUNC (delete_layout_cb), layout);

    g_object_unref (gui);

    return TRUE;
}

static void
gdl_dock_layout_populate_models (GdlDockLayout *layout)
{
    GList *items, *l;
    GtkTreeIter iter;

    g_return_if_fail (layout != NULL);
    g_return_if_fail (layout->items_model != NULL);
    g_return_if_fail (layout->layouts_model != NULL);

    /* build items list */
    gtk_list_store_clear (layout->items_model);
    if (layout->dock) {
        items = gdl_dock_get_named_items (layout->dock);
        for (l = items; l; l = l->next) {
            GdlDockItem *item = l->data;
            gchar *long_name;

            g_object_get (item, "long_name", &long_name, NULL);
            gtk_list_store_append (layout->items_model, &iter);
            gtk_list_store_set (layout->items_model, &iter, 
                                COLUMN_ITEM, item,
                                COLUMN_NAME, long_name,
                                COLUMN_SHOW, GDL_DOCK_ITEM_IS_SHOWN (item),
                                -1);
            g_free (long_name);
        };
        g_list_free (items);
    };

    /* build layouts list */
    gtk_list_store_clear (layout->layouts_model);
    items = gdl_dock_layout_get_layouts (layout, FALSE);
    for (l = items; l; l = l->next) {
        gtk_list_store_append (layout->layouts_model, &iter);
        gtk_list_store_set (layout->layouts_model, &iter,
                            COLUMN_NAME, l->data,
                            -1);
        free (l->data);
    };
    g_list_free (items);
}



/* ----- Public interface ----- */

GType
gdl_dock_layout_get_type (void)
{
    static GType dock_layout_type = 0;

    if (dock_layout_type == 0) {
	GTypeInfo dock_layout_info = {
	    sizeof (GdlDockLayoutClass),
	    NULL,		/* base_init  */
	    NULL,		/* base_finalize */
	    (GClassInitFunc) gdl_dock_layout_class_init, 
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof (GdlDockLayout),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) gdl_dock_layout_init,
	    NULL		/* value_table */
	};

	dock_layout_type = g_type_register_static (
	    G_TYPE_OBJECT, "GdlDockLayout", &dock_layout_info, 0);
    };

    return dock_layout_type;
}

GdlDockLayout *
gdl_dock_layout_new (GdlDock *dock)
{
    return g_object_new (GDL_TYPE_DOCK_LAYOUT, "dock", dock, NULL);
}

void
gdl_dock_layout_attach (GdlDockLayout *layout,
                        GdlDock       *dock)
{
    g_return_if_fail (layout != NULL);
    
    if (layout->dock)
        g_object_unref (layout->dock);
  
    layout->dock = dock;
    if (layout->dock)
        g_object_ref (layout->dock);
}

gboolean
gdl_dock_layout_load_layout (GdlDockLayout *layout,
                             const gchar   *name)
{
    xmlNodePtr  node;
    gchar      *layout_name;

    g_return_val_if_fail (layout != NULL, FALSE);
    
    if (!layout->doc || !layout->dock)
        return FALSE;

    if (!name)
        layout_name = DEFAULT_LAYOUT;
    else
        layout_name = (gchar *) name;

    node = gdl_dock_layout_find_layout (layout, layout_name);
    if (!node && !name)
        /* return the first layout if the default name failed to load */
        node = gdl_dock_layout_find_layout (layout, NULL);

    if (node) {
        gdl_dock_layout_load (layout->dock, node);
        return TRUE;
    } else
        return FALSE;
}

void
gdl_dock_layout_save_layout (GdlDockLayout *layout,
                             const gchar   *name)
{
    xmlNodePtr  node;
    gchar      *layout_name;

    g_return_if_fail (layout != NULL);
    g_return_if_fail (layout->dock != NULL);
    
    if (!layout->doc)
        gdl_dock_layout_build_doc (layout);

    if (!name)
        layout_name = DEFAULT_LAYOUT;
    else
        layout_name = (gchar *) name;

    /* delete any previously node with the same name */
    node = gdl_dock_layout_find_layout (layout, layout_name);
    if (node) {
        xmlUnlinkNode (node);
        xmlFreeNode (node);
    };

    /* create the new node */
    node = xmlNewChild (layout->doc->children, NULL, 
                        LAYOUT_ELEMENT_NAME, NULL);
    xmlSetProp (node, NAME_ATTRIBUTE_NAME, layout_name);

    /* save the layout */
    gdl_dock_layout_save (layout->dock, node);
    layout->dirty = TRUE;
}

void
gdl_dock_layout_delete_layout (GdlDockLayout *layout,
                               const gchar   *name)
{
    xmlNodePtr node;

    g_return_if_fail (layout != NULL);

    /* don't allow the deletion of the default layout */
    if (!name || !strcmp (DEFAULT_LAYOUT, name))
        return;
    
    node = gdl_dock_layout_find_layout (layout, name);
    if (node) {
        xmlUnlinkNode (node);
        xmlFreeNode (node);
        layout->dirty = TRUE;
    };
}

void
gdl_dock_layout_run_manager (GdlDockLayout *layout)
{
    g_return_if_fail (layout != NULL);

    if (!layout->dock)
        /* not attached to a dock yet */
        return;

    if (!gdl_dock_layout_construct_dialog (layout))
        return;

    /* fill in data */
    gdl_dock_layout_populate_models (layout);

    layout->changed_by_user = FALSE;

    gtk_dialog_run (GTK_DIALOG (layout->dialog));
    gtk_widget_hide (layout->dialog);

    if (layout->changed_by_user) {
        /* save the default (current) layout */
        gdl_dock_layout_save_layout (layout, NULL);
    };
}

gboolean
gdl_dock_layout_load_from_file (GdlDockLayout *layout,
                                const gchar   *filename)
{
    gboolean retval = FALSE;

    if (layout->doc) {
        xmlFreeDoc (layout->doc);
        layout->doc = NULL;
        layout->dirty = FALSE;
    };

    /* FIXME: cannot open symlinks */
    if (g_file_test (filename, G_FILE_TEST_IS_REGULAR)) {
        layout->doc = xmlParseFile (filename);
        if (layout->doc) {
            xmlNodePtr root = layout->doc->children;
            /* minimum validation: test the root element */
            if (root && !strcmp (root->name, ROOT_ELEMENT)) {
                retval = TRUE;
            } else {
                xmlFreeDoc (layout->doc);
                layout->doc = NULL;
            }		
        }
    }

    return retval;
}

gboolean
gdl_dock_layout_save_to_file (GdlDockLayout *layout,
                              const gchar   *filename)
{
    FILE     *file_handle;
    int       bytes;
    gboolean  retval = FALSE;

    g_return_val_if_fail (layout != NULL, FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    /* if there is still no xml doc, create an empty one */
    if (!layout->doc)
        gdl_dock_layout_build_doc (layout);

    file_handle = fopen (filename, "w");
    if (file_handle) {
        bytes = xmlDocDump (file_handle, layout->doc);
        if (bytes >= 0) {
            layout->dirty = FALSE;
            retval = TRUE;
        };
        fclose (file_handle);
    };

    return retval;
}

gboolean
gdl_dock_layout_is_dirty (GdlDockLayout *layout)
{
    g_return_val_if_fail (layout != NULL, FALSE);

    return layout->dirty;
};

GList *
gdl_dock_layout_get_layouts (GdlDockLayout *layout,
                             gboolean       include_default)
{
    GList      *retval = NULL;
    xmlNodePtr  node;

    g_return_val_if_fail (layout != NULL, NULL);

    if (!layout->doc)
        return NULL;

    node = layout->doc->children;
    for (node = node->children; node; node = node->next) {
        gchar *name;

        if (strcmp (node->name, LAYOUT_ELEMENT_NAME))
            continue;

        name = xmlGetProp (node, NAME_ATTRIBUTE_NAME);
        if (include_default || strcmp (name, DEFAULT_LAYOUT))
            /* FIXME: is libxml mem allocation completely compatible 
               with glib's? */
            retval = g_list_prepend (retval, name);
        else
            free (name);
    };
    retval = g_list_reverse (retval);

    return retval;
}
