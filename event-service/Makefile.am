INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datatdir)/locals"\" \
	-I$(top_srcdir)/src/lib -I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = \
	gdf-event-server

gdf_event_server_SOURCES =					\
	gdf-event-server.c					\
	gdf-event-server.h					\
	generic-factory-impl.c					\
	generic-factory.h					\
	event-channel-impl.c					\
	event-channel.h

gdf_event_server_LDADD =					\
	../lib/libgdf.la
