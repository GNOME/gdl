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

#include "gdf-event-server.h"
#include "event-channel.h"
#include "generic-factory.h"

/*** Main Program ***/

static void
signal_handler(int signo)
{
	/* syslog(LOG_ERR,"Receveived signal %d\nshutting down.", signo); */
	exit(1);
}

int
main (int argc, char *argv[])
{
	CORBA_ORB orb;
	CORBA_Object nameserver;
	CORBA_Environment ev;
	CosEventChannelAdmin_EventChannelFactory factory;
	PortableServer_POA poa;
	PortableServer_POAManager pm;
	struct sigaction act;
	sigset_t         empty_mask;

	sigemptyset(&empty_mask);
	act.sa_handler = signal_handler;
	act.sa_mask    = empty_mask;
	act.sa_flags   = 0;
	
	sigaction(SIGINT,  &act, 0);
	sigaction(SIGHUP,  &act, 0);
	sigaction(SIGSEGV, &act, 0);
	sigaction(SIGABRT, &act, 0);
	
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &act, 0);


	CORBA_exception_init (&ev);

	goad_register_arguments ();

	orb = gnome_CORBA_init ("gdf-event-server", VERSION, &argc, argv,
                            GNORBA_INIT_SERVER_FUNC, &ev);

	poa = (PortableServer_POA)
	    CORBA_ORB_resolve_initial_references (orb, "RootPOA", &ev);	

	factory = impl_GNOME_GenericFactory__create (poa, &ev);
	
	nameserver = gnome_name_service_get ();
	goad_server_register (nameserver, factory, 
                          "gdf_event_channel_factory", 
                          "object",
                          &ev);

	pm = PortableServer_POA__get_the_POAManager (poa, &ev);
	PortableServer_POAManager_activate (pm, &ev);	

	//CORBA_ORB_run (orb, &ev);
	gtk_main ();

	CORBA_exception_free (&ev);

	return 0;
}
