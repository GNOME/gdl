/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * gdl-dock-tablabel.c
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Gir�ldez <gustavo.giraldez@gmx.net>
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
#include <libgnome/gnome-macros.h>
#include <gtk/gtk.h>

#include "gdl-dock-tablabel.h"
#include "gdl-tools.h"
#include "gdl-dock-item.h"
#include "libgdlmarshal.h"


/* ----- Private prototypes ----- */

static void  gdl_dock_tablabel_class_init    (GdlDockTablabelClass *klass);
static void  gdl_dock_tablabel_instance_init (GdlDockTablabel      *tablabel);

static void  gdl_dock_tablabel_set_property  (GObject              *object,
                                              guint                 prop_id,
                                              const GValue         *value,
                                              GParamSpec           *pspec);
static void  gdl_dock_tablabel_get_property  (GObject              *object,
                                              guint                 prop_id,
                                              GValue               *value,
                                              GParamSpec           *pspec);

static void  gdl_dock_tablabel_item_notify   (GObject            *master,
                                              GParamSpec         *pspec,
                                              gpointer            data);

static void  gdl_dock_tablabel_size_request  (GtkWidget          *widget,
                                              GtkRequisition     *requisition);
static void  gdl_dock_tablabel_size_allocate (GtkWidget          *widget,
                                              GtkAllocation      *allocation);
                                              
static void  gdl_dock_tablabel_paint         (GtkWidget      *widget,
                                              GdkEventExpose *event);
static gint  gdl_dock_tablabel_expose        (GtkWidget      *widget,
                                              GdkEventExpose *event);

static gboolean gdl_dock_tablabel_button_event  (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean gdl_dock_tablabel_motion_event  (GtkWidget      *widget,
                                                 GdkEventMotion *event);

static void  gdl_dock_tablabel_realize (GtkWidget *widget);
static void  gdl_dock_tablabel_unrealize (GtkWidget *widget);
static void  gdl_dock_tablabel_map (GtkWidget *widget);
static void  gdl_dock_tablabel_unmap (GtkWidget *widget);

/* ----- Private data types and variables ----- */

#define DEFAULT_DRAG_HANDLE_SIZE 10
#define HANDLE_RATIO 1.0

enum {
    BUTTON_PRESSED_HANDLE,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_ITEM
};


static guint dock_tablabel_signals [LAST_SIGNAL] = { 0 };


/* ----- Private interface ----- */

GNOME_CLASS_BOILERPLATE (GdlDockTablabel, gdl_dock_tablabel,
                         GtkBin, GTK_TYPE_BIN);

static void
gdl_dock_tablabel_class_init (GdlDockTablabelClass *klass)
{
    GObjectClass      *g_object_class;
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    g_object_class = G_OBJECT_CLASS (klass);
    object_class = GTK_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);
    container_class = GTK_CONTAINER_CLASS (klass);
    
    g_object_class->set_property = gdl_dock_tablabel_set_property;
    g_object_class->get_property = gdl_dock_tablabel_get_property;

    widget_class->size_request = gdl_dock_tablabel_size_request;
    widget_class->size_allocate = gdl_dock_tablabel_size_allocate;
    widget_class->expose_event = gdl_dock_tablabel_expose;
    widget_class->button_press_event = gdl_dock_tablabel_button_event;
    widget_class->button_release_event = gdl_dock_tablabel_button_event;
    widget_class->motion_notify_event = gdl_dock_tablabel_motion_event;
    widget_class->realize = gdl_dock_tablabel_realize;
    widget_class->unrealize = gdl_dock_tablabel_unrealize;
    widget_class->map = gdl_dock_tablabel_map;
    widget_class->unmap = gdl_dock_tablabel_unmap;

    g_object_class_install_property (
        g_object_class, PROP_ITEM,
        g_param_spec_object ("item", _("Controlling dock item"),
                             _("Dockitem which 'owns' this tablabel"),
                             GDL_TYPE_DOCK_ITEM,
                             G_PARAM_READWRITE));

    dock_tablabel_signals [BUTTON_PRESSED_HANDLE] =
        g_signal_new ("button_pressed_handle",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GdlDockTablabelClass, 
                                       button_pressed_handle),
                      NULL, NULL,
                      gdl_marshal_VOID__BOXED,
                      G_TYPE_NONE,
                      1,
                      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

    klass->button_pressed_handle = NULL;
}

static void
gdl_dock_tablabel_instance_init (GdlDockTablabel *tablabel)
{
    GtkWidget *widget;
    GtkWidget *label_widget;

    widget = GTK_WIDGET (tablabel);

    tablabel->drag_handle_size = DEFAULT_DRAG_HANDLE_SIZE;
    tablabel->item = NULL;

    label_widget = gtk_label_new ("Dock item");
    gtk_container_add (GTK_CONTAINER (tablabel), label_widget);
    gtk_widget_show (label_widget);

    tablabel->active = FALSE;
    gtk_widget_set_state (GTK_WIDGET (tablabel), GTK_STATE_ACTIVE);
}

static void
gdl_dock_tablabel_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    GdlDockTablabel *tablabel;
    GtkBin          *bin;

    tablabel = GDL_DOCK_TABLABEL (object);

    switch (prop_id) {
        case PROP_ITEM:
            if (tablabel->item) {
                g_object_remove_weak_pointer (G_OBJECT (tablabel->item), 
                                              (gpointer *) &tablabel->item);
                g_signal_handlers_disconnect_by_func (
                    tablabel->item, gdl_dock_tablabel_item_notify, tablabel);
            };

            tablabel->item = g_value_get_object (value);
            if (tablabel->item) {
                gboolean locked;
                gchar   *long_name;
                
                g_object_add_weak_pointer (G_OBJECT (tablabel->item), 
                                           (gpointer *) &tablabel->item);

                g_signal_connect (tablabel->item, "notify::locked",
                                  G_CALLBACK (gdl_dock_tablabel_item_notify),
                                  tablabel);
                g_signal_connect (tablabel->item, "notify::long_name",
                                  G_CALLBACK (gdl_dock_tablabel_item_notify),
                                  tablabel);
                g_signal_connect (tablabel->item, "notify::grip_size",
                                  G_CALLBACK (gdl_dock_tablabel_item_notify),
                                  tablabel);

                g_object_get (tablabel->item,
                              "locked", &locked,
                              "long_name", &long_name,
                              "grip_size", &tablabel->drag_handle_size,
                              NULL);

                if (locked)
                    tablabel->drag_handle_size = 0;
                
                bin = GTK_BIN (tablabel);
                if (bin->child && g_object_class_find_property (
                    G_OBJECT_GET_CLASS (bin->child), "label"))
                    g_object_set (bin->child, "label", long_name, NULL);
                g_free (long_name);
            };
            break;
            
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_tablabel_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    GdlDockTablabel *tablabel;

    tablabel = GDL_DOCK_TABLABEL (object);

    switch (prop_id) {
        case PROP_ITEM:
            g_value_set_object (value, tablabel->item);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_tablabel_item_notify (GObject    *master,
                               GParamSpec *pspec,
                               gpointer    data)
{
    GdlDockTablabel *tablabel = GDL_DOCK_TABLABEL (data);
    gboolean         locked;
    gchar           *label;
    GtkBin          *bin;
    
    g_object_get (master,
                  "locked", &locked,
                  "grip_size", &tablabel->drag_handle_size,
                  "long_name", &label,
                  NULL);

    if (locked)
        tablabel->drag_handle_size = 0;

    bin = GTK_BIN (tablabel);
    if (bin->child && g_object_class_find_property (
        G_OBJECT_GET_CLASS (bin->child), "label"))
        g_object_set (bin->child, "label", label, NULL);
    g_free (label);

    gtk_widget_queue_resize (GTK_WIDGET (tablabel));
}

static void
gdl_dock_tablabel_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
    GtkBin          *bin;
    GtkRequisition   child_req;
    GdlDockTablabel *tablabel;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_TABLABEL (widget));
    g_return_if_fail (requisition != NULL);

    tablabel = GDL_DOCK_TABLABEL (widget);
    bin = GTK_BIN (widget);

    requisition->width = tablabel->drag_handle_size;
    requisition->height = 0;

    if (bin->child)
        gtk_widget_size_request (bin->child, &child_req);
    else
        child_req.width = child_req.height = 0;
        
    requisition->width += child_req.width;
    requisition->height += child_req.height;

    requisition->width += GTK_CONTAINER (widget)->border_width * 2;
    requisition->height += GTK_CONTAINER (widget)->border_width * 2;

    widget->requisition = *requisition;
}

static void
gdl_dock_tablabel_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
    GtkBin          *bin;
    GdlDockTablabel *tablabel;
    gint             border_width;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_TABLABEL (widget));
    g_return_if_fail (allocation != NULL);
  
    bin = GTK_BIN (widget);
    tablabel = GDL_DOCK_TABLABEL (widget);

    border_width = GTK_CONTAINER (widget)->border_width;
  
    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize (tablabel->event_window, 
                                allocation->x, 
                                allocation->y,
                                allocation->width, 
                                allocation->height);

    if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
        GtkAllocation  child_allocation;

        child_allocation.x = widget->allocation.x + border_width;
        child_allocation.y = widget->allocation.y + border_width;

        allocation->width = MAX (1, (int) allocation->width - 
                                 (int) tablabel->drag_handle_size);
        child_allocation.x += tablabel->drag_handle_size;

        child_allocation.width = 
            MAX (1, (int) allocation->width - 2 * border_width);
        child_allocation.height = 
            MAX (1, (int) allocation->height - 2 * border_width);

        gtk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
gdl_dock_tablabel_paint (GtkWidget      *widget,
                         GdkEventExpose *event)
{
    GdkRectangle     dest, rect;
    GtkBin          *bin;
    GdlDockTablabel *tablabel;
    gint             border_width;

    bin = GTK_BIN (widget);
    tablabel = GDL_DOCK_TABLABEL (widget);
    border_width = GTK_CONTAINER (widget)->border_width;

    rect.x = widget->allocation.x + border_width;
    rect.y = widget->allocation.y + border_width;
    rect.width = tablabel->drag_handle_size * HANDLE_RATIO;
    rect.height = widget->allocation.height - 2*border_width;

    if (gdk_rectangle_intersect (&event->area, &rect, &dest)) {
        gtk_paint_handle (widget->style, widget->window, 
                          tablabel->active ? GTK_STATE_NORMAL : GTK_STATE_ACTIVE, 
                          GTK_SHADOW_NONE,
                          &dest, widget, "dock_tablabel",
                          rect.x, rect.y, rect.width, rect.height,
                          GTK_ORIENTATION_VERTICAL);
    };
}

static gint
gdl_dock_tablabel_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_TABLABEL (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget)) {
        GNOME_CALL_PARENT (GTK_WIDGET_CLASS, expose_event, (widget, event));
        gdl_dock_tablabel_paint (widget, event);
    };
  
    return FALSE;
}

static gboolean 
gdl_dock_tablabel_button_event (GtkWidget      *widget,
                                GdkEventButton *event)
{
    GdlDockTablabel *tablabel;
    gboolean         event_handled;
  
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_TABLABEL (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    
    tablabel = GDL_DOCK_TABLABEL (widget);
    
    event_handled = FALSE;

    if (event->window != tablabel->event_window)
        return FALSE;
    
    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (tablabel->active) {
                gboolean in_handle;
                gint     rel_x, rel_y;
                guint    border_width;
                GtkBin  *bin;

                bin = GTK_BIN (widget);
                border_width = GTK_CONTAINER (widget)->border_width;

                rel_x = event->x - border_width;
                rel_y = event->y - border_width;

                /* Check if user clicked on the drag handle. */      
                in_handle = (rel_x < tablabel->drag_handle_size * HANDLE_RATIO) &&
                    (rel_x > 0);

                if (event->button == 1) {
                    tablabel->pre_drag = TRUE;
                    tablabel->drag_start_event = *event;
                }
                else {
                    g_signal_emit (widget, 
                                   dock_tablabel_signals [BUTTON_PRESSED_HANDLE],
                                   0,
                                   event);
                }
                
                event_handled = TRUE;
            }
            break;

        case GDK_BUTTON_RELEASE:
            tablabel->pre_drag = FALSE;
            break;

        default:
            break;
    }
    
    if (!event_handled) {
        /* propagate the event to the parent's gdkwindow */
        GdkEventButton e;

        e = *event;
        e.window = gtk_widget_get_parent_window (widget);
        e.x += widget->allocation.x;
        e.y += widget->allocation.y;
        
        gdk_event_put ((GdkEvent *) &e);
    };

    return event_handled;
}

static gboolean 
gdl_dock_tablabel_motion_event (GtkWidget      *widget,
                                GdkEventMotion *event)
{
    GdlDockTablabel *tablabel;
    gboolean         event_handled;
  
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_TABLABEL (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    
    tablabel = GDL_DOCK_TABLABEL (widget);
    
    event_handled = FALSE;

    if (event->window != tablabel->event_window)
        return FALSE;
    
    if (tablabel->pre_drag) {
        if (gtk_drag_check_threshold (widget,
                                      tablabel->drag_start_event.x,
                                      tablabel->drag_start_event.y,
                                      event->x,
                                      event->y)) {
            tablabel->pre_drag = FALSE;
            g_signal_emit (widget, 
                           dock_tablabel_signals [BUTTON_PRESSED_HANDLE],
                           0,
                           &tablabel->drag_start_event);
            event_handled = TRUE;
        }
    }
    
    if (!event_handled) {
        /* propagate the event to the parent's gdkwindow */
        GdkEventMotion e;

        e = *event;
        e.window = gtk_widget_get_parent_window (widget);
        e.x += widget->allocation.x;
        e.y += widget->allocation.y;
        
        gdk_event_put ((GdkEvent *) &e);
    };

    return event_handled;
}

static void   
gdl_dock_tablabel_realize (GtkWidget *widget)
{
    GdlDockTablabel *tablabel;
    GdkWindowAttr attributes;
    int attributes_mask;
    
    tablabel = GDL_DOCK_TABLABEL (widget);
    
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_ONLY;
    attributes.event_mask = gtk_widget_get_events (widget);
    attributes.event_mask |= (GDK_EXPOSURE_MASK | 
                              GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK | 
                              GDK_ENTER_NOTIFY_MASK | 
                              GDK_POINTER_MOTION_MASK | 
                              GDK_LEAVE_NOTIFY_MASK);
    attributes_mask = GDK_WA_X | GDK_WA_Y;
    
    widget->window = gtk_widget_get_parent_window (widget);
    g_object_ref (widget->window);
    
    tablabel->event_window = 
        gdk_window_new (gtk_widget_get_parent_window (widget),
                        &attributes, attributes_mask);
    gdk_window_set_user_data (tablabel->event_window, widget);
    
    widget->style = gtk_style_attach (widget->style, widget->window);
    
    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
}

static void   
gdl_dock_tablabel_unrealize (GtkWidget *widget)
{
    GdlDockTablabel *tablabel = GDL_DOCK_TABLABEL (widget);
    
    if (tablabel->event_window) {
        gdk_window_set_user_data (tablabel->event_window, NULL);
        gdk_window_destroy (tablabel->event_window);
        tablabel->event_window = NULL;
    }
    
    GNOME_CALL_PARENT (GTK_WIDGET_CLASS, unrealize, (widget));
}

static void  
gdl_dock_tablabel_map (GtkWidget *widget)
{
    GdlDockTablabel *tablabel = GDL_DOCK_TABLABEL (widget);
    
    GNOME_CALL_PARENT (GTK_WIDGET_CLASS, map, (widget));
    
    gdk_window_show (tablabel->event_window);
}

static void   
gdl_dock_tablabel_unmap (GtkWidget *widget)
{
    GdlDockTablabel *tablabel = GDL_DOCK_TABLABEL (widget);

    gdk_window_hide (tablabel->event_window);

    GNOME_CALL_PARENT (GTK_WIDGET_CLASS, unmap, (widget));
}

/* ----- Public interface ----- */

GtkWidget *
gdl_dock_tablabel_new (GdlDockItem *item)
{
    GdlDockTablabel *tablabel;

    tablabel = GDL_DOCK_TABLABEL (g_object_new (GDL_TYPE_DOCK_TABLABEL,
                                                "item", item,
                                                NULL));
    
    return GTK_WIDGET (tablabel);
}

void
gdl_dock_tablabel_activate (GdlDockTablabel *tablabel)
{
    g_return_if_fail (tablabel != NULL);

    tablabel->active = TRUE;
    gtk_widget_set_state (GTK_WIDGET (tablabel), GTK_STATE_NORMAL);
}

void
gdl_dock_tablabel_deactivate (GdlDockTablabel *tablabel)
{
    g_return_if_fail (tablabel != NULL);

    tablabel->active = FALSE;
    /* yeah, i know it contradictive */
    gtk_widget_set_state (GTK_WIDGET (tablabel), GTK_STATE_ACTIVE);
}
