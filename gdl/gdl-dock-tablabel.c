/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "gdl-dock-tablabel.h"
#include "gdl-tools.h"


/* Private prototypes */

static void  gdl_dock_tablabel_class_init    (GdlDockTablabelClass *klass);
static void  gdl_dock_tablabel_init          (GdlDockTablabel      *tablabel);

static void  gdl_dock_tablabel_set_arg       (GtkObject            *object,
                                              GtkArg               *arg,
                                              guint                 arg_id);
static void  gdl_dock_tablabel_get_arg       (GtkObject            *object,
                                              GtkArg               *arg,
                                              guint                 arg_id);

static void  gdl_dock_tablabel_size_request  (GtkWidget          *widget,
                                              GtkRequisition     *requisition);
static void  gdl_dock_tablabel_size_allocate (GtkWidget          *widget,
                                              GtkAllocation      *allocation);
                                              
static void  gdl_dock_tablabel_paint         (GtkWidget      *widget,
                                              GdkEventExpose *event,
                                              GdkRectangle   *area);
static void  gdl_dock_tablabel_draw          (GtkWidget      *widget,
                                              GdkRectangle   *area);
static gint  gdl_dock_tablabel_expose        (GtkWidget      *widget,
                                              GdkEventExpose *event);

static gint  gdl_dock_tablabel_button_pressed (GtkWidget      *widget,
                                               GdkEventButton *event);


/* Module globals */

static GtkEventBoxClass *parent_class = NULL;

#define DEFAULT_DRAG_HANDLE_SIZE 10
#define HANDLE_RATIO 0.8

enum {
    BUTTON_PRESSED_HANDLE,
    LAST_SIGNAL
};

enum {
    ARG_0,
    ARG_HANDLE_SIZE,
    ARG_ORIENTATION,
    ARG_LABEL
};


static guint dock_tablabel_signals [LAST_SIGNAL] = { 0 };


/* Private interface */

static void
gdl_dock_tablabel_class_init (GdlDockTablabelClass *klass)
{
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    gtk_object_add_arg_type ("GdlDockTablabel::handle_size",
                             GTK_TYPE_INT, GTK_ARG_READWRITE,
                             ARG_HANDLE_SIZE);

    gtk_object_add_arg_type ("GdlDockTablabel::orientation",
                             GTK_TYPE_ORIENTATION, GTK_ARG_READWRITE,
                             ARG_ORIENTATION);

    gtk_object_add_arg_type ("GdlDockTablabel::label",
                             GTK_TYPE_STRING, GTK_ARG_READWRITE,
                             ARG_LABEL);

    object_class->set_arg = gdl_dock_tablabel_set_arg;
    object_class->get_arg = gdl_dock_tablabel_get_arg;

    widget_class->size_request = gdl_dock_tablabel_size_request;
    widget_class->size_allocate = gdl_dock_tablabel_size_allocate;
    widget_class->draw = gdl_dock_tablabel_draw;
    widget_class->expose_event = gdl_dock_tablabel_expose;
    widget_class->button_press_event = gdl_dock_tablabel_button_pressed;

    dock_tablabel_signals [BUTTON_PRESSED_HANDLE] =
        gtk_signal_new ("button_pressed_handle",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (GdlDockTablabelClass, 
                                           button_pressed_handle),
                        gtk_marshal_NONE__INT_INT,
                        GTK_TYPE_NONE, 2, GTK_TYPE_INT, GTK_TYPE_INT);

    gtk_object_class_add_signals (object_class, 
                                  dock_tablabel_signals, LAST_SIGNAL);

    klass->button_pressed_handle = NULL;

    parent_class = gtk_type_class (gtk_event_box_get_type ());
}

static void
gdl_dock_tablabel_init (GdlDockTablabel *tablabel)
{
    GtkWidget *widget;

    widget = GTK_WIDGET (tablabel);
    GTK_WIDGET_UNSET_FLAGS (widget, GTK_NO_WINDOW);

    /* FIXME: is it possible to draw a label vertically? if it is so,
       then orientation has its reason to exist.
       I'm almost sure Gtk2 allows that */
    tablabel->orientation = GTK_ORIENTATION_HORIZONTAL;
    tablabel->drag_handle_size = DEFAULT_DRAG_HANDLE_SIZE;

    /* by default, tablabels are active, and not normal */
    tablabel->active = FALSE;
    gtk_widget_set_state (widget, GTK_STATE_ACTIVE);
}

static void
gdl_dock_tablabel_set_arg (GtkObject *object,
                           GtkArg    *arg,
                           guint      arg_id)
{
    GdlDockTablabel *tablabel;
    GtkBin          *bin;

    tablabel = GDL_DOCK_TABLABEL (object);

    switch (arg_id) {
    case ARG_HANDLE_SIZE:
        tablabel->drag_handle_size = GTK_VALUE_INT (*arg);
        gtk_widget_queue_resize (GTK_WIDGET (tablabel));
        break;
    case ARG_ORIENTATION:
        tablabel->orientation = GTK_VALUE_ENUM (*arg);
        gtk_widget_queue_resize (GTK_WIDGET (tablabel));
        break;
    case ARG_LABEL:
        bin = GTK_BIN (tablabel);
        if (bin->child && GTK_IS_LABEL (bin->child))
            gtk_object_set (GTK_OBJECT (bin->child), 
                            "label", GTK_VALUE_STRING (*arg));
        break;
    default:
        break;
    };
}

static void
gdl_dock_tablabel_get_arg (GtkObject *object,
                           GtkArg    *arg,
                           guint      arg_id)
{
    GdlDockTablabel *tablabel;
    GtkBin          *bin;

    tablabel = GDL_DOCK_TABLABEL (object);

    switch (arg_id) {
    case ARG_HANDLE_SIZE:
        GTK_VALUE_INT (*arg) = tablabel->drag_handle_size;
        break;
    case ARG_ORIENTATION:
        GTK_VALUE_ENUM (*arg) = tablabel->orientation;
        break;
    case ARG_LABEL:
        bin = GTK_BIN (tablabel);
        if (bin->child && GTK_IS_LABEL (bin->child)) {
            gchar *label;
            gtk_object_get (GTK_OBJECT (bin->child), 
                            "label", &label,
                            NULL);
            GTK_VALUE_STRING (*arg) = label;
        } else
            GTK_VALUE_STRING (*arg) = NULL;
        break;
    default:
        arg->type = GTK_TYPE_INVALID;
        break;
    }
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

    if (tablabel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        requisition->width = tablabel->drag_handle_size;
        requisition->height = 0;
    } else {
        requisition->width = 0;
        requisition->height = tablabel->drag_handle_size;
    };

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
        gdk_window_move_resize (widget->window, 
                                allocation->x, 
                                allocation->y,
                                allocation->width, 
                                allocation->height);

    if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
        GtkAllocation  child_allocation;

        child_allocation.x = border_width;
        child_allocation.y = border_width;

        if (tablabel->orientation == GTK_ORIENTATION_HORIZONTAL) {
            allocation->width = MAX (1, (int) allocation->width - 
                                     (int) tablabel->drag_handle_size);
            child_allocation.x += tablabel->drag_handle_size;
        } else {
            allocation->height = MAX (1, (int) allocation->height - 
                                      (int) tablabel->drag_handle_size);
        };

        child_allocation.width = 
            MAX (1, (int) allocation->width - 2 * border_width);
        child_allocation.height = 
            MAX (1, (int) allocation->height - 2 * border_width);

        gtk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
gdl_dock_tablabel_paint (GtkWidget      *widget,
                         GdkEventExpose *event,
                         GdkRectangle   *area)
{
    GdkRectangle     dest, rect;
    GtkBin          *bin;
    GdlDockTablabel *tablabel;
    gint             border_width;

    bin = GTK_BIN (widget);
    tablabel = GDL_DOCK_TABLABEL (widget);
    border_width = GTK_CONTAINER (widget)->border_width;

    if (!event)
        gtk_paint_flat_box (widget->style,
                            widget->window,
                            GTK_WIDGET_STATE (widget),
                            GTK_SHADOW_NONE,
                            area, widget,
                            "dockitem_bin",
                            0, 0, -1, -1);
    else
        gtk_paint_flat_box (widget->style,
                            widget->window,
                            GTK_WIDGET_STATE (widget),
                            GTK_SHADOW_NONE,
                            &event->area, widget,
                            "dockitem_bin",
                            0, 0, -1, -1);

    rect.x = border_width;
    rect.y = border_width;
    if (tablabel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        rect.width = tablabel->drag_handle_size * HANDLE_RATIO;
        rect.height = widget->allocation.height - 2*border_width;
    } else {
        if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
            rect.y += bin->child->allocation.height
                + tablabel->drag_handle_size * (1-HANDLE_RATIO);
        rect.width = widget->allocation.width - 2*border_width;
        rect.height = tablabel->drag_handle_size * HANDLE_RATIO;
    };

    if (gdk_rectangle_intersect (event ? &event->area : area, &rect, &dest)) {
        gtk_paint_handle (widget->style, widget->window, 
                          GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                          event ? &event->area : area, widget, "dock_tablabel",
                          rect.x, rect.y, rect.width, rect.height, 
                          GTK_ORIENTATION_VERTICAL);
    };
    
    if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
        GdkRectangle   child_area;
        GdkEventExpose child_event;

        if (!event) { /* we were called from draw() */
            if (gtk_widget_intersect (bin->child, area, &child_area))
                gtk_widget_draw (bin->child, &child_area);

        } else { /* we were called from expose() */
            child_event = *event;
          
            if (GTK_WIDGET_NO_WINDOW (bin->child) 
                && gtk_widget_intersect (bin->child, &event->area, 
                                         &child_event.area))
                gtk_widget_event (bin->child, (GdkEvent *) &child_event);
        }
    }
}

static void
gdl_dock_tablabel_draw (GtkWidget    *widget,
                        GdkRectangle *area)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_TABLABEL (widget));

    if (GTK_WIDGET_DRAWABLE (widget))
        gdl_dock_tablabel_paint (widget, NULL, area);
}

static gint
gdl_dock_tablabel_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_TABLABEL (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (GTK_WIDGET_DRAWABLE (widget))
        gdl_dock_tablabel_paint (widget, event, NULL);
  
    return FALSE;
}

static gint
gdl_dock_tablabel_button_pressed (GtkWidget      *widget,
                                  GdkEventButton *event)
{
    GdlDockTablabel *tablabel;
    gboolean         event_handled;
  
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_TABLABEL (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    
    tablabel = GDL_DOCK_TABLABEL (widget);
    
    if (event->window != widget->window)
        return FALSE;

    event_handled = FALSE;

    {
        gboolean in_handle;
        gint     rel_x, rel_y;
        guint    border_width;
        GtkBin  *bin;

        bin = GTK_BIN (widget);
        border_width = GTK_CONTAINER (widget)->border_width;

        rel_x = event->x - border_width;
        rel_y = event->y - border_width;

        /* Check if user clicked on the drag handle. */      
        switch (tablabel->orientation) {
        case GTK_ORIENTATION_HORIZONTAL:
            in_handle = (rel_x < tablabel->drag_handle_size * HANDLE_RATIO) 
                && (rel_x > 0);
            break;
	case GTK_ORIENTATION_VERTICAL:
            in_handle = (rel_y > (bin->child->allocation.height
                                  + (1-HANDLE_RATIO) 
                                  * tablabel->drag_handle_size))
                && (rel_y < widget->allocation.height - 2*border_width);
            break;
	default:
            in_handle = FALSE;
            break;
	}

        if (in_handle && tablabel->active) {
            gtk_signal_emit (GTK_OBJECT (widget), 
                             dock_tablabel_signals [BUTTON_PRESSED_HANDLE],
                             event->button, event->time);
            event_handled = TRUE;
        };
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



/* Public interface */

GtkType
gdl_dock_tablabel_get_type (void)
{
    static GtkType dock_tablabel_type = 0;

    if (dock_tablabel_type == 0) {
        GtkTypeInfo dock_tablabel_info = {
            "GdlDockTablabel",
            sizeof (GdlDockTablabel),
            sizeof (GdlDockTablabelClass),
            (GtkClassInitFunc) gdl_dock_tablabel_class_init,
            (GtkObjectInitFunc) gdl_dock_tablabel_init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL,
        };

        dock_tablabel_type = gtk_type_unique (gtk_event_box_get_type (), 
                                              &dock_tablabel_info);
    }

    return dock_tablabel_type;
}

GtkWidget *
gdl_dock_tablabel_new (const gchar *label)
{
    GdlDockTablabel *tablabel;

    tablabel = GDL_DOCK_TABLABEL (
        gtk_type_new (gdl_dock_tablabel_get_type ()));

    if (label) {
        GtkWidget *label_widget;

        label_widget = gtk_label_new (label);
        gtk_container_add (GTK_CONTAINER (tablabel), label_widget);
        gtk_widget_show (label_widget);
    };

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
