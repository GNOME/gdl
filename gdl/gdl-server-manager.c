/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * This file is part of the GNOME Devtool Libraries.
 * 
 * Copyright (C) 1999-2000 Dave Camp <dave@helixcode.com>
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
#include "gdl-server-manager.h"

#include <gtk/gtksignal.h>

static GdlServerManagerCallback all_destroyed_callback = NULL;
static int num_servers = 0;

static void destroy_cb (GtkObject *server, gpointer data);

/**
 * gdl_server_manager_init:
 * @callback: Function to call when all servants have been destroyed.
 * 
 * Initializes the server manager.  @callback is called when all registered 
 * servers have been destroyed.
 **/
void
gdl_server_manager_init (GdlServerManagerCallback callback)
{
    all_destroyed_callback = callback;
    num_servers = 0;
}

/**
 * gdl_server_manager_register_object:
 * @server: The object to register.
 * 
 * Registers an object with the server manager.
 **/
void
gdl_server_manager_register_object (GtkObject *object)
{
    gtk_signal_connect (GTK_OBJECT (object),
                        "destroy",
                        GTK_SIGNAL_FUNC (destroy_cb),
                        NULL);
    num_servers++;
}

/* private functions */

void
destroy_cb (GtkObject *server, gpointer data)
{
    num_servers--;

    if (num_servers == 0 && all_destroyed_callback) {
        all_destroyed_callback ();
    }
}




