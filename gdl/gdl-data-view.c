/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 * 
 * This file is part of the GNOME Devtools Libraries.
 * 
 * Copyright (C) 2001 Dave Camp <dave@ximian.com>
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

#include <config.h>
#include "gdl-data-view.h"

#include "tree-expand.xpm"
#include "tree-contract.xpm"

#include <libgnome/libgnome.h>
#include "gdl-data-frame.h"

struct _GdlDataViewPrivate {
	GList *objects;
	GdlDataFrame *selected_frame;
    
	GdkPixbuf *close_pixbuf;
	GdkPixbuf *expand_pixbuf;
	GdkPixbuf *contract_pixbuf;
};

static void gdl_data_view_instance_init (GdlDataView *dv);
static void gdl_data_view_class_init (GdlDataViewClass *klass);

GNOME_CLASS_BOILERPLATE (GdlDataView, gdl_data_view, 
			 GtkLayout, GTK_TYPE_LAYOUT);


#define GRID_SPACING 15

static void
paint_grid (GtkWidget *widget, GdkDrawable *drawable,
	    int offset_x, int offset_y, int width, int height)
{
	GdlDataView *dv;
	int x;
	int y;
	
	g_return_if_fail (GDL_IS_DATA_VIEW (widget));
	dv = GDL_DATA_VIEW (widget);
	
	x = offset_x + ((GRID_SPACING - (offset_x % GRID_SPACING)) % GRID_SPACING);

	/* Draw grid points */
	for (; x  < width; x += GRID_SPACING) {
		y = offset_y + ((GRID_SPACING - (offset_y % GRID_SPACING)) % GRID_SPACING);
		
		for (; y < height; y += GRID_SPACING) {
			gdk_draw_point (drawable,
					widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
					x, y);
		}
	}	
}

static void
expose_children (GdlDataView *view, GdkEventExpose *event)
{
	GList *l;

	for (l = view->priv->objects; l != NULL; l = l->next) {
		gdl_data_frame_draw (GDL_DATA_FRAME (l->data),
				     GTK_LAYOUT(view)->bin_window);
	}	
}


static gboolean
gdl_data_view_expose (GtkWidget *widget,
		      GdkEventExpose *event)
{
	if (GTK_WIDGET_DRAWABLE (widget)) {
		if (event->window == GTK_LAYOUT (widget)->bin_window) {
			paint_grid (widget, GTK_LAYOUT (widget)->bin_window,
				    event->area.x, event->area.y,
				    event->area.width, event->area.height);
			expose_children (GDL_DATA_VIEW (widget), event);
			return TRUE;
		} else {
			GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
		}
	}
	return FALSE;
}

static GdlDataFrame *
frame_at (GdlDataView *dv, int x, int y)
{
	GList *l;
	for (l = dv->priv->objects; l != NULL; l = l->next) {
		GdlDataFrame *frame = l->data;
		if (x >= frame->area.x && x <= frame->area.x + frame->area.width 
		    && y >= frame->area.y && y <= frame->area.y + frame->area.height) {
			return frame;
		}
	}
	return NULL;
}

static gboolean
button_press_event_cb (GdlDataView *dv, GdkEventButton *event, gpointer data)
{
	GdlDataFrame *frame;
	
	frame = frame_at (dv, event->x, event->y);
	if (frame) {
		if (dv->priv->selected_frame) {
			gdl_data_frame_set_selected (dv->priv->selected_frame, FALSE);
		}
		gdl_data_frame_set_selected (frame, TRUE);
		dv->priv->selected_frame = frame;
		
		return gdl_data_frame_button_press (frame, event);
	} 
	return FALSE;
}

static void
gdl_data_view_instance_init (GdlDataView *dv)
{
	GTK_WIDGET_SET_FLAGS (dv, GTK_CAN_FOCUS);
	dv->priv = g_new0 (GdlDataViewPrivate, 1);

	g_signal_connect (G_OBJECT (dv), "button_press_event",
			  G_CALLBACK (button_press_event_cb), 
			  NULL);

	dv->priv->close_pixbuf = gtk_widget_render_icon (GTK_WIDGET (dv),
							 "gtk-close",
							 GTK_ICON_SIZE_MENU,
							 "gdl-data-view-close");
	
	dv->priv->expand_pixbuf = 
		gdk_pixbuf_new_from_xpm_data ((const char **)tree_expand_xpm);
	
	dv->priv->contract_pixbuf = 
		gdk_pixbuf_new_from_xpm_data ((const char **)tree_contract_xpm);
}

static void
gdl_data_view_destroy (GtkObject *obj)
{
	GdlDataView *dv = GDL_DATA_VIEW (obj);
	
	if (dv->priv) {
		GList *l;
		for (l = dv->priv->objects; l != NULL; l = l->next) {
			g_object_unref (G_OBJECT (l->data));
		}
		g_list_free (dv->priv->objects);
		
		g_object_unref (dv->priv->close_pixbuf);
		g_object_unref (dv->priv->expand_pixbuf);
		g_object_unref (dv->priv->contract_pixbuf);

		g_free (dv->priv);
		dv->priv = NULL;
	}
	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (obj));
}

static void
gdl_data_view_class_init (GdlDataViewClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

	parent_class = gtk_type_class (GTK_TYPE_LAYOUT);
	
	widget_class->expose_event = gdl_data_view_expose;
	object_class->destroy = gdl_data_view_destroy;
}

GtkWidget *
gdl_data_view_new (void)
{
	GdlDataView *dv;
	dv = g_object_new (gdl_data_view_get_type (), NULL);
	return GTK_WIDGET (dv);
}

void
gdl_data_view_set_model (GdlDataView *dv, GdlDataModel *model)
{
	GtkTreePath *path;
	GdlDataIter iter;
	gboolean iter_valid;
	int x = 5;

	dv->model = model;

	path = gtk_tree_path_new_from_string ("0");

	iter_valid = gdl_data_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	
	while (iter_valid) {
		GValue value = {0,};
		char *name;
		char *path_string;
		GObject *frame;
		
		gdl_data_model_get_value (model, &iter, &value);
		gdl_data_model_get_name (model, &iter, &name);
		path = gdl_data_model_get_path (model, &iter);

		path_string = gtk_tree_path_to_string (path);
		
		frame = gdl_data_frame_new (dv, path_string);
		gdl_data_frame_set_position (frame, x, 5);

		dv->priv->objects = g_list_append (dv->priv->objects,
						   frame);
		
		g_free (path_string);
		gtk_tree_path_free (path);
		
		x += 150;

		iter_valid = gdl_data_model_iter_next (model, &iter);
	}
}

void 
gdl_data_view_layout (GdlDataView *view)
{
	GList *l;
	for (l = view->priv->objects; l != NULL; l = l->next) {
		gdl_data_frame_layout (GDL_DATA_FRAME (l->data));
	}
}

GdkPixbuf *
gdl_data_view_get_close_pixbuf (GdlDataView *view)
{
	return view->priv->close_pixbuf;
}

void
gdl_data_view_set_close_pixbuf (GdlDataView *view, GdkPixbuf *pixbuf)
{
	if (view->priv->close_pixbuf) {
		g_object_unref (view->priv->close_pixbuf);
	}

	view->priv->close_pixbuf = g_object_ref (pixbuf);
}

GdkPixbuf *
gdl_data_view_get_expand_pixbuf (GdlDataView *view)
{
	return view->priv->expand_pixbuf;
}

void
gdl_data_view_set_expand_pixbuf (GdlDataView *view, GdkPixbuf *pixbuf)
{
	if (view->priv->expand_pixbuf) {
		g_object_unref (view->priv->expand_pixbuf);
	}

	view->priv->expand_pixbuf = g_object_ref (pixbuf);
}

GdkPixbuf *
gdl_data_view_get_contract_pixbuf (GdlDataView *view)
{
	return view->priv->contract_pixbuf;
}

void
gdl_data_view_set_contract_pixbuf (GdlDataView *view, GdkPixbuf *pixbuf)
{
	if (view->priv->contract_pixbuf) {
		g_object_unref (view->priv->contract_pixbuf);
	}

	view->priv->contract_pixbuf = g_object_ref (pixbuf);
}

