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
#include <libgnome/gnome-macros.h>
#include "gdl-dock-item.h"
#include "gdl-dock-item-grip.h"
#include "gdl-dock.h"
#include "gdl-tools.h"


enum {
    ACTIVATE,
    LAST_SIGNAL
};
static guint signals [LAST_SIGNAL];

static AtkObjectClass *a11y_parent_class = NULL;

GNOME_CLASS_BOILERPLATE (GdlDockItemGrip, gdl_dock_item_grip,
			 GtkWidget, GTK_TYPE_WIDGET);

static gint
gdl_dock_item_grip_expose (GtkWidget      *widget,
			   GdkEventExpose *event)
{
    GdkRectangle *clip = &event->area;
    GdkRectangle *rect = &widget->allocation;
    GdlDockItemGrip *grip = (GdlDockItemGrip *) widget;
    GtkShadowType shadow = GTK_SHADOW_OUT;

    gtk_paint_handle (widget->style,
                      widget->window,
                      GTK_WIDGET_STATE (widget),
                      shadow,
                      clip, widget, "dockitem",
                      rect->x, rect->y, rect->width, rect->height, 
                      grip->item->orientation);

    if (GTK_WIDGET_HAS_FOCUS (widget)) {
        gint focus_width;
        gint focus_pad;
        GdkRectangle focus;
		
        gtk_widget_style_get (GTK_WIDGET (widget),
                              "focus-line-width", &focus_width,
                              "focus-padding", &focus_pad,
                              NULL); 
		
        focus = *rect;
        focus.x += widget->style->xthickness + focus_pad;
        focus.y += widget->style->ythickness + focus_pad;
        focus.width -= 2 * (widget->style->xthickness + focus_pad);
        focus.height -= 2 * (widget->style->xthickness + focus_pad);
		
        gtk_paint_focus (widget->style, widget->window,
                         GTK_WIDGET_STATE (widget),
                         clip, widget, "dockitem",
                         focus.x, focus.y,
                         focus.width, focus.height);
    }

    return FALSE;
}

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

#if 0
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
#endif

#if 0
static void
grip_item_a11y_class_init (AtkObjectClass *klass)
{
    a11y_parent_class = g_type_class_peek_parent (klass);

    klass->initialize = grip_item_a11y_initialize;
    klass->ref_state_set = grip_item_a11y_ref_state_set;
}
#endif

static AtkObject *
gdl_dock_item_grip_get_accessible (GtkWidget *widget)
{
#if 0
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
#else
    return NULL;
#endif
}

static void
gdl_dock_item_grip_activate (GdlDockItemGrip *grip)
{
#if 0
    if (grip->item->is_floating)
        gdl_dock_item_grip_dock (grip);
    else
        gdl_dock_item_grip_undock (grip);
#endif
}

static void
gdl_dock_item_grip_dispose (GObject *object)
{
    GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
}

static void
gdl_dock_item_grip_instance_init (GdlDockItemGrip *grip)
{
    GTK_WIDGET_SET_FLAGS (grip, GTK_CAN_FOCUS);
    GTK_WIDGET_SET_FLAGS (grip, GTK_NO_WINDOW);
}

#if 0
static BonoboDockBand *
get_dock_band (GtkWidget *widget)
{
    while (widget && !BONOBO_IS_DOCK_BAND (widget))
        widget = widget->parent;

    return (BonoboDockBand *) widget;
}
#endif

static gint
gdl_dock_item_grip_key_press_event (GtkWidget   *widget,
                                    GdkEventKey *event)
{
#if 0
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
#endif

    return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static void
gdl_dock_item_grip_class_init (GdlDockItemGripClass *klass)
{
    GtkBindingSet  *binding_set;
    GObjectClass   *gobject_class = (GObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->dispose = gdl_dock_item_grip_dispose;

    widget_class->expose_event = gdl_dock_item_grip_expose;
    widget_class->get_accessible = gdl_dock_item_grip_get_accessible;
    widget_class->key_press_event = gdl_dock_item_grip_key_press_event;

    klass->activate = gdl_dock_item_grip_activate;

    binding_set = gtk_binding_set_by_class (klass);

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
    GdlDockItemGrip *grip = g_object_new (GDL_TYPE_DOCK_ITEM_GRIP, NULL);

    grip->item = item;

    return GTK_WIDGET (grip);
}
