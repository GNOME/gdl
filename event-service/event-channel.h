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

#ifndef EVENT_CHANNEL_H
#define EVENT_CHANNEL_H 1

#include "gdf.h"

typedef struct {
    POA_CosEventChannelAdmin_EventChannel servant;
    PortableServer_POA poa;
  
    CosEventChannelAdmin_SupplierAdmin supplier_admin;
    CosEventChannelAdmin_ConsumerAdmin consumer_admin; 	
    GList *push_supplier; 
    GList *pull_supplier; 	
    GList *push_consumer;
    GList *pull_consumer; 
} impl_POA_CosEventChannelAdmin_EventChannel;

typedef struct {
    POA_CosEventChannelAdmin_ProxyPushConsumer servant;
    PortableServer_POA poa;
	
    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant;
    CosEventComm_PushSupplier supplier;
} impl_POA_CosEventChannelAdmin_ProxyPushConsumer;

typedef struct {
    POA_CosEventChannelAdmin_ProxyPushSupplier servant;
    PortableServer_POA poa;
	
    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant;
    CosEventComm_PushConsumer consumer;
} impl_POA_CosEventChannelAdmin_ProxyPushSupplier;

typedef struct {
    POA_CosEventChannelAdmin_ProxyPullSupplier servant;
    PortableServer_POA poa;
	
    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant;
    CosEventComm_PullConsumer consumer;
    GSList *event_queue_front;
    GSList *event_queue_back;
} impl_POA_CosEventChannelAdmin_ProxyPullSupplier;

typedef struct {
    POA_CosEventChannelAdmin_ProxyPullConsumer servant;
    PortableServer_POA poa;
	
    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant;
    CosEventComm_PullSupplier supplier;
} impl_POA_CosEventChannelAdmin_ProxyPullConsumer;

typedef struct {
    POA_CosEventChannelAdmin_ConsumerAdmin servant;
    PortableServer_POA poa;
	
    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant;
} impl_POA_CosEventChannelAdmin_ConsumerAdmin;

typedef struct {
    POA_CosEventChannelAdmin_SupplierAdmin servant;
    PortableServer_POA poa;
	
    impl_POA_CosEventChannelAdmin_EventChannel* channel_servant;
} impl_POA_CosEventChannelAdmin_SupplierAdmin;

extern POA_CosEventComm_PushConsumer__epv impl_CosEventComm_PushConsumer_epv;
extern POA_CosEventComm_PushSupplier__epv impl_CosEventComm_PushSupplier_epv;
extern POA_CosEventComm_PullSupplier__epv impl_CosEventComm_PullSupplier_epv;
extern POA_CosEventComm_PullConsumer__epv impl_CosEventComm_PullConsumer_epv;
extern PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPushConsumer_base_epv;
extern POA_CosEventChannelAdmin_ProxyPushConsumer__epv impl_CosEventChannelAdmin_ProxyPushConsumer_epv;
extern PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPullSupplier_base_epv;
extern POA_CosEventChannelAdmin_ProxyPullSupplier__epv impl_CosEventChannelAdmin_ProxyPullSupplier_epv;
extern PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPullConsumer_base_epv;
extern POA_CosEventChannelAdmin_ProxyPullConsumer__epv impl_CosEventChannelAdmin_ProxyPullConsumer_epv;
extern PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ProxyPushSupplier_base_epv;
extern POA_CosEventChannelAdmin_ProxyPushSupplier__epv impl_CosEventChannelAdmin_ProxyPushSupplier_epv;
extern PortableServer_ServantBase__epv impl_CosEventChannelAdmin_ConsumerAdmin_base_epv;
extern POA_CosEventChannelAdmin_ConsumerAdmin__epv impl_CosEventChannelAdmin_ConsumerAdmin_epv;
extern PortableServer_ServantBase__epv impl_CosEventChannelAdmin_SupplierAdmin_base_epv;
extern POA_CosEventChannelAdmin_SupplierAdmin__epv impl_CosEventChannelAdmin_SupplierAdmin_epv;
extern PortableServer_ServantBase__epv impl_CosEventChannelAdmin_EventChannel_base_epv;
extern POA_CosEventChannelAdmin_EventChannel__epv impl_CosEventChannelAdmin_EventChannel_epv;

extern CosEventChannelAdmin_EventChannel 
impl_CosEventChannelAdmin_EventChannel__create (PortableServer_POA,
                                                CORBA_Environment *);

#endif /* EVENT_CHANNEL_H */
