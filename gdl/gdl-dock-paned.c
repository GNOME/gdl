/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* GdlDockPaned - A docking paned based on EPaned

 * EPaned - A slightly more advanced paned widget.
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Author: Christopher James Lahey <clahey@helixcode.com>
 *
 * based on GtkPaned from Gtk+.  Gtk+ Copyright notice follows.
 */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "gdl-tools.h"
#include "gdl-dock.h"
#include "gdl-dock-paned.h"


enum {
    PROP_0,
    PROP_HANDLE_SIZE,
    PROP_QUANTUM,
};


static void     gdl_dock_paned_class_init     (GdlDockPanedClass  *klass);
static void     gdl_dock_paned_init           (GdlDockPaned       *paned);
static void     gdl_dock_paned_set_property   (GObject        *object,
                                               guint           prop_id,
                                               const GValue   *value,
                                               GParamSpec     *pspec);
static void     gdl_dock_paned_get_property   (GObject        *object,
                                               guint           prop_id,
                                               GValue         *value,
                                               GParamSpec     *pspec);
static void     gdl_dock_paned_realize        (GtkWidget      *widget);
static void     gdl_dock_paned_map            (GtkWidget      *widget);
static void     gdl_dock_paned_unmap          (GtkWidget      *widget);
static void     gdl_dock_paned_unrealize      (GtkWidget      *widget);
static void     gdl_dock_paned_paint          (GtkWidget      *widget,
                                               GdkRectangle   *area);
static gint     gdl_dock_paned_expose         (GtkWidget      *widget,
                                               GdkEventExpose *event);
static void     gdl_dock_paned_add            (GtkContainer *container,
                                               GtkWidget    *widget);
static void     gdl_dock_paned_remove         (GtkContainer *container,
                                               GtkWidget    *widget);
static void     gdl_dock_paned_forall         (GtkContainer *container,
                                               gboolean      include_internals,
                                               GtkCallback   callback,
                                               gpointer      callback_data);
static void     gdl_dock_paned_style_set      (GtkWidget *widget,
                                               GtkStyle  *previous_style);
static GtkType  gdl_dock_paned_child_type     (GtkContainer *container);
static void     gdl_dock_paned_size_request   (GtkWidget      *widget,
                                               GtkRequisition *requisition);
static void     gdl_dock_paned_size_allocate  (GtkWidget     *widget,
                                               GtkAllocation *allocation);
static void     gdl_dock_paned_xor_line       (GdlDockPaned *paned);
static gboolean gdl_dock_paned_button_press   (GtkWidget      *widget,
                                               GdkEventButton *event);
static gboolean gdl_dock_paned_button_release (GtkWidget      *widget,
                                               GdkEventButton *event);
static gboolean gdl_dock_paned_motion         (GtkWidget      *widget,
                                               GdkEventMotion *event);
static gboolean gdl_dock_paned_handle_shown   (GdlDockPaned *paned);
static void     gdl_dock_paned_compute_position (GdlDockPaned *paned,
                                                 gint allocation,
                                                 gint child1_req,
                                                 gint child2_req);
static gint     gdl_dock_paned_quantized_size  (GdlDockPaned *paned,
                                                int           size);
static void     gdl_dock_paned_auto_reduce     (GdlDockItem *item);
static gboolean gdl_dock_paned_dock_request    (GdlDockItem        *item, 
                                                gint                x,
                                                gint                y, 
                                                GdlDockRequestInfo *target);
static void     gdl_dock_paned_set_orientation (GdlDockItem    *item,
                                                GtkOrientation  orientation);
static void     gdl_dock_paned_layout_save     (GdlDockItem *item,
                                                xmlNodePtr   node);
static void     gdl_dock_paned_hide            (GdlDockItem *item);

static gchar   *gdl_dock_paned_get_pos_hint    (GdlDockItem      *item,
                                                GdlDockItem      *caller,
                                                GdlDockPlacement *position);

#define DEFAULT_DRAG_HANDLE_SIZE 5

static GdlDockItemClass *parent_class = NULL;


/* Private functions */

static void
gdl_dock_paned_class_init (GdlDockPanedClass *klass)
{
    GObjectClass *g_object_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    GdlDockItemClass *dock_item_class;

    g_object_class = (GObjectClass *) klass;
    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;
    dock_item_class = (GdlDockItemClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    g_object_class->set_property = gdl_dock_paned_set_property;
    g_object_class->get_property = gdl_dock_paned_get_property;

    widget_class->realize = gdl_dock_paned_realize;
    widget_class->map = gdl_dock_paned_map;
    widget_class->unmap = gdl_dock_paned_unmap;
    widget_class->unrealize = gdl_dock_paned_unrealize;
    widget_class->expose_event = gdl_dock_paned_expose;
    widget_class->style_set = gdl_dock_paned_style_set;

    widget_class->size_request = gdl_dock_paned_size_request;
    widget_class->size_allocate = gdl_dock_paned_size_allocate;
    widget_class->button_press_event = gdl_dock_paned_button_press;
    widget_class->button_release_event = gdl_dock_paned_button_release;
    widget_class->motion_notify_event = gdl_dock_paned_motion;

    container_class->add = gdl_dock_paned_add;
    container_class->remove = gdl_dock_paned_remove;
    container_class->forall = gdl_dock_paned_forall;
    container_class->child_type = gdl_dock_paned_child_type;

    dock_item_class->auto_reduce = gdl_dock_paned_auto_reduce;
    dock_item_class->dock_request = gdl_dock_paned_dock_request;
    dock_item_class->set_orientation = gdl_dock_paned_set_orientation;
    dock_item_class->save_layout = gdl_dock_paned_layout_save;
    dock_item_class->item_hide = gdl_dock_paned_hide;
    dock_item_class->get_pos_hint = gdl_dock_paned_get_pos_hint;

    g_object_class_install_property (
        g_object_class, PROP_HANDLE_SIZE,
        g_param_spec_uint ("handle_size", _("Handle size"),
                           _("Size in pixels of the paned separator"),
                           0, 100, DEFAULT_DRAG_HANDLE_SIZE,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        g_object_class, PROP_QUANTUM,
        g_param_spec_uint ("quantum", _("Quantum"),
                           _("Quantum in pixels for splits"),
                           0, 100, 1,
                           G_PARAM_READWRITE));
}

static GtkType
gdl_dock_paned_child_type (GtkContainer *container)
{
    if (!GDL_DOCK_PANED (container)->child1 || 
        !GDL_DOCK_PANED (container)->child2)
        return GTK_TYPE_WIDGET;
    else
        return G_TYPE_NONE;
}

static void
gdl_dock_paned_init (GdlDockPaned *paned)
{
    GTK_WIDGET_UNSET_FLAGS (paned, GTK_NO_WINDOW);
  
    paned->child1 = NULL;
    paned->child2 = NULL;
    paned->handle = NULL;
    paned->xor_gc = NULL;
    paned->cursor_type = GDK_CROSS;
  
    paned->handle_width = DEFAULT_DRAG_HANDLE_SIZE;
    paned->handle_height = DEFAULT_DRAG_HANDLE_SIZE;
    paned->handle_size = DEFAULT_DRAG_HANDLE_SIZE;
    paned->position_set = FALSE;
    paned->last_allocation = -1;
    paned->in_drag = FALSE;
  
    paned->handle_xpos = -1;
    paned->handle_ypos = -1;
  
    paned->old_child1_size = 0;
    paned->child1_size = 0;
    paned->quantum = 1;
}

static void
gdl_dock_paned_set_property (GObject        *object,
                             guint           prop_id,
                             const GValue   *value,
                             GParamSpec     *pspec)
{
    GdlDockPaned *paned = GDL_DOCK_PANED (object);
  
    switch (prop_id) {
    case PROP_HANDLE_SIZE:
        gdl_dock_paned_set_handle_size (paned, g_value_get_uint (value));
        break;
    case PROP_QUANTUM:
        paned->quantum = g_value_get_uint (value);
        if (paned->quantum == 0)
            paned->quantum = 1;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gdl_dock_paned_get_property (GObject        *object,
                             guint           prop_id,
                             GValue         *value,
                             GParamSpec     *pspec)
{
    GdlDockPaned *paned = GDL_DOCK_PANED (object);

    switch (prop_id) {
    case PROP_HANDLE_SIZE:
        g_value_set_uint (value, paned->handle_size);
        break;
    case PROP_QUANTUM:
        g_value_set_uint (value, paned->quantum);
        break;
    default: 
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gdl_dock_paned_realize (GtkWidget *widget)
{
    GdlDockPaned  *paned;
    GdkWindowAttr  attributes;
    gint           attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
    paned = GDL_DOCK_PANED (widget);

    /* widget window */
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new (gtk_widget_get_parent_window(widget),
                                     &attributes, attributes_mask);
    gdk_window_set_user_data (widget->window, paned);

    /* handle window */
    attributes.x = paned->handle_xpos;
    attributes.y = paned->handle_ypos;
    attributes.width = paned->handle_width;
    attributes.height = paned->handle_height;
    attributes.cursor = gdk_cursor_new (paned->cursor_type);
    attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK |
                              GDK_POINTER_MOTION_HINT_MASK);
    attributes_mask |= GDK_WA_CURSOR;

    paned->handle = gdk_window_new (widget->window,
                                    &attributes, attributes_mask);
    gdk_window_set_user_data (paned->handle, paned);
    gdk_cursor_destroy (attributes.cursor);

    widget->style = gtk_style_attach (widget->style, widget->window);

    gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
    gtk_style_set_background (widget->style, paned->handle, GTK_STATE_NORMAL);

    gdk_window_set_back_pixmap (widget->window, NULL, TRUE);

    if (gdl_dock_paned_handle_shown (paned))
        gdk_window_show (paned->handle);
}

static void
gdl_dock_paned_style_set (GtkWidget *widget, 
                          GtkStyle  *previous_style)
{
    GdlDockPaned *paned;

    g_return_if_fail (widget != NULL);

    paned = GDL_DOCK_PANED (widget);

    if (GTK_WIDGET_REALIZED (widget)) {
        gtk_style_set_background (widget->style, widget->window, 
                                  GTK_STATE_NORMAL);
        gtk_style_set_background (widget->style, paned->handle, 
                                  GTK_STATE_NORMAL);

        gdk_window_clear (paned->handle);
        gdk_window_clear (widget->window);
    }
}

static void
gdl_dock_paned_map (GtkWidget *widget)
{
    GdlDockPaned *paned;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
    paned = GDL_DOCK_PANED (widget);

    if (paned->child1 && 
        GTK_WIDGET_VISIBLE (paned->child1) &&
        !GTK_WIDGET_MAPPED (paned->child1))
        gtk_widget_map (paned->child1);
    if (paned->child2 &&
        GTK_WIDGET_VISIBLE (paned->child2) &&
        !GTK_WIDGET_MAPPED (paned->child2))
        gtk_widget_map (paned->child2);

    gdk_window_show (widget->window);
}

static void
gdl_dock_paned_unmap (GtkWidget *widget)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (widget));

    GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

    gdk_window_hide (widget->window);
}

static void
gdl_dock_paned_unrealize (GtkWidget *widget)
{
    GdlDockPaned *paned;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (widget));

    paned = GDL_DOCK_PANED (widget);

    if (paned->xor_gc) {
        gdk_gc_destroy (paned->xor_gc);
        paned->xor_gc = NULL;
    }

    if (paned->handle) {
        gdk_window_set_user_data (paned->handle, NULL);
        gdk_window_destroy (paned->handle);
        paned->handle = NULL;
    }

    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
        (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
gdl_dock_paned_paint (GtkWidget    *widget,
                      GdkRectangle *area)
{
    GdlDockPaned *paned;
    guint16       border_width;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (widget));

    if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget)) {
        paned = GDL_DOCK_PANED (widget);
        border_width = GTK_CONTAINER (paned)->border_width;

        gdk_window_clear_area (widget->window,
                               area->x, area->y, area->width,
                               area->height);
    }
}

static gint
gdl_dock_paned_expose (GtkWidget      *widget,
                       GdkEventExpose *event)
{
    GdlDockPaned   *paned;
    GdkEventExpose  child_event;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_PANED (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (GTK_WIDGET_DRAWABLE (widget)) {
        paned = GDL_DOCK_PANED (widget);

        if (paned->handle && event->window == paned->handle) {
            if (gdl_dock_paned_handle_shown (paned)) {
                child_event = *event;
                event->area.x += paned->handle_xpos;
                event->area.y += paned->handle_ypos;
                gdl_dock_paned_paint (widget, &event->area);
	    }

	} else {
            child_event = *event;
            if (paned->child1 &&
                GTK_WIDGET_NO_WINDOW (paned->child1) &&
                gtk_widget_intersect (paned->child1, &event->area, 
                                      &child_event.area))
                gtk_widget_event (paned->child1, (GdkEvent *) &child_event);

            if (paned->child2 &&
                GTK_WIDGET_NO_WINDOW (paned->child2) &&
                gtk_widget_intersect (paned->child2, &event->area, 
                                      &child_event.area))
                gtk_widget_event (paned->child2, (GdkEvent *) &child_event);
	}
    }

    return FALSE;
}

static void
gdl_dock_paned_add (GtkContainer *container,
                    GtkWidget    *widget)
{
    GdlDockPaned *paned;

    g_return_if_fail (container != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (container));
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    paned = GDL_DOCK_PANED (container);

    if (!paned->child1)
        gdl_dock_paned_add1 (GDL_DOCK_PANED (container), widget);
    else if (!paned->child2)
        gdl_dock_paned_add2 (GDL_DOCK_PANED (container), widget);
}

static void
gdl_dock_paned_remove (GtkContainer *container,
                       GtkWidget    *widget)
{
    GdlDockPaned *paned;
    gboolean      was_visible;

    g_return_if_fail (container != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (container));
    g_return_if_fail (widget != NULL);

    paned = GDL_DOCK_PANED (container);
    was_visible = GTK_WIDGET_VISIBLE (widget);

    if (paned->child1 == widget) {
        gtk_widget_unparent (widget);

        paned->child1 = NULL;

        if (was_visible && GTK_WIDGET_VISIBLE (container))
            gtk_widget_queue_resize (GTK_WIDGET (container));

    } else if (paned->child2 == widget) {
        gtk_widget_unparent (widget);

        paned->child2 = NULL;

        if (was_visible && GTK_WIDGET_VISIBLE (container))
            gtk_widget_queue_resize (GTK_WIDGET (container));
    }
}

static void
gdl_dock_paned_forall (GtkContainer *container,
                       gboolean      include_internals,
                       GtkCallback   callback,
                       gpointer      callback_data)
{
    GdlDockPaned *paned;

    g_return_if_fail (container != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (container));
    g_return_if_fail (callback != NULL);

    paned = GDL_DOCK_PANED (container);

    if (paned->child1)
        (*callback) (paned->child1, callback_data);
    if (paned->child2)
        (*callback) (paned->child2, callback_data);
}

static void
gdl_dock_paned_compute_position (GdlDockPaned *paned,
                                 gint          allocation,
                                 gint          child1_req,
                                 gint          child2_req)
{
    gboolean child1_resize, child1_shrink;
    gboolean child2_resize, child2_shrink;

    g_return_if_fail (paned != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (paned));
  
    child1_resize = child1_shrink = child2_resize = child2_shrink = TRUE;
    if (paned->child1)
        g_object_get (paned->child1, 
                      "resize", &child1_resize,
                      "shrink", &child1_shrink,
                      NULL);

    if (paned->child2)
        g_object_get (paned->child2, 
                      "resize", &child2_resize,
                      "shrink", &child2_shrink,
                      NULL);

    if (gdl_dock_paned_handle_shown (paned))
        allocation -= (gint) paned->handle_size;

    paned->min_position = child1_shrink ? 0 : child1_req;

    paned->max_position = allocation;
    if (!child2_shrink)
        paned->max_position = MAX (1, paned->max_position - child2_req);

    if (!paned->position_set) {
        if (child1_resize && !child2_resize)
            paned->child1_size = MAX (1, allocation - child2_req);
        else if (!child1_resize && child2_resize)
            paned->child1_size = child1_req;
        else if (child1_req + child2_req != 0)
            paned->child1_size = allocation * ((gdouble) child1_req / 
                                               (child1_req + child2_req));
        else
            paned->child1_size = allocation * 0.5;
    } else {
        /* If the position was set before the initial allocation.
         * (paned->last_allocation <= 0) just clamp it and leave it.
         */
        if (paned->last_allocation > 0) {
            if (child1_resize && !child2_resize)
                paned->child1_size += allocation - paned->last_allocation;
            else if (child1_resize && child2_resize)
                paned->child1_size = allocation * 
                    ((gdouble) paned->child1_size / (paned->last_allocation));
	}
    }

    paned->child1_real_size = CLAMP (paned->child1_size,
                                     paned->min_position,
                                     paned->max_position);

    paned->last_allocation = allocation;
}

static gboolean
gdl_dock_paned_handle_shown (GdlDockPaned *paned)
{
    return ((paned->child1 && paned->child2) &&
            (GTK_WIDGET_VISIBLE (paned->child1) && 
             GTK_WIDGET_VISIBLE (paned->child2)));
}

static gint
gdl_dock_paned_quantized_size (GdlDockPaned *paned,
                               gint          size)
{
    gint quantization = size - paned->old_child1_size;

    if (quantization > 0)
        quantization += paned->quantum / 2;
    else
        quantization -= paned->quantum / 2;
    quantization /= paned->quantum;
    quantization *= paned->quantum;
    return paned->old_child1_size + quantization;
}

static void
gdl_dock_paned_size_request (GtkWidget      *widget,
                             GtkRequisition *requisition)
{
    GdlDockPaned   *paned;
    GtkRequisition  child_requisition;
    gboolean        is_horizontal;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (widget));
    g_return_if_fail (requisition != NULL);

    paned = GDL_DOCK_PANED (widget);
    requisition->width = 0;
    requisition->height = 0;

    is_horizontal = (GDL_DOCK_ITEM (paned)->orientation == 
                     GTK_ORIENTATION_HORIZONTAL);

    if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1)) {
        gtk_widget_size_request (paned->child1, &child_requisition);

        requisition->height = child_requisition.height;
        requisition->width = child_requisition.width;
    }

    if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2)) {
        gtk_widget_size_request (paned->child2, &child_requisition);

        if (is_horizontal) {
            requisition->height = MAX (requisition->height, 
                                       child_requisition.height);
            requisition->width += child_requisition.width;
        } else {
            requisition->width = MAX (requisition->width, 
                                      child_requisition.width);
            requisition->height += child_requisition.height;
        };
    }

    requisition->width += GTK_CONTAINER (paned)->border_width * 2;
    requisition->height += GTK_CONTAINER (paned)->border_width * 2;
    if (gdl_dock_paned_handle_shown (paned)) {
        if (is_horizontal)
            requisition->width += paned->handle_size;
        else
            requisition->height += paned->handle_size;
    };
}

static void
gdl_dock_paned_size_allocate (GtkWidget     *widget,
                              GtkAllocation *allocation)
{
    GdlDockPaned   *paned;
    GtkRequisition  child1_requisition;
    GtkRequisition  child2_requisition;
    GtkAllocation   child1_allocation;
    GtkAllocation   child2_allocation;
    gint            border_width;
    gboolean        handle_shown;
    gboolean        is_horizontal;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (widget));
    g_return_if_fail (allocation != NULL);

    paned = GDL_DOCK_PANED (widget);

    is_horizontal = (GDL_DOCK_ITEM (paned)->orientation == 
                     GTK_ORIENTATION_HORIZONTAL);

    widget->allocation = *allocation;
    border_width = GTK_CONTAINER (paned)->border_width;
  
    if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1))
        gtk_widget_get_child_requisition (paned->child1, &child1_requisition);
    else {
        if (is_horizontal) 
            child1_requisition.width = 0;
        else
            child1_requisition.height = 0;
    };

    if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
        gtk_widget_get_child_requisition (paned->child2, &child2_requisition);
    else {
        if (is_horizontal) 
            child2_requisition.width = 0;
        else
            child2_requisition.height = 0;
    };

    if (is_horizontal) {
        gdl_dock_paned_compute_position 
            (paned, MAX (1, (gint) widget->allocation.width - 
                         2 * border_width),
             child1_requisition.width,
             child2_requisition.width);
    } else {
        gdl_dock_paned_compute_position 
            (paned, MAX (1, (gint) widget->allocation.height - 
                         2 * border_width),
             child1_requisition.height,
             child2_requisition.height);
    };

    /* Move the handle before the children so we don't get extra 
       expose events */

    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize (widget->window,
                                allocation->x, allocation->y,
                                allocation->width,
                                allocation->height);
  
    handle_shown = gdl_dock_paned_handle_shown (paned);
    if (handle_shown) {
        if (is_horizontal) {
            paned->handle_xpos = paned->child1_real_size + border_width;
            paned->handle_ypos = border_width;
            paned->handle_width = paned->handle_size;
            paned->handle_height = MAX (1, (gint) widget->allocation.height - 
                                        2 * border_width);
        } else {
            paned->handle_xpos = border_width;
            paned->handle_ypos = paned->child1_real_size + border_width;
            paned->handle_width = MAX (1, (gint) widget->allocation.width - 
                                       2 * border_width);
            paned->handle_height = paned->handle_size;
        };

        if (GTK_WIDGET_REALIZED (widget)) {
            gdk_window_move_resize (paned->handle,
                                    paned->handle_xpos,
                                    paned->handle_ypos,
                                    paned->handle_width,
                                    paned->handle_height);
            if (paned->handle)
                gdk_window_show (paned->handle);
	}
    } else {
        if (paned->handle && GTK_WIDGET_REALIZED (widget))
            gdk_window_hide (paned->handle);
    }

    if (is_horizontal) {
        child1_allocation.height = child2_allocation.height = 
            MAX (1, ((int) allocation->height - border_width * 2));
        child1_allocation.width = MAX (1, paned->child1_real_size);
        child1_allocation.x = border_width;
        child1_allocation.y = child2_allocation.y = border_width;
        if (handle_shown)
            child2_allocation.x = (child1_allocation.x + 
                                   (int) child1_allocation.width +
                                   (int) paned->handle_width);
        else
            child2_allocation.x = (child1_allocation.x + 
                                   (int) child1_allocation.width);

        child2_allocation.width = MAX (1, (gint) allocation->width - 
                                       child2_allocation.x - border_width);

    } else {
        child1_allocation.width = child2_allocation.width = 
            MAX (1, ((int) allocation->width - border_width * 2));
        child1_allocation.height = MAX (1, paned->child1_real_size);
        child1_allocation.y = border_width;
        child1_allocation.x = child2_allocation.x = border_width;
        if (handle_shown)
            child2_allocation.y = (child1_allocation.y + 
                                   (int) child1_allocation.height +
                                   (int) paned->handle_height);
        else
            child2_allocation.y = (child1_allocation.y + 
                                   (int) child1_allocation.height);

        child2_allocation.height = MAX (1, (gint) allocation->height - 
                                        child2_allocation.y - border_width);

    };
  
    /* Now allocate the childen, making sure, when resizing not to
     * overlap the windows */
    if (GTK_WIDGET_MAPPED (widget) &&
        paned->child1 && GTK_WIDGET_VISIBLE (paned->child1) &&
        is_horizontal && 
        paned->child1->allocation.width < child1_allocation.width) {

        if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
            gtk_widget_size_allocate (paned->child2, 
                                      &child2_allocation);
        gtk_widget_size_allocate (paned->child1, &child1_allocation);

    } else if (GTK_WIDGET_MAPPED (widget) &&
               paned->child1 && GTK_WIDGET_VISIBLE (paned->child1) &&
               !is_horizontal && 
               paned->child1->allocation.height < child1_allocation.height) {

        if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
            gtk_widget_size_allocate (paned->child2, 
                                      &child2_allocation);
        gtk_widget_size_allocate (paned->child1, &child1_allocation);

    } else {
        if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1))
            gtk_widget_size_allocate (paned->child1, &child1_allocation);
        if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
            gtk_widget_size_allocate (paned->child2, &child2_allocation);
    }
}

static void
gdl_dock_paned_xor_line (GdlDockPaned *paned)
{
    GtkWidget *widget;
    guint16    half_size;
    gint8      dash_list [2];

    widget = GTK_WIDGET (paned);

    if (!paned->xor_gc) {
        if (GTK_WIDGET_REALIZED (GTK_WIDGET (paned))) {
            GdkGCValues values;

            values.function = GDK_INVERT;
            values.subwindow_mode = GDK_INCLUDE_INFERIORS;
            paned->xor_gc = gdk_gc_new_with_values 
                (widget->window, &values, GDK_GC_FUNCTION | GDK_GC_SUBWINDOW);

        } else 
            return;
    }

    gdk_gc_set_line_attributes (paned->xor_gc, 1, GDK_LINE_ON_OFF_DASH,
                                GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);

    dash_list[0] = 1;
    dash_list[1] = 1;
    gdk_gc_set_dashes (paned->xor_gc, 1, dash_list, 2);
    half_size = paned->handle_size / 2;

    if (GDL_DOCK_ITEM (paned)->orientation == GTK_ORIENTATION_HORIZONTAL) {
        guint16 xpos;

        xpos = paned->child1_real_size + 
            GTK_CONTAINER (paned)->border_width + half_size;

        gdk_draw_line (widget->window, paned->xor_gc,
                       xpos - half_size,
                       0,
                       xpos - half_size,
                       widget->allocation.height - 1);

        gdk_draw_line (widget->window, paned->xor_gc,
                       xpos + half_size,
                       0,
                       xpos + half_size,
                       widget->allocation.height - 1);

        gdk_gc_set_dashes (paned->xor_gc, 0, dash_list, 2);

        gdk_draw_line (widget->window, paned->xor_gc,
                       xpos - half_size + 1,
                       0,
                       xpos - half_size + 1,
                       widget->allocation.height - 1);

        gdk_draw_line (widget->window, paned->xor_gc,
                       xpos + half_size + 1,
                       0,
                       xpos + half_size + 1,
                       widget->allocation.height - 1);
    } else {
        guint16 ypos;

        ypos = paned->child1_real_size
            + GTK_CONTAINER (paned)->border_width + half_size;

        gdk_draw_line (widget->window, paned->xor_gc,
                       0,
                       ypos - half_size,
                       widget->allocation.width - 1,
                       ypos - half_size);

        gdk_draw_line (widget->window, paned->xor_gc,
                       0,
                       ypos + half_size,
                       widget->allocation.width - 1,
                       ypos + half_size);

        gdk_gc_set_dashes (paned->xor_gc, 0, dash_list, 2);

        gdk_draw_line (widget->window, paned->xor_gc,
                       0,
                       ypos - half_size + 1,
                       widget->allocation.width - 1,
                       ypos - half_size + 1);

        gdk_draw_line (widget->window, paned->xor_gc,
                       0,
                       ypos + half_size + 1,
                       widget->allocation.width - 1,
                       ypos + half_size + 1);

    };
}

static gboolean
gdl_dock_paned_button_press (GtkWidget      *widget,
                             GdkEventButton *event)
{
    GdlDockPaned *paned;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_PANED (widget), FALSE);

    paned = GDL_DOCK_PANED (widget);

    if (!paned->in_drag &&
        event->window == paned->handle && event->button == 1) {
        paned->old_child1_size = paned->child1_size;
        paned->in_drag = TRUE;
        /* We need a server grab here, not gtk_grab_add(), since
         * we don't want to pass events on to the widget's children */
        gdk_pointer_grab (paned->handle, FALSE,
                          GDK_POINTER_MOTION_HINT_MASK
                          | GDK_BUTTON1_MOTION_MASK
                          | GDK_BUTTON_RELEASE_MASK,
                          NULL, NULL, event->time);

        if (GDL_DOCK_ITEM (paned)->orientation == GTK_ORIENTATION_HORIZONTAL) {
            paned->child1_size = gdl_dock_paned_quantized_size 
                (paned, paned->child1_size + event->x -
                 paned->handle_size / 2);
            paned->child1_size = 
                CLAMP (paned->child1_size, 0,
                       widget->allocation.width
                       - paned->handle_size
                       - 2 * GTK_CONTAINER (paned)->border_width);
        } else {
            paned->child1_size = gdl_dock_paned_quantized_size 
                (paned, paned->child1_size + event->y -
                 paned->handle_size / 2);
            paned->child1_size = 
                CLAMP (paned->child1_size, 0,
                       widget->allocation.height
                       - paned->handle_size
                       - 2 * GTK_CONTAINER (paned)->border_width);
        };
        paned->child1_real_size = paned->child1_size;
        gdl_dock_paned_xor_line (paned);

        return TRUE;
    }

    return FALSE;
}

static gboolean
gdl_dock_paned_button_release (GtkWidget      *widget,
                               GdkEventButton *event)
{
    GdlDockPaned *paned;
    
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_PANED (widget), FALSE);

    paned = GDL_DOCK_PANED (widget);

    if (paned->in_drag && (event->button == 1)) {
        gdl_dock_paned_xor_line (paned);
        paned->in_drag = FALSE;
        paned->position_set = TRUE;
        gdk_pointer_ungrab (event->time);
        gtk_widget_queue_resize (GTK_WIDGET (paned));

        return TRUE;
    }

    return FALSE;
}

static gboolean
gdl_dock_paned_motion (GtkWidget      *widget,
                       GdkEventMotion *event)
{
    GdlDockPaned *paned;
    gint          x, y;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_PANED (widget), FALSE);

    paned = GDL_DOCK_PANED (widget);

    if (event->is_hint || event->window != widget->window)
        gtk_widget_get_pointer (widget, &x, &y);
    else {
        x = event->x;
        y = event->y;
    };

    if (paned->in_drag) {
        gint size;
        gint new_child1_size;

        if (GDL_DOCK_ITEM (paned)->orientation == GTK_ORIENTATION_HORIZONTAL) {
            size = x - GTK_CONTAINER (paned)->border_width - 
                paned->handle_size / 2;
        } else {
            size = y - GTK_CONTAINER (paned)->border_width - 
                paned->handle_size / 2;
        };
        new_child1_size = CLAMP (gdl_dock_paned_quantized_size (paned, size),
                                 paned->min_position,
                                 paned->max_position);

        if (new_child1_size == paned->child1_size)
            return TRUE;

        gdl_dock_paned_xor_line (paned);
        paned->child1_size = new_child1_size;
        paned->child1_real_size = paned->child1_size;
        gdl_dock_paned_xor_line (paned);
    }

    return TRUE;
}

static void
gdl_dock_paned_auto_reduce (GdlDockItem *item)
{
    GdlDockPaned *paned;
    GtkWidget    *child = NULL;

    g_return_if_fail (GTK_WIDGET (item)->parent != NULL);

    paned = GDL_DOCK_PANED (item);

    if (paned->child1 == NULL)
        child = paned->child2;
    else if (paned->child2 == NULL)
        child = paned->child1;

    if (child) {
        GtkContainer *parent;

        parent = GTK_CONTAINER (GTK_WIDGET (item)->parent);
        gtk_widget_ref (child);
        gtk_container_remove (GTK_CONTAINER (item), child);
        gtk_container_remove (parent, GTK_WIDGET (item));
        gtk_container_add (parent, child);
        gtk_widget_unref (child);
    };
}

#define SPLIT_RATIO  0.3

static gboolean
gdl_dock_paned_dock_request (GdlDockItem        *item, 
                             gint                x,
                             gint                y, 
                             GdlDockRequestInfo *target)
{
    GtkAllocation *alloc;
    gint           dx, dy;
    gboolean       snap = TRUE;

    alloc = &(GTK_WIDGET (item)->allocation);

    dx = x - alloc->x;
    dy = y - alloc->y;

    if (dx > 0 && dx < alloc->width && dy > 0 && dy < alloc->height) {
        gint bw;

        bw = GTK_CONTAINER (item)->border_width;
        /* it's inside our area... */
        dx -= bw;
        dy -= bw;
        
        target->target = GTK_WIDGET (item);
        snap = TRUE;

        target->rect.x = 0;
        target->rect.y = 0;
        target->rect.width = alloc->width;
        target->rect.height = alloc->height;

        /* ... see if it's in the border_width band */
        if (dx < 0) {
            target->rect.width *= SPLIT_RATIO;
            target->position = GDL_DOCK_LEFT;

        } else if (dx > alloc->width - 2 * bw) {
            target->rect.x += target->rect.width * (1 - SPLIT_RATIO);
            target->rect.width *= SPLIT_RATIO;
            target->position = GDL_DOCK_RIGHT;

        } else if (dy < 0) {
            target->rect.height *= SPLIT_RATIO;
            target->position = GDL_DOCK_TOP;

        } else if (dy > alloc->height - 2 * bw) {
            target->rect.y += target->rect.height * (1 - SPLIT_RATIO);
            target->rect.height *= SPLIT_RATIO;
            target->position = GDL_DOCK_BOTTOM;

        /* otherwise try our children */
        } else {
            GdlDockPaned *paned = GDL_DOCK_PANED (item);

            snap = FALSE;
            /* FIXME: this is not very wise... we have the handle position */
            if (paned->child1) 
                snap = gdl_dock_item_dock_request 
                    (GDL_DOCK_ITEM (paned->child1), dx + bw, dy + bw, target);
            if (!snap && paned->child2) {
                snap = gdl_dock_item_dock_request
                    (GDL_DOCK_ITEM (paned->child2), dx + bw, dy + bw, target);
                if (snap) {
                    if (item->orientation == GTK_ORIENTATION_HORIZONTAL)
                        target->rect.x += paned->child2->allocation.x - bw;
                    else
                        target->rect.y += paned->child2->allocation.y - bw;
                };
            };
            if (snap) {
                target->rect.x += bw;
                target->rect.y += bw;
            };
        };
        return snap;
    }
    return FALSE;
}

static void
gdl_dock_paned_set_orientation (GdlDockItem    *item,
                                GtkOrientation  orientation)
{
    GdlDockPaned *paned;

    g_return_if_fail (GDL_IS_DOCK_PANED (item));

    paned = GDL_DOCK_PANED (item);
    switch (item->orientation) {
    case GTK_ORIENTATION_HORIZONTAL:
        paned->cursor_type = GDK_SB_H_DOUBLE_ARROW;
        break;
    case GTK_ORIENTATION_VERTICAL:
        paned->cursor_type = GDK_SB_V_DOUBLE_ARROW;
        break;
    default:
        paned->cursor_type = GDK_CROSS;
        break;
    };

    if (paned->handle) {
        GdkCursor *cursor;

        cursor = gdk_cursor_new (paned->cursor_type);
        gdk_window_set_cursor (paned->handle, cursor);
        gdk_cursor_destroy (cursor);
    };
}

static void
gdl_dock_paned_layout_save (GdlDockItem *item,
                            xmlNodePtr   node)
{
    GdlDockPaned *paned;
    xmlNodePtr paned_node;
    gchar divider[8];

    g_return_if_fail (item != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (item));
    
    paned = GDL_DOCK_PANED (item);
    
    /* Create "paned" node. */
    paned_node = xmlNewChild (node, NULL, "paned", NULL);
    if (item->orientation == GTK_ORIENTATION_HORIZONTAL) {
        xmlSetProp (paned_node, "orientation", "horizontal");
        sprintf (divider, "%i", GDL_DOCK_PANED (item)->handle_xpos);
        xmlSetProp (paned_node, "divider", divider);
    } else {
        xmlSetProp (paned_node, "orientation", "vertical");    
        sprintf (divider, "%i", GDL_DOCK_PANED (item)->handle_ypos);
        xmlSetProp (paned_node, "divider", divider);
    }

    /* Save child1 layout. */
    if (paned->child1)
        gdl_dock_item_save_layout (GDL_DOCK_ITEM (paned->child1), paned_node);

    /* Save child2 layout. */
    if (paned->child2)
        gdl_dock_item_save_layout (GDL_DOCK_ITEM (paned->child2), paned_node);
}

static void
gdl_dock_paned_hide (GdlDockItem *item)
{
    GtkWidget    *parent, *real_parent;
    GdlDockPaned *paned;

    g_return_if_fail (item != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (item));

    paned = GDL_DOCK_PANED (item);
    real_parent = GTK_WIDGET (item)->parent;
    GDL_DOCK_ITEM_GET_PARENT (item, parent);
    
    /* Unfloat item. */
    if (GDL_DOCK_ITEM_IS_FLOATING (item))
        gdl_dock_item_window_sink (item);
    
    /* Remove children. */
    if (paned->child1)
        gdl_dock_item_hide (GDL_DOCK_ITEM (paned->child1));
    if (paned->child2)
        gdl_dock_item_hide (GDL_DOCK_ITEM (paned->child2));

    /* Remove item. */
    if (real_parent)
        gtk_container_remove (GTK_CONTAINER (real_parent),
                              GTK_WIDGET (item));
    /* Auto reduce parent. */
    if (parent && GDL_IS_DOCK_ITEM (parent))
        gdl_dock_item_auto_reduce (GDL_DOCK_ITEM (parent));
}

static gchar *
gdl_dock_paned_get_pos_hint (GdlDockItem      *item,
                             GdlDockItem      *caller,
                             GdlDockPlacement *position)
{
    GdlDockPaned *paned;

    g_return_val_if_fail (item != NULL, NULL);

    paned = GDL_DOCK_PANED (item);
    if (caller) {
        GdlDockPlacement place;

        /* going up the hierarchy */
        /* FIXME: handle the case when the item has not been auto_reduced 
           and propagate the call to the parent */
        if (GTK_WIDGET (caller) == paned->child1) {
            if (item->orientation == GTK_ORIENTATION_HORIZONTAL)
                *position = GDL_DOCK_LEFT;
            else
                *position = GDL_DOCK_TOP;
            return gdl_dock_item_get_pos_hint (GDL_DOCK_ITEM (paned->child2), 
                                               NULL, &place);

        }
        else if (GTK_WIDGET (caller) == paned->child2) {
            if (item->orientation == GTK_ORIENTATION_HORIZONTAL)
                *position = GDL_DOCK_RIGHT;
            else
                *position = GDL_DOCK_BOTTOM;
            return gdl_dock_item_get_pos_hint (GDL_DOCK_ITEM (paned->child1), 
                                               NULL, &place);

        }
        else {
            g_warning (_("gdl_dock_paned_get_pos_hint called with a "
                         "contained child"));
            return NULL;
        };

    } else {
        /* going down the hierarchy */
        if (item->name)
            return g_strdup (item->name);

        /* try with our children */
        if (paned->child1)
            return gdl_dock_item_get_pos_hint (GDL_DOCK_ITEM (paned->child1), 
                                               NULL, position);
        else if (paned->child2)
            return gdl_dock_item_get_pos_hint (GDL_DOCK_ITEM (paned->child2), 
                                               NULL, position);
        else 
            return NULL;
    };
}


/* Public interface */

GType
gdl_dock_paned_get_type (void)
{
    static GType paned_type = 0;
  
    if (!paned_type) {
        GTypeInfo paned_info = {
            sizeof (GdlDockPanedClass),

            NULL,               /* base_init */
            NULL,               /* base_finalize */

            (GClassInitFunc) gdl_dock_paned_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */

            sizeof (GdlDockPaned),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) gdl_dock_paned_init,
            NULL                /* value_table */
        };

        paned_type = g_type_register_static (GDL_TYPE_DOCK_ITEM, "GdlDockPaned", 
                                            &paned_info, 0);
    }
  
    return paned_type;
}

GtkWidget *
gdl_dock_paned_new (GtkOrientation orientation)
{
    GdlDockPaned *paned;

    paned = GDL_DOCK_PANED (g_object_new (GDL_TYPE_DOCK_PANED, NULL));

    gdl_dock_item_set_orientation (GDL_DOCK_ITEM (paned), orientation);

    return GTK_WIDGET (paned);
}

void
gdl_dock_paned_add1 (GdlDockPaned *paned,
                     GtkWidget    *child)
{
    GdlDockItem *item;

    g_return_if_fail (paned != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (paned));
    g_return_if_fail (GDL_IS_DOCK_ITEM (child));

    item = GDL_DOCK_ITEM (child);
    GDL_DOCK_ITEM_CHECK_AND_BIND (item, paned);

    if (GDL_DOCK_ITEM_IS_FLOATING (item))
        gdl_dock_item_window_sink (item);

    if (!paned->child1) {
        paned->child1 = child;

        gtk_widget_set_parent (child, GTK_WIDGET (paned));

        gdl_dock_item_show_handle (item);

        if (GTK_WIDGET_REALIZED (child->parent))
            gtk_widget_realize (child);

        if (GTK_WIDGET_VISIBLE (child->parent) && GTK_WIDGET_VISIBLE (child)) {
            if (GTK_WIDGET_MAPPED (child->parent))
                gtk_widget_map (child);

            gtk_widget_queue_resize (child);
	}
    }
}

void
gdl_dock_paned_add2 (GdlDockPaned *paned,
                     GtkWidget *child)
{
    GdlDockItem *item;

    g_return_if_fail (paned != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (paned));
    g_return_if_fail (GDL_IS_DOCK_ITEM (child));

    item = GDL_DOCK_ITEM (child);
    GDL_DOCK_ITEM_CHECK_AND_BIND (item, paned);

    if (GDL_DOCK_ITEM_IS_FLOATING (item))
        gdl_dock_item_window_sink (item);

    if (!paned->child2) {
        paned->child2 = child;

        gtk_widget_set_parent (child, GTK_WIDGET (paned));

        gdl_dock_item_show_handle (item);

        if (GTK_WIDGET_REALIZED (child->parent))
            gtk_widget_realize (child);

        if (GTK_WIDGET_VISIBLE (child->parent) && GTK_WIDGET_VISIBLE (child)) {
            if (GTK_WIDGET_MAPPED (child->parent))
                gtk_widget_map (child);

            gtk_widget_queue_resize (child);
	}
    }
}

gint
gdl_dock_paned_get_position (GdlDockPaned  *paned)
{
    g_return_val_if_fail (paned != NULL, 0);
    g_return_val_if_fail (GDL_IS_DOCK_PANED (paned), 0);

    return paned->child1_real_size;
}

void
gdl_dock_paned_set_position (GdlDockPaned *paned,
                             gint          position)
{
    g_return_if_fail (paned != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (paned));

    if (position >= 0) {
        /* We don't clamp here - the assumption is that
         * if the total allocation changes at the same time
         * as the position, the position set is with reference
         * to the new total size. If only the position changes,
         * then clamping will occur in gdl_dock_paned_compute_position()
         */
        paned->child1_size = position;
        paned->position_set = TRUE;

    } else
        paned->position_set = FALSE;
    
    gtk_widget_queue_resize (GTK_WIDGET (paned));
}

void
gdl_dock_paned_set_handle_size (GdlDockPaned *paned,
                                guint16       size)
{
    g_return_if_fail (paned != NULL);
    g_return_if_fail (GDL_IS_DOCK_PANED (paned));

    gtk_widget_queue_resize (GTK_WIDGET (paned));

    paned->handle_size = size;
}

void
gdl_dock_paned_reorder (GdlDockPaned     *paned,
                        GtkWidget        *child1,
                        GdlDockPlacement  position)
{
    g_return_if_fail (paned != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (child1));
    g_return_if_fail (child1 == paned->child1 || child1 == paned->child2);

    if (position == GDL_DOCK_LEFT || position == GDL_DOCK_RIGHT)
        gdl_dock_item_set_orientation (GDL_DOCK_ITEM (paned), 
                                       GTK_ORIENTATION_HORIZONTAL);
    else
        gdl_dock_item_set_orientation (GDL_DOCK_ITEM (paned),
                                       GTK_ORIENTATION_VERTICAL);

    if (child1 == paned->child1) {
        if (position == GDL_DOCK_RIGHT || position == GDL_DOCK_BOTTOM) {
            paned->child1 = paned->child2;
            paned->child2 = child1;
        };
    } else {
        if (position == GDL_DOCK_LEFT || position == GDL_DOCK_TOP) {
            paned->child2 = paned->child1;
            paned->child1 = child1;
        };
    };
    gtk_widget_queue_resize (GTK_WIDGET (paned));
}

