/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 *
 * gdl-dock-placeholder.c - Placeholders for docking items
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
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

#include "gdl-tools.h"
#include "gdl-dock-placeholder.h"
#include "gdl-dock-item.h"
#include "libgdltypebuiltins.h"


#undef PLACEHOLDER_DEBUG

/* ----- Private prototypes ----- */

static void     gdl_dock_placeholder_class_init     (GdlDockPlaceholderClass *klass);
static void     gdl_dock_placeholder_instance_init  (GdlDockPlaceholder      *ph);

static void     gdl_dock_placeholder_set_property   (GObject                 *g_object,
                                                     guint                    prop_id,
                                                     const GValue            *value,
                                                     GParamSpec              *pspec);
static void     gdl_dock_placeholder_get_property   (GObject                 *g_object,
                                                     guint                    prop_id,
                                                     GValue                  *value,
                                                     GParamSpec              *pspec);

static void     gdl_dock_placeholder_destroy        (GtkObject               *object);

static void     gdl_dock_placeholder_add            (GtkContainer            *container,
                                                     GtkWidget               *widget);

static void     gdl_dock_placeholder_detach         (GdlDockObject           *object,
                                                     gboolean                 recursive);
static void     gdl_dock_placeholder_reduce         (GdlDockObject           *object);
static void     gdl_dock_placeholder_dock           (GdlDockObject           *object,
                                                     GdlDockObject           *requestor,
                                                     GdlDockPlacement         position,
                                                     GValue                  *other_data);

static void     gdl_dock_placeholder_weak_notify    (gpointer                 data,
                                                     GObject                 *old_object);

static void     disconnect_host                     (GdlDockPlaceholder      *ph);
static void     connect_host                        (GdlDockPlaceholder      *ph,
                                                     GdlDockObject           *new_host);
static void     do_excursion                        (GdlDockPlaceholder      *ph);

static void     gdl_dock_placeholder_present        (GdlDockObject           *object,
                                                     GdlDockObject           *child);


/* ----- Private variables and data structures ----- */

enum {
    PROP_0,
    PROP_STICKY,
    PROP_HOST,
    PROP_NEXT_PLACEMENT
};

struct _GdlDockPlaceholderPrivate {
    /* current object this placeholder is pinned to */
    GdlDockObject    *host;
    gboolean          sticky;
    
    /* when the placeholder is moved up the hierarchy, this stack
       keeps track of the necessary dock positions needed to get the
       placeholder to the original position */
    GSList           *placement_stack;

    /* connected signal handlers */
    guint             host_detach_handler;
    guint             host_dock_handler;
};


/* ----- Private interface ----- */

GNOME_CLASS_BOILERPLATE (GdlDockPlaceholder, gdl_dock_placeholder,
			 GdlDockObject, GDL_TYPE_DOCK_OBJECT);

static void 
gdl_dock_placeholder_class_init (GdlDockPlaceholderClass *klass)
{
    GObjectClass       *g_object_class;
    GtkObjectClass     *gtk_object_class;
    GtkContainerClass  *container_class;
    GdlDockObjectClass *object_class;
    
    g_object_class = G_OBJECT_CLASS (klass);
    gtk_object_class = GTK_OBJECT_CLASS (klass);
    container_class = GTK_CONTAINER_CLASS (klass);
    object_class = GDL_DOCK_OBJECT_CLASS (klass);

    g_object_class->get_property = gdl_dock_placeholder_get_property;
    g_object_class->set_property = gdl_dock_placeholder_set_property;
    
    g_object_class_install_property (
	g_object_class, PROP_STICKY,
	g_param_spec_boolean ("sticky", _("Sticky"),
			      _("Whether the placeholder will stick to its host or "
				"move up the hierarchy when the host is redocked"),
			      FALSE,
			      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    
    g_object_class_install_property (
	g_object_class, PROP_HOST,
	g_param_spec_object ("host", _("Host"),
			     _("The dock object this placeholder is attached to"),
			     GDL_TYPE_DOCK_OBJECT,
			     G_PARAM_READWRITE));
    
    /* this will return the top of the placement stack */
    g_object_class_install_property (
	g_object_class, PROP_NEXT_PLACEMENT,
	g_param_spec_enum ("next_placement", _("Next placement"),
			   _("The position an item will be docked to our host if a "
			     "request is made to dock to us"),
			   GDL_TYPE_DOCK_PLACEMENT,
			   GDL_DOCK_CENTER,
			   G_PARAM_READWRITE |
                           GDL_DOCK_PARAM_EXPORT | GDL_DOCK_PARAM_AFTER));
    
    gtk_object_class->destroy = gdl_dock_placeholder_destroy;
    container_class->add = gdl_dock_placeholder_add;
    
    object_class->is_compound = FALSE;
    object_class->detach = gdl_dock_placeholder_detach;
    object_class->reduce = gdl_dock_placeholder_reduce;
    object_class->dock = gdl_dock_placeholder_dock;
    object_class->present = gdl_dock_placeholder_present;
}

static void 
gdl_dock_placeholder_instance_init (GdlDockPlaceholder *ph)
{
    GTK_WIDGET_SET_FLAGS (ph, GTK_NO_WINDOW);
    GTK_WIDGET_UNSET_FLAGS (ph, GTK_CAN_FOCUS);
    
    ph->_priv = g_new0 (GdlDockPlaceholderPrivate, 1);
}

static void 
gdl_dock_placeholder_set_property (GObject      *g_object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (g_object);

    switch (prop_id) {
	case PROP_STICKY:
            if (ph->_priv)
                ph->_priv->sticky = g_value_get_boolean (value);
	    break;
	case PROP_HOST:
            gdl_dock_placeholder_attach (ph, g_value_get_object (value));
	    break;
        case PROP_NEXT_PLACEMENT:
            if (ph->_priv) {
                ph->_priv->placement_stack =
                    g_slist_prepend (ph->_priv->placement_stack,
                                     (gpointer) g_value_get_enum (value));
            }
            break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
	    break;
    }
}

static void 
gdl_dock_placeholder_get_property (GObject    *g_object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (g_object);

    switch (prop_id) {
	case PROP_STICKY:
            if (ph->_priv)
                g_value_set_boolean (value, ph->_priv->sticky);
            else
                g_value_set_boolean (value, FALSE);
	    break;
	case PROP_HOST:
            if (ph->_priv)
                g_value_set_object (value, ph->_priv->host);
            else
                g_value_set_object (value, NULL);
	    break;
	case PROP_NEXT_PLACEMENT:
            if (ph->_priv && ph->_priv->placement_stack)
                g_value_set_enum (value, (GdlDockPlacement) ph->_priv->placement_stack->data);
            else
                g_value_set_enum (value, GDL_DOCK_CENTER);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
	    break;
    }
}

static void
gdl_dock_placeholder_destroy (GtkObject *object)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (object);

    if (ph->_priv) {
        if (ph->_priv->host)
            gdl_dock_placeholder_detach (GDL_DOCK_OBJECT (object), FALSE);
        g_free (ph->_priv);
        ph->_priv = NULL;
    }

    GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void 
gdl_dock_placeholder_add (GtkContainer *container,
                          GtkWidget    *widget)
{
    GdlDockPlaceholder *ph;
    GdlDockPlacement    pos = GDL_DOCK_CENTER;   /* default position */
    
    g_return_if_fail (GDL_IS_DOCK_PLACEHOLDER (container));
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    ph = GDL_DOCK_PLACEHOLDER (container);
    if (ph->_priv->placement_stack)
        pos = (GdlDockPlacement) ph->_priv->placement_stack->data;
        
    gdl_dock_object_dock (GDL_DOCK_OBJECT (ph), GDL_DOCK_OBJECT (widget),
                          pos, NULL);
}

static void
gdl_dock_placeholder_detach (GdlDockObject *object,
                             gboolean       recursive)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (object);

    /* disconnect handlers */
    disconnect_host (ph);
    
    /* free the placement stack */
    g_slist_free (ph->_priv->placement_stack);
    ph->_priv->placement_stack = NULL;

    GDL_DOCK_OBJECT_UNSET_FLAGS (object, GDL_DOCK_ATTACHED);
}

static void 
gdl_dock_placeholder_reduce (GdlDockObject *object)
{
    /* placeholders are not reduced */
    return;
}

static void 
gdl_dock_placeholder_dock (GdlDockObject    *object,
			   GdlDockObject    *requestor,
			   GdlDockPlacement  position,
			   GValue           *other_data)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (object);
    
    if (ph->_priv->host) {
        /* we simply act as a proxy for our host */
        gdl_dock_object_dock (ph->_priv->host, requestor,
                              position, other_data);
    }
    else {
        GdlDockObject *toplevel;
        
        if (!gdl_dock_object_is_bound (GDL_DOCK_OBJECT (ph))) {
            g_warning (_("Attempt to dock a dock object to an unbound placeholder"));
            return;
        }
        
        /* dock the item as a floating of the controller */
        toplevel = gdl_dock_master_get_controller (GDL_DOCK_OBJECT_GET_MASTER (ph));
        gdl_dock_object_dock (toplevel, requestor,
                              GDL_DOCK_FLOATING, NULL);
    }
}

#ifdef PLACEHOLDER_DEBUG
static void
print_placement_stack (GdlDockPlaceholder *ph)
{
    GSList *s = ph->_priv->placement_stack;
    GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (GDL_TYPE_DOCK_PLACEMENT));
    GEnumValue *enum_value;
    gchar *name;
    GString *message;

    message = g_string_new (NULL);
    g_string_printf (message, "[%p] host: %p (%s), stack: ",
                     ph, ph->_priv->host, G_OBJECT_TYPE_NAME (ph->_priv->host));
    for (; s; s = s->next) {
        enum_value = g_enum_get_value (enum_class, (GdlDockPlacement) s->data);
        name = enum_value ? enum_value->value_name : NULL;
        g_string_append_printf (message, "%s, ", name);
    }
    g_message ("%s", message->str);
    
    g_string_free (message, TRUE);
    g_type_class_unref (enum_class);
}
#endif

static void 
gdl_dock_placeholder_present (GdlDockObject *object,
                              GdlDockObject *child)
{
    /* do nothing */
    return;
}

/* ----- Public interface ----- */

GtkWidget * 
gdl_dock_placeholder_new (gchar            *name,
                          GdlDockObject    *object,
                          GdlDockPlacement  position,
                          gboolean          sticky)
{
    GdlDockPlaceholder *ph;

    ph = GDL_DOCK_PLACEHOLDER (g_object_new (GDL_TYPE_DOCK_PLACEHOLDER,
                                             "name", name,
                                             "sticky", sticky,
                                             NULL));
    GDL_DOCK_OBJECT_UNSET_FLAGS (ph, GDL_DOCK_AUTOMATIC);

    if (object) {
        gdl_dock_placeholder_attach (ph, object);
        if (position == GDL_DOCK_NONE)
            position = GDL_DOCK_CENTER;
        g_object_set (G_OBJECT (ph), "next_placement", position, NULL);
        if (GDL_IS_DOCK (object)) {
            /* the top placement will be consumed by the toplevel
               dock, so add a dummy placement */
            g_object_set (G_OBJECT (ph), "next_placement", GDL_DOCK_CENTER, NULL);
        }
        /* try a recursion */
        do_excursion (ph);
    }
    
    return GTK_WIDGET (ph);
}

static void 
gdl_dock_placeholder_weak_notify (gpointer data,
                                  GObject *old_object)
{
    GdlDockPlaceholder *ph;
    
    g_return_if_fail (data != NULL && GDL_IS_DOCK_PLACEHOLDER (data));

    ph = GDL_DOCK_PLACEHOLDER (data);

    /* we shouldn't get here, so perform an emergency detach. instead
       we should have gotten a detach signal from our host */
    ph->_priv->host = NULL;
    /* free the placement stack */
    g_slist_free (ph->_priv->placement_stack);
    ph->_priv->placement_stack = NULL;

    GDL_DOCK_OBJECT_UNSET_FLAGS (ph, GDL_DOCK_ATTACHED);
}

static void
detach_cb (GdlDockObject *object,
           gboolean       recursive,
           gpointer       user_data)
{
    GdlDockPlaceholder *ph;
    GdlDockObject      *new_host, *obj;

    g_return_if_fail (user_data != NULL && GDL_IS_DOCK_PLACEHOLDER (user_data));
    
    /* we go up in the hierarchy and we store the hinted placement in
     * the placement stack so we can rebuild the docking layout later
     * when we get the host's dock signal.  */

    ph = GDL_DOCK_PLACEHOLDER (user_data);
    obj = ph->_priv->host;
    if (obj != object) {
        g_warning (_("Got a detach signal from an object (%p) who is not "
                     "our host %p"), object, ph->_priv->host);
        return;
    }
    
    /* skip sticky objects */
    if (ph->_priv->sticky)
        return;
    
    /* go up in the hierarchy */
    new_host = gdl_dock_object_get_parent_object (obj);

    while (new_host) {
        GdlDockPlacement pos = GDL_DOCK_NONE;
        
        /* get placement hint from the new host */
        if (gdl_dock_object_child_placement (new_host, obj, &pos)) {
            ph->_priv->placement_stack = g_slist_prepend (
                ph->_priv->placement_stack, (gpointer) pos);
        }
        else {
            g_warning (_("Something weird happened while getting the child "
                         "placement for %p from parent %p"), obj, new_host);
        }

        if (!GDL_DOCK_OBJECT_IN_DETACH (new_host))
            /* we found a "stable" dock object */
            break;
        
        obj = new_host;
        new_host = gdl_dock_object_get_parent_object (obj);
    }

    /* disconnect host */
    disconnect_host (ph);

    if (!new_host) {
        /* the toplevel was detached: we attach ourselves to the
           controller with an initial placement of floating */
        new_host = gdl_dock_master_get_controller (GDL_DOCK_OBJECT_GET_MASTER (ph));
        ph->_priv->placement_stack = g_slist_prepend (
            ph->_priv->placement_stack, (gpointer) GDL_DOCK_FLOATING);
    }
    if (new_host)
        connect_host (ph, new_host);

#ifdef PLACEHOLDER_DEBUG
    print_placement_stack (ph);
#endif
}

/**
 * do_excursion:
 * @ph: placeholder object
 *
 * Tries to shrink the placement stack by examining the host's
 * children and see if any of them matches the placement which is at
 * the top of the stack.  If this is the case, it tries again with the
 * new host.
 **/
static void
do_excursion (GdlDockPlaceholder *ph)
{
    if (ph->_priv->host &&
        !ph->_priv->sticky &&
        ph->_priv->placement_stack &&
        gdl_dock_object_is_compound (ph->_priv->host)) {

        GdlDockPlacement pos, stack_pos =
            (GdlDockPlacement) ph->_priv->placement_stack->data;
        GList           *children, *l;
        GdlDockObject   *host = ph->_priv->host;
        
        children = gtk_container_get_children (GTK_CONTAINER (host));
        for (l = children; l; l = l->next) {
            pos = stack_pos;
            gdl_dock_object_child_placement (GDL_DOCK_OBJECT (host),
                                             GDL_DOCK_OBJECT (l->data),
                                             &pos);
            if (pos == stack_pos) {
                /* remove the stack position */
                ph->_priv->placement_stack =
                    g_slist_remove_link (ph->_priv->placement_stack,
                                         ph->_priv->placement_stack);
                
                /* connect to the new host */
                disconnect_host (ph);
                connect_host (ph, GDL_DOCK_OBJECT (l->data));

                /* recurse... */
                if (!GDL_DOCK_OBJECT_IN_REFLOW (l->data))
                    do_excursion (ph);
                
                break;
            }
        }
        g_list_free (children);
    }
}

static void 
dock_cb (GdlDockObject    *object,
         GdlDockObject    *requestor,
         GdlDockPlacement  position,
         GValue           *other_data,
         gpointer          user_data)
{
    GdlDockPlacement    pos = GDL_DOCK_NONE;
    GdlDockPlaceholder *ph;
    
    g_return_if_fail (user_data != NULL && GDL_IS_DOCK_PLACEHOLDER (user_data));
    ph = GDL_DOCK_PLACEHOLDER (user_data);
    g_return_if_fail (ph->_priv->host == object);
    
    /* see if the given position is compatible for the stack's top
       element */
    if (!ph->_priv->sticky && ph->_priv->placement_stack) {
        pos = (GdlDockPlacement) ph->_priv->placement_stack->data;
        if (gdl_dock_object_child_placement (object, requestor, &pos)) {
            if (pos == (GdlDockPlacement) ph->_priv->placement_stack->data) {
                /* the position is compatible: excurse down */
                do_excursion (ph);
            }
        }
    }
#ifdef PLACEHOLDER_DEBUG
    print_placement_stack (ph);
#endif
}

static void
disconnect_host (GdlDockPlaceholder *ph)
{
    if (!ph->_priv->host)
        return;
    
    if (ph->_priv->host_detach_handler)
        g_signal_handler_disconnect (ph->_priv->host, ph->_priv->host_detach_handler);
    if (ph->_priv->host_dock_handler)
        g_signal_handler_disconnect (ph->_priv->host, ph->_priv->host_dock_handler);
    ph->_priv->host_detach_handler = 0;
    ph->_priv->host_dock_handler = 0;

    /* remove weak ref to object */
    g_object_weak_unref (G_OBJECT (ph->_priv->host),
                         gdl_dock_placeholder_weak_notify, ph);
    ph->_priv->host = NULL;
}

static void
connect_host (GdlDockPlaceholder *ph,
              GdlDockObject      *new_host)
{
    if (ph->_priv->host)
        disconnect_host (ph);
    
    ph->_priv->host = new_host;
    g_object_weak_ref (G_OBJECT (ph->_priv->host),
                       gdl_dock_placeholder_weak_notify, ph);

    ph->_priv->host_detach_handler =
        g_signal_connect (ph->_priv->host,
                          "detach",
                          (GCallback) detach_cb,
                          (gpointer) ph);
    
    ph->_priv->host_dock_handler =
        g_signal_connect (ph->_priv->host,
                          "dock",
                          (GCallback) dock_cb,
                          (gpointer) ph);
}

void
gdl_dock_placeholder_attach (GdlDockPlaceholder *ph,
                             GdlDockObject      *object)
{
    g_return_if_fail (ph != NULL && GDL_IS_DOCK_PLACEHOLDER (ph));
    g_return_if_fail (ph->_priv != NULL);
    g_return_if_fail (object != NULL);
    
    /* object binding */
    if (!gdl_dock_object_is_bound (GDL_DOCK_OBJECT (ph)))
        gdl_dock_object_bind (GDL_DOCK_OBJECT (ph), object->master);

    g_return_if_fail (GDL_DOCK_OBJECT (ph)->master == object->master);
        
    gdl_dock_object_freeze (GDL_DOCK_OBJECT (ph));
    
    /* detach from previous host first */
    if (ph->_priv->host)
        gdl_dock_object_detach (GDL_DOCK_OBJECT (ph), FALSE);

    connect_host (ph, object);
    
    GDL_DOCK_OBJECT_SET_FLAGS (ph, GDL_DOCK_ATTACHED);
    
    gdl_dock_object_thaw (GDL_DOCK_OBJECT (ph));
}

