/*  -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * This file is part of the GNOME Debugging Framework.
 * 
 * Copyright (C) 1999-2000 Dave Camp <campd@oit.edu>
 *                         Martin Baulig <martin@home-of-linux.org>
 *                         Sebastian Wilhelmi
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

#ifndef GENERIC_FACTORY_H
#define GENERIC_FACTORY_H 1

#include "gdf-event-server.h"
#include <bonobo.h>

typedef struct {
    POA_Bonobo_GenericFactory servant;
    PortableServer_POA poa;
} impl_POA_Bonobo_GenericFactory;

extern Bonobo_GenericFactory
impl_Bonobo_GenericFactory__create (PortableServer_POA, CORBA_Environment *);

#endif /* GENERIC_FACTORY_H */
