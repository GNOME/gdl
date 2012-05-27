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

#include "gdl.h"

#ifndef GDL_DISABLE_DEPRECATED

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
#endif
