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

#include <libgnorba/gnorba.h>
#include "generic-factory.h"
#include "event-channel.h"

/*** Implementation stub prototypes ***/

static void
impl_Bonobo_GenericFactory__destroy(impl_POA_Bonobo_GenericFactory *servant,
                                   CORBA_Environment * ev);
static CORBA_Object
impl_Bonobo_GenericFactory_create_object (impl_POA_Bonobo_GenericFactory *servant, 
                                         const CORBA_char *obj_goad_id,
                                         const GNOME_stringlist *params,
                                         CORBA_Environment *ev);
static CORBA_boolean
impl_Bonobo_GenericFactory_supports (impl_POA_Bonobo_GenericFactory *servant,
                                    const CORBA_char *obj_goad_id, 
                                    CORBA_Environment *ev);

/*** epv structures ***/

static PortableServer_ServantBase__epv impl_Bonobo_GenericFactory_base_epv =
{
    NULL,                        /* _private data */
    (gpointer) & impl_Bonobo_GenericFactory__destroy, /* finalize routine */
    NULL,                        /* default_POA routine */
};

static POA_Bonobo_GenericFactory__epv impl_Bonobo_GenericFactory_epv = 
{
    NULL,                        /* _private */
    (gpointer) & impl_Bonobo_GenericFactory_supports,
    (gpointer) & impl_Bonobo_GenericFactory_create_object
};

/*** vepv structures ***/

static POA_Bonobo_GenericFactory__vepv impl_Bonobo_GenericFactory_vepv =
{
    &impl_Bonobo_GenericFactory_base_epv,
    &impl_Bonobo_GenericFactory_epv,
};

/*** GenericFactory ***/

Bonobo_GenericFactory 
impl_Bonobo_GenericFactory__create(PortableServer_POA poa, 
                                  CORBA_Environment * ev)
{
    Bonobo_GenericFactory retval;
    impl_POA_Bonobo_GenericFactory *newservant;
    PortableServer_ObjectId *objid;
	
    newservant = g_new0(impl_POA_Bonobo_GenericFactory, 1);
    newservant->servant.vepv = &impl_Bonobo_GenericFactory_vepv;
    newservant->poa = poa;
    POA_Bonobo_GenericFactory__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    return retval;
}

static void
impl_Bonobo_GenericFactory__destroy(impl_POA_Bonobo_GenericFactory *servant, 
                                   CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;

    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_Bonobo_GenericFactory__fini((PortableServer_Servant) servant, ev);
    g_free(servant);
}

static CORBA_Object
impl_Bonobo_GenericFactory_create_object (impl_POA_Bonobo_GenericFactory *servant, 
                                         const CORBA_char *obj_goad_id,
                                         const GNOME_stringlist *params,
                                         CORBA_Environment *ev)
{
    CORBA_Object retval = CORBA_OBJECT_NIL;

    if (!strcmp (obj_goad_id, "gdf_event_channel")) {
        retval = impl_CosEventChannelAdmin_EventChannel__create
            (servant->poa, ev);
    }
    if (retval != CORBA_OBJECT_NIL)
        return CORBA_Object_duplicate (retval, ev);
    else
        return CORBA_OBJECT_NIL;
}

static CORBA_boolean
impl_Bonobo_GenericFactory_supports (impl_POA_Bonobo_GenericFactory *servant,
                                    const CORBA_char *obj_goad_id, 
                                    CORBA_Environment *ev)
{
    if (strcmp (obj_goad_id, "gdf_event_channel"))
        return CORBA_FALSE;
    else 
        return CORBA_TRUE;
}
