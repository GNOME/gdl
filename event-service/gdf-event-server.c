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

#include <gnome.h>
#include <liboaf/liboaf.h>

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
	CORBA_Environment ev;
	CosEventChannelAdmin_EventChannelFactory factory;
	PortableServer_POA poa;
	PortableServer_POAManager pm;
	struct sigaction act;
	sigset_t         empty_mask;
    OAF_RegistrationResult result;

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

#if 0 /* goad stuff */
	goad_register_arguments ();

	orb = gnome_CORBA_init ("gdf-event-server", VERSION, &argc, argv,
                            GNORBA_INIT_SERVER_FUNC, &ev);
#endif
    gnome_init_with_popt_table ("gdf-event-server", VERSION, argc, argv, 
                                oaf_popt_options, 0, NULL);
    oaf_init (argc, argv);

	poa = (PortableServer_POA)
	    CORBA_ORB_resolve_initial_references (oaf_orb_get (), 
                                              "RootPOA", &ev);	

	factory = impl_GNOME_ObjectFactory__create (poa, &ev);

#if 0 /* goad stuff */
	nameserver = gnome_name_service_get ();
	goad_server_register (nameserver, factory, 
                          "gdf_event_channel_factory", 
                          "object",
                          &ev);
#endif
    result = oaf_active_server_register ("OAFIID:gdf_event_channel_factory:b0007e1e-e684-4294-9ac5-ae9454e413f0", 
                                         factory);
    switch (result) {
    case OAF_REG_SUCCESS :
        break;
    case OAF_REG_NOT_LISTED:
        g_error ("Cannot register the commander because it is not listed.");
        return 1;
    case OAF_REG_ALREADY_ACTIVE:
        g_error ("Cannot register the commander because it is already active.");        return 1;
    case OAF_REG_ERROR :
    default :
        g_error ("Cannot register the commander because of an unknown error.");
        return 1;
    }

	pm = PortableServer_POA__get_the_POAManager (poa, &ev);
	PortableServer_POAManager_activate (pm, &ev);	

	/*CORBA_ORB_run (orb, &ev);*/
	gtk_main ();

	CORBA_exception_free (&ev);

	return 0;
}
