/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "gdl-tools.h"
#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-notebook.h"
#include "gdl-dock-paned.h"

/* Private prototypes */

static void  gdl_dock_item_class_init    (GdlDockItemClass *class);
static void  gdl_dock_item_init          (GdlDockItem *item);

static void  gdl_dock_item_set_arg       (GtkObject *object,
                                          GtkArg *arg,
                                          guint arg_id);
static void  gdl_dock_item_get_arg       (GtkObject *object,
                                          GtkArg *arg,
                                          guint arg_id);
static void  gdl_dock_item_destroy       (GtkObject *object);

static void  gdl_dock_item_size_request  (GtkWidget *widget,
                                          GtkRequisition *requisition);
static void  gdl_dock_item_size_allocate (GtkWidget *widget,
                                          GtkAllocation *allocation);
static void  gdl_dock_item_map           (GtkWidget *widget);
static void  gdl_dock_item_unmap         (GtkWidget *widget);
static void  gdl_dock_item_draw          (GtkWidget *widget,
                                          GdkRectangle *area);
static gint  gdl_dock_item_expose        (GtkWidget *widget,
                                          GdkEventExpose *event);
static void  gdl_dock_item_realize       (GtkWidget *widget);
static void  gdl_dock_item_unrealize     (GtkWidget *widget);
static void  gdl_dock_item_style_set     (GtkWidget *widget,
                                          GtkStyle  *previous_style);
static void  gdl_dock_item_set_floating  (GdlDockItem *item, 
                                          gboolean val);
static gint  gdl_dock_item_button_changed (GtkWidget *widget,
                                           GdkEventButton *event);
static gint  gdl_dock_item_delete_event  (GtkWidget *widget,
                                          GdkEventAny *event);
static gint  gdl_dock_item_motion        (GtkWidget *widget,
                                          GdkEventMotion *event);
static void  gdl_dock_item_grab_pointer  (GdlDockItem *item);
static void  gdl_dock_item_location      (GtkWidget *widget,
                                          GdlDockItem *item);


/* Class variables and definitions */

enum {
    ARG_0,
    ARG_ORIENTATION,
    ARG_RESIZE,
    ARG_SHRINK
};

static GtkBinClass *parent_class = NULL;

/* FIXME: should be an arg of the widget */
#define DRAG_HANDLE_SIZE  10
#define GDL_DOCK_ITEM_NOT_LOCKED(x) (!(GDL_DOCK_ITEM (x)->behavior && GDL_DOCK_ITEM_BEH_LOCKED))

/* FIXME: implement the rest of the behaviors */

#define SPLIT_RATIO  0.4

/* Private functions */

static void
gdl_dock_item_class_init (GdlDockItemClass *klass)
{
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    parent_class = gtk_type_class (gtk_bin_get_type ());

    gtk_object_add_arg_type ("GdlDockItem::orientation",
                             GTK_TYPE_ORIENTATION, GTK_ARG_READWRITE,
                             ARG_ORIENTATION);

    gtk_object_add_arg_type ("GdlDockItem::resize",
                             GTK_TYPE_BOOL, GTK_ARG_READWRITE,
                             ARG_RESIZE);

    gtk_object_add_arg_type ("GdlDockItem::shrink",
                             GTK_TYPE_BOOL, GTK_ARG_READWRITE,
                             ARG_SHRINK);

    object_class->set_arg = gdl_dock_item_set_arg;
    object_class->get_arg = gdl_dock_item_get_arg;
    object_class->destroy = gdl_dock_item_destroy;

    widget_class->realize = gdl_dock_item_realize;
    widget_class->unrealize = gdl_dock_item_unrealize;
    widget_class->map = gdl_dock_item_map;
    widget_class->unmap = gdl_dock_item_unmap;
    widget_class->size_request = gdl_dock_item_size_request;
    widget_class->size_allocate = gdl_dock_item_size_allocate;
    widget_class->style_set = gdl_dock_item_style_set;
    widget_class->draw = gdl_dock_item_draw;
    widget_class->expose_event = gdl_dock_item_expose;
    widget_class->button_press_event = gdl_dock_item_button_changed;
    widget_class->button_release_event = gdl_dock_item_button_changed;
    widget_class->motion_notify_event = gdl_dock_item_motion;
    widget_class->delete_event = gdl_dock_item_delete_event;

    klass->auto_reduce = NULL;
    klass->drag_request = NULL;
    klass->set_orientation = NULL;
}

static void
gdl_dock_item_init (GdlDockItem *item)
{
    GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (item), GTK_NO_WINDOW);

    item->name = NULL;
    item->bin_window = NULL;
    item->float_window = NULL;

    item->is_floating = FALSE;
    item->float_window_mapped = FALSE;
    item->resize = TRUE;
    item->shrink = FALSE;
    item->orientation = GTK_ORIENTATION_HORIZONTAL;
    item->behavior = GDL_DOCK_ITEM_BEH_NORMAL;

    item->float_x = item->float_y = 0;
    item->float_width = item->float_height = 0;
    item->dragoff_x = item->dragoff_y = 0;
    item->in_drag = FALSE;
    item->in_resize = FALSE;
}

static void
gdl_dock_item_set_arg (GtkObject *object,
                       GtkArg    *arg,
                       guint      arg_id)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (object);

    switch (arg_id) {
    case ARG_ORIENTATION:
        gdl_dock_item_set_orientation (item, GTK_VALUE_ENUM (*arg));
        break;
    case ARG_RESIZE:
        item->resize = GTK_VALUE_BOOL (*arg);
        gtk_widget_queue_resize (GTK_WIDGET (object));
        break;
    case ARG_SHRINK:
        item->shrink = GTK_VALUE_BOOL (*arg);
        gtk_widget_queue_resize (GTK_WIDGET (object));
        break;
    default:
        break;
    }
}

static void
gdl_dock_item_get_arg (GtkObject *object,
                       GtkArg    *arg,
                       guint      arg_id)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (object);
    
    switch (arg_id) {
    case ARG_ORIENTATION:
        GTK_VALUE_ENUM (*arg) = item->orientation;
        break;
    case ARG_RESIZE:
        GTK_VALUE_BOOL (*arg)= item->resize;
        break;
    case ARG_SHRINK:
        GTK_VALUE_BOOL (*arg)= item->shrink;
        break;
    default:
        arg->type = GTK_TYPE_INVALID;
        break;
    }
}

static void
gdl_dock_item_destroy (GtkObject *object)
{
    g_free (GDL_DOCK_ITEM (object)->name);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gdl_dock_item_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
    GtkBin         *bin;
    GtkRequisition  child_requisition;
    GdlDockItem    *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));
    g_return_if_fail (requisition != NULL);

    bin = GTK_BIN (widget);
    item = GDL_DOCK_ITEM (widget);

    /* If our child is not visible, we still request its size, since
       we won't have any useful hint for our size otherwise.  */
    if (bin->child)
        gtk_widget_size_request (bin->child, &child_requisition);
    else {
        child_requisition.width = 0;
        child_requisition.height = 0;
    }

    if (item->orientation == GTK_ORIENTATION_HORIZONTAL) {
        requisition->width = 
            GDL_DOCK_ITEM_NOT_LOCKED (item) ? DRAG_HANDLE_SIZE : 0;
        if (bin->child) {
            requisition->width += child_requisition.width;
            requisition->height = child_requisition.height;
        } else
            requisition->height = 0;
    } else {
        requisition->height = 
            GDL_DOCK_ITEM_NOT_LOCKED (item) ? DRAG_HANDLE_SIZE : 0;
        if (bin->child) {
            requisition->width = child_requisition.width;
            requisition->height += child_requisition.height;
        } else
            requisition->width = 0;
    }

    requisition->width += GTK_CONTAINER (widget)->border_width * 2;
    requisition->height += GTK_CONTAINER (widget)->border_width * 2;

    widget->requisition = *requisition;
}

static void
gdl_dock_item_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
    GtkBin      *bin;
    GdlDockItem *item;
  
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));
    g_return_if_fail (allocation != NULL);
  
    bin = GTK_BIN (widget);
    item = GDL_DOCK_ITEM (widget);

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize (widget->window,
                                widget->allocation.x,
                                widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height);

    if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
        GtkWidget     *child;
        GtkAllocation  child_allocation;
        int            border_width;

        child = bin->child;
        border_width = GTK_CONTAINER (widget)->border_width;

        child_allocation.x = border_width;
        child_allocation.y = border_width;

        if (GDL_DOCK_ITEM_NOT_LOCKED (item)) {
            if (item->orientation == GTK_ORIENTATION_HORIZONTAL)
                child_allocation.x += DRAG_HANDLE_SIZE;
            else
                child_allocation.y += DRAG_HANDLE_SIZE;
        };

        if (item->is_floating) {
            GtkRequisition  child_requisition;
            gint            float_width, float_height;
            gint            desired_width, desired_height;

            desired_width = item->float_width;
            desired_height = item->float_height;

            gtk_widget_get_child_requisition (child, &child_requisition);

            child_allocation.width = child_requisition.width;
            child_allocation.height = child_requisition.height;

            float_width = child_allocation.width + 2 * border_width;
            float_height = child_allocation.height + 2 * border_width;
          
            if (item->orientation == GTK_ORIENTATION_HORIZONTAL)
                float_width += 1.5 * DRAG_HANDLE_SIZE;
            else
                float_height += 1.5 * DRAG_HANDLE_SIZE;

            /* recalculate child allocation if floating size was bigger */
            if (item->float_width > float_width)
                child_allocation.width += (item->float_width - float_width);
            else
                item->float_width = float_width;
                
            if (item->float_height > float_height)
                child_allocation.height += (item->float_height - float_height);
            else
                item->float_height = float_height;

            if (GTK_WIDGET_REALIZED (item)) {
                gdk_window_resize (item->float_window,
                                   item->float_width, item->float_height);
                gdk_window_move_resize (item->bin_window,
                                        0, 0,
                                        item->float_width, item->float_height);
            }

            if (item->in_resize) {
                item->dragoff_x += item->float_width - desired_width;
                item->dragoff_y += item->float_height - desired_height;
            };

        } else {
            child_allocation.width = 
                MAX (1, (int) widget->allocation.width - 2 * border_width);
            child_allocation.height = 
                MAX (1, (int) widget->allocation.height - 2 * border_width);

            if (GDL_DOCK_ITEM_NOT_LOCKED (item)) {
                if (item->orientation == GTK_ORIENTATION_HORIZONTAL)
                    child_allocation.width = MAX 
                        ((int) child_allocation.width - DRAG_HANDLE_SIZE, 1);
                else
                    child_allocation.height = MAX 
                        ((int) child_allocation.height - DRAG_HANDLE_SIZE, 1);
            };

            if (GTK_WIDGET_REALIZED (item))
                gdk_window_move_resize (item->bin_window,
                                        0, 0,
                                        widget->allocation.width,
                                        widget->allocation.height);
        }

        gtk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
gdl_dock_item_map (GtkWidget *widget)
{
    GtkBin      *bin;
    GdlDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

    bin = GTK_BIN (widget);
    item = GDL_DOCK_ITEM (widget);

    gdk_window_show (item->bin_window);
    if (!item->is_floating)
        gdk_window_show (widget->window);

    if (item->is_floating && !item->float_window_mapped)
        gdl_dock_item_window_float (item);

    if (bin->child
        && GTK_WIDGET_VISIBLE (bin->child)
        && !GTK_WIDGET_MAPPED (bin->child))
        gtk_widget_map (bin->child);
}

static void
gdl_dock_item_unmap (GtkWidget *widget)
{
    GdlDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
    
    item = GDL_DOCK_ITEM (widget);

    gdk_window_hide (widget->window);
    if (item->float_window_mapped) {
        gdk_window_hide (item->float_window);
        item->float_window_mapped = FALSE;
    }
}

static void
gdl_dock_item_realize (GtkWidget *widget)
{
    GdkWindowAttr  attributes;
    gint           attributes_mask;
    GdlDockItem   *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    item = GDL_DOCK_ITEM (widget);

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    /* widget window */
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = (gtk_widget_get_events (widget)
                             | GDK_EXPOSURE_MASK);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), 
                                     &attributes, attributes_mask);
    gdk_window_set_user_data (widget->window, widget);

    /* bin_window: the window used for non-floating items */
    attributes.x = 0;
    attributes.y = 0;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask |= (gtk_widget_get_events (widget) |
                              GDK_EXPOSURE_MASK |
                              GDK_BUTTON1_MOTION_MASK |
                              GDK_POINTER_MOTION_HINT_MASK |
                              GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    item->bin_window = gdk_window_new (widget->window, 
                                       &attributes, attributes_mask);
    gdk_window_set_user_data (item->bin_window, widget);
    if (GTK_BIN (item)->child)
        gtk_widget_set_parent_window (GTK_BIN (item)->child, item->bin_window);

    /* floating window: this one is a toplevel */
    attributes.x = 0;
    attributes.y = 0;
    attributes.width = widget->requisition.width;
    attributes.height = widget->requisition.height;
    attributes.window_type = GDK_WINDOW_TOPLEVEL;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = (gtk_widget_get_events (widget) |
                             GDK_KEY_PRESS_MASK |
                             GDK_ENTER_NOTIFY_MASK |
                             GDK_LEAVE_NOTIFY_MASK |
                             GDK_FOCUS_CHANGE_MASK |
                             GDK_STRUCTURE_MASK);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    item->float_window = gdk_window_new (NULL, &attributes, attributes_mask);
    gdk_window_set_transient_for (item->float_window, 
                                  gdk_window_get_toplevel (widget->window));
    gdk_window_set_user_data (item->float_window, widget);
    gdk_window_set_decorations (item->float_window, 0);
  
    widget->style = gtk_style_attach (widget->style, widget->window);
    gtk_style_set_background (widget->style, widget->window, 
                              GTK_WIDGET_STATE (item));
    gtk_style_set_background (widget->style, item->bin_window, 
                              GTK_WIDGET_STATE (item));
    gtk_style_set_background (widget->style, item->float_window, 
                              GTK_WIDGET_STATE (item));
    gdk_window_set_back_pixmap (widget->window, NULL, TRUE);

    if (item->is_floating)
        gdl_dock_item_window_float (item);
}

static void
gdl_dock_item_unrealize (GtkWidget *widget)
{
    GdlDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    item = GDL_DOCK_ITEM (widget);

    if (item->bin_window) {
        gdk_window_set_user_data (item->bin_window, NULL);
        gdk_window_destroy (item->bin_window);
        item->bin_window = NULL;
    };

    if (item->float_window) {
        gdk_window_set_user_data (item->float_window, NULL);
        gdk_window_destroy (item->float_window);
        item->float_window = NULL;
    };

    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
        (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}


static void
gdl_dock_item_style_set (GtkWidget *widget,
                         GtkStyle  *previous_style)
{
    GdlDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    item = GDL_DOCK_ITEM (widget);

    if (GTK_WIDGET_REALIZED (widget) && !GTK_WIDGET_NO_WINDOW (widget)) {
        gtk_style_set_background (widget->style, widget->window,
                                  widget->state);
        gtk_style_set_background (widget->style, item->bin_window, 
                                  widget->state);
        gtk_style_set_background (widget->style, item->float_window, 
                                  widget->state);
        if (GTK_WIDGET_DRAWABLE (widget))
            gdk_window_clear (widget->window);
    }
}

static void
draw_textured_frame (GtkWidget     *widget, 
                     GdkWindow     *window, 
                     GdkRectangle  *rect, 
                     GtkShadowType  shadow,
                     GdkRectangle  *clip)
{
    /* FIXME: make a nicer dragbar */
    /* FIXME: if the item is floating and is resizable draw a handle
       to resize it and implement the event hooks */
    gtk_paint_handle (widget->style, window, GTK_STATE_NORMAL, shadow,
                      clip, widget, "dockitem",
                      rect->x, rect->y, rect->width, rect->height, 
                      GTK_ORIENTATION_VERTICAL);
}

static void
gdl_dock_item_paint (GtkWidget      *widget,
                     GdkEventExpose *event,
                     GdkRectangle   *area)
{
    GtkBin       *bin;
    GdlDockItem  *item;
    guint         width, height;
    guint         border_width;
    GdkRectangle  rect;
    gint          drag_handle_size = DRAG_HANDLE_SIZE;

    if (!GDL_DOCK_ITEM_NOT_LOCKED (widget))
        drag_handle_size = 0;

    bin = GTK_BIN (widget);
    item = GDL_DOCK_ITEM (widget);

    border_width = GTK_CONTAINER (widget)->border_width;

    if (item->is_floating) {
        width = bin->child->allocation.width + 2 * border_width;
        height = bin->child->allocation.height + 2 * border_width;
    } else if (item->orientation == GTK_ORIENTATION_HORIZONTAL) {
        width = widget->allocation.width - drag_handle_size;
        height = widget->allocation.height;
    } else {
        width = widget->allocation.width;
        height = widget->allocation.height - drag_handle_size;
    }

    if (!event)
        gtk_paint_box (widget->style,
                       item->bin_window,
                       GTK_WIDGET_STATE (widget),
                       GTK_SHADOW_NONE,
                       area, widget,
                       "dockitem_bin",
                       0, 0, -1, -1);
    else
        gtk_paint_box (widget->style,
                       item->bin_window,
                       GTK_WIDGET_STATE (widget),
                       GTK_SHADOW_NONE,
                       &event->area, widget,
                       "dockitem_bin",
                       0, 0, -1, -1);

    /* We currently draw the handle _above_ the relief of the dockitem.
       It could also be drawn on the same level...  */
    if (GDL_DOCK_ITEM_NOT_LOCKED (widget)) {
        GdkRectangle dest;
    
        rect.x = 0;
        rect.y = 0;
      
        if (item->orientation == GTK_ORIENTATION_HORIZONTAL) {
            rect.width = DRAG_HANDLE_SIZE;
            rect.height = height;
        } else {
            rect.width = width;
            rect.height = DRAG_HANDLE_SIZE;
        }

        if (gdk_rectangle_intersect (event ? &event->area : area, 
                                     &rect, &dest))
            draw_textured_frame (widget, item->bin_window, &rect,
                                 GTK_SHADOW_OUT,
                                 event ? &event->area : area);
    }
  
    if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
        GdkRectangle   child_area;
        GdkEventExpose child_event;

        if (!event) { /* we were called from draw() */
            if (gtk_widget_intersect (bin->child, area, &child_area))
                gtk_widget_draw (bin->child, &child_area);
        } else { /* we were called from expose() */
            child_event = *event;
          
            if (GTK_WIDGET_NO_WINDOW (bin->child) &&
                gtk_widget_intersect (bin->child, &event->area, 
                                      &child_event.area))
                gtk_widget_event (bin->child, (GdkEvent *) &child_event);
        }
    }
}

static void
gdl_dock_item_draw (GtkWidget    *widget,
                    GdkRectangle *area)
{
    GdlDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    item = GDL_DOCK_ITEM (widget);

    if (GTK_WIDGET_DRAWABLE (widget)) {
        if (item->is_floating) {
            GdkRectangle r;

            /* The area parameter does not make sense in this case, so
               we repaint everything.  */
            r.x = 0;
            r.y = 0;
            r.width = (2 * GTK_CONTAINER (item)->border_width
                       + DRAG_HANDLE_SIZE);
            r.height = r.width + GTK_BIN (item)->child->allocation.height;
            r.width += GTK_BIN (item)->child->allocation.width;

            gdl_dock_item_paint (widget, NULL, &r);

        } else
            gdl_dock_item_paint (widget, NULL, area);
    }
}

static gint
gdl_dock_item_expose (GtkWidget      *widget,
                      GdkEventExpose *event)
{
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (GTK_WIDGET_DRAWABLE (widget) && event->window != widget->window)
        gdl_dock_item_paint (widget, event, NULL);
  
    return FALSE;
}

/* FIXME: should this be virtual? */
static void
gdl_dock_item_set_floating (GdlDockItem *item, 
                            gboolean     val)
{
    item->is_floating = val;

    /* If there is a child and it supports the 'is_floating' flag
     * set that too.
     */
    if (item->bin.child != NULL) {
        GtkArgInfo *info_p;
        gchar      *error;

        error = gtk_object_arg_get_info (GTK_OBJECT_TYPE (item->bin.child), 
                                         "is_floating", &info_p);
        if (error)
            g_free (error);
        else
            gtk_object_set (GTK_OBJECT (item->bin.child), 
                            "is_floating", val, NULL);
    }
}

static void
gdl_dock_item_location (GtkWidget *widget,
                        GdlDockItem *item)
{
    guint position;
    GtkWidget *parent;
    
    /* Reposition item. */
    position = GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (widget), "position"));    
    parent = GTK_WIDGET (item)->parent;    
    while (parent && !GDL_IS_DOCK_ITEM (parent) && !GDL_IS_DOCK (parent))
        parent = parent->parent;
    gdl_dock_item_dock_to (item, GDL_DOCK_ITEM (parent), position);    
}

static gint
gdl_dock_item_button_changed (GtkWidget      *widget,
                              GdkEventButton *event)
{
    GdlDockItem *item;
    gboolean     event_handled;
  
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    
    item = GDL_DOCK_ITEM (widget);
    
    if (event->window != item->bin_window)
        return FALSE;

    /* Verify that the item is not locked. */
    if (!GDL_DOCK_ITEM_NOT_LOCKED (widget))
        return FALSE;

    event_handled = FALSE;

    /* Left mousebutton click on dockitem. */
    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        GtkWidget *child;
        gboolean   in_handle;
        gboolean   in_resize_handle;

        child = GTK_BIN (item)->child;

        /* Check if user clicked on the drag handle. */      
        switch (item->orientation) {
        case GTK_ORIENTATION_HORIZONTAL:
            in_handle = event->x < DRAG_HANDLE_SIZE;
            in_resize_handle = event->x > item->float_width - DRAG_HANDLE_SIZE / 2;
            break;
	case GTK_ORIENTATION_VERTICAL:
            in_handle = event->y < DRAG_HANDLE_SIZE;
            in_resize_handle = event->y > item->float_height - DRAG_HANDLE_SIZE / 2;
            break;
	default:
            in_handle = FALSE;
            in_resize_handle = FALSE;
            break;
	}

        /* If the dockitem doesn't contain a child, do nothing. */
        if (!child) {
            in_handle = FALSE;
            in_resize_handle = FALSE;
            event_handled = TRUE;
	}

        /* Set in_drag flag, grab pointer and call begin drag operation. */      
        if (in_handle) {
            item->dragoff_x = event->x;
            item->dragoff_y = event->y;
            item->in_drag = TRUE;
            
            /* Grab pointer? For what reason? */
            gdl_dock_item_grab_pointer (item);
            
            if (!item->dock) {
                GtkWidget *dock;
                
                /* traverse the widget hierarchy looking for the dock */
                dock = widget->parent;
                while (dock && !GDL_IS_DOCK (dock))
                    dock = dock->parent;
                item->dock = dock;
            };
            
            /* FIXME: should we use signals as GnomeDockItem does? */
            gdl_dock_drag_begin (GDL_DOCK (item->dock), item); 
            event_handled = TRUE;

            if (!item->is_floating) {
                /* if float_width and float_height have not yet been set,
                   use the item requisition as default values */
                if (item->float_width == 0 || item->float_height == 0) {
                    GtkRequisition req;
                    gtk_widget_size_request (widget, &req);
                    item->float_width = req.width;
                    item->float_height = req.height;
                };
                /* make corrections to dragoff_{x,y} according to relative sizes */
                switch (item->orientation) {
                case GTK_ORIENTATION_HORIZONTAL:
                    item->dragoff_y *= (float) item->float_height / 
                        widget->allocation.height;
                    break;
                case GTK_ORIENTATION_VERTICAL:
                    item->dragoff_x *= (float) item->float_width / 
                        widget->allocation.width;
                    break;
                };
            };

        } else if (in_resize_handle && item->is_floating) {
            gdk_window_get_root_origin (item->bin_window, 
                                        &item->dragoff_x, &item->dragoff_y);
            item->dragoff_x += event->x;
            item->dragoff_y += event->y;

            /* Grab pointer? For what reason? */
            gdl_dock_item_grab_pointer (item);
            
            /* resize only when the item is floating */
            item->in_resize = TRUE;
            event_handled = TRUE;
        };

    } else if (event->type == GDK_BUTTON_RELEASE && 
               (item->in_drag || item->in_resize)) {
        /* User dropped widget somewhere. */
        
        /* Release pointer. */
        gdk_pointer_ungrab (GDK_CURRENT_TIME);

        if (item->in_drag)
            /* FIXME: should we use signals as GnomeDockItem does? */
            gdl_dock_drag_end (GDL_DOCK (item->dock), item);

        item->in_drag = FALSE;
        item->in_resize = FALSE;

        event_handled = TRUE;

    } else if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
        /* Right mousebutton click on dockitem. */
        GtkWidget *popup, *menuitem, *menu;

        /* Create popup menu. */
        popup = gtk_menu_new ();
        
        /* Dock/Undock menuitem. */
        if (item->is_floating)
            menuitem = gtk_menu_item_new_with_label (_("Dock"));
        else
            menuitem = gtk_menu_item_new_with_label (_("Undock"));
        gtk_menu_append (GTK_MENU (popup), menuitem);
        /*gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_location), item);*/
        
        /* Hide menuitem. */
        menuitem = gtk_menu_item_new_with_label (_("Hide"));
        gtk_menu_append (GTK_MENU (popup), menuitem);
        /*gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_hide), item);*/

        /* Horizontal line. */
        menuitem = gtk_menu_item_new ();
        gtk_menu_append (GTK_MENU (popup), menuitem);

        /* Location menu. */
        menu = gtk_menu_new ();
        menuitem = gtk_menu_item_new_with_label (_("Location"));
        gtk_menu_append (GTK_MENU (popup), menuitem);        
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
        gtk_widget_set_sensitive (menuitem, !item->is_floating);
        
        /* Top. */
        menuitem = gtk_check_menu_item_new_with_label (_("Top"));
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                        item->placement == GDL_DOCK_TOP);
        gtk_object_set_data (GTK_OBJECT (menuitem), "position", 
                             GUINT_TO_POINTER (GDL_DOCK_TOP));
        gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_location), item);
        
        /* Left. */                    
        menuitem = gtk_check_menu_item_new_with_label (_("Left"));
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                        item->placement == GDL_DOCK_LEFT);
        gtk_object_set_data (GTK_OBJECT (menuitem), "position", 
                             GUINT_TO_POINTER (GDL_DOCK_LEFT));
        gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_location), item);
        
        /* Center. */                    
        menuitem = gtk_check_menu_item_new_with_label (_("Center"));
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                        item->placement == GDL_DOCK_CENTER);
        gtk_object_set_data (GTK_OBJECT (menuitem), "position", 
                             GUINT_TO_POINTER (GDL_DOCK_CENTER));
        gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_location), item);
        
        /* Right. */                    
        menuitem = gtk_check_menu_item_new_with_label (_("Right"));
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                        item->placement == GDL_DOCK_RIGHT);
        gtk_object_set_data (GTK_OBJECT (menuitem), "position", 
                             GUINT_TO_POINTER (GDL_DOCK_RIGHT));
        gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_location), item);
        
        /* Bottom. */                    
        menuitem = gtk_check_menu_item_new_with_label (_("Bottom"));
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                        item->placement == GDL_DOCK_BOTTOM);
        gtk_object_set_data (GTK_OBJECT (menuitem), "position", 
                             GUINT_TO_POINTER (GDL_DOCK_BOTTOM));
        gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_location), item);
        
        /* Separator. */                    
        menuitem = gtk_menu_item_new ();
        gtk_menu_append (GTK_MENU (menu), menuitem);        
        
        /* Floating. */
        menuitem = gtk_check_menu_item_new_with_label (_("Floating"));
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                        item->placement == GDL_DOCK_FLOATING);
        gtk_object_set_data (GTK_OBJECT (menuitem), "position", 
                             GUINT_TO_POINTER (GDL_DOCK_FLOATING));
        gtk_signal_connect (GTK_OBJECT (menuitem), "toggled", 
                            GTK_SIGNAL_FUNC (gdl_dock_item_location), item);
        
        gtk_widget_show_all (popup);

        /* Show popup menu. */
        gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL, 
                        event->button, event->time);

        event_handled = TRUE;    	
    }

    return event_handled;
}

static gint
gdl_dock_item_motion (GtkWidget      *widget,
                      GdkEventMotion *event)
{
    GdlDockItem *item;
    gint         new_x, new_y;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    item = GDL_DOCK_ITEM (widget);
    if (event->window != item->bin_window)
        return FALSE;

    if (!item->in_drag && !item->in_resize)
        return FALSE;

    gdk_window_get_pointer (NULL, &new_x, &new_y, NULL);
  
    if (item->in_drag) 
        /* FIXME: should we use signals as GnomeDockItem does? */
        gdl_dock_drag_motion (GDL_DOCK (item->dock), item, new_x, new_y);
    else {
        item->float_width += new_x - item->dragoff_x;
        item->float_height += new_y - item->dragoff_y;
        item->dragoff_x = new_x;
        item->dragoff_y = new_y;
        gtk_widget_queue_resize (widget);
    };

    return TRUE;
}

static gint
gdl_dock_item_delete_event (GtkWidget   *widget,
                            GdkEventAny *event)
{
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    return TRUE;
}

static void
gdl_dock_item_grab_pointer (GdlDockItem *item)
{
    GdkCursor *fleur;

    fleur = gdk_cursor_new (GDK_FLEUR);

    /* Hm, not sure this is the right thing to do, but it seems to work.  */
    while (gdk_pointer_grab (item->bin_window,
                             FALSE,
                             (GDK_BUTTON1_MOTION_MASK |
                              GDK_POINTER_MOTION_HINT_MASK |
                              GDK_BUTTON_RELEASE_MASK),
                             NULL,
                             fleur,
                             GDK_CURRENT_TIME) != 0);

    gdk_cursor_destroy (fleur);
}


/* ----------------------------------------------------------------------
 * Public interface 
 * ---------------------------------------------------------------------- */

GtkWidget *
gdl_dock_item_new (const gchar         *name,
                   GdlDockItemBehavior  behavior)
{
    GdlDockItem *item;

    item = GDL_DOCK_ITEM (gtk_type_new (gdl_dock_item_get_type ()));

    item->name = g_strdup (name);
    item->behavior = behavior;

    return GTK_WIDGET (item);
}

guint
gdl_dock_item_get_type (void)
{
    static GtkType dock_item_type = 0;

    if (dock_item_type == 0) {
        GtkTypeInfo dock_item_info = {
            "GdlDockItem",
            sizeof (GdlDockItem),
            sizeof (GdlDockItemClass),
            (GtkClassInitFunc) gdl_dock_item_class_init,
            (GtkObjectInitFunc) gdl_dock_item_init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL,
        };

        dock_item_type = gtk_type_unique (gtk_bin_get_type (), 
                                          &dock_item_info);
    }

    return dock_item_type;
}

static void
notebook_merge (GtkWidget *widget,
                gpointer   data)
{
    GdlDockNotebook *nb;

    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));
    g_return_if_fail (GDL_IS_DOCK_NOTEBOOK (data));

    nb = (GdlDockNotebook *) data;

    gtk_widget_ref (widget);
    gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
    gtk_container_add (GTK_CONTAINER (nb), widget);
    gtk_widget_unref (widget);
}


/* Dock item with target_item at the specified position, or make it float. */
void
gdl_dock_item_dock_to (GdlDockItem      *item,
                       GdlDockItem      *target_item,
                       GdlDockPlacement  position)
{
    GtkWidget *widget, *old_parent, *real_old_parent;
    gboolean   auto_reduce_parent = TRUE;

    g_return_if_fail (item != NULL);
    g_return_if_fail (item != target_item);
    g_return_if_fail (target_item != NULL || position == GDL_DOCK_FLOATING);

    GDL_TRACE ();

    widget = GTK_WIDGET (item);
    
    /* Increase the refcount so the widget won't be automatically destroyed 
       when we remove it from its parent container. */
    gtk_widget_ref (widget);

    /* Find current DockItem/Dock parent */
    real_old_parent = old_parent = widget->parent;
    while (old_parent && 
           !GDL_IS_DOCK_ITEM (old_parent) && 
           !GDL_IS_DOCK (old_parent))
        old_parent = GTK_WIDGET (old_parent)->parent;
    if (!old_parent) {
        /* Fallback case: the item was contained in a non-DockItem widget...
           very strange */
        old_parent = widget->parent;
    };

    /* Item wants to float. */
    if (position == GDL_DOCK_FLOATING || !target_item) {
        /* Remove widget from current container. */
        if (real_old_parent)           
            gtk_container_remove (GTK_CONTAINER (real_old_parent), widget);

        /* Create new floating item for widget. */
        if (!item->is_floating) {
            item->dragoff_x = item->dragoff_y = 0;
            gdl_dock_add_floating_item (GDL_DOCK (item->dock), item, 
                                        item->float_x, item->float_y, 
                                        item->orientation);
        };

    } else {
        GtkWidget *target, *parent, *real_parent;

        target = GTK_WIDGET (target_item);

        real_parent = parent = target->parent;
        while (parent && !GDL_IS_DOCK_ITEM (parent) && !GDL_IS_DOCK (parent))
            parent = GTK_WIDGET (parent)->parent;
        if (!parent)
            parent = target->parent;

        /* Propagate dock object. */
        item->dock = target_item->dock;

        /* Unfloat item. */
        if (item->is_floating)
            gdl_dock_item_window_sink (item);

        /* Dock widget on top of another. */
        if (position == GDL_DOCK_CENTER) {
            
            GtkWidget *nb;

            /* If the target is already a notebook, we use it to add the 
               current item.  Otherwise, we create a GdlDockNotebook and
               add it in place of the target. */
            if (!GDL_IS_DOCK_NOTEBOOK (target)) {
                gtk_widget_ref (target);
                gtk_container_remove (GTK_CONTAINER (real_parent), target);
                nb = gdl_dock_notebook_new ();
                GDL_DOCK_ITEM (nb)->dock = target_item->dock;
                gtk_container_add (GTK_CONTAINER (parent), nb);
                gtk_widget_show (nb);
                gtk_container_add (GTK_CONTAINER (nb), target);
                gtk_widget_unref (target);
            } else
                nb = target;

            /* If the item had a parent, remove it from the container */
            if (real_old_parent)
                gtk_container_remove (GTK_CONTAINER (real_old_parent), widget);

            if (GDL_IS_DOCK_NOTEBOOK (widget)) {
                /* FIXME: find some other way to handle this */
                gtk_container_foreach (GTK_CONTAINER (
                    GDL_DOCK_NOTEBOOK (widget)->notebook), 
                                       notebook_merge,
                                       (gpointer) nb);
                /* The current GdlDockNotebook will be destroyed later
                   when unrefing it */
            } else {
                /* simply append the widget to the notebook */
                gtk_container_add (GTK_CONTAINER (nb), widget);
            };

        } else if (parent == old_parent && GDL_IS_DOCK_PANED (parent)) {
            /* FIXME: maybe this should be a virtual of GdlDockItem. */
            gdl_dock_paned_reorder (GDL_DOCK_PANED (parent), widget, position);
            /* No need to update parent container layout, since the item
               hasn't been removed from it, only reordered. */
            auto_reduce_parent = FALSE;

        } else { /* Put widget in a GdlDockPaned. */
            GdlDockPaned *paned;

            /* Remove widget from current container. */
            if (real_old_parent)
                gtk_container_remove (GTK_CONTAINER (real_old_parent), widget);

            /* Increase refcount so widget won't be destroyed when removed 
               from parent container. */
            gtk_widget_ref (target);
            
            /* Remove widget from parent container. */
            gtk_container_remove (GTK_CONTAINER (real_parent), target);

            /* Unfloat item. */
            if (item->is_floating)
                gdl_dock_item_window_sink (item);

            /* Create horizontal paned. */                
            if (position == GDL_DOCK_LEFT || position == GDL_DOCK_RIGHT)
                paned = GDL_DOCK_PANED (gdl_dock_paned_new 
                                        (GTK_ORIENTATION_HORIZONTAL));
            else /* Vertical paned. */
                paned = GDL_DOCK_PANED (gdl_dock_paned_new 
                                        (GTK_ORIENTATION_VERTICAL));

            /* Add widgets in correct order. */
            if (position == GDL_DOCK_LEFT || position == GDL_DOCK_TOP) {
                gdl_dock_paned_add1 (paned, widget);
                gdl_dock_paned_add2 (paned, target);
            } else {
                gdl_dock_paned_add1 (paned, target);
                gdl_dock_paned_add2 (paned, widget);
            }

            /* Show & add new GdlDockPaned to parent container. */
            gtk_widget_show (GTK_WIDGET (paned));
            gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (paned));
            gtk_widget_unref (target);
        }
    }
    
    /* Set item placement. */
    item->placement = position;

    /* Decrease refcount (was increased to prevent destruction). */
    gtk_widget_unref (widget);
    
    /* Update layout of parent container from which the item came. */
    if (old_parent && auto_reduce_parent && GDL_IS_DOCK_ITEM (old_parent))
        gdl_dock_item_auto_reduce (GDL_DOCK_ITEM (old_parent));
}

void
gdl_dock_item_set_orientation (GdlDockItem    *item,
                               GtkOrientation  orientation)
{
    GdlDockItemClass *klass = GDL_DOCK_ITEM_CLASS (GTK_OBJECT (item)->klass);
    GtkArgInfo       *info_p;
    gchar            *error;

    g_return_if_fail (item != NULL);

    if (item->orientation != orientation) {
        item->orientation = orientation;

        if (item->bin.child != NULL) {
            error = gtk_object_arg_get_info (GTK_OBJECT_TYPE (item->bin.child),
                                             "orientation", &info_p);
            if (error)
                g_free (error);
            else {
                gtk_object_set (GTK_OBJECT (item->bin.child),
                                "orientation", orientation,
                                NULL);
            };
        }
        if (GTK_WIDGET_DRAWABLE (item))
            gtk_widget_queue_clear (GTK_WIDGET (item));
        gtk_widget_queue_resize (GTK_WIDGET (item));

    }
    if (klass->set_orientation)
        (* klass->set_orientation) (item, orientation);
}

void
gdl_dock_item_auto_reduce (GdlDockItem *item)
{
    GdlDockItemClass *klass = GDL_DOCK_ITEM_CLASS (GTK_OBJECT (item)->klass);

    GDL_TRACE ();

    /* Call auto_reduce handler on GdlDockItem. */
    if (klass->auto_reduce)
        (* klass->auto_reduce) (item);
}

gboolean
gdl_dock_item_drag_request (GdlDockItem        *item, 
                            gint                x,
                            gint                y, 
                            GdlDockRequestInfo *target)
{
    GdlDockItemClass *klass = GDL_DOCK_ITEM_CLASS (GTK_OBJECT (item)->klass);

    /* Delegate request if possible. */
    if (klass->drag_request)
        return (* klass->drag_request) (item, x, y, target);

    else {
        GtkAllocation *alloc;
        gint           dx, dy;

	/* Get item's allocation. */
        alloc = &(GTK_WIDGET (item)->allocation);

	/* Difference between drag location and item location. */
        dx = x - alloc->x;
        dy = y - alloc->y;

	/* Location is inside the GdlDockItem. */
        if (dx > 0 && dx < alloc->width && dy > 0 && dy < alloc->height) {
            float rx, ry;

	    /* Calculate location in terms of the available space (0..1). */
            rx = (float) dx / alloc->width;
            ry = (float) dy / alloc->height;

	    /* Determine dock location. */
            if (rx < SPLIT_RATIO)
                target->position = GDL_DOCK_LEFT;
            else if (rx > (1 - SPLIT_RATIO)) {
                target->position = GDL_DOCK_RIGHT;
                rx = 1 - rx;
            } else if (ry < SPLIT_RATIO && ry < rx)
                target->position = GDL_DOCK_TOP;
            else if (ry > (1 - SPLIT_RATIO) && (1 - ry) < rx)
                target->position = GDL_DOCK_BOTTOM;
            else
            	target->position = GDL_DOCK_CENTER;

	    /* Reset rectangle coordinates to entire item. */
            target->rect.x = 0;
            target->rect.y = 0;
            target->rect.width = alloc->width;
            target->rect.height = alloc->height;

	    /* Calculate docking indicator rectangle size for new locations. Only
	       do this when we're not over the item's current location. */
            if (target->requestor != item) {
                switch (target->position) {
                case GDL_DOCK_TOP:
                    target->rect.height *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_BOTTOM:
                    target->rect.y += target->rect.height * (1 - SPLIT_RATIO);
                    target->rect.height *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_LEFT:
                    target->rect.width *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_RIGHT:
                    target->rect.x += target->rect.width * (1 - SPLIT_RATIO);
                    target->rect.width *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_CENTER:
                    target->rect.x = target->rect.width * SPLIT_RATIO/2;
                    target->rect.y = target->rect.height * SPLIT_RATIO/2;
                    target->rect.width = (target->rect.width * (1 - 
                    			 SPLIT_RATIO/2)) - target->rect.x;
                    target->rect.height = (target->rect.height * (1 - 
                    			  SPLIT_RATIO/2)) - target->rect.y;
                    break;
                default:
                    break;
                }
            }

	    /* Set possible target location and return TRUE. */            
            target->target = GTK_WIDGET (item);            
            return TRUE;                
        } else /* No docking possible at this location. */            
            return FALSE;
    }
}

void
gdl_dock_item_drag_floating (GdlDockItem *item, 
                             gint         x, 
                             gint         y)
{
    x -= item->dragoff_x;
    y -= item->dragoff_y;

    if (item->is_floating) {
        gdk_window_move (item->float_window, x, y);
        gdk_window_raise (item->float_window);
    }
    
    item->float_x = x;
    item->float_y = y;
}

void
gdl_dock_item_window_sink (GdlDockItem *item)
{
    if (GTK_WIDGET_REALIZED (GTK_WIDGET (item))) {
        gdk_window_move_resize (GTK_WIDGET (item)->window, -1, -1, 0, 0);
        gdk_window_hide (item->float_window);
        gdk_window_reparent (item->bin_window, 
                             GTK_WIDGET (item)->window, 0, 0);
        gdk_window_show (GTK_WIDGET (item)->window);
    };

    item->float_window_mapped = FALSE;
    gdl_dock_item_set_floating (item, FALSE);
}

void
gdl_dock_item_window_float (GdlDockItem *item)
{
    GtkRequisition requisition;
    GtkAllocation  allocation;

    item->is_floating = TRUE;
    item->dock = GTK_WIDGET (item)->parent;

    if (!GTK_WIDGET_REALIZED (item))
        return;

    gtk_widget_size_request (GTK_WIDGET (item), &requisition);
    item->float_width = MAX (item->float_width, requisition.width);
    item->float_height = MAX (item->float_height, requisition.height);

    gdk_window_move_resize (item->float_window,
                            item->float_x, item->float_y,
                            item->float_width, item->float_height);

    gdk_window_reparent (item->bin_window, item->float_window, 0, 0);
    gdk_window_set_hints (item->float_window, item->float_x, item->float_y, 
                          0, 0, 0, 0, GDK_HINT_POS);

    allocation.x = allocation.y = 0;
    allocation.width = item->float_width;
    allocation.height = item->float_height;
    gtk_widget_size_allocate (GTK_WIDGET (item), &allocation);

    gdk_window_hide (GTK_WIDGET (item)->window);

    gdk_window_set_transient_for
        (item->float_window, gdk_window_get_toplevel
         (GTK_WIDGET (item)->window));

    gdk_window_show (item->float_window);
    item->float_window_mapped = TRUE;

    gdl_dock_item_draw (GTK_WIDGET (item), NULL);

    /* FIXME: this is dumb... this could be combined with the resize above, but
       the item resizing is weird if we don't make this call separately and last */
    gdk_window_move (item->float_window,
                     item->float_x, item->float_y);
}
