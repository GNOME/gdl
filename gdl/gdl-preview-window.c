/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * gdl
 * Copyright (C) Johannes Schmid 2010 <jhs@gnome.org>
 *
 * gdl is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * gdl is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gdl-preview-window.h"

/**
 * SECTION:gdl-preview-window
 * @title: GdlPreviewWindow
 * @short_description: show destination docking site.
 * @stability: Private
 *
 *The #GdlPreviewWindow is used to indicate the position where the widget
 * will be dropped. Depending on the capability of the screen it can use
 * a transparent window or just a border.
 */


G_DEFINE_TYPE (GdlPreviewWindow, gdl_preview_window, GTK_TYPE_WINDOW);

static void
gdl_preview_window_init (GdlPreviewWindow *window)
{
    GdkScreen *screen;
    GdkVisual *visual;

    screen = gdk_screen_get_default ();
    visual = gdk_screen_get_rgba_visual (screen);

    if (gdk_screen_is_composited (screen) &&
        visual)
    {
        gtk_widget_set_visual (GTK_WIDGET(window), visual);
        gtk_widget_set_app_paintable (GTK_WIDGET(window), TRUE);
    }
}

static gboolean
gdl_preview_window_draw (GtkWidget *window,
                         cairo_t *cr)
{
    GtkAllocation allocation;
    GtkStyleContext *context;
    GdkRGBA selected;

    context = gtk_widget_get_style_context (window);
    gtk_style_context_get_background_color (context, GTK_STATE_FLAG_SELECTED,
                                            &selected);

    if (gtk_widget_get_app_paintable (window))
    {
        cairo_set_line_width (cr, 1.0);

        gtk_widget_get_allocation (window, &allocation);

        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba (cr, 0, 0, 0, 0);
        cairo_paint (cr);

        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        selected.alpha = 0.25;
        gdk_cairo_set_source_rgba (cr, &selected);
        cairo_paint (cr);

        cairo_rectangle (cr,
                         allocation.x + 0.5, allocation.y + 0.5,
                         allocation.width - 1, allocation.height - 1);
        cairo_stroke (cr);
    }
    else
    {
        gdk_cairo_set_source_rgba (cr, &selected);
        cairo_paint (cr);
    }

    return FALSE;
}

static void
gdl_preview_window_class_init (GdlPreviewWindowClass *klass)
{
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);

    widget_class->draw = gdl_preview_window_draw;
}


/**
 * gdl_preview_window_new:
 *
 * Creates a new #GdlPreviewWindow
 *
 * Returns: New #GdlPreviewWindow item.
 */
GtkWidget*
gdl_preview_window_new (void)
{
    return GTK_WIDGET (g_object_new (GDL_TYPE_PREVIEW_WINDOW,
                                     "type", GTK_WINDOW_POPUP, NULL));
}

/**
 * gdl_preview_window_update:
 * @window: a #GdlPreviewWindow
 * @rect: the new position and size of the window
 *
 * Update the size and position of the preview window. This function is
 * called on drag_motion event to update the representation of the docking site
 * in real time.
 */
void
gdl_preview_window_update (GdlPreviewWindow * window, GdkRectangle *rect)
{
    if (rect->width <= 0 || rect->height <= 0)
    {
        gtk_widget_hide (GTK_WIDGET (window));
        return;
    }

    gtk_window_move (GTK_WINDOW (window), rect->x, rect->y);
    gtk_window_resize (GTK_WINDOW (window), rect->width, rect->height);
    gtk_widget_show (GTK_WIDGET (window));

    /* We (ab)use app-paintable to indicate if we have an RGBA window */
    if (!gtk_widget_get_app_paintable (GTK_WIDGET (window)))
    {
        GdkWindow *gdkwindow = gtk_widget_get_window (GTK_WIDGET (window));

        /* Shape the window to make only the outline visible */
        if (rect->width > 2 && rect->height > 2)
        {
            cairo_region_t *region, *region2;
            cairo_rectangle_int_t region_rect = { 0, 0,
                rect->width - 2, rect->height - 2 };

            region = cairo_region_create_rectangle (&region_rect);
            region_rect.x++;
            region_rect.y++;
            region_rect.width -= 2;
            region_rect.height -= 2;
            region2 = cairo_region_create_rectangle (&region_rect);
            cairo_region_subtract (region, region2);

            gdk_window_shape_combine_region (gdkwindow, region, 0, 0);

            cairo_region_destroy (region);
            cairo_region_destroy (region2);
        }
        else
            gdk_window_shape_combine_region (gdkwindow, NULL, 0, 0);
    }
}
