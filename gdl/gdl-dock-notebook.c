/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include "gdl-switcher.h"

#include "gdl-dock-notebook.h"

/**
 * SECTION:gdl-dock-notebook
 * @title: GdlDockNotebook
 * @short_description: Arrange dock widgets in a tabbed notebook
 * @stability: Unstable
 * @see_also: #GdlDockPaned, #GdlDockMaster, #GdlSwitcher
 *
 * A #GdlDockNotebook is a compound dock widget. It can dock
 * an unlimited number of widget displaying them in a notebook. This dock
 * widget is normally created automatically when a child is docked in
 * the center of another one.
 * A #GdlDockNotebook cannot contain other compound widgets, like a #GdlDockPaned.
 *
 * A #GdlDockNotebook derives from #GdlDockItem and contains a #GdlSwitcher
 * used to display all children.
 */


/* Private prototypes */

static void  gdl_dock_notebook_class_init    (GdlDockNotebookClass *klass);
static void  gdl_dock_notebook_set_property  (GObject              *object,
                                              guint                 prop_id,
                                              const GValue         *value,
                                              GParamSpec           *pspec);
static void  gdl_dock_notebook_get_property  (GObject              *object,
                                              guint                 prop_id,
                                              GValue               *value,
                                              GParamSpec           *pspec);

static void  gdl_dock_notebook_destroy       (GtkWidget    *object);

static void  gdl_dock_notebook_add           (GtkContainer *container,
					      GtkWidget    *widget);
static void  gdl_dock_notebook_forall        (GtkContainer *container,
					      gboolean      include_internals,
					      GtkCallback   callback,
					      gpointer      callback_data);
static GType gdl_dock_notebook_child_type    (GtkContainer *container);

static void  gdl_dock_notebook_dock          (GdlDockObject    *object,
                                              GdlDockObject    *requestor,
                                              GdlDockPlacement  position,
                                              GValue           *other_data);

static void  gdl_dock_notebook_switch_page_cb  (GtkNotebook     *nb,
                                                GtkWidget       *page,
                                                gint             page_num,
                                                gpointer         data);

static void  gdl_dock_notebook_set_orientation (GdlDockItem     *item,
                                                GtkOrientation   orientation);

static gboolean gdl_dock_notebook_child_placement (GdlDockObject    *object,
                                                   GdlDockObject    *child,
                                                   GdlDockPlacement *placement);

static void     gdl_dock_notebook_present         (GdlDockObject    *object,
                                                   GdlDockObject    *child);

static gboolean gdl_dock_notebook_reorder         (GdlDockObject    *object,
                                                   GdlDockObject    *requestor,
                                                   GdlDockPlacement  new_position,
                                                   GValue           *other_data);


/* Class variables and definitions */

struct _GdlDockNotebookClassPrivate {
    GtkCssProvider *css;
};

enum {
    PROP_0,
    PROP_PAGE
};

struct _GdlDockNotebookPrivate {
    gboolean    user_action;
};

/* ----- Private functions ----- */

G_DEFINE_TYPE_WITH_CODE (GdlDockNotebook, gdl_dock_notebook, GDL_TYPE_DOCK_ITEM,
                         g_type_add_class_private (g_define_type_id, sizeof (GdlDockNotebookClassPrivate)))

static void
gdl_dock_notebook_class_init (GdlDockNotebookClass *klass)
{
    GObjectClass       *g_object_class;
    GtkWidgetClass     *widget_class;
    GtkContainerClass  *container_class;
    GdlDockObjectClass *object_class;
    GdlDockItemClass   *item_class;
    static const gchar notebook_style[] =
       "* {\n"
           "padding: 2px;\n"
       "}";

    g_object_class = G_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);
    container_class = GTK_CONTAINER_CLASS (klass);
    object_class = GDL_DOCK_OBJECT_CLASS (klass);
    item_class = GDL_DOCK_ITEM_CLASS (klass);

    g_object_class->set_property = gdl_dock_notebook_set_property;
    g_object_class->get_property = gdl_dock_notebook_get_property;

    widget_class->destroy = gdl_dock_notebook_destroy;

    container_class->add = gdl_dock_notebook_add;
    container_class->forall = gdl_dock_notebook_forall;
    container_class->child_type = gdl_dock_notebook_child_type;

    gdl_dock_object_class_set_is_compound (object_class, TRUE);
    object_class->dock = gdl_dock_notebook_dock;
    object_class->child_placement = gdl_dock_notebook_child_placement;
    object_class->present = gdl_dock_notebook_present;
    object_class->reorder = gdl_dock_notebook_reorder;

    gdl_dock_item_class_set_has_grip (item_class, FALSE);
    item_class->set_orientation = gdl_dock_notebook_set_orientation;

    g_object_class_install_property (
        g_object_class, PROP_PAGE,
        g_param_spec_int ("page", _("Page"),
                          _("The index of the current page"),
                          -1, G_MAXINT,
                          -1,
                          G_PARAM_READWRITE |
                          GDL_DOCK_PARAM_EXPORT | GDL_DOCK_PARAM_AFTER));

    g_type_class_add_private (object_class, sizeof (GdlDockNotebookPrivate));

    /* set the style */
    klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GDL_TYPE_DOCK_NOTEBOOK, GdlDockNotebookClassPrivate);

    klass->priv->css = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (klass->priv->css, notebook_style, -1, NULL);
}

static void
gdl_dock_notebook_notify_cb (GObject    *g_object,
                             GParamSpec *pspec,
                             gpointer    user_data)
{
    g_return_if_fail (user_data != NULL && GDL_IS_DOCK_NOTEBOOK (user_data));

    /* chain the notify signal */
    g_object_notify (G_OBJECT (user_data), pspec->name);
}

static gboolean
gdl_dock_notebook_button_cb (GtkWidget      *widget,
                             GdkEventButton *event,
                             gpointer        user_data)
{
    GDL_DOCK_NOTEBOOK (user_data)->priv->user_action = event->type == GDK_BUTTON_PRESS;

    return FALSE;
}

static void
gdl_dock_notebook_init (GdlDockNotebook *notebook)
{
    GdlDockItem *item = GDL_DOCK_ITEM (notebook);
    GtkWidget *child;

    notebook->priv = G_TYPE_INSTANCE_GET_PRIVATE (notebook,
                                                  GDL_TYPE_DOCK_NOTEBOOK,
                                                  GdlDockNotebookPrivate);
    notebook->priv->user_action = FALSE;

    /* create the container notebook */
    child = gdl_switcher_new ();
    gdl_dock_item_set_child (item, child);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (child), GTK_POS_BOTTOM);
    g_signal_connect (child, "switch-page",
                      (GCallback) gdl_dock_notebook_switch_page_cb, (gpointer) item);
    g_signal_connect (child, "notify::page",
                      (GCallback) gdl_dock_notebook_notify_cb, (gpointer) item);
    g_signal_connect (child, "button-press-event",
                      (GCallback) gdl_dock_notebook_button_cb, (gpointer) item);
    g_signal_connect (child, "button-release-event",
                      (GCallback) gdl_dock_notebook_button_cb, (gpointer) item);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (child), TRUE);
    gtk_widget_show (child);
}

static void
gdl_dock_notebook_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    GtkWidget *child;

    switch (prop_id) {
        case PROP_PAGE:
            child = gdl_dock_item_get_child (item);
            if (child && GTK_IS_NOTEBOOK (child)) {
                gtk_notebook_set_current_page (GTK_NOTEBOOK (child),
                                               g_value_get_int (value));
            }

            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_notebook_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    GtkWidget *child;

    switch (prop_id) {
        case PROP_PAGE:
            child = gdl_dock_item_get_child (item);
            if (child && GTK_IS_NOTEBOOK (child)) {
                g_value_set_int (value, gtk_notebook_get_current_page
                                 (GTK_NOTEBOOK (child)));
            }

            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
gdl_dock_notebook_destroy (GtkWidget *object)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);

    /* we need to call the virtual first, since in GdlDockDestroy our
       children dock objects are detached */
    GTK_WIDGET_CLASS (gdl_dock_notebook_parent_class)->destroy (object);

    /* after that we can remove the GtkNotebook */
    gdl_dock_item_set_child (item, NULL);
}

static void
gdl_dock_notebook_switch_page_cb (GtkNotebook     *nb,
                                  GtkWidget       *page,
                                  gint             page_num,
                                  gpointer         data)
{
    GdlDockNotebook *notebook;
    gint            current_page;
    GtkWidget       *tablabel;
    GdlDockItem     *current_item;
    GdlDockItem     *new_item;

    notebook = GDL_DOCK_NOTEBOOK (data);
    current_page = gtk_notebook_get_current_page (nb);

    if (notebook->priv->user_action)
        gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (notebook));

    /* Signal that the old dock has been deselected */
    current_item = GDL_DOCK_ITEM (gtk_notebook_get_nth_page (nb, current_page));

    gdl_dock_item_notify_deselected (current_item);

    /* Signal that a new dock item has been selected */
    new_item = GDL_DOCK_ITEM (gtk_notebook_get_nth_page (nb, page_num));
    gdl_dock_item_notify_selected (new_item);
}

static void
gdl_dock_notebook_add (GtkContainer *container,
		       GtkWidget    *widget)
{
    g_return_if_fail (container != NULL && widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (container));
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    gdl_dock_object_dock (GDL_DOCK_OBJECT (container),
                          GDL_DOCK_OBJECT (widget),
                          GDL_DOCK_CENTER,
                          NULL);
}

static void
gdl_dock_notebook_forall (GtkContainer *container,
			  gboolean      include_internals,
			  GtkCallback   callback,
			  gpointer      callback_data)
{
    GdlDockItem *item;
    GtkWidget *child;

    g_return_if_fail (container != NULL);
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (container));
    g_return_if_fail (callback != NULL);

    if (include_internals) {
        /* use GdlDockItem's forall */
        GTK_CONTAINER_CLASS (gdl_dock_notebook_parent_class)->forall
            (container, include_internals, callback, callback_data);
    }
    else {
        item = GDL_DOCK_ITEM (container);
        child = gdl_dock_item_get_child (GDL_DOCK_ITEM (container));
        if (child)
            gtk_container_foreach (GTK_CONTAINER (child), callback, callback_data);
    }
}

static GType
gdl_dock_notebook_child_type (GtkContainer *container)
{
    return GDL_TYPE_DOCK_ITEM;
}

static void
gdl_dock_notebook_dock_child (GdlDockObject *requestor,
                              gpointer       user_data)
{
    struct {
        GdlDockObject    *object;
        GdlDockPlacement  position;
        GValue           *other_data;
    } *data = user_data;

    gdl_dock_object_dock (data->object, requestor, data->position, data->other_data);
}

static void
gdl_dock_notebook_dock (GdlDockObject    *object,
                        GdlDockObject    *requestor,
                        GdlDockPlacement  position,
                        GValue           *other_data)
{
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (object));
    g_return_if_fail (GDL_IS_DOCK_ITEM (requestor));

    /* we only add support for GDL_DOCK_CENTER docking strategy here... for the rest
       use our parent class' method */
    if (position == GDL_DOCK_CENTER) {
        /* we can only dock simple (not compound) items */
        if (gdl_dock_object_is_compound (requestor)) {
            struct {
                GdlDockObject    *object;
                GdlDockPlacement  position;
                GValue           *other_data;
            } data;

            gdl_dock_object_freeze (requestor);

            data.object = object;
            data.position = position;
            data.other_data = other_data;

            gtk_container_foreach (GTK_CONTAINER (requestor),
                                   (GtkCallback) gdl_dock_notebook_dock_child, &data);

            gdl_dock_object_thaw (requestor);
        }
        else {
            GdlDockItem *item = GDL_DOCK_ITEM (object);
            GdlDockItem *requestor_item = GDL_DOCK_ITEM (requestor);
            gchar       *long_name, *stock_id;
            GdkPixbuf   *pixbuf_icon;
            GtkWidget   *label;
            gint         position = -1;

            g_object_get (requestor_item, "long-name", &long_name,
                          "stock-id", &stock_id, "pixbuf-icon", &pixbuf_icon, NULL);
            label = gdl_dock_item_get_tablabel (requestor_item);
            if (!label) {
                label = gtk_label_new (long_name);
                gdl_dock_item_set_tablabel (requestor_item, label);
            }

            if (other_data && G_VALUE_HOLDS (other_data, G_TYPE_INT))
                position = g_value_get_int (other_data);

            position = gdl_switcher_insert_page (GDL_SWITCHER (gdl_dock_item_get_child (item)),
                                                 GTK_WIDGET (requestor), label,
                                                 long_name, long_name,
                                                 stock_id, pixbuf_icon, position);

            if (gtk_widget_get_visible (GTK_WIDGET (requestor))) {
                /* Set current page to the newly docked widget. set current page
                 * really doesn't work if the page widget is not shown
                 */
                    gtk_notebook_set_current_page (GTK_NOTEBOOK (gdl_dock_item_get_child (item)),
                                                   position);
            }
            g_free (long_name);
            g_free (stock_id);
        }
    }
    else
        GDL_DOCK_OBJECT_CLASS (gdl_dock_notebook_parent_class)->dock (object, requestor, position, other_data);
}

static void
gdl_dock_notebook_set_orientation (GdlDockItem    *item,
                                   GtkOrientation  orientation)
{
    GtkWidget *child = gdl_dock_item_get_child (item);

    if (child && GTK_IS_NOTEBOOK (child)) {
        if (orientation == GTK_ORIENTATION_HORIZONTAL)
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (child), GTK_POS_TOP);
        else
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (child), GTK_POS_LEFT);
    }

    GDL_DOCK_ITEM_CLASS (gdl_dock_notebook_parent_class)->set_orientation (item, orientation);
}

static gboolean
gdl_dock_notebook_child_placement (GdlDockObject    *object,
                                   GdlDockObject    *child,
                                   GdlDockPlacement *placement)
{
    GdlDockItem      *item = GDL_DOCK_ITEM (object);
    GdlDockPlacement  pos = GDL_DOCK_NONE;

    if (gdl_dock_item_get_child (item)) {
        GList *children, *l;

        children = gtk_container_get_children (GTK_CONTAINER (gdl_dock_item_get_child (item)));
        for (l = children; l; l = l->next) {
            if (l->data == (gpointer) child) {
                pos = GDL_DOCK_CENTER;
                break;
            }
        }
        g_list_free (children);
    }

    if (pos != GDL_DOCK_NONE) {
        if (placement)
            *placement = pos;
        return TRUE;
    }
    else
        return FALSE;
}

static void
gdl_dock_notebook_present (GdlDockObject *object,
                           GdlDockObject *child)
{
    GtkWidget *notebook = gdl_dock_item_get_child (GDL_DOCK_ITEM (object));
    int i;

    i = gtk_notebook_page_num (GTK_NOTEBOOK (notebook),
                               GTK_WIDGET (child));
    if (i >= 0)
        gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), i);

    GDL_DOCK_OBJECT_CLASS (gdl_dock_notebook_parent_class)->present (object, child);
}

static gboolean
gdl_dock_notebook_reorder (GdlDockObject    *object,
                           GdlDockObject    *requestor,
                           GdlDockPlacement  new_position,
                           GValue           *other_data)
{
    GtkWidget *child = gdl_dock_item_get_child (GDL_DOCK_ITEM (object));
    gint         current_position, new_pos = -1;
    gboolean     handled = FALSE;

    if (child && new_position == GDL_DOCK_CENTER) {
        current_position = gtk_notebook_page_num (GTK_NOTEBOOK (child),
                                                  GTK_WIDGET (requestor));
        if (current_position >= 0) {
            handled = TRUE;

            if (other_data && G_VALUE_HOLDS (other_data, G_TYPE_INT))
                new_pos = g_value_get_int (other_data);

            gtk_notebook_reorder_child (GTK_NOTEBOOK (child),
                                        GTK_WIDGET (requestor),
                                        new_pos);
        }
    }
    return handled;
}

/* ----- Public interface ----- */

/**
 * gdl_dock_notebook_new:
 *
 * Creates a new manual #GdlDockNotebook widget. This function is seldom useful as
 * such widget is normally created and destroyed automatically when needed by
 * the master.
 *
 * Returns: The newly created #GdlDockNotebook.
 */
GtkWidget *
gdl_dock_notebook_new (void)
{
    GdlDockNotebook *notebook;

    notebook = GDL_DOCK_NOTEBOOK (g_object_new (GDL_TYPE_DOCK_NOTEBOOK, NULL));
    gdl_dock_object_set_manual (GDL_DOCK_OBJECT (notebook));

    return GTK_WIDGET (notebook);
}
