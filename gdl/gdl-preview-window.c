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
    GtkStyle *style;

    style = gtk_widget_get_style (window);

    if (gtk_widget_get_app_paintable (window))
    {
        cairo_set_line_width (cr, 1.0);

        gtk_widget_get_allocation (window, &allocation);

        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba (cr, 0, 0, 0, 0);
        cairo_paint (cr);

        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_SELECTED]);
        cairo_paint_with_alpha (cr, 0.25);

        cairo_rectangle (cr,
                         allocation.x + 0.5, allocation.y + 0.5,
                         allocation.width - 1, allocation.height - 1);
        cairo_stroke (cr);
    }
    else
    {
        gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_SELECTED]);
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


GtkWidget*
gdl_preview_window_new (void)
{
	return GTK_WIDGET (g_object_new (GDL_TYPE_PREVIEW_WINDOW,
	                                 "type", GTK_WINDOW_POPUP, NULL));
}

void
gdl_preview_window_update (GdlPreviewWindow * pre_window, GdkRectangle *rect)
{
    GtkWidget* window = GTK_WIDGET (pre_window);
    
    if (rect->width <= 0 || rect->height <= 0)
    {
        gtk_widget_hide (window);
        return;
    }

    gtk_window_move (GTK_WINDOW (window), rect->x, rect->y);
    gtk_window_resize (GTK_WINDOW (window), rect->width, rect->height);
    gtk_widget_show (window);

    /* We (ab)use app-paintable to indicate if we have an RGBA window */
    if (!gtk_widget_get_app_paintable (window))
    {
        GdkWindow *gdkwindow = gtk_widget_get_window (window);

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
