INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locales"\" \
	-I$(includedir) $(GNOME_INCLUDEDIR) $(BONOBO_CFLAGS)

lib_LTLIBRARIES = libgdl.la

CORBA_GENERATED_HEADER_FILES = GDL.h

CORBA_GENERATED = \
	$(CORBA_GENERATED_HEADER_FILES) \
	GDL-common.c \
	GDL-stubs.c \
	GDL-skels.c

BUILT_SOURCES=$(CORBA_GENERATED)

idl_dir = $(top_srcdir)/idl
IDL_FLAGS = -D__GDL_COMPILATION -I $(idl_dir) -I $(datadir)/dil $(IDL_CFLAGS)

$(CORBA_GENERATED) : $(idl_dir)/GDL.idl \
		     $(idl_dir)/editor-buffer.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(idl_dir)/GDL.idl

libgdl_la_SOURCES = \
	$(CORBA_GENERATED) \
	gdl-server-manager.c

libgdl_la_LIBADD = \
	$(BONOBO_LIBS) \
	$(GNOME_LIBS)

libgdlincludedir = $(includedir)/gdl
libgdlinclude_HEADERS = \
	$(CORBA_GENERATED_HEADER_FILES) \
	gdl.h \
	gdl-server-manager.h \
	gdl-tools.h