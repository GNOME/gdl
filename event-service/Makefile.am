INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datatdir)/locals"\" \
	-I$(top_srcdir)/src/lib -I$(includedir) $(GNOME_INCLUDEDIR)


lib_LTLIBRARIES=libgdf-event-server.la

libgdf_event_server_la_SOURCES =					\
	gdf-event-server.c					\
	gdf-event-server.h					\
	generic-factory-impl.c					\
	generic-factory.h					\
	event-channel-impl.c					\
	event-channel.h

#libgdf_event_server_la_LIBADD =					\
#	../lib/libgdf.la
