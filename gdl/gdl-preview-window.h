/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef _GDL_PREVIEW_WINDOW_H_
#define _GDL_PREVIEW_WINDOW_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDL_TYPE_PREVIEW_WINDOW             (gdl_preview_window_get_type ())
#define GDL_PREVIEW_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDL_TYPE_PREVIEW_WINDOW, GdlPreviewWindow))
#define GDL_PREVIEW_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_PREVIEW_WINDOW, GdlPreviewWindowClass))
#define GDL_IS_PREVIEW_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDL_TYPE_PREVIEW_WINDOW))
#define GDL_IS_PREVIEW_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_PREVIEW_WINDOW))
#define GDL_PREVIEW_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDL_TYPE_PREVIEW_WINDOW, GdlPreviewWindowClass))

typedef struct _GdlPreviewWindowClass GdlPreviewWindowClass;
typedef struct _GdlPreviewWindow GdlPreviewWindow;

struct _GdlPreviewWindowClass
{
	GtkWindowClass parent_class;
};

/**
 * GdlPreviewWindow:
 *
 * The GdlDockLayout struct contains only private fields
 * and should not be directly accessed.
 */
struct _GdlPreviewWindow
{
	GtkWindow parent_instance;
};

GType gdl_preview_window_get_type (void) G_GNUC_CONST;

GtkWidget* gdl_preview_window_new (void);
void gdl_preview_window_update (GdlPreviewWindow * window, GdkRectangle *rect);

G_END_DECLS

#endif /* _GDL_PREVIEW_WINDOW_H_ */
