/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <gdk/gdkx.h>
#include "gdl-tools.h"
#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-paned.h"
#include "gdl-dock-notebook.h"

/* Private prototypes */

static void  gdl_dock_class_init      (GdlDockClass *class);
static void  gdl_dock_init            (GdlDock *dock);
static void  gdl_dock_destroy         (GtkObject *object);

static void  gdl_dock_size_request    (GtkWidget      *widget,
                                       GtkRequisition *requisition);
static void  gdl_dock_size_allocate   (GtkWidget     *widget,
                                       GtkAllocation *allocation);
static void  gdl_dock_map             (GtkWidget *widget);
static void  gdl_dock_unmap           (GtkWidget *widget);
static void  gdl_dock_draw            (GtkWidget    *widget,
                                       GdkRectangle *area);
static void  gdl_dock_add             (GtkContainer *container,
                                       GtkWidget    *widget);
static void  gdl_dock_remove          (GtkContainer *container,
                                       GtkWidget    *widget);
static void  gdl_dock_forall          (GtkContainer *container,
                                       gboolean      include_internals,
                                       GtkCallback   callback,
                                       gpointer      callback_data);
static void  gdl_dock_drag_begin      (GdlDockItem  *item,
                                       gpointer      data);
static void  gdl_dock_drag_end        (GdlDockItem  *item, 
                                       gpointer      data);
static void  gdl_dock_drag_motion     (GdlDockItem  *item, 
                                       gint          x, 
                                       gint          y,
                                       gpointer      data);
static gint  item_name_compare        (gconstpointer a,
                                       gconstpointer b);
static void  gdl_dock_layout_build    (GdlDock     *dock,
                                       GdlDockItem *item,
                                       xmlNodePtr   node);


/* Class variables and definitions */

enum {
    LAYOUT_CHANGED,
    LAST_SIGNAL
};

static GtkContainerClass *parent_class = NULL;

static guint dock_signals [LAST_SIGNAL] = { 0 };

/* FIXME: should we calculate this from the items' properties */
#define SPLIT_RATIO  0.3

/* Private functions */

static void
gdl_dock_class_init (GdlDockClass *klass)
{
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    parent_class = gtk_type_class (gtk_container_get_type ());

    object_class->destroy = gdl_dock_destroy;

    widget_class->size_request = gdl_dock_size_request;
    widget_class->size_allocate = gdl_dock_size_allocate;
    widget_class->map = gdl_dock_map;
    widget_class->unmap = gdl_dock_unmap;
    widget_class->draw = gdl_dock_draw;
    container_class->add = gdl_dock_add;
    container_class->remove = gdl_dock_remove;
    container_class->forall = gdl_dock_forall;

    dock_signals [LAYOUT_CHANGED] =
        gtk_signal_new ("layout_changed",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (GdlDockClass, layout_changed),
                        gtk_marshal_NONE__NONE,
                        GTK_TYPE_NONE, 0);

    gtk_object_class_add_signals (object_class, dock_signals, LAST_SIGNAL);

    klass->layout_changed = NULL;
}

static void
gdl_dock_init (GdlDock *dock)
{
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (dock), GTK_NO_WINDOW);

    dock->root = NULL;
    dock->floating = NULL;
    dock->items = NULL;
    dock->xor_gc = NULL;
    dock->root_xor_gc = NULL;
}

static void
gdl_dock_destroy (GtkObject *object)
{
    GdlDock *dock = GDL_DOCK (object);

    /* destroy the xor gc */
    if (dock->xor_gc) {
        gdk_gc_destroy (dock->xor_gc);
        dock->xor_gc = NULL;
    }

    if (dock->root_xor_gc) {
        gdk_gc_destroy (dock->root_xor_gc);
        dock->root_xor_gc = NULL;
    }

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gdl_dock_size_request (GtkWidget      *widget,
                       GtkRequisition *requisition)
{
    GdlDock      *dock;
    GtkContainer *container;
    GList        *lp;
    guint         border_width;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK (widget));

    dock = GDL_DOCK (widget);
    container = GTK_CONTAINER (widget);
    border_width = container->border_width;

    /* make request to root */
    if (dock->root && GTK_WIDGET_VISIBLE (dock->root))
        gtk_widget_size_request (dock->root, requisition);
    else {
        requisition->width = 0;
        requisition->height = 0;
    };

    requisition->width += 2 * border_width;
    requisition->height += 2 * border_width;

    widget->requisition = *requisition;

    /* request to all floating dock items */
    for (lp = dock->floating; lp; lp = lp->next) {
        GtkWidget     *w;
        GtkRequisition float_item_requisition;

        w = lp->data;
        gtk_widget_size_request (w, &float_item_requisition);
    };
}


static void
gdl_dock_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
    GdlDock      *dock;
    GtkContainer *container;
    GList        *lp;
    guint         border_width;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK (widget));
    
    dock = GDL_DOCK (widget);
    container = GTK_CONTAINER (widget);
    border_width = container->border_width;

    widget->allocation = *allocation;

    /* reduce allocation by border width */
    allocation->x += border_width;
    allocation->y += border_width;
    allocation->width = MAX (1, allocation->width - 2 * border_width);
    allocation->height = MAX (1, allocation->height - 2 * border_width);

    if (dock->root && GTK_WIDGET_VISIBLE (dock->root))
        gtk_widget_size_allocate (dock->root, allocation);

    /* do allocation in floating items */
    for (lp = dock->floating; lp; lp = lp->next) {
        GtkWidget     *w;
        GtkAllocation  float_item_allocation;

        w = lp->data;
        float_item_allocation.x = 0;
        float_item_allocation.y = 0;
        float_item_allocation.width = w->requisition.width;
        float_item_allocation.height = w->requisition.height;
        gtk_widget_size_allocate (w, &float_item_allocation);
    };
}

static void
map_widget (GtkWidget *w)
{
    if (w != NULL && GTK_WIDGET_VISIBLE (w) && !GTK_WIDGET_MAPPED (w))
        gtk_widget_map (w);
}

static void
unmap_widget (GtkWidget *w)
{
    if (w != NULL && GTK_WIDGET_VISIBLE (w) && GTK_WIDGET_MAPPED (w))
        gtk_widget_unmap (w);
}

static void
map_widget_foreach (gpointer data,
                    gpointer user_data)
{
    map_widget (GTK_WIDGET (data));
}

static void
unmap_widget_foreach (gpointer data,
                      gpointer user_data)
{
    unmap_widget (GTK_WIDGET (data));
}

static void
gdl_dock_map (GtkWidget *widget)
{
    GdlDock *dock;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK(widget));

    if (GTK_WIDGET_CLASS (parent_class)->map != NULL)
        (* GTK_WIDGET_CLASS (parent_class)->map) (widget);

    dock = GDL_DOCK (widget);
    map_widget (dock->root);

    g_list_foreach (dock->floating, map_widget_foreach, NULL);
}

static void
gdl_dock_unmap (GtkWidget *widget)
{
    GdlDock *dock;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK(widget));

    dock = GDL_DOCK (widget);
    unmap_widget (dock->root);

    g_list_foreach (dock->floating, unmap_widget_foreach, NULL);

    if (GTK_WIDGET_CLASS (parent_class)->unmap != NULL)
        (* GTK_WIDGET_CLASS (parent_class)->unmap) (widget);
}

static void
draw_widget (GtkWidget    *widget, 
             GdkRectangle *area)
{
    GdkRectangle d_area;

    if (widget != NULL && gtk_widget_intersect (widget, area, &d_area))
        gtk_widget_draw (widget, &d_area);
}

static void
gdl_dock_draw (GtkWidget    *widget,
               GdkRectangle *area)
{
    GdlDock *dock;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK(widget));

    if (GTK_WIDGET_DRAWABLE (widget)) {
        GList *p;

        dock = GDL_DOCK (widget);
        draw_widget (dock->root, area);
        for (p = dock->floating; p != NULL; p = p->next)
            draw_widget (GTK_WIDGET (p->data), area);
    };
}

static void
gdl_dock_add (GtkContainer *container,
              GtkWidget    *widget)
{
    g_return_if_fail (container != NULL);
    g_return_if_fail (GDL_IS_DOCK (container));
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    gdl_dock_add_item (GDL_DOCK (container), 
                       GDL_DOCK_ITEM (widget), 
                       GDL_DOCK_TOP);  /* default position */
}

static void
gdl_dock_remove (GtkContainer *container,
                 GtkWidget    *widget)
{
    GdlDock  *dock;
    gboolean  was_visible;

    g_return_if_fail (container != NULL);
    g_return_if_fail (widget != NULL);

    dock = GDL_DOCK (container);
    was_visible = GTK_WIDGET_VISIBLE (widget);

    if (dock->root == widget) {
        dock->root = NULL;
        gtk_widget_unparent (widget);
        if (was_visible && GTK_WIDGET_VISIBLE (GTK_WIDGET (container)))
            gtk_widget_queue_resize (GTK_WIDGET (dock));

    } else {
        GList *lp;

        for (lp = dock->floating; lp; lp = lp->next) {
            GtkWidget *w;

            w = lp->data;
            if (w == widget) {
                gtk_widget_unparent (w);
                dock->floating = g_list_remove_link (dock->floating, lp);
                g_list_free (lp);
                return;
            }
        }
    }
}

static void
gdl_dock_forall (GtkContainer *container,
                 gboolean      include_internals,
                 GtkCallback   callback,
                 gpointer      callback_data)
{
    GdlDock *dock;
    GList   *lp;

    g_return_if_fail (container != NULL);
    g_return_if_fail (GDL_IS_DOCK (container));
    g_return_if_fail (callback != NULL);

    dock = GDL_DOCK (container);

    if (dock->root)
        (*callback) (dock->root, callback_data);

    for (lp = dock->floating; lp; lp = lp->next) {
        GtkWidget *w;
        
        w = lp->data;
        (* callback) (w, callback_data);
    };
}

static void
gdl_dock_xor_rect (GdlDock *dock, GdkRectangle *rect, gboolean floating)
{
    GtkWidget *widget;
    gint8      dash_list [2];
    GdkWindow *window;
    GdkGC     *gc;

    widget = GTK_WIDGET (dock);

    if (!dock->xor_gc) {
        if (GTK_WIDGET_REALIZED (GTK_WIDGET (dock))) {
            GdkGCValues values;

            values.function = GDK_INVERT;
            values.subwindow_mode = GDK_INCLUDE_INFERIORS;
            dock->xor_gc = gdk_gc_new_with_values 
                (widget->window, &values, GDK_GC_FUNCTION | GDK_GC_SUBWINDOW);
        } else 
            return;
    };

    if (!dock->root_xor_gc) {
        if (GTK_WIDGET_REALIZED (GTK_WIDGET (dock))) {
            GdkGCValues values;

            values.function = GDK_INVERT;
            values.subwindow_mode = GDK_INCLUDE_INFERIORS;
            dock->root_xor_gc = gdk_gc_new_with_values 
                (GDK_ROOT_PARENT (), &values, GDK_GC_FUNCTION | GDK_GC_SUBWINDOW);
        } else 
            return;
    };

    if (floating) {
        window = GDK_ROOT_PARENT ();
        gc = dock->root_xor_gc;
    } else {
        window = widget->window;
        gc = dock->xor_gc;
    };

    gdk_gc_set_line_attributes (gc, 1, GDK_LINE_ON_OFF_DASH,
                                GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);
    
    dash_list[0] = 1;
    dash_list[1] = 1;
    gdk_gc_set_dashes (gc, 1, dash_list, 2);

    gdk_draw_rectangle (window, gc, 0, 
                        rect->x, rect->y,
                        rect->width, rect->height);

    gdk_gc_set_dashes (gc, 0, dash_list, 2);

    gdk_draw_rectangle (window, gc, 0, 
                        rect->x + 1, rect->y + 1,
                        rect->width - 2, rect->height - 2);

    dock->rect_drawn = ~dock->rect_drawn;
}

static gint
item_name_compare (gconstpointer a,
                   gconstpointer b)
{
    GdlDockItem *item;
    item = GDL_DOCK_ITEM (a);
    return strcmp (item->name, b);
}

static void
gdl_dock_drag_begin (GdlDockItem *item,
                     gpointer     data)
{
    GdlDock *dock;

    g_return_if_fail (data != NULL);
    g_return_if_fail (item != NULL);

    dock = GDL_DOCK (data);

    /* Set the target to itself so it won't go floating with just a click. */
    dock->possible_target.target = GTK_WIDGET (item);
    dock->possible_target.position = GDL_DOCK_FLOATING;
    dock->possible_target.requestor = item;
    dock->rect_drawn = FALSE;
}

static void
gdl_dock_drag_end (GdlDockItem *item, 
                   gpointer     data)
{
    GdlDock   *dock;
    GtkWidget *widget, *target;
    gboolean   layout_changed = TRUE;

    g_return_if_fail (data != NULL);
    g_return_if_fail (item != NULL);

    dock = GDL_DOCK (data);
    widget = GTK_WIDGET (item);
    target = dock->possible_target.target;

    /* Erase previously drawn rectangle */
    if (dock->rect_drawn)
        gdl_dock_xor_rect (dock, &dock->possible_target.rect,
                           (dock->possible_target.position 
                            == GDL_DOCK_FLOATING));

    if (target) {
        if (target == GTK_WIDGET (dock)) {
            /* It's a floating item that wants to dock with us, but we don't
               have non-floating items yet. gdl_dock_add_item instead of
               gdl_dock_item_dock_to. */
            gtk_widget_ref (widget);
            gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
            gdl_dock_add_item (dock, item, dock->possible_target.position);
            gtk_widget_unref (widget);
        } else if (target != widget)
            gdl_dock_item_dock_to (item, GDL_DOCK_ITEM (target),
                                   dock->possible_target.position, -1);
        else
            layout_changed = FALSE;

    } else if (!GDL_DOCK_ITEM_IS_FLOATING (item))
        /* Undock item and make it float. */
        gdl_dock_item_dock_to (item, NULL, GDL_DOCK_FLOATING, -1);

    if (layout_changed)
        gtk_signal_emit (GTK_OBJECT (dock), dock_signals [LAYOUT_CHANGED]);
}

static void
gdl_dock_drag_motion (GdlDockItem *item, 
                      gint         x, 
                      gint         y,
                      gpointer     data)
{
    GdlDock       *dock;
    GtkWidget     *widget;
    GtkAllocation *alloc;
    gboolean       should_float = TRUE;
    gint           win_x, win_y;
    gint           border_width;
    gint           rel_x, rel_y;
    GdkRectangle   old_rect;
    gboolean       was_floating;

    g_return_if_fail (data != NULL);
    g_return_if_fail (item != NULL);

    dock = GDL_DOCK (data);
    widget = GTK_WIDGET (item);
    border_width = GTK_CONTAINER (dock)->border_width;

    /* Calculate position relative to window origin. */
    gdk_window_get_origin (GTK_WIDGET (dock)->window, &win_x, &win_y);
    rel_x = x - win_x;
    rel_y = y - win_y;

    /* Get dock size. */
    alloc = &(GTK_WIDGET (dock)->allocation);
    
    old_rect = dock->possible_target.rect;
    was_floating = dock->possible_target.position == GDL_DOCK_FLOATING;

    /* Check if coordinates are in GdlDock widget. */
    if (rel_x > alloc->x && rel_x < alloc->x + alloc->width &&
        rel_y > alloc->y && rel_y < alloc->y + alloc->height) {

        gint new_x, new_y;

        /* It's inside our area. */
        new_x = rel_x - (alloc->x + border_width);
        new_y = rel_y - (alloc->y + border_width);
        
        /* Coordinates are inside GdlDock, so no floating atm. */
        should_float = FALSE;

	/* Set docking indicator rectangle to the GdlDock size. */
        dock->possible_target.rect.x = alloc->x + border_width;
        dock->possible_target.rect.y = alloc->y + border_width;
        dock->possible_target.rect.width = alloc->width - 2 * border_width;
        dock->possible_target.rect.height = alloc->height - 2 * border_width;

	/* If GdlDock has no root item yet, set the dock itself as 
	   possible target. */
        if (!dock->root) {
            dock->possible_target.position = GDL_DOCK_TOP;
            dock->possible_target.target = GTK_WIDGET (dock);
        } else {
            dock->possible_target.target = dock->root;

            /* See if it's in the border_width band. */
            if (new_x < 0) {
                dock->possible_target.position = GDL_DOCK_LEFT;
                dock->possible_target.rect.width *= SPLIT_RATIO;
            } else if (new_x > alloc->width - 2 * border_width) {
                dock->possible_target.position = GDL_DOCK_RIGHT;
                dock->possible_target.rect.x += 
                    dock->possible_target.rect.width * (1 - SPLIT_RATIO);
                dock->possible_target.rect.width *= SPLIT_RATIO;
            } else if (new_y < 0) {
                dock->possible_target.position = GDL_DOCK_TOP;
                dock->possible_target.rect.height *= SPLIT_RATIO;
            } else if (new_y > alloc->height - 2 * border_width) {
                dock->possible_target.position = GDL_DOCK_BOTTOM;
                dock->possible_target.rect.y += 
                    dock->possible_target.rect.height * (1 - SPLIT_RATIO);
                dock->possible_target.rect.height *= SPLIT_RATIO;
            } else { /* Otherwise try our children. */
                should_float = 
                    !gdl_dock_item_dock_request (GDL_DOCK_ITEM (dock->root), 
                                                 rel_x, rel_y,
                                                 &dock->possible_target);
                if (!should_float) {
                    dock->possible_target.rect.x += alloc->x + border_width;
                    dock->possible_target.rect.y += alloc->y + border_width;
                }
            }
        }
    }
    
    /* FIXME: should we try to snap to floating items too? */

    if (should_float) {
        dock->possible_target.target = NULL;
        dock->possible_target.position = GDL_DOCK_FLOATING;

        dock->possible_target.rect.width = item->float_width;
        dock->possible_target.rect.height = item->float_height;
            
        gdl_dock_item_drag_floating (item, x, y);
        dock->possible_target.rect.x = item->float_x;
        dock->possible_target.rect.y = item->float_y;
    };

    /* if the rectangle hasn't changed, and we should erase it and then
       redraw it, don't do it to avoid flicker */
    if (!((old_rect.x == dock->possible_target.rect.x &&
           old_rect.y == dock->possible_target.rect.y &&
           old_rect.width == dock->possible_target.rect.width &&
           old_rect.height == dock->possible_target.rect.height) &&
          dock->rect_drawn &&
          (!GDL_DOCK_ITEM_IS_FLOATING (item) || !should_float))) {
        /* Erase xor'ed rectangle */
        if (dock->rect_drawn)
            gdl_dock_xor_rect (dock, &old_rect, was_floating);
        
        if (!GDL_DOCK_ITEM_IS_FLOATING (item) || !should_float)
            gdl_dock_xor_rect (dock, &dock->possible_target.rect, 
                               should_float);
    };
}

static void
gdl_dock_layout_build (GdlDock     *dock,
                       GdlDockItem *item,
                       xmlNodePtr   node)
{
    gchar     *name;
    GtkWidget *widget;
    
    g_return_if_fail (dock != NULL);
    g_return_if_fail (node != NULL);
        
    for (; node; node = node->next) {
        name = (gchar *)node->name;
    
        /* Create/get root container/item. */
        if (!strcmp ("paned", name)) {
            /* Create new GdlDockPaned with specified orientation. */
            if (!strcmp ("horizontal", xmlGetProp (node, "orientation")))
                widget = gdl_dock_paned_new (GTK_ORIENTATION_HORIZONTAL);
            else
                widget = gdl_dock_paned_new (GTK_ORIENTATION_VERTICAL);
            
            gdl_dock_bind_item (dock, GDL_DOCK_ITEM (widget));

            /* Recurse. */
            if (node->childs)
                gdl_dock_layout_build (dock, GDL_DOCK_ITEM (widget), 
                                       node->childs);
        
            /* Set divider location. */
            gdl_dock_paned_set_position (GDL_DOCK_PANED (widget), 
                                         atoi (xmlGetProp (node, "divider")));

        } else if (!strcmp ("notebook", name)) {
            /* Create new GdlDockNotebook. */
            widget = gdl_dock_notebook_new ();
        
            gdl_dock_bind_item (dock, GDL_DOCK_ITEM (widget));

            /* Recurse. */
            if (node->childs)
                gdl_dock_layout_build (dock, GDL_DOCK_ITEM (widget), 
                                       node->childs);

            /* Set tab index. */
            gtk_notebook_set_page (GTK_NOTEBOOK (
                    GDL_DOCK_NOTEBOOK (widget)->notebook), 
                    atoi (xmlGetProp (node, "index")));

        } else
            widget = GTK_WIDGET (gdl_dock_get_item_by_name (
                dock, (gchar *)xmlGetProp (node, "name")));

        /* Add widget to dock. */
        if (!item)
            gtk_container_add (GTK_CONTAINER (dock), widget);
        else
            gtk_container_add (GTK_CONTAINER (item), widget);
    }        
}

/* Public interface */

GtkWidget *
gdl_dock_new (void)
{
    return gtk_type_new (gdl_dock_get_type ());
}

guint
gdl_dock_get_type (void)
{
    static GtkType dock_type = 0;

    if (dock_type == 0) {
        GtkTypeInfo dock_info = {
            "GdlDock",
            sizeof (GdlDock),
            sizeof (GdlDockClass),
            (GtkClassInitFunc) gdl_dock_class_init,
            (GtkObjectInitFunc) gdl_dock_init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL,
        };

        dock_type = gtk_type_unique (gtk_container_get_type (), &dock_info);
    }

    return dock_type;
}

void
gdl_dock_add_item (GdlDock          *dock,
                   GdlDockItem      *item,
                   GdlDockPlacement  placement)
{
    g_return_if_fail (dock != NULL);
    g_return_if_fail (item != NULL);

    gdl_dock_bind_item (dock, item);
    g_return_if_fail (GDL_DOCK_ITEM_CHECK_BOND (item, dock));
    
    /* Add the item and make it float. */
    if (placement == GDL_DOCK_FLOATING)
        gdl_dock_add_floating_item (dock, item, 0, 0, 
                                    GTK_ORIENTATION_HORIZONTAL);
    else { /* Non-floating item. */
    	/* If a widget has already been added to the GdlDock, add the item
    	   to GdlDock using gdl_dock_item_dock_to. */
        if (dock->root)
            gdl_dock_item_dock_to (item, GDL_DOCK_ITEM (dock->root), 
                                   placement, -1);
        else { /* Item about to be added is root item. */
            GtkWidget *widget;

            dock->root = widget = GTK_WIDGET (item);
            item->dock = GTK_WIDGET (dock);
            
            /* Unfloat item. */
            if (GDL_DOCK_ITEM_IS_FLOATING (item))
                gdl_dock_item_window_sink (item);

            gdl_dock_item_show_handle (item);

            /* Set widget parent to GdlDock. */
            gtk_widget_set_parent (widget, GTK_WIDGET (dock));

	    /* Realize the item (create its corresponding GdkWindow) when 
	       GdlDock has been realized. */
            if (GTK_WIDGET_REALIZED (widget->parent))
                gtk_widget_realize (widget);

	    /* Map the widget if it's visible and the parent is visible and has 
	       been mapped. This is done to make sure that the GdkWindow is 
	       visible. */
            if (GTK_WIDGET_VISIBLE (widget->parent) && 
                GTK_WIDGET_VISIBLE (widget)) {
                if (GTK_WIDGET_MAPPED (widget->parent))
                    gtk_widget_map (widget);

		/* Make the widget resize. */
                gtk_widget_queue_resize (widget);
            }
        }
    }
}

void
gdl_dock_add_floating_item (GdlDock        *dock,
                            GdlDockItem    *item,
                            gint            x,
                            gint            y,
                            GtkOrientation  orientation)
{
    GtkWidget *widget;

    g_return_if_fail (dock != NULL);
    g_return_if_fail (item != NULL);
    
    gdl_dock_bind_item (dock, item);
    g_return_if_fail (GDL_DOCK_ITEM_CHECK_BOND (item, dock));
    
    /* Keep a list of all floating items. */
    dock->floating = g_list_prepend (dock->floating, item);

    widget = GTK_WIDGET (item);
    
    /* Set widget parent to GdlDock. */
    gtk_widget_set_parent (widget, GTK_WIDGET (dock));

    /* show item's drag handle so the user can move it around */
    gdl_dock_item_show_handle (item);

    /* Realize the item (create its corresponding GdkWindow) when GdlDock
       has been realized. */
    if (GTK_WIDGET_REALIZED (widget->parent))
        gtk_widget_realize (widget);

    /* Map the widget if it's visible and the parent is visible and has 
       been mapped. This is done to make sure that the GdkWindow is visible. */
    if (GTK_WIDGET_VISIBLE (widget->parent) && 
        GTK_WIDGET_VISIBLE (widget)) {
        if (GTK_WIDGET_MAPPED (widget->parent))
            gtk_widget_map (widget);

	/* Make the widget resize. */
        gtk_widget_queue_resize (widget);
    }

    /* Make the item float. */
    gdl_dock_item_drag_floating (item, x, y);
    gdl_dock_item_window_float (item);
}

void
gdl_dock_layout_load (GdlDock *dock, xmlNodePtr node)
{
    GList     *lp;
    GtkWidget *widget;
    
    g_return_if_fail (dock != NULL);
    g_return_if_fail (node != NULL);

    /* Remove all items. */
    for (lp = dock->items; lp; lp = lp->next) {
        widget = GTK_WIDGET (lp->data);
        if (widget->parent)
            gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
    };
    for (lp = dock->floating; lp; ) {
        widget = GTK_WIDGET (lp->data);
        lp = lp->next;
        gtk_container_remove (GTK_CONTAINER (dock), widget);
    };
    if (dock->root)
        gtk_container_remove (GTK_CONTAINER (dock), dock->root);

    /* FIXME: we need to correctly restore floating hierarchies.
       Right now, they are saved fine because of the recursive nature of
       the process, but it's only restored the top widget, which besides
       that must have a name */
    /* Build layout. */    
    for (; node; node = node->next) {
        if (!strcmp ("docked", node->name))
            gdl_dock_layout_build (dock, NULL, node->childs);
        else if (!strcmp ("undocked", node->name)) {
            gdl_dock_add_floating_item (dock, 
                                        gdl_dock_get_item_by_name (
                                            dock,
                                            xmlGetProp (node->childs, "name")), 
                                        atoi (xmlGetProp (node, "x")),
                                        atoi (xmlGetProp (node, "y")), 
                                        GTK_ORIENTATION_HORIZONTAL);
        }
    }
    
    /* Show all items. */
    gtk_widget_show_all (dock->root);
}

void
gdl_dock_layout_save (GdlDock *dock, xmlNodePtr node)
{
    GList *lp;
    xmlNodePtr child_node;
    GdlDockItem *item;
    gchar buffer[8];
    
    g_return_if_fail (dock != NULL);
    g_return_if_fail (node != NULL);
    
    /* Check if there is something to save. */
    if (dock->root && GDL_IS_DOCK_ITEM (dock->root)) {    
        /* Recursively save the layout. */
        child_node = xmlNewChild (node, NULL, "docked", NULL);
        gdl_dock_item_save_layout (GDL_DOCK_ITEM (dock->root), child_node);
    }
        
    /* Save layout of all floating items. */
    for (lp = dock->floating; lp; lp = lp->next) {
        item = GDL_DOCK_ITEM (lp->data);
        child_node = xmlNewChild (node, NULL, "undocked", NULL);
        sprintf (buffer, "%i", item->float_x);
        xmlSetProp (child_node, "x", buffer);
        sprintf (buffer, "%i", item->float_y);
        xmlSetProp (child_node, "y", buffer);
        sprintf (buffer, "%i", item->float_width);
        xmlSetProp (child_node, "width", buffer);
        sprintf (buffer, "%i", item->float_height);
        xmlSetProp (child_node, "height", buffer);
        
        gdl_dock_item_save_layout (item, child_node);
    }    
}

void
gdl_dock_bind_item (GdlDock          *dock,
                    GdlDockItem      *item)
{
    g_return_if_fail (dock != NULL);
    g_return_if_fail (item != NULL);

    if (item->dock == GTK_WIDGET (dock))
        /* nothing to do */
        return;

    if (GDL_DOCK_ITEM_IS_BOUND (item)) {
        g_warning (_("Attempt to bind an already bound dockitem"));
        return;
    };

    /* FIXME: check for item name duplication */

    /* bind the item: keep a ref only if the item has a name */
    if (item->name) {
        gtk_widget_ref (GTK_WIDGET (item));
        dock->items = g_list_prepend (dock->items, item);
    };

    item->dock = GTK_WIDGET (dock);

    /* connect the signals */
    gtk_signal_connect (GTK_OBJECT (item), "dock_drag_begin",
                        (GtkSignalFunc) gdl_dock_drag_begin,
                        (gpointer) dock);
    gtk_signal_connect (GTK_OBJECT (item), "dock_drag_motion",
                        (GtkSignalFunc) gdl_dock_drag_motion,
                        (gpointer) dock);
    gtk_signal_connect (GTK_OBJECT (item), "dock_drag_end",
                        (GtkSignalFunc) gdl_dock_drag_end,
                        (gpointer) dock);
}

void
gdl_dock_unbind_item (GdlDock          *dock,
                      GdlDockItem      *item)
{
    GList     *link;
    GtkWidget *widget;

    g_return_if_fail (dock != NULL);
    g_return_if_fail (item != NULL);
    g_return_if_fail (GDL_DOCK_ITEM_IS_BOUND (item));
    g_return_if_fail (item->dock == GTK_WIDGET (dock));
    
    /* disconnect signals */
    gtk_signal_disconnect_by_data (GTK_OBJECT (item), (gpointer) dock);

    /* undock item if it's docked */
    item->dock = NULL;
    widget = GTK_WIDGET (item);
    if (widget->parent)
        gtk_container_remove (GTK_CONTAINER (widget->parent), widget);

    /* find the item in the list */
    link = g_list_find (dock->items, item);
    if (link) {
        dock->items = g_list_remove_link (dock->items, link);
        gtk_widget_unref (widget);
    };
}

GdlDockItem *
gdl_dock_get_item_by_name (GdlDock     *dock,
                           const gchar *name)
{
    GList *l;

    g_return_val_if_fail (dock != NULL, NULL);
    g_return_val_if_fail (name != NULL, NULL);

    l = g_list_find_custom (dock->items, (gpointer) name, item_name_compare);
    if (l)
        return GDL_DOCK_ITEM (l->data);
    else
        return NULL;
}

