/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "gdl-dock-tablabel.h"
#include "gdl-tools.h"

#include "libgdlmarshal.h"


/* Private prototypes */

static void  gdl_dock_tablabel_class_init    (GdlDockTablabelClass *klass);
static void  gdl_dock_tablabel_init          (GdlDockTablabel      *tablabel);

static void  gdl_dock_tablabel_set_property  (GObject              *object,
                                              guint                 prop_id,
                                              const GValue         *value,
                                              GParamSpec           *pspec);
static void  gdl_dock_tablabel_get_property  (GObject              *object,
                                              guint                 prop_id,
                                              GValue               *value,
                                              GParamSpec           *pspec);

static void  gdl_dock_tablabel_size_request  (GtkWidget          *widget,
                                              GtkRequisition     *requisition);
static void  gdl_dock_tablabel_size_allocate (GtkWidget          *widget,
                                              GtkAllocation      *allocation);
                                              
static void  gdl_dock_tablabel_paint         (GtkWidget      *widget,
                                              GdkEventExpose *event);
static gint  gdl_dock_tablabel_expose        (GtkWidget      *widget,
                                              GdkEventExpose *event);

static gint  gdl_dock_tablabel_button_pressed (GtkWidget      *widget,
                                               GdkEventButton *event);


/* Module globals */

static GtkEventBoxClass *parent_class = NULL;

#define DEFAULT_DRAG_HANDLE_SIZE 10
#define HANDLE_RATIO 1.0

enum {
    BUTTON_PRESSED_HANDLE,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_HANDLE_SIZE,
    PROP_ORIENTATION,
    PROP_LABEL
};


static guint dock_tablabel_signals [LAST_SIGNAL] = { 0 };


/* Private interface */

static void
gdl_dock_tablabel_class_init (GdlDockTablabelClass *klass)
{
    GObjectClass      *g_object_class;
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    g_object_class = (GObjectClass *) klass;
    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    parent_class = g_type_class_peek_parent (klass);
    
    g_object_class->set_property = gdl_dock_tablabel_set_property;
    g_object_class->get_property = gdl_dock_tablabel_get_property;

    widget_class->size_request = gdl_dock_tablabel_size_request;
    widget_class->size_allocate = gdl_dock_tablabel_size_allocate;
    widget_class->expose_event = gdl_dock_tablabel_expose;
    widget_class->button_press_event = gdl_dock_tablabel_button_pressed;

    g_object_class_install_property (
        g_object_class, PROP_HANDLE_SIZE,
        g_param_spec_uint ("handle_size", _("Handle size"),
                           _("Size in pixels of the label handle to drag "
                             "the dock item"),
                           0, 100, DEFAULT_DRAG_HANDLE_SIZE,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        g_object_class, PROP_ORIENTATION,
        g_param_spec_enum ("orientation", _("Orientation"),
                           _("Orientation of the tab label"),
                           GTK_TYPE_ORIENTATION, GTK_ORIENTATION_HORIZONTAL,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        g_object_class, PROP_LABEL,
        g_param_spec_string ("label", _("Label"),
                             _("Text for the dock item tab label"),
                             NULL, G_PARAM_READWRITE));

    dock_tablabel_signals [BUTTON_PRESSED_HANDLE] =
        g_signal_new ("button_pressed_handle",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GdlDockTablabelClass, 
                                       button_pressed_handle),
                      NULL, NULL,
                      gdl_marshal_VOID__UINT_UINT,
                      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

    klass->button_pressed_handle = NULL;
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
gdl_dock_tablabel_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    GdlDockTablabel *tablabel;
    GtkBin          *bin;

    tablabel = GDL_DOCK_TABLABEL (object);

    switch (prop_id) {
    case PROP_HANDLE_SIZE:
        tablabel->drag_handle_size = g_value_get_uint (value);
        gtk_widget_queue_resize (GTK_WIDGET (tablabel));
        break;
    case PROP_ORIENTATION:
        tablabel->orientation = g_value_get_enum (value);
        gtk_widget_queue_resize (GTK_WIDGET (tablabel));
        break;
    case PROP_LABEL:
        bin = GTK_BIN (tablabel);
        if (bin->child && GTK_IS_LABEL (bin->child))
            /* FIXME: use parameter introspection to see if the child
               support the label parameter */
            g_object_set (bin->child, "label", g_value_get_string (value));
        break;
    default:
        break;
    };
}

static void
gdl_dock_tablabel_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    GdlDockTablabel *tablabel;
    GtkBin          *bin;

    tablabel = GDL_DOCK_TABLABEL (object);

    switch (prop_id) {
    case PROP_HANDLE_SIZE:
        g_value_set_uint (value, tablabel->drag_handle_size);
        break;
    case PROP_ORIENTATION:
        g_value_set_enum (value, tablabel->orientation);
        break;
    case PROP_LABEL:
        bin = GTK_BIN (tablabel);
        if (bin->child && GTK_IS_LABEL (bin->child)) {
            /* FIXME: use parameter introspection to see if the child
               support the label argument */
            g_value_set_string (value, gtk_label_get_text (GTK_LABEL (bin->child)));
        } else
            g_value_set_string (value, NULL);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
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
                         GdkEventExpose *event)
{
    GdkRectangle     dest, rect;
    GtkBin          *bin;
    GdlDockTablabel *tablabel;
    gint             border_width;

    bin = GTK_BIN (widget);
    tablabel = GDL_DOCK_TABLABEL (widget);
    border_width = GTK_CONTAINER (widget)->border_width;

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

    if (gdk_rectangle_intersect (&event->area, &rect, &dest)) {
        gtk_paint_handle (widget->style, widget->window, 
                          GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
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

    if (GTK_WIDGET_DRAWABLE (widget)) {
        (* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);
        gdl_dock_tablabel_paint (widget, event);
    };
  
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
            g_signal_emit (widget, 
                           dock_tablabel_signals [BUTTON_PRESSED_HANDLE],
                           0, event->button, (guint) event->time);
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

GType
gdl_dock_tablabel_get_type (void)
{
    static GType dock_tablabel_type = 0;

    if (dock_tablabel_type == 0) {
        GTypeInfo dock_tablabel_info = {
            sizeof (GdlDockTablabelClass),

            NULL,               /* base_init */
            NULL,               /* base_finalize */

            (GClassInitFunc) gdl_dock_tablabel_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */

            sizeof (GdlDockTablabel),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) gdl_dock_tablabel_init,
            NULL                /* value_table */
        };

        dock_tablabel_type = g_type_register_static (
            GTK_TYPE_EVENT_BOX, "GdlDockTablabel", &dock_tablabel_info, 0);
    }

    return dock_tablabel_type;
}

GtkWidget *
gdl_dock_tablabel_new (const gchar *label)
{
    GdlDockTablabel *tablabel;

    tablabel = GDL_DOCK_TABLABEL (g_object_new (GDL_TYPE_DOCK_TABLABEL, NULL));

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
