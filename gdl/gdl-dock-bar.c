/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2003 Jeroen Zwartepoorte <jeroen@xs4all.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gdl-i18n.h"
#include <stdlib.h>
#include <string.h>

#include "gdl-tools.h"
#include "gdl-dock.h"
#include "gdl-dock-master.h"
#include "gdl-dock-bar.h"

enum {
    PROP_0,
    PROP_MASTER,
};

/* ----- Private prototypes ----- */

static void  gdl_dock_bar_class_init      (GdlDockBarClass *klass);
static void  gdl_dock_bar_instance_init   (GdlDockBar      *dockbar);

static void  gdl_dock_bar_get_property    (GObject         *object,
                                           guint            prop_id,
                                           GValue          *value,
                                           GParamSpec      *pspec);
static void  gdl_dock_bar_set_property    (GObject         *object,
                                           guint            prop_id,
                                           const GValue    *value,
                                           GParamSpec      *pspec);

static void  gdl_dock_bar_destroy         (GtkObject       *object);

static void  gdl_dock_bar_attach          (GdlDockBar      *dockbar,
                                           GdlDockMaster   *master);

/* ----- Class variables and definitions ----- */

struct _GdlDockBarPrivate {
    GdlDockMaster *master;
    GSList        *items;
    GtkTooltips   *tooltips;
};

/* ----- Private functions ----- */

GDL_CLASS_BOILERPLATE (GdlDockBar, gdl_dock_bar, GtkVBox, GTK_TYPE_VBOX);

static void
gdl_dock_bar_class_init (GdlDockBarClass *klass)
{
    GObjectClass       *g_object_class;
    GtkObjectClass     *gtk_object_class;
    
    g_object_class = G_OBJECT_CLASS (klass);
    gtk_object_class = GTK_OBJECT_CLASS (klass);

    g_object_class->get_property = gdl_dock_bar_get_property;
    g_object_class->set_property = gdl_dock_bar_set_property;

    gtk_object_class->destroy = gdl_dock_bar_destroy;

    g_object_class_install_property (
        g_object_class, PROP_MASTER,
        g_param_spec_object ("master", _("Master"),
                             _("GdlDockMaster object which the dockbar widget "
                               "is attached to"),
                             GDL_TYPE_DOCK_MASTER, 
                             G_PARAM_READWRITE));
}

static void
gdl_dock_bar_instance_init (GdlDockBar *dockbar)
{
    dockbar->_priv = g_new0 (GdlDockBarPrivate, 1);
    dockbar->_priv->master = NULL;
    dockbar->_priv->items = NULL;
    dockbar->_priv->tooltips = gtk_tooltips_new ();
    g_object_ref (dockbar->_priv->tooltips);
    gtk_object_sink (GTK_OBJECT (dockbar->_priv->tooltips));
}

static void
gdl_dock_bar_get_property (GObject         *object,
                           guint            prop_id,
                           GValue          *value,
                           GParamSpec      *pspec)
{
    GdlDockBar *dockbar = GDL_DOCK_BAR (object);

    switch (prop_id) {
        case PROP_MASTER:
            g_value_set_object (value, dockbar->_priv->master);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    };
}

static void
gdl_dock_bar_set_property (GObject         *object,
                           guint            prop_id,
                           const GValue    *value,
                           GParamSpec      *pspec)
{
    GdlDockBar *dockbar = GDL_DOCK_BAR (object);

    switch (prop_id) {
        case PROP_MASTER:
            gdl_dock_bar_attach (dockbar, g_value_get_object (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    };
}

static void
gdl_dock_bar_destroy (GtkObject *object)
{
    GdlDockBar *dockbar = GDL_DOCK_BAR (object);

    if (dockbar->_priv) {
        GdlDockBarPrivate *priv = dockbar->_priv;
        
        if (priv->master) {
            g_signal_handlers_disconnect_matched (priv->master,
                                                  G_SIGNAL_MATCH_DATA,
                                                  0, 0, NULL, NULL, dockbar);
            g_object_unref (priv->master);
            priv->master = NULL;
        }

        if (priv->tooltips) {
            g_object_unref (priv->tooltips);
            priv->tooltips = NULL;
        }
        
        dockbar->_priv = NULL;

        g_free (priv);
    }
    
    GDL_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gdl_dock_bar_remove_item (GdlDockBar  *dockbar,
                          GdlDockItem *item)
{
    GdlDockBarPrivate *priv;
    GtkWidget *button;

    g_return_if_fail (GDL_IS_DOCK_BAR (dockbar));
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    priv = dockbar->_priv;

    if (g_slist_index (priv->items, item) == -1) {
        g_warning ("Item has not been added to the dockbar");
        return;
    }
    
    priv->items = g_slist_remove (priv->items, item);
    
    button = g_object_get_data (G_OBJECT (item), "GdlDockBarButton");
    g_assert (button != NULL);
    gtk_container_remove (GTK_CONTAINER (dockbar), button);
    g_object_set_data (G_OBJECT (item), "GdlDockBarButton", NULL);
}

static void
gdl_dock_bar_item_clicked (GtkWidget   *button,
                           GdlDockItem *item)
{
    GdlDockBar *dockbar;
    GdlDockObject *controller;

    g_return_if_fail (item != NULL);
    
    dockbar = g_object_get_data (G_OBJECT (item), "GdlDockBar");
    g_assert (dockbar != NULL);
    g_object_set_data (G_OBJECT (item), "GdlDockBar", NULL);

    controller = gdl_dock_master_get_controller (GDL_DOCK_OBJECT_GET_MASTER (item));

    GDL_DOCK_OBJECT_UNSET_FLAGS (item, GDL_DOCK_ICONIFIED);
    gdl_dock_item_show_item (item);
    gdl_dock_bar_remove_item (dockbar, item);
    gtk_widget_queue_resize (GTK_WIDGET (controller));
}

static void
gdl_dock_bar_add_item (GdlDockBar  *dockbar,
                       GdlDockItem *item)
{
    GdlDockBarPrivate *priv;
    GtkWidget *button;
    gchar *stock_id;
    gchar *name;
    GtkWidget *image;

    g_return_if_fail (GDL_IS_DOCK_BAR (dockbar));
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    priv = dockbar->_priv;

    if (g_slist_index (priv->items, item) != -1) {
        g_warning ("Item has already been added to the dockbar");
        return;
    }

    priv->items = g_slist_append (priv->items, item);
    
    /* Create a button for the item. */
    button = gtk_button_new ();
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    
    g_object_get (item, "stock_id", &stock_id, "long_name", &name, NULL);
    if (stock_id) {
        image = gtk_image_new_from_stock (stock_id,
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
        g_free (stock_id);
    } else {
        image = gtk_image_new_from_stock (GTK_STOCK_NEW,
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
    }
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_box_pack_start (GTK_BOX (dockbar), button, FALSE, FALSE, 0);

    gtk_tooltips_set_tip (priv->tooltips, button, name, name);
    g_free (name);

    g_object_set_data (G_OBJECT (item), "GdlDockBar", dockbar);
    g_object_set_data (G_OBJECT (item), "GdlDockBarButton", button);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (gdl_dock_bar_item_clicked), item);

    gtk_widget_show_all (button);
}

static void
build_list (GdlDockObject *object, GList **list)
{
    /* add only items, not toplevels */
    if (GDL_IS_DOCK_ITEM (object))
        *list = g_list_prepend (*list, object);
}

static void
update_dock_items (GdlDockBar *dockbar)
{
    GdlDockMaster *master;
    GList *items, *l;

    g_return_if_fail (dockbar != NULL);
    
    if (!dockbar->_priv->master)
        return;

    master = dockbar->_priv->master;
    
    /* build items list */
    items = NULL;
    gdl_dock_master_foreach (master, (GFunc) build_list, &items);
    
    for (l = items; l != NULL; l = l->next) {
        GdlDockItem *item = GDL_DOCK_ITEM (l->data);
        
        if (g_slist_index (dockbar->_priv->items, item) != -1 &&
            !GDL_DOCK_ITEM_ICONIFIED (item))
	    gdl_dock_bar_remove_item (dockbar, item);
	else if (g_slist_index (dockbar->_priv->items, item) == -1 &&
	    GDL_DOCK_ITEM_ICONIFIED (item))
	    gdl_dock_bar_add_item (dockbar, item);
    }
    
    g_list_free (items);
}

static void
gdl_dock_bar_layout_changed_cb (GdlDockMaster *master,
                                GdlDockBar    *dockbar)
{
    update_dock_items (dockbar);
}

static void
gdl_dock_bar_attach (GdlDockBar    *dockbar,
                     GdlDockMaster *master)
{
    g_return_if_fail (dockbar != NULL);
    g_return_if_fail (master == NULL || GDL_IS_DOCK_MASTER (master));
    
    if (dockbar->_priv->master) {
        g_signal_handlers_disconnect_matched (dockbar->_priv->master,
                                              G_SIGNAL_MATCH_DATA,
                                              0, 0, NULL, NULL, dockbar);
        g_object_unref (dockbar->_priv->master);
    }
    
    dockbar->_priv->master = master;
    if (dockbar->_priv->master) {
        g_object_ref (dockbar->_priv->master);
        g_signal_connect (dockbar->_priv->master, "layout_changed",
                          G_CALLBACK (gdl_dock_bar_layout_changed_cb),
                          dockbar);
    }

    update_dock_items (dockbar);
}

GtkWidget *
gdl_dock_bar_new (GdlDock *dock)
{
    GdlDockMaster *master = NULL;
    
    /* get the master of the given dock */
    if (dock)
        master = GDL_DOCK_OBJECT_GET_MASTER (dock);

    return g_object_new (GDL_TYPE_DOCK_BAR,
                         "master", master, NULL);
}
