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

#include "gdl-i18n.h"
#include "gdl-switcher.h"

#include "gdl-dock-notebook.h"
#include "gdl-dock-tablabel.h"

/**
 * SECTION:gdl-dock-notebook
 * @title: GdlDockNotebook
 * @short_description: Group dock widgets in a notebook
 * @stability: Unstable
 * @see_also: #GdlDockPaned, #GdlDockMaster
 *
 * A #GdlDockNotebook is a compound dock widget like #GdlDockPaned.
 * Other dock widgets can be added to it, simply by dropping them on the
 * widget. Contrary to the #GdlDockPaned, a #GdlDockNotebook can contain
 * only simple dock widget.
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
           "padding: 2;\n"
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

    object_class->is_compound = TRUE;
    object_class->dock = gdl_dock_notebook_dock;
    object_class->child_placement = gdl_dock_notebook_child_placement;
    object_class->present = gdl_dock_notebook_present;
    object_class->reorder = gdl_dock_notebook_reorder;

    item_class->has_grip = FALSE;
    item_class->set_orientation = gdl_dock_notebook_set_orientation;

    g_object_class_install_property (
        g_object_class, PROP_PAGE,
        g_param_spec_int ("page", _("Page"),
                          _("The index of the current page"),
                          0, G_MAXINT,
                          0,
                          G_PARAM_READWRITE |
                          GDL_DOCK_PARAM_EXPORT | GDL_DOCK_PARAM_AFTER));

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
    if (event->type == GDK_BUTTON_PRESS)
        GDL_DOCK_ITEM_SET_FLAGS (user_data, GDL_DOCK_USER_ACTION);
    else
        GDL_DOCK_ITEM_UNSET_FLAGS (user_data, GDL_DOCK_USER_ACTION);

    return FALSE;
}

static void
gdl_dock_notebook_init (GdlDockNotebook *notebook)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (notebook);

    /* create the container notebook */
    item->child = gdl_switcher_new ();
    gtk_widget_set_parent (item->child, GTK_WIDGET (notebook));
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (item->child), GTK_POS_BOTTOM);
    g_signal_connect (item->child, "switch-page",
                      (GCallback) gdl_dock_notebook_switch_page_cb, (gpointer) item);
    g_signal_connect (item->child, "notify::page",
                      (GCallback) gdl_dock_notebook_notify_cb, (gpointer) item);
    g_signal_connect (item->child, "button-press-event",
                      (GCallback) gdl_dock_notebook_button_cb, (gpointer) item);
    g_signal_connect (item->child, "button-release-event",
                      (GCallback) gdl_dock_notebook_button_cb, (gpointer) item);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (item->child), TRUE);
    gtk_widget_show (item->child);
}

static void
gdl_dock_notebook_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);

    switch (prop_id) {
        case PROP_PAGE:
            if (item->child && GTK_IS_NOTEBOOK (item->child)) {
                gtk_notebook_set_current_page (GTK_NOTEBOOK (item->child),
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

    switch (prop_id) {
        case PROP_PAGE:
            if (item->child && GTK_IS_NOTEBOOK (item->child)) {
                g_value_set_int (value, gtk_notebook_get_current_page
                                 (GTK_NOTEBOOK (item->child)));
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
    if (item->child) {
        gtk_widget_unparent (item->child);
        item->child = NULL;
    };
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

    /* deactivate old tablabel */
    if (current_page) {
        tablabel = gtk_notebook_get_tab_label (
            nb, gtk_notebook_get_nth_page (
                nb, current_page));
        if (tablabel && GDL_IS_DOCK_TABLABEL (tablabel))
            gdl_dock_tablabel_deactivate (GDL_DOCK_TABLABEL (tablabel));
    };

    /* activate new label */
    tablabel = gtk_notebook_get_tab_label (
        nb, page);
    if (tablabel && GDL_IS_DOCK_TABLABEL (tablabel))
        gdl_dock_tablabel_activate (GDL_DOCK_TABLABEL (tablabel));

    if (GDL_DOCK_ITEM_USER_ACTION (notebook) &&
        GDL_DOCK_OBJECT (notebook)->master)
        g_signal_emit_by_name (GDL_DOCK_OBJECT (notebook)->master,
                               "layout-changed");

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
        if (item->child)
            gtk_container_foreach (GTK_CONTAINER (item->child), callback, callback_data);
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
#if 0
            if (GDL_IS_DOCK_TABLABEL (label)) {
                gdl_dock_tablabel_deactivate (GDL_DOCK_TABLABEL (label));
                /* hide the item grip, as we will use the tablabel's */
                gdl_dock_item_hide_grip (requestor_item);
            }
#endif

            if (other_data && G_VALUE_HOLDS (other_data, G_TYPE_INT))
                position = g_value_get_int (other_data);

            position = gdl_switcher_insert_page (GDL_SWITCHER (item->child),
                                                 GTK_WIDGET (requestor), label,
                                                 long_name, long_name,
                                                 stock_id, pixbuf_icon, position);

            GDL_DOCK_OBJECT_SET_FLAGS (requestor, GDL_DOCK_ATTACHED);

            /* Set current page to the newly docked widget. set current page
             * really doesn't work if the page widget is not shown
             */
            gtk_widget_show (GTK_WIDGET (requestor));
            gtk_notebook_set_current_page (GTK_NOTEBOOK (item->child),
                                           position);
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
    if (item->child && GTK_IS_NOTEBOOK (item->child)) {
        if (orientation == GTK_ORIENTATION_HORIZONTAL)
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (item->child), GTK_POS_TOP);
        else
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (item->child), GTK_POS_LEFT);
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

    if (item->child) {
        GList *children, *l;

        children = gtk_container_get_children (GTK_CONTAINER (item->child));
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
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    int i;

    i = gtk_notebook_page_num (GTK_NOTEBOOK (item->child),
                               GTK_WIDGET (child));
    if (i >= 0)
        gtk_notebook_set_current_page (GTK_NOTEBOOK (item->child), i);

    GDL_DOCK_OBJECT_CLASS (gdl_dock_notebook_parent_class)->present (object, child);
}

static gboolean
gdl_dock_notebook_reorder (GdlDockObject    *object,
                           GdlDockObject    *requestor,
                           GdlDockPlacement  new_position,
                           GValue           *other_data)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    gint         current_position, new_pos = -1;
    gboolean     handled = FALSE;

    if (item->child && new_position == GDL_DOCK_CENTER) {
        current_position = gtk_notebook_page_num (GTK_NOTEBOOK (item->child),
                                                  GTK_WIDGET (requestor));
        if (current_position >= 0) {
            handled = TRUE;

            if (other_data && G_VALUE_HOLDS (other_data, G_TYPE_INT))
                new_pos = g_value_get_int (other_data);

            gtk_notebook_reorder_child (GTK_NOTEBOOK (item->child),
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
 * Creates a new GDL notebook. This widget is normally created and destroyed
 * automatically when needed by the master.
 *
 * Returns: The newly created #GdlDockNotebook.
 */
GtkWidget *
gdl_dock_notebook_new (void)
{
    GdlDockNotebook *notebook;

    notebook = GDL_DOCK_NOTEBOOK (g_object_new (GDL_TYPE_DOCK_NOTEBOOK, NULL));
    GDL_DOCK_OBJECT_UNSET_FLAGS (notebook, GDL_DOCK_AUTOMATIC);

    return GTK_WIDGET (notebook);
}
