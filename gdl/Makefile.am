INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locales"\" \
	-I$(includedir) $(GNOME_INCLUDEDIR) $(BONOBO_CFLAGS) $(GNOME_CFLAGS) \
	$(XML_CFLAGS)

lib_LTLIBRARIES = libgdl.la

CORBA_GENERATED_HEADER_FILES = GDL.h

CORBA_GENERATED = \
	$(CORBA_GENERATED_HEADER_FILES) \
	GDL-common.c \
	GDL-stubs.c \
	GDL-skels.c

BUILT_SOURCES=$(CORBA_GENERATED)

idl_dir = $(top_srcdir)/idl
IDL_FLAGS = -D__GDL_COMPILATION -I $(idl_dir) -I $(datadir)/idl $(IDL_CFLAGS)

$(CORBA_GENERATED) : $(idl_dir)/GDL.idl \
		     $(idl_dir)/editor-buffer.idl \
		     $(idl_dir)/editor-gutter.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(idl_dir)/GDL.idl

libgdl_la_SOURCES = \
	$(CORBA_GENERATED) \
	gdl-server-manager.c \
	gdl-dock.c \
	gdl-dock-item.c \
	gdl-dock-notebook.c \
	gdl-dock-paned.c \
	gdl-dock-tablabel.c

libgdl_la_LIBADD = \
	$(BONOBO_LIBS) \
	$(GNOME_LIBS) \
	$(XML_LIBS)

noinst_PROGRAMS=test-dock

test_dock_sources=test-dock.c
test_dock_LDADD=libgdl.la $(XML_LIBS)

libgdlincludedir = $(includedir)/gdl
libgdlinclude_HEADERS = \
	$(CORBA_GENERATED_HEADER_FILES) \
	gdl.h \
	gdl-server-manager.h \
	gdl-tools.h \
	gdl-dock.h \
	gdl-dock-item.h \
	gdl-dock-notebook.h \
	gdl-dock-paned.h \
	gdl-dock-tablabel.h

