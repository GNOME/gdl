/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2012 Sébastien Granjoux <seb.sfo@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#undef GDL_DISABLE_DEPRECATED

#include "gdl.h"

/**
 * gdl_dock_bar_get_orientation:
 * @dockbar: a #GdlDockBar
 *
 * Retrieves the orientation of the @dockbar.
 *
 * Returns: the orientation of the @docbar
 *
 * Deprecated: 3.6: Use gtk_orientable_get_orientation() instead.
 */
GtkOrientation gdl_dock_bar_get_orientation (GdlDockBar *dockbar)
{
    return gtk_orientable_get_orientation (GTK_ORIENTABLE (dockbar));
}

/**
 * gdl_dock_bar_set_orientation:
 * @dockbar: a #GdlDockBar
 * @orientation: the new orientation
 *
 * Set the orientation of the @dockbar.
 *
 * Deprecated: 3.6: Use gtk_orientable_set_orientation() instead.
 */
void gdl_dock_bar_set_orientation (GdlDockBar *dockbar,
	                             GtkOrientation orientation)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (dockbar), orientation);
}

/**
 * gdl_dock_xor_rect:
 * @dock: A #GdlDock widget
 * @rect: The position and the size of the preview window
 *
 * Show a preview window used to materialize the dock target.
 *
 * Deprecated: 3.6: Use gdl_dock_show_preview instead.
 */
void
gdl_dock_xor_rect (GdlDock      *dock,
		   cairo_rectangle_int_t *rect)
{
    gdl_dock_show_preview (dock, rect);
}

/**
 * gdl_dock_xor_rect_hide:
 * @dock: A #GdlDock widget
 *
 * Hide the preview window used to materialize the dock target.
 *
 * Deprecated: 3.6: Use gdl_dock_hide_preview instead.
 */
void
gdl_dock_xor_rect_hide (GdlDock      *dock)
{
    gdl_dock_hide_preview (dock);
}

/**
 * gdl_dock_get_placeholder_by_name:
 * @dock: A #GdlDock widget
 * @name: An item name
 *
 * Looks for an #GdlDockPlaceholder object bound to the master of the dock item.
 * It does not search only in the children of this particular dock widget.
 *
 * Returns: (transfer none): A #GdlDockPlaceholder object or %NULL
 *
 * Deprecated: 3.6: This function is always returning %NULL.
 */
GdlDockPlaceholder *
gdl_dock_get_placeholder_by_name (GdlDock     *dock,
                                  const gchar *name)
{
    return NULL;
}

/**
 * gdl_dock_item_set_default_position:
 * @item: The dock item
 * @reference: The GdlDockObject which is the default dock for @item
 *
 * This method has only an effect when you add you dock_item with
 * GDL_DOCK_ITEM_BEH_NEVER_FLOATING. In this case you have to assign
 * it a default position.*
 *
 * Deprecated 3.6: This function is doing nothing now.
 **/
void
gdl_dock_item_set_default_position (GdlDockItem   *item,
                                    GdlDockObject *reference)
{
    g_return_if_fail (item != NULL);
}

/**
 * gdl_dock_layout_attach:
 * @layout: The layout object
 * @master: The master object to which the layout will be attached
 *
 * Attach the @layout to the @master and delete the reference to
 * the master that the layout attached previously.
 *
 * Deprecated 3.6: Use gdl_dock_layout_set_master() instead.
 */
void
gdl_dock_layout_attach (GdlDockLayout *layout,
                        GdlDockMaster *master)
{
    g_return_if_fail (master == NULL || GDL_IS_DOCK_MASTER (master));

    gdl_dock_layout_set_master (layout, G_OBJECT (master));
}
