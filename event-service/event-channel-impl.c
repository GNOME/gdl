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

#include "event-channel.h"

static gint num_channels = 0; /* Number of currently instantiated channels */

/*** Implementation stub prototypes ***/

static void
impl_CosEventChannelAdmin_ProxyPushConsumer_push(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant,
                                                 CORBA_any * data,
                                                 CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_ProxyPushConsumer_disconnect_push_consumer(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant,
                                                                     CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_ProxyPushSupplier_disconnect_push_supplier(impl_POA_CosEventChannelAdmin_ProxyPushSupplier * servant,
                                                                     CORBA_Environment * ev);

static CORBA_any *
impl_CosEventChannelAdmin_ProxyPullSupplier_pull(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                 CORBA_Environment * ev);

static CORBA_any *
impl_CosEventChannelAdmin_ProxyPullSupplier_try_pull(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                     CORBA_boolean * has_event,
                                                     CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_ProxyPullSupplier_disconnect_pull_supplier(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                                     CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_ProxyPullConsumer_disconnect_pull_consumer(impl_POA_CosEventChannelAdmin_ProxyPullConsumer * servant,
                                                                     CORBA_Environment * ev);

static void 
impl_CosEventChannelAdmin_ProxyPushConsumer__destroy(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant,
                                                     CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_ProxyPushConsumer_connect_push_supplier(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant,
                                                                  CosEventComm_PushSupplier push_supplier,
                                                                  CORBA_Environment * ev);

static void 
impl_CosEventChannelAdmin_ProxyPullSupplier__destroy(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                     CORBA_Environment * ev);
static void
impl_CosEventChannelAdmin_ProxyPullSupplier_connect_pull_consumer(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                                  CosEventComm_PullConsumer pull_consumer,
                                                                  CORBA_Environment * ev);

static void 
impl_CosEventChannelAdmin_ProxyPullConsumer__destroy(impl_POA_CosEventChannelAdmin_ProxyPullConsumer * servant,
                                                     CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_ProxyPullConsumer_connect_pull_supplier(impl_POA_CosEventChannelAdmin_ProxyPullConsumer * servant,
                                                                  CosEventComm_PullSupplier pull_supplier,
                                                                  CORBA_Environment * ev);

static void 
impl_CosEventChannelAdmin_ProxyPushSupplier__destroy(impl_POA_CosEventChannelAdmin_ProxyPushSupplier * servant,
                                                     CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_ProxyPushSupplier_connect_push_consumer(impl_POA_CosEventChannelAdmin_ProxyPushSupplier * servant,
                                                                  CosEventComm_PushConsumer push_consumer,
                                                                  CORBA_Environment * ev);

static void 
impl_CosEventChannelAdmin_ConsumerAdmin__destroy(impl_POA_CosEventChannelAdmin_ConsumerAdmin * servant,
                                                 CORBA_Environment * ev);

static CosEventChannelAdmin_ProxyPushSupplier
impl_CosEventChannelAdmin_ConsumerAdmin_obtain_push_supplier(impl_POA_CosEventChannelAdmin_ConsumerAdmin * servant,
                                                             CORBA_Environment * ev);

static CosEventChannelAdmin_ProxyPullSupplier
impl_CosEventChannelAdmin_ConsumerAdmin_obtain_pull_supplier(impl_POA_CosEventChannelAdmin_ConsumerAdmin * servant,
                                                             CORBA_Environment * ev);

static void 
impl_CosEventChannelAdmin_SupplierAdmin__destroy(impl_POA_CosEventChannelAdmin_SupplierAdmin * servant,
                                                 CORBA_Environment * ev);

static CosEventChannelAdmin_ProxyPushConsumer
impl_CosEventChannelAdmin_SupplierAdmin_obtain_push_consumer(impl_POA_CosEventChannelAdmin_SupplierAdmin * servant,
                                                             CORBA_Environment * ev);

static CosEventChannelAdmin_ProxyPullConsumer
impl_CosEventChannelAdmin_SupplierAdmin_obtain_pull_consumer(impl_POA_CosEventChannelAdmin_SupplierAdmin * servant,
                                                             CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_EventChannel__destroy(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                                CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_EventChannel_destroy(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                               CORBA_Environment * ev);

static CosEventChannelAdmin_ConsumerAdmin
impl_CosEventChannelAdmin_EventChannel_for_consumers(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                                     CORBA_Environment * ev);

static CosEventChannelAdmin_SupplierAdmin
impl_CosEventChannelAdmin_EventChannel_for_suppliers(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                                     CORBA_Environment * ev);

static void
impl_CosEventChannelAdmin_EventChannel_destroy(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                               CORBA_Environment * ev);

/*** epv structures ***/

POA_CosEventComm_PushConsumer__epv impl_CosEventComm_PushConsumer_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPushConsumer_push,
    (gpointer) & impl_CosEventChannelAdmin_ProxyPushConsumer_disconnect_push_consumer,
};

POA_CosEventComm_PushSupplier__epv impl_CosEventComm_PushSupplier_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPushSupplier_disconnect_push_supplier,
};

POA_CosEventComm_PullSupplier__epv impl_CosEventComm_PullSupplier_epv =
{
    NULL,                        /* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullSupplier_pull,
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullSupplier_try_pull,
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullSupplier_disconnect_pull_supplier,
};

POA_CosEventComm_PullConsumer__epv impl_CosEventComm_PullConsumer_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullConsumer_disconnect_pull_consumer,
};

PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPushConsumer_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPushConsumer__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

POA_CosEventChannelAdmin_ProxyPushConsumer__epv impl_CosEventChannelAdmin_ProxyPushConsumer_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPushConsumer_connect_push_supplier,
};

PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPullSupplier_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullSupplier__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

POA_CosEventChannelAdmin_ProxyPullSupplier__epv impl_CosEventChannelAdmin_ProxyPullSupplier_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullSupplier_connect_pull_consumer,
};

PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPullConsumer_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullConsumer__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

POA_CosEventChannelAdmin_ProxyPullConsumer__epv impl_CosEventChannelAdmin_ProxyPullConsumer_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPullConsumer_connect_pull_supplier,
};

PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPushSupplier_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPushSupplier__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

POA_CosEventChannelAdmin_ProxyPushSupplier__epv impl_CosEventChannelAdmin_ProxyPushSupplier_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ProxyPushSupplier_connect_push_consumer,
};

PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ConsumerAdmin_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_CosEventChannelAdmin_ConsumerAdmin__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

POA_CosEventChannelAdmin_ConsumerAdmin__epv impl_CosEventChannelAdmin_ConsumerAdmin_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_ConsumerAdmin_obtain_push_supplier,
    (gpointer) & impl_CosEventChannelAdmin_ConsumerAdmin_obtain_pull_supplier,
};

PortableServer_ServantBase__epv impl_CosEventChannelAdmin_SupplierAdmin_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_CosEventChannelAdmin_SupplierAdmin__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

POA_CosEventChannelAdmin_SupplierAdmin__epv impl_CosEventChannelAdmin_SupplierAdmin_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_SupplierAdmin_obtain_push_consumer,
    (gpointer) & impl_CosEventChannelAdmin_SupplierAdmin_obtain_pull_consumer,
};

PortableServer_ServantBase__epv impl_CosEventChannelAdmin_EventChannel_base_epv =
{
    NULL,			/* _private data */
    (gpointer) & impl_CosEventChannelAdmin_EventChannel__destroy,	/* finalize routine */
    NULL,			/* default_POA routine */
};

POA_CosEventChannelAdmin_EventChannel__epv impl_CosEventChannelAdmin_EventChannel_epv =
{
    NULL,			/* _private */
    (gpointer) & impl_CosEventChannelAdmin_EventChannel_for_consumers,
    (gpointer) & impl_CosEventChannelAdmin_EventChannel_for_suppliers,
    (gpointer) & impl_CosEventChannelAdmin_EventChannel_destroy,
};

/*** vepv structures ***/


static POA_CosEventChannelAdmin_ProxyPushConsumer__vepv impl_CosEventChannelAdmin_ProxyPushConsumer_vepv =
{
    &impl_CosEventChannelAdmin_ProxyPushConsumer_base_epv,
    &impl_CosEventComm_PushConsumer_epv,
    &impl_CosEventChannelAdmin_ProxyPushConsumer_epv,
};

static POA_CosEventChannelAdmin_ProxyPullSupplier__vepv impl_CosEventChannelAdmin_ProxyPullSupplier_vepv =
{
    &impl_CosEventChannelAdmin_ProxyPullSupplier_base_epv,
    &impl_CosEventComm_PullSupplier_epv,
    &impl_CosEventChannelAdmin_ProxyPullSupplier_epv,
};

static POA_CosEventChannelAdmin_ProxyPullConsumer__vepv impl_CosEventChannelAdmin_ProxyPullConsumer_vepv =
{
    &impl_CosEventChannelAdmin_ProxyPullConsumer_base_epv,
    &impl_CosEventComm_PullConsumer_epv,
    &impl_CosEventChannelAdmin_ProxyPullConsumer_epv,
};

static POA_CosEventChannelAdmin_ProxyPushSupplier__vepv impl_CosEventChannelAdmin_ProxyPushSupplier_vepv =
{
    &impl_CosEventChannelAdmin_ProxyPushSupplier_base_epv,
    &impl_CosEventComm_PushSupplier_epv,
    &impl_CosEventChannelAdmin_ProxyPushSupplier_epv,
};

static POA_CosEventChannelAdmin_ConsumerAdmin__vepv impl_CosEventChannelAdmin_ConsumerAdmin_vepv =
{
    &impl_CosEventChannelAdmin_ConsumerAdmin_base_epv,
    &impl_CosEventChannelAdmin_ConsumerAdmin_epv,
};

static POA_CosEventChannelAdmin_SupplierAdmin__vepv impl_CosEventChannelAdmin_SupplierAdmin_vepv =
{
    &impl_CosEventChannelAdmin_SupplierAdmin_base_epv,
    &impl_CosEventChannelAdmin_SupplierAdmin_epv,
};

static POA_CosEventChannelAdmin_EventChannel__vepv impl_CosEventChannelAdmin_EventChannel_vepv =
{
    &impl_CosEventChannelAdmin_EventChannel_base_epv,
    &impl_CosEventChannelAdmin_EventChannel_epv,
};

/*** Other ***/
static void 
event_channel_notify (impl_POA_CosEventChannelAdmin_EventChannel *servant,
                      CORBA_any *data,
                      CORBA_Environment *ev);

/*** Macros ***/

#define THROW(name,arg,ev) \
    CORBA_exception_set (ev, CORBA_USER_EXCEPTION, ex_##name, arg)

#define THROW_AlreadyConnected(ev) \
    THROW(CosEventChannelAdmin_AlreadyConnected, NULL, ev)
#define THROW_Disconnected(ev) \
    THROW(CosEventChannelAdmin_Disconnected, NULL, ev)
#define THROW_BadParam(ev) \
    CORBA_exception_set_system (ev, 0, ex_CORBA_BAD_PARAM);

/*** ProxyPushConsumer ***/

static CosEventChannelAdmin_ProxyPushConsumer 
impl_CosEventChannelAdmin_ProxyPushConsumer__create(PortableServer_POA poa,
                                                    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant,
                                                    CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPushConsumer retval;
    impl_POA_CosEventChannelAdmin_ProxyPushConsumer *newservant;
    PortableServer_ObjectId *objid;

    newservant = g_new0(impl_POA_CosEventChannelAdmin_ProxyPushConsumer, 1);
    newservant->servant.vepv = &impl_CosEventChannelAdmin_ProxyPushConsumer_vepv;
    newservant->poa = poa;             
    newservant->channel_servant = channel_servant;
    channel_servant->push_consumer = 
        g_list_prepend (channel_servant->push_consumer, 
                        newservant); 
    newservant->supplier = CORBA_OBJECT_NIL;
    POA_CosEventChannelAdmin_ProxyPushConsumer__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    return retval;
}

static void
impl_CosEventChannelAdmin_ProxyPushConsumer__destroy(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant, CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;
	
    if (!CORBA_Object_is_nil (servant->supplier, ev))
        CORBA_Object_release (servant->supplier, ev);

    servant->channel_servant->push_consumer = 
        g_list_remove (servant->channel_servant->push_consumer, 
                       servant);
	
    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_CosEventChannelAdmin_ProxyPushConsumer__fini((PortableServer_Servant) servant, ev);
    g_free(servant);
	
}

void
impl_CosEventChannelAdmin_ProxyPushConsumer_push(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant,
                                                 CORBA_any * data,
                                                 CORBA_Environment * ev)
{
    event_channel_notify (servant->channel_servant, data, ev);
}

void
impl_CosEventChannelAdmin_ProxyPushConsumer_disconnect_push_consumer(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant,
                                                                     CORBA_Environment * ev)
{	
    /* FIXME: Call disconnect on the supplier? */
    impl_CosEventChannelAdmin_ProxyPushConsumer__destroy (servant,
                                                          ev);
}

void
impl_CosEventChannelAdmin_ProxyPushConsumer_connect_push_supplier(impl_POA_CosEventChannelAdmin_ProxyPushConsumer * servant,
                                                                  CosEventComm_PushSupplier push_supplier,
                                                                  CORBA_Environment * ev)
{
    if (CORBA_Object_is_nil (push_supplier, ev)) {
        THROW_BadParam (ev);
        return;
    }
	
    if (!CORBA_Object_is_nil (servant->supplier, ev)) {
        THROW_AlreadyConnected (ev);
        return;
    }
	
    servant->supplier = CORBA_Object_duplicate (push_supplier, ev);
}

/*** ProxyPushSupplier ***/

static CosEventChannelAdmin_ProxyPushSupplier 
impl_CosEventChannelAdmin_ProxyPushSupplier__create(PortableServer_POA poa, 
                                                    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant, 
                                                    CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPushSupplier retval;
    impl_POA_CosEventChannelAdmin_ProxyPushSupplier *newservant;
    PortableServer_ObjectId *objid;
	
    newservant = g_new0(impl_POA_CosEventChannelAdmin_ProxyPushSupplier, 1);
    newservant->servant.vepv = &impl_CosEventChannelAdmin_ProxyPushSupplier_vepv;
    newservant->poa = poa;
    
    newservant->channel_servant = channel_servant; 
    channel_servant->push_supplier = 
        g_list_prepend (channel_servant->push_supplier, 
                        newservant); 
    newservant->consumer = CORBA_OBJECT_NIL;
	
    POA_CosEventChannelAdmin_ProxyPushSupplier__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    return retval;
}

static void
impl_CosEventChannelAdmin_ProxyPushSupplier__destroy(impl_POA_CosEventChannelAdmin_ProxyPushSupplier * servant, CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;
		
    if (!CORBA_Object_is_nil (servant->consumer, ev))
        CORBA_Object_release (servant->consumer, ev);

    servant->channel_servant->push_supplier = 
        g_list_remove (servant->channel_servant->push_supplier, 
                       servant);
	
    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_CosEventChannelAdmin_ProxyPushSupplier__fini((PortableServer_Servant) servant, ev);
    g_free(servant);
}

void
impl_CosEventChannelAdmin_ProxyPushSupplier_disconnect_push_supplier(impl_POA_CosEventChannelAdmin_ProxyPushSupplier * servant,
                                                                     CORBA_Environment * ev)
{
    /* FIXME: Call disconnect on the consumer? */
    impl_CosEventChannelAdmin_ProxyPushSupplier__destroy (servant,
                                                          ev);
}

void
impl_CosEventChannelAdmin_ProxyPushSupplier_connect_push_consumer (impl_POA_CosEventChannelAdmin_ProxyPushSupplier * servant,
                                                                   CosEventComm_PushConsumer push_consumer,
                                                                   CORBA_Environment * ev)
{
    if (CORBA_Object_is_nil (push_consumer, ev)) {
        THROW_BadParam (ev);
        return;
    }

    if (!CORBA_Object_is_nil (servant->consumer, ev)) {
        THROW_AlreadyConnected (ev);
        return;
    }
	
    servant->consumer = CORBA_Object_duplicate (push_consumer, ev);
}

static void
push_supplier_notify (impl_POA_CosEventChannelAdmin_ProxyPushSupplier *servant,
                      CORBA_any *data,
                      CORBA_Environment *ev)
{
    CosEventComm_PushConsumer_push (servant->consumer, data, ev);
}

/*** ProxyPullConsumer ***/

static CosEventChannelAdmin_ProxyPullConsumer 
impl_CosEventChannelAdmin_ProxyPullConsumer__create(PortableServer_POA poa,
                                                    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant, 
                                                    CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPullConsumer retval;
    impl_POA_CosEventChannelAdmin_ProxyPullConsumer *newservant;
    PortableServer_ObjectId *objid;
	
    newservant = g_new0(impl_POA_CosEventChannelAdmin_ProxyPullConsumer, 1);
    newservant->servant.vepv = &impl_CosEventChannelAdmin_ProxyPullConsumer_vepv;
    newservant->poa = poa;
	
    newservant->channel_servant = channel_servant;
    channel_servant->pull_consumer = 
        g_list_prepend (channel_servant->pull_consumer, 
                        newservant);
	
    POA_CosEventChannelAdmin_ProxyPullConsumer__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    return retval;
}

static void
impl_CosEventChannelAdmin_ProxyPullConsumer__destroy(impl_POA_CosEventChannelAdmin_ProxyPullConsumer * servant, CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;
	
	
    if (!CORBA_Object_is_nil (servant->supplier, ev))
        CORBA_Object_release (servant->supplier, ev);

    servant->channel_servant->pull_consumer = 
        g_list_remove (servant->channel_servant->pull_consumer, servant);
	
    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_CosEventChannelAdmin_ProxyPullConsumer__fini((PortableServer_Servant) servant, ev);
    g_free(servant);
}

void
impl_CosEventChannelAdmin_ProxyPullConsumer_disconnect_pull_consumer(impl_POA_CosEventChannelAdmin_ProxyPullConsumer * servant,
                                                                     CORBA_Environment * ev)
{
    /* FIXME: Call disconnect on the supplier? */
    impl_CosEventChannelAdmin_ProxyPullConsumer__destroy (servant, ev);
}

void
impl_CosEventChannelAdmin_ProxyPullConsumer_connect_pull_supplier(impl_POA_CosEventChannelAdmin_ProxyPullConsumer * servant,
                                                                  CosEventComm_PullSupplier pull_supplier,
                                                                  CORBA_Environment * ev)
{
    if (CORBA_Object_is_nil (pull_supplier, ev)) {
        THROW_BadParam (ev);
        return;
    }

    if (!CORBA_Object_is_nil (servant->supplier, ev)) {
        THROW_AlreadyConnected (ev);
        return;
    }
	
    servant->supplier = CORBA_Object_duplicate (pull_supplier, ev);
}

/*** ProxyPullSupplier ***/

static CosEventChannelAdmin_ProxyPullSupplier 
impl_CosEventChannelAdmin_ProxyPullSupplier__create(PortableServer_POA poa, 
                                                    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant, 
                                                    CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPullSupplier retval;
    impl_POA_CosEventChannelAdmin_ProxyPullSupplier *newservant;
    PortableServer_ObjectId *objid;
	
    newservant = g_new0(impl_POA_CosEventChannelAdmin_ProxyPullSupplier, 1);
    newservant->servant.vepv = &impl_CosEventChannelAdmin_ProxyPullSupplier_vepv;
    newservant->poa = poa;
	
    newservant->channel_servant = channel_servant;
    channel_servant->pull_supplier = 
        g_list_prepend (channel_servant->pull_supplier, 
                        newservant); 
    newservant->event_queue_front = NULL; 
    newservant->event_queue_back = NULL;

    POA_CosEventChannelAdmin_ProxyPullSupplier__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    return retval;
}

static void
impl_CosEventChannelAdmin_ProxyPullSupplier__destroy(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant, CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;
    GSList *tmp, *tmp2;

    /* Destroy the event queue */
    tmp = servant->event_queue_front;
    while (tmp) {
        CORBA_free ((CORBA_any*)tmp->data);
        tmp2 = g_slist_next (tmp);
        g_slist_free_1 (tmp);
        tmp = tmp2;
    }	

    servant->channel_servant->pull_supplier = 
        g_list_remove (servant->channel_servant->pull_supplier,
                       servant);

    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_CosEventChannelAdmin_ProxyPullSupplier__fini((PortableServer_Servant) servant, ev);
    g_free(servant);
}


CORBA_any *
impl_CosEventChannelAdmin_ProxyPullSupplier_pull(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                 CORBA_Environment * ev)
{
    CORBA_any *retval;

    /* FIXME: Implement this */
    g_assert (FALSE);
    retval = CORBA_any_alloc ();
    return retval;
}

CORBA_any *
impl_CosEventChannelAdmin_ProxyPullSupplier_try_pull(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                     CORBA_boolean * has_event,
                                                     CORBA_Environment * ev)
{
    CORBA_any *retval;

    if (servant->event_queue_front) {
        *has_event = CORBA_TRUE;
        retval = (CORBA_any*)servant->event_queue_front->data;

        /* Take the item out of the queue */
        servant->event_queue_front = 
            g_slist_remove_link (servant->event_queue_front,
                                 servant->event_queue_front);
		
        if (!servant->event_queue_front)
            servant->event_queue_back = NULL;
    } else {
        retval = CORBA_any_alloc ();
        *has_event = CORBA_FALSE;

    }

    return retval;	
}

void
impl_CosEventChannelAdmin_ProxyPullSupplier_disconnect_pull_supplier(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                                     CORBA_Environment * ev)
{
    /* FIXME: Call disconnect on the consumer? */
    impl_CosEventChannelAdmin_ProxyPullSupplier__destroy (servant, ev);
}

void
impl_CosEventChannelAdmin_ProxyPullSupplier_connect_pull_consumer(impl_POA_CosEventChannelAdmin_ProxyPullSupplier * servant,
                                                                  CosEventComm_PullConsumer pull_consumer,
                                                                  CORBA_Environment * ev)
{
    if (CORBA_Object_is_nil (pull_consumer, ev)) {
        THROW_BadParam (ev);
        return;
    }	

    if (!CORBA_Object_is_nil (servant->consumer, ev)) {
        THROW_AlreadyConnected (ev);
        return;
    }
	
    servant->consumer = CORBA_Object_duplicate (pull_consumer, ev);	
}

static void
pull_supplier_notify (impl_POA_CosEventChannelAdmin_ProxyPullSupplier *servant,
                      CORBA_any *data,
                      CORBA_Environment *ev)
{
    CORBA_any *data_copy;

    data_copy = CORBA_any_alloc ();
    CORBA_any__copy (data_copy, data);
	
    servant->event_queue_front = g_slist_append (servant->event_queue_back,
                                                 data_copy);
    if (servant->event_queue_back) {
        servant->event_queue_back = servant->event_queue_back->next;
    } else {
        servant->event_queue_back = servant->event_queue_front;
    }
	
}

/*** ConsumerAdmin ***/

static CosEventChannelAdmin_ConsumerAdmin 
impl_CosEventChannelAdmin_ConsumerAdmin__create(PortableServer_POA poa, 
                                                impl_POA_CosEventChannelAdmin_EventChannel* channel_servant,
                                                CORBA_Environment * ev)
{
    CosEventChannelAdmin_ConsumerAdmin retval;
    impl_POA_CosEventChannelAdmin_ConsumerAdmin *newservant;
    PortableServer_ObjectId *objid;
	
    newservant = g_new0(impl_POA_CosEventChannelAdmin_ConsumerAdmin, 1);
    newservant->servant.vepv = &impl_CosEventChannelAdmin_ConsumerAdmin_vepv;
    newservant->poa = poa;
	
    newservant->channel_servant = channel_servant;
	
    POA_CosEventChannelAdmin_ConsumerAdmin__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    return retval;
}

static void
impl_CosEventChannelAdmin_ConsumerAdmin__destroy(impl_POA_CosEventChannelAdmin_ConsumerAdmin * servant, CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;

    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_CosEventChannelAdmin_ConsumerAdmin__fini((PortableServer_Servant) servant, ev);
    g_free(servant);
}

CosEventChannelAdmin_ProxyPushSupplier
impl_CosEventChannelAdmin_ConsumerAdmin_obtain_push_supplier(impl_POA_CosEventChannelAdmin_ConsumerAdmin * servant,
                                                             CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPushSupplier retval = 
        impl_CosEventChannelAdmin_ProxyPushSupplier__create( servant->poa, servant->channel_servant, ev );
	
    return retval;
}

CosEventChannelAdmin_ProxyPullSupplier
impl_CosEventChannelAdmin_ConsumerAdmin_obtain_pull_supplier(impl_POA_CosEventChannelAdmin_ConsumerAdmin * servant,
                                                             CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPullSupplier retval;
	
    retval = 
        impl_CosEventChannelAdmin_ProxyPullSupplier__create (servant->poa,
                                                             servant->channel_servant,
                                                             ev);
	
    return retval;
}

/*** SupplierAdmin ***/

static CosEventChannelAdmin_SupplierAdmin 
impl_CosEventChannelAdmin_SupplierAdmin__create(PortableServer_POA poa, 
                                                impl_POA_CosEventChannelAdmin_EventChannel* channel_servant,
                                                CORBA_Environment * ev)
{
    CosEventChannelAdmin_SupplierAdmin retval;
    impl_POA_CosEventChannelAdmin_SupplierAdmin *newservant;
    PortableServer_ObjectId *objid;
	
    newservant = g_new0(impl_POA_CosEventChannelAdmin_SupplierAdmin, 1);
    newservant->servant.vepv = &impl_CosEventChannelAdmin_SupplierAdmin_vepv;
    newservant->poa = poa;
	
    newservant->channel_servant = channel_servant; 
	
    POA_CosEventChannelAdmin_SupplierAdmin__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    return retval;
}

static void
impl_CosEventChannelAdmin_SupplierAdmin__destroy(impl_POA_CosEventChannelAdmin_SupplierAdmin * servant, 
                                                 CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;

    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_CosEventChannelAdmin_SupplierAdmin__fini((PortableServer_Servant) servant, ev);
    g_free(servant);
}

CosEventChannelAdmin_ProxyPushConsumer
impl_CosEventChannelAdmin_SupplierAdmin_obtain_push_consumer(impl_POA_CosEventChannelAdmin_SupplierAdmin * servant,
                                                             CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPushConsumer retval;
	
    retval = impl_CosEventChannelAdmin_ProxyPushConsumer__create( servant->poa, servant->channel_servant, ev );
	
    return retval;
}

CosEventChannelAdmin_ProxyPullConsumer
impl_CosEventChannelAdmin_SupplierAdmin_obtain_pull_consumer(impl_POA_CosEventChannelAdmin_SupplierAdmin * servant,
                                                             CORBA_Environment * ev)
{
    CosEventChannelAdmin_ProxyPullConsumer retval;

    retval = 
        impl_CosEventChannelAdmin_ProxyPullConsumer__create (servant->poa,
                                                             servant->channel_servant,
                                                             ev);
	
    return retval;
}

/*** EventChannel ***/

CosEventChannelAdmin_EventChannel 
impl_CosEventChannelAdmin_EventChannel__create(PortableServer_POA poa, CORBA_Environment * ev)
{
    CosEventChannelAdmin_EventChannel retval;
    impl_POA_CosEventChannelAdmin_EventChannel *newservant;
    PortableServer_ObjectId *objid;
	
    newservant = g_new0(impl_POA_CosEventChannelAdmin_EventChannel, 1);
    newservant->servant.vepv = &impl_CosEventChannelAdmin_EventChannel_vepv;
    newservant->poa = poa;
	
    newservant->supplier_admin = 
        impl_CosEventChannelAdmin_SupplierAdmin__create (poa, newservant, ev);
	
    newservant->consumer_admin = 
        impl_CosEventChannelAdmin_ConsumerAdmin__create (poa, newservant, ev);

    newservant->push_supplier = NULL;
    newservant->pull_supplier = NULL;
    newservant->push_consumer = NULL;
    newservant->pull_consumer = NULL;
	
    POA_CosEventChannelAdmin_EventChannel__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	
    num_channels++;

    return retval;
}

static void
impl_CosEventChannelAdmin_EventChannel__destroy(impl_POA_CosEventChannelAdmin_EventChannel * servant, CORBA_Environment * ev)
{
    PortableServer_ObjectId *objid;
    impl_POA_CosEventChannelAdmin_SupplierAdmin *supplier_admin_servant;
    impl_POA_CosEventChannelAdmin_ConsumerAdmin *consumer_admin_servant;

    while (servant->push_supplier) {
        impl_CosEventChannelAdmin_ProxyPushSupplier_disconnect_push_supplier ((impl_POA_CosEventChannelAdmin_ProxyPushSupplier*)servant->push_supplier->data, ev);
    }
	
    while (servant->pull_supplier) {
        impl_CosEventChannelAdmin_ProxyPullSupplier_disconnect_pull_supplier ((impl_POA_CosEventChannelAdmin_ProxyPullSupplier*)servant->pull_supplier->data, ev);
    }
    
    while (servant->push_consumer) {
        impl_CosEventChannelAdmin_ProxyPushConsumer_disconnect_push_consumer ((impl_POA_CosEventChannelAdmin_ProxyPushConsumer*)servant->push_consumer->data, ev);
    }
    while (servant->push_consumer) {
        impl_CosEventChannelAdmin_ProxyPullConsumer_disconnect_pull_consumer ((impl_POA_CosEventChannelAdmin_ProxyPullConsumer*)servant->pull_consumer->data, ev);
    }

/* FIXME: Fix _reference_to_servant() to work without an active profile, then
 * take out this #if */
#if 0
    consumer_admin_servant = 
        PortableServer_POA_reference_to_servant (servant->poa,
                                                 servant->consumer_admin,
                                                 ev);
    impl_CosEventChannelAdmin_ConsumerAdmin__destroy (consumer_admin_servant,
                                                      ev);
    supplier_admin_servant 
        = PortableServer_POA_reference_to_servant (servant->poa,
                                                   servant->supplier_admin,
                                                   ev);
    impl_CosEventChannelAdmin_SupplierAdmin__destroy (supplier_admin_servant,
                                                      ev);
#endif     

    
    objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
    PortableServer_POA_deactivate_object(servant->poa, objid, ev);
    CORBA_free(objid);
	
    POA_CosEventChannelAdmin_EventChannel__fini((PortableServer_Servant) servant, ev);
    g_free(servant);

    /* Quit if this is the last channel being served */
    num_channels--;
    if (num_channels == 0) 
        gtk_main_quit ();
    
}


CosEventChannelAdmin_ConsumerAdmin
impl_CosEventChannelAdmin_EventChannel_for_consumers(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                                     CORBA_Environment * ev)
{
    return CORBA_Object_duplicate (servant->consumer_admin, ev);
}

CosEventChannelAdmin_SupplierAdmin
impl_CosEventChannelAdmin_EventChannel_for_suppliers(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                                     CORBA_Environment * ev)
{
    return CORBA_Object_duplicate (servant->supplier_admin, ev);
}

void
impl_CosEventChannelAdmin_EventChannel_destroy(impl_POA_CosEventChannelAdmin_EventChannel * servant,
                                               CORBA_Environment * ev)
{
    impl_CosEventChannelAdmin_EventChannel__destroy (servant, ev);
}

void
event_channel_notify (impl_POA_CosEventChannelAdmin_EventChannel *servant,
                      CORBA_any *data,
                      CORBA_Environment *ev)
{
    GList *supplier;

    supplier = servant->push_supplier;
    while (supplier) {
        push_supplier_notify ((impl_POA_CosEventChannelAdmin_ProxyPushSupplier*)supplier->data, data, ev);
        supplier = g_list_next (supplier);
    }
	
    supplier = servant->pull_supplier;
    while (supplier) {
        pull_supplier_notify ((impl_POA_CosEventChannelAdmin_ProxyPullSupplier*)supplier->data, data, ev);
        supplier = g_list_next (supplier);
    }
}

