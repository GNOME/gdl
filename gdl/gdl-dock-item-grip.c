/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 8 -*- */
/**
 * gdl-dock-item-grip.c
 *
 * Based on bonobo-dock-item-grip.  Original copyright notice follows.
 *
 * Author:
 *    Michael Meeks
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gdl-i18n.h"
#include <string.h>
#include <glib-object.h>
#include <atk/atkstateset.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkaccessible.h>
#include <gtk/gtkbindings.h>
#include "gdl-dock-item.h"
#include "gdl-dock-item-grip.h"
#include "gdl-dock.h"
#include "gdl-stock.h"
#include "gdl-tools.h"

#define A11Y_UNFINISHED

enum {
    ACTIVATE,
    LAST_SIGNAL
};
static guint signals [LAST_SIGNAL];

enum {
    PROP_0,
    PROP_ITEM
};
 
struct _GdlDockItemGripPrivate {
    GtkWidget   *close_button;
    GtkWidget   *iconify_button;
    GtkTooltips *tooltips;

    GdkPixbuf   *icon_pixbuf;
    PangoLayout *title_layout;
};
 
GDL_CLASS_BOILERPLATE (GdlDockItemGrip, gdl_dock_item_grip,
		       GtkContainer, GTK_TYPE_CONTAINER);

static void
gdl_dock_item_grip_get_title_area (GdlDockItemGrip *grip,
                                   GdkRectangle    *area)
{
    GtkWidget *widget = GTK_WIDGET (grip);
    gint       border = GTK_CONTAINER (grip)->border_width;
    gint       alloc_height;

    area->width = (widget->allocation.width - 2 * border);
    
    pango_layout_get_pixel_size (grip->_priv->title_layout, NULL, &alloc_height);
    
    if (GTK_WIDGET_VISIBLE (grip->_priv->close_button)) {
        if (grip->_priv->close_button->allocation.height > alloc_height) {
            alloc_height = grip->_priv->close_button->allocation.height;
	}
        area->width -= grip->_priv->close_button->allocation.width;
    }
    if (GTK_WIDGET_VISIBLE (grip->_priv->iconify_button)) {
        if (grip->_priv->iconify_button->allocation.height > alloc_height) {
	    alloc_height = grip->_priv->iconify_button->allocation.height;
	}
        area->width -= grip->_priv->iconify_button->allocation.width;
    }

    area->x      = widget->allocation.x + border;
    area->y      = widget->allocation.y + border;
    area->height = alloc_height;

    if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        area->x += (widget->allocation.width - 2 * border) - area->width;
}
  
static gint
gdl_dock_item_grip_expose (GtkWidget      *widget,
			   GdkEventExpose *event)
{
    GdlDockItemGrip *grip;
    gint             border;
    gchar           *stock_id;
    GdkRectangle     pixbuf_rect;
    gint             pixbuf_width;
    GdkRectangle     title_area;
    GdkRectangle     expose_area;
    gchar           *name;
    gint             layout_width;
    gint             layout_height;
    gint             text_x;
    gint             text_y;

    grip = GDL_DOCK_ITEM_GRIP (widget);
    border = GTK_CONTAINER (grip)->border_width;

    g_object_get (G_OBJECT (grip->item), "stock_id", &stock_id, NULL);
    if (stock_id) {
        GdkPixbuf *pixbuf;
        
        if (!grip->_priv->icon_pixbuf) {
            pixbuf = gtk_widget_render_icon (widget, stock_id,
                                             GTK_ICON_SIZE_MENU, "");
            grip->_priv->icon_pixbuf = pixbuf;
        } else {
            pixbuf = grip->_priv->icon_pixbuf;
        }
        
        g_free (stock_id);

        pixbuf_rect.width = gdk_pixbuf_get_width (pixbuf);
        pixbuf_rect.height = gdk_pixbuf_get_height (pixbuf);
        if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            pixbuf_rect.x = widget->allocation.x + widget->allocation.width -
                border - pixbuf_rect.width;
        else
            pixbuf_rect.x = widget->allocation.x + border;
        pixbuf_rect.y = widget->allocation.y + border +
                        (widget->allocation.height - pixbuf_rect.height) / 2;

        if (gdk_rectangle_intersect (&event->area, &pixbuf_rect, &expose_area)) {
            GdkGC *gc;
            GtkStyle *style;

            style = gtk_widget_get_style (widget);
            gc = style->bg_gc[widget->state];
            gdk_draw_pixbuf (GDK_DRAWABLE (widget->window), gc, pixbuf,
                             0, 0, pixbuf_rect.x, pixbuf_rect.y,
                             pixbuf_rect.width, pixbuf_rect.height,
                             GDK_RGB_DITHER_NONE, 0, 0);
	}

	pixbuf_width = pixbuf_rect.width + 1;
    } else {
        pixbuf_width = 0;
    }

    gdl_dock_item_grip_get_title_area (grip, &title_area);

    if (gdk_rectangle_intersect (&title_area, &event->area, &expose_area)) {
        if (!grip->_priv->title_layout) {
            g_object_get (G_OBJECT (grip->item), "long_name", &name, NULL);
            grip->_priv->title_layout = gtk_widget_create_pango_layout (widget,
                                                                        name);
            g_free (name);
        }

        pango_layout_get_pixel_size (grip->_priv->title_layout, &layout_width,
                                     &layout_height);

        if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            text_x = title_area.x + title_area.width - layout_width - pixbuf_width;
        else
            text_x = title_area.x + pixbuf_width;

        text_y = title_area.y + (title_area.height - layout_height) / 2;

        gtk_paint_layout (widget->style, widget->window, widget->state, TRUE,
                          &expose_area, widget, NULL, text_x, text_y,
                          grip->_priv->title_layout);
    }

    return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}  

#ifndef A11Y_UNFINISHED

static AtkObjectClass *a11y_parent_class = NULL;

static void
grip_item_a11y_initialize (AtkObject *accessible, gpointer widget)
{
    accessible->role = ATK_ROLE_SEPARATOR;
    atk_object_set_name (accessible, "grip");

    a11y_parent_class->initialize (accessible, widget);
}

static AtkStateSet*
grip_item_a11y_ref_state_set (AtkObject *accessible)
{
    AtkStateSet *state_set;
    GdlDockItemGrip *grip;

    state_set = a11y_parent_class->ref_state_set (accessible);
    grip = GDL_DOCK_ITEM_GRIP (
        GTK_ACCESSIBLE (accessible)->widget);

    if (grip == NULL)
        return state_set;

    if (grip->item->orientation == GTK_ORIENTATION_VERTICAL) {
        atk_state_set_add_state (state_set, ATK_STATE_VERTICAL);
        atk_state_set_remove_state (state_set, ATK_STATE_HORIZONTAL);
    } else {
        atk_state_set_add_state (state_set, ATK_STATE_HORIZONTAL);
        atk_state_set_remove_state (state_set, ATK_STATE_VERTICAL);
    }

    return state_set;
}

static GdlDock *
get_dock (GtkWidget *widget)
{
    while (widget && !GDL_IS_DOCK (widget))
        widget = widget->parent;

    return (GdlDock *) widget;
}

static void
gdl_dock_item_grip_dock (GdlDockItemGrip *grip)
{
    GdlDock *dock;

    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (grip));

    dock = get_dock (GTK_WIDGET (grip->item));
    g_return_if_fail (dock != NULL);

    gdl_dock_item_unfloat (grip->item);
		
    g_object_ref (G_OBJECT (grip->item));
    gtk_container_remove (
        GTK_CONTAINER (
            GTK_WIDGET (grip->item)->parent),
        GTK_WIDGET (grip->item));
    gdl_dock_add_item (
        dock, grip->item,
        BONOBO_DOCK_TOP, 2, 0, 0, TRUE);
    g_object_unref (G_OBJECT (grip->item));
}

static void
gdl_dock_item_grip_undock (GdlDockItemGrip *grip)
{
    guint x, y;

    g_return_if_fail (BONOBO_IS_DOCK_ITEM_GRIP (grip));

    if (grip->item->is_floating)
        return;

    gdk_window_get_position (
        GTK_WIDGET (grip)->window, &x, &y);

    gdl_dock_item_detach (grip->item, x, y);
}

enum {
    ACTION_DOCK,
    ACTION_UNDOCK,
    ACTION_LAST
};

static gboolean
gdl_dock_item_grip_do_action (AtkAction *action,
			      gint       i)
{
    GdlDockItemGrip *grip;

    grip = GDL_DOCK_ITEM_GRIP (
        GTK_ACCESSIBLE (action)->widget);

    if (grip->item->behavior & GDL_DOCK_ITEM_BEH_LOCKED)
        return FALSE;

    switch (i) {
    case ACTION_DOCK:
        gdl_dock_item_grip_dock (grip);
        break;
    case ACTION_UNDOCK:
        gdl_dock_item_grip_undock (grip);
        break;
    default:
        break;
    }
    return FALSE;
}

static gint
gdl_dock_item_grip_get_n_actions (AtkAction *action)
{
    GdlDockItemGrip *grip;

    grip = GDL_DOCK_ITEM_GRIP (
        GTK_ACCESSIBLE (action)->widget);

    if (grip->item->behavior & GDL_DOCK_ITEM_BEH_LOCKED)
        return 0;
    else
        return ACTION_LAST;
}

static void
grip_item_a11y_class_init (AtkObjectClass *klass)
{
    a11y_parent_class = g_type_class_peek_parent (klass);

    klass->initialize = grip_item_a11y_initialize;
    klass->ref_state_set = grip_item_a11y_ref_state_set;
}

#endif /* A11Y_UNFINISHED */


static AtkObject *
gdl_dock_item_grip_get_accessible (GtkWidget *widget)
{
#ifndef A11Y_UNFINISHED
    AtkObject *accessible;
    static GType a11y_type = 0;

    if (!a11y_type) {
        AtkActionIface action_if;

        a11y_type = bonobo_a11y_get_derived_type_for (
            GDL_TYPE_DOCK_ITEM_GRIP,
            NULL, grip_item_a11y_class_init);

        memset (&action_if, 0, sizeof (AtkActionIface));
        action_if.do_action = gdl_dock_item_grip_do_action;
        action_if.get_n_actions = gdl_dock_item_grip_get_n_actions;

        bonobo_a11y_add_actions_interface (
            a11y_type, &action_if,
            ACTION_DOCK,   "dock",   _("Dock the toolbar"),    "<Enter>",
            ACTION_UNDOCK, "undock", _("Un dock the toolbar"), "<Enter>",
            -1);
    }

    if ((accessible = bonobo_a11y_get_atk_object (widget)))
        return accessible;

    return bonobo_a11y_set_atk_object_ret (
        widget, g_object_new (a11y_type, NULL));
#else /* !A11Y_UNFINISHED */
    return NULL;
#endif /* A11Y_UNFINISHED */
}

static void
gdl_dock_item_grip_activate (GdlDockItemGrip *grip)
{
#ifndef A11Y_UNFINISHED
    if (grip->item->is_floating)
        gdl_dock_item_grip_dock (grip);
    else
        gdl_dock_item_grip_undock (grip);
#endif /* A11Y_UNFINISHED */
}

static void
gdl_dock_item_grip_item_notify (GObject    *master,
                                GParamSpec *pspec,
                                gpointer    data)
{
    GdlDockItemGrip *grip;
    gchar           *name;
    gchar           *stock_id;

    grip = GDL_DOCK_ITEM_GRIP (data);

    g_object_get (master, "long_name", &name, "stock_id", &stock_id, NULL);
    if (name && grip->_priv->title_layout) {
        g_object_unref (grip->_priv->title_layout);
        grip->_priv->title_layout = NULL;
        g_free (name);
    }
    if (stock_id && grip->_priv->icon_pixbuf) {
        g_object_unref (grip->_priv->icon_pixbuf);
        grip->_priv->icon_pixbuf = NULL;
        g_free (stock_id);
    }
    if (grip->_priv->close_button) {
        if (GDL_DOCK_ITEM_CANT_CLOSE (grip->item)) {
	    gtk_widget_hide (GTK_WIDGET (grip->_priv->close_button));
	} else {
	    gtk_widget_show (GTK_WIDGET (grip->_priv->close_button));
	}
    }
    if (grip->_priv->iconify_button) {
        if (GDL_DOCK_ITEM_CANT_ICONIFY (grip->item)) {
	    gtk_widget_hide (GTK_WIDGET (grip->_priv->iconify_button));
	} else {
	    gtk_widget_show (GTK_WIDGET (grip->_priv->iconify_button));
	}
    }

    gtk_widget_queue_resize (GTK_WIDGET (grip));
}

static void
gdl_dock_item_grip_destroy (GtkObject *object)
{
    GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (object);
    
    if (grip->_priv) {
        GdlDockItemGripPrivate *priv = grip->_priv;

        if (priv->title_layout) {
            g_object_unref (priv->title_layout);
            priv->title_layout = NULL;
        }
        
        if (priv->icon_pixbuf) {
            g_object_unref (priv->icon_pixbuf);
            priv->icon_pixbuf = NULL;
        }

        if (priv->tooltips) {
            gtk_object_destroy (GTK_OBJECT (priv->tooltips));
            priv->tooltips = NULL;
        }

        g_signal_handlers_disconnect_by_func (grip->item,
                                              gdl_dock_item_grip_item_notify,
                                              grip);
        grip->item = NULL;

        grip->_priv = NULL;
        g_free (priv);
    }

    GDL_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gdl_dock_item_grip_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    GdlDockItemGrip *grip;

    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (object));

    grip = GDL_DOCK_ITEM_GRIP (object);
    
    switch (prop_id) {
        case PROP_ITEM:
            grip->item = g_value_get_object (value);
            if (grip->item) {
                g_signal_connect (grip->item, "notify::long_name",
                                  G_CALLBACK (gdl_dock_item_grip_item_notify),
                                  grip);
                g_signal_connect (grip->item, "notify::stock_id",
                                  G_CALLBACK (gdl_dock_item_grip_item_notify),
                                  grip);
		g_signal_connect (grip->item, "notify::behavior",
		                  G_CALLBACK (gdl_dock_item_grip_item_notify),
				  grip);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_item_grip_close_clicked (GtkWidget       *widget,
                                  GdlDockItemGrip *grip)
{
    g_return_if_fail (grip->item != NULL);

    gdl_dock_item_hide_item (grip->item);
}

static void
gdl_dock_item_grip_iconify_clicked (GtkWidget       *widget,
                                    GdlDockItemGrip *grip)
{
    g_return_if_fail (grip->item != NULL);

    gdl_dock_item_iconify_item (grip->item);
    
    /* Workaround to unhighlight the iconify button. */
    GTK_BUTTON (grip->_priv->iconify_button)->in_button = FALSE;
    gtk_button_leave (GTK_BUTTON (grip->_priv->iconify_button));
}
  
static void
gdl_dock_item_grip_instance_init (GdlDockItemGrip *grip)
{
    GtkWidget *image;

    GTK_WIDGET_SET_FLAGS (grip, GTK_NO_WINDOW);
    
    grip->_priv = g_new0 (GdlDockItemGripPrivate, 1);
    grip->_priv->icon_pixbuf = NULL;
    grip->_priv->title_layout = NULL;

    gdl_stock_init ();

    gtk_widget_push_composite_child ();
    grip->_priv->close_button = gtk_button_new ();
    gtk_widget_pop_composite_child ();
    
    GTK_WIDGET_UNSET_FLAGS (grip->_priv->close_button, GTK_CAN_FOCUS);
    gtk_widget_set_parent (grip->_priv->close_button, GTK_WIDGET (grip));
    gtk_button_set_relief (GTK_BUTTON (grip->_priv->close_button), GTK_RELIEF_NONE);
    gtk_widget_show (grip->_priv->close_button);

    image = gtk_image_new_from_stock (GDL_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (grip->_priv->close_button), image);
    gtk_widget_show (image);

    g_signal_connect (G_OBJECT (grip->_priv->close_button), "clicked",
                      G_CALLBACK (gdl_dock_item_grip_close_clicked), grip);

    gtk_widget_push_composite_child ();
    grip->_priv->iconify_button = gtk_button_new ();
    gtk_widget_pop_composite_child ();
    
    GTK_WIDGET_UNSET_FLAGS (grip->_priv->iconify_button, GTK_CAN_FOCUS);
    gtk_widget_set_parent (grip->_priv->iconify_button, GTK_WIDGET (grip));
    gtk_button_set_relief (GTK_BUTTON (grip->_priv->iconify_button), GTK_RELIEF_NONE);
    gtk_widget_show (grip->_priv->iconify_button);

    image = gtk_image_new_from_stock (GDL_STOCK_MENU_LEFT, GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (grip->_priv->iconify_button), image);
    gtk_widget_show (image);

    g_signal_connect (G_OBJECT (grip->_priv->iconify_button), "clicked",
                      G_CALLBACK (gdl_dock_item_grip_iconify_clicked), grip);

    grip->_priv->tooltips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (grip->_priv->tooltips, grip->_priv->iconify_button,
                          _("Iconify"), _("Iconify this dock"));
    gtk_tooltips_set_tip (grip->_priv->tooltips, grip->_priv->close_button,
                          _("Close"), _("Close this dock"));
}

#ifndef A11Y_UNFINISHED
static BonoboDockBand *
get_dock_band (GtkWidget *widget)
{
    while (widget && !BONOBO_IS_DOCK_BAND (widget))
        widget = widget->parent;

    return (BonoboDockBand *) widget;
}
#endif /* A11Y_UNFINISHED */

static gint
gdl_dock_item_grip_key_press_event (GtkWidget   *widget,
                                    GdkEventKey *event)
{
#ifndef A11Y_UNFINISHED
    gboolean had_focus = GTK_WIDGET_HAS_FOCUS (widget);
    BonoboDockBand *band = get_dock_band (widget);
    BonoboDockItemGrip *grip = (BonoboDockItemGrip *) widget;

    if (!grip->item->is_floating && band &&
        bonobo_dock_band_handle_key_nav (band, grip->item, event))
    {
        if (had_focus && !GTK_WIDGET_HAS_FOCUS (widget))
            gtk_widget_grab_focus (widget);
        return TRUE;
    }
#endif /* A11Y_UNFINISHED */

    return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static void
gdl_dock_item_grip_realize (GtkWidget *widget)
{
    GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (widget);

    GTK_WIDGET_CLASS (parent_class)->realize (widget);

    if (!grip->title_window) {
        GdkWindowAttr  attributes;
        GdkRectangle   area;
        GdkCursor     *cursor;

        gdl_dock_item_grip_get_title_area (grip, &area);

        attributes.x                 = area.x;
        attributes.y                 = area.y;
        attributes.width             = area.width;
        attributes.height            = area.height;
        attributes.window_type       = GDK_WINDOW_TEMP;
        attributes.wclass            = GDK_INPUT_ONLY;
        attributes.override_redirect = TRUE;
        attributes.event_mask        = (GDK_BUTTON_PRESS_MASK   |
                                        GDK_BUTTON_RELEASE_MASK |
                                        GDK_BUTTON_MOTION_MASK  |
                                        gtk_widget_get_events (widget));

        grip->title_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                             &attributes,
                                             (GDK_WA_X |
                                              GDK_WA_Y |
                                              GDK_WA_NOREDIR));

        gdk_window_set_user_data (grip->title_window, widget);
  
        cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                             GDK_HAND2);
        gdk_window_set_cursor (grip->title_window, cursor);
        gdk_cursor_unref (cursor);
    }
}

static void
gdl_dock_item_grip_unrealize (GtkWidget *widget)
{
    GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (widget);

    if (grip->title_window) {
        gdk_window_set_user_data (grip->title_window, NULL);
        gdk_window_destroy (grip->title_window);
        grip->title_window = NULL;
    }

    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gdl_dock_item_grip_map (GtkWidget *widget)
{
    GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (widget);

    GTK_WIDGET_CLASS (parent_class)->map (widget);

    if (grip->title_window)
        gdk_window_show (grip->title_window);
}

static void
gdl_dock_item_grip_unmap (GtkWidget *widget)
{
    GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (widget);

    if (grip->title_window)
        gdk_window_hide (grip->title_window);

    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gdl_dock_item_grip_size_request (GtkWidget      *widget,
                                 GtkRequisition *requisition)
{
    GtkRequisition   child_requisition;
    GtkContainer    *container;
    GdlDockItemGrip *grip;
    GdkRectangle     title_rect;
    gchar           *stock_id;
    gchar           *name;
    gint             layout_height;

    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (widget));
    g_return_if_fail (requisition != NULL);

    container = GTK_CONTAINER (widget);
    grip = GDL_DOCK_ITEM_GRIP (widget);
    
    requisition->width = container->border_width * 2;
    requisition->height = container->border_width * 2;

    if (!grip->_priv->title_layout) {
        g_object_get (G_OBJECT (grip->item), "long_name", &name, NULL);
        grip->_priv->title_layout = gtk_widget_create_pango_layout (widget,
                                                                    name);
        g_free (name);
    }

    pango_layout_get_pixel_size (grip->_priv->title_layout, NULL, &layout_height);

    if (GTK_WIDGET_VISIBLE (grip->_priv->close_button)) {
        gtk_widget_size_request (grip->_priv->close_button, &child_requisition);

        requisition->width += child_requisition.width;
	if (child_requisition.height > layout_height) {
            layout_height = child_requisition.height;
	}
    }
    
    if (GTK_WIDGET_VISIBLE (grip->_priv->iconify_button)) {
        gtk_widget_size_request (grip->_priv->iconify_button, &child_requisition);

        requisition->width += child_requisition.width;
	if (child_requisition.height > layout_height) {
	    layout_height = child_requisition.height;
	}
    }

    requisition->height += layout_height;

    gdl_dock_item_grip_get_title_area (grip, &title_rect);
    requisition->width += title_rect.width;

    g_object_get (G_OBJECT (grip->item), "stock_id", &stock_id, NULL);
    if (stock_id) {
        GdkPixbuf *pixbuf;
        
        if (!grip->_priv->icon_pixbuf) {
            pixbuf = gtk_widget_render_icon (widget, stock_id,
                                             GTK_ICON_SIZE_MENU, "");
            grip->_priv->icon_pixbuf = pixbuf;
        } else {
            pixbuf = grip->_priv->icon_pixbuf;
        }

        requisition->width += gdk_pixbuf_get_width (pixbuf) + 1;
        g_free (stock_id);
    }
}

static void
gdl_dock_item_grip_size_allocate (GtkWidget     *widget,
                                  GtkAllocation *allocation)
{
    GdlDockItemGrip *grip;
    GtkContainer    *container;
    GtkRequisition   button_requisition = { 0, };
    GtkAllocation    child_allocation;

    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (widget));
    g_return_if_fail (allocation != NULL);
  
    grip = GDL_DOCK_ITEM_GRIP (widget);
    container = GTK_CONTAINER (widget);

    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

    if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        child_allocation.x = allocation->x + container->border_width;
    else
        child_allocation.x = allocation->x + allocation->width - container->border_width;
    child_allocation.y = allocation->y + container->border_width;

    if (GTK_WIDGET_VISIBLE (grip->_priv->close_button)) {
        gtk_widget_size_request (grip->_priv->close_button, &button_requisition);

        if (gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL)
            child_allocation.x -= button_requisition.width;

        child_allocation.width = button_requisition.width;
        child_allocation.height = button_requisition.height;

        gtk_widget_size_allocate (grip->_priv->close_button, &child_allocation);

        if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            child_allocation.x += button_requisition.width;
    }

    if (GTK_WIDGET_VISIBLE (grip->_priv->iconify_button)) {
        gtk_widget_size_request (grip->_priv->iconify_button, &button_requisition);

        if (gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL)
            child_allocation.x -= button_requisition.width;

        child_allocation.width = button_requisition.width;
        child_allocation.height = button_requisition.height;

        gtk_widget_size_allocate (grip->_priv->iconify_button, &child_allocation);

        if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            child_allocation.x += button_requisition.width;
    }
    
    if (grip->title_window) {
        GdkRectangle area;
        
        gdl_dock_item_grip_get_title_area (grip, &area);
        
        gdk_window_move_resize (grip->title_window,
                                area.x, area.y, area.width, area.height);
    }
}

static void 
gdl_dock_item_grip_add (GtkContainer *container,
                        GtkWidget    *widget)
{
    g_warning ("gtk_container_add not implemented for GdlDockItemGrip");
}

static void  
gdl_dock_item_grip_remove (GtkContainer *container,
                           GtkWidget    *widget)
{
    g_warning ("gtk_container_remove not implemented for GdlDockItemGrip");
}

static void
gdl_dock_item_grip_forall (GtkContainer *container,
                           gboolean      include_internals,
                           GtkCallback   callback,
                           gpointer      callback_data)
{
    GdlDockItemGrip *grip;
    
    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (container));

    grip = GDL_DOCK_ITEM_GRIP (container);

    if (include_internals) {
        (* callback) (grip->_priv->close_button, callback_data);
        (* callback) (grip->_priv->iconify_button, callback_data);
    }
}

static GtkType
gdl_dock_item_grip_child_type (GtkContainer *container)
{
    return GTK_TYPE_WIDGET;
}

static void
gdl_dock_item_grip_class_init (GdlDockItemGripClass *klass)
{
    GtkBindingSet *binding_set;
    GObjectClass *gobject_class;
    GtkObjectClass *gtk_object_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);
    gtk_object_class = GTK_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);
    container_class = GTK_CONTAINER_CLASS (klass);

    gobject_class->set_property = gdl_dock_item_grip_set_property;

    gtk_object_class->destroy = gdl_dock_item_grip_destroy;

    widget_class->expose_event = gdl_dock_item_grip_expose;
    widget_class->get_accessible = gdl_dock_item_grip_get_accessible;
    widget_class->key_press_event = gdl_dock_item_grip_key_press_event;
    widget_class->realize = gdl_dock_item_grip_realize;
    widget_class->unrealize = gdl_dock_item_grip_unrealize;
    widget_class->map = gdl_dock_item_grip_map;
    widget_class->unmap = gdl_dock_item_grip_unmap;
    widget_class->size_request = gdl_dock_item_grip_size_request;
    widget_class->size_allocate = gdl_dock_item_grip_size_allocate;

    container_class->add = gdl_dock_item_grip_add;
    container_class->remove = gdl_dock_item_grip_remove;
    container_class->forall = gdl_dock_item_grip_forall;
    container_class->child_type = gdl_dock_item_grip_child_type;

    klass->activate = gdl_dock_item_grip_activate;

    binding_set = gtk_binding_set_by_class (klass);

    g_object_class_install_property (
        gobject_class, PROP_ITEM,
        g_param_spec_object ("item", _("Controlling dock item"),
                             _("Dockitem which 'owns' this grip"),
                             GDL_TYPE_DOCK_ITEM,
                             G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
    signals [ACTIVATE] =
        g_signal_new ("activate",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (
                          GdlDockItemGripClass, activate),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
    widget_class->activate_signal = signals [ACTIVATE];

    gtk_binding_entry_add_signal (binding_set, GDK_Return, 0,
                                  "activate", 0);
    gtk_binding_entry_add_signal (binding_set, GDK_KP_Enter, 0,
                                  "activate", 0);
}

GtkWidget *
gdl_dock_item_grip_new (GdlDockItem *item)
{
    GdlDockItemGrip *grip = g_object_new (GDL_TYPE_DOCK_ITEM_GRIP, "item", item,
                                          NULL);

    return GTK_WIDGET (grip);
}
