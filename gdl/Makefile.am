INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locales"\" 	\
	-DGDL_GLADEDIR=\""$(gladedir)"\"		\
	-DG_DISABLE_DEPRECATED 				\
	-DGTK_DISABLE_DEPRECATED 			\
	-DGDK_DISABLE_DEPRECATED 			\
	-I$(includedir) -I$(top_srcdir)			\
	$(WARN_CFLAGS)					\
        -DPREFIX=\""$(prefix)"\"                        \
        -DSYSCONFDIR=\""$(sysconfdir)"\"                \
        -DDATADIR=\""$(datadir)"\"                      \
        -DLIBDIR=\""$(libdir)"\"                        \
	$(GDL_DEPENDENCIES_CFLAGS) $(XML_CFLAGS)

lib_LTLIBRARIES = libgdl-1.la

CORBA_GENERATED_HEADER_FILES = GDL.h

CORBA_GENERATED = \
	$(CORBA_GENERATED_HEADER_FILES) \
	GDL-common.c \
	GDL-stubs.c \
	GDL-skels.c

BUILT_SOURCES=$(CORBA_GENERATED)

idl_dir = $(top_srcdir)/idl

IDL_FLAGS = -D__GDL_COMPILATION -I $(idl_dir) -I $(datadir)/idl \
	-I $(LIBBONOBO_IDL_DIR)  				\
	-I $(BONOBO_ACTIVATION_IDL_DIR)

$(CORBA_GENERATED): $(idl_dir)/GDL.idl \
		    $(idl_dir)/editor-buffer.idl \
		    $(idl_dir)/editor-gutter.idl \
	            $(idl_dir)/symbol-browser.idl 
	$(ORBIT_IDL) $(IDL_FLAGS) $(idl_dir)/GDL.idl

CLEANFILES = $(CORBA_GENERATED) 		\
	libgdltypebuiltins.c			\
	libgdltypebuiltins.h			\
	libgdlmarshal.c				\
	libgdlmarshal.h

libgdl_1_la_SOURCES = \
	$(CORBA_GENERATED) \
	gdl-dock.c \
	gdl-dock-item.c \
	gdl-dock-notebook.c \
	gdl-dock-paned.c \
	gdl-dock-tablabel.c \
	gdl-dock-layout.c \
	gdl-file-selector-util.c \
	libgdltypebuiltins.c \
	libgdlmarshal.c

$(libgdl_1_la_OBJECTS): \
	libgdltypebuiltins.h \
	libgdlmarshal.h

libgdl_1_la_LIBADD = \
	$(GDL_DEPENDENCIES_LIBS) \
	$(XML_LIBS)

noinst_PROGRAMS=test-dock test-dataview

test_dock_sources=test-dock.c
test_dock_LDADD=libgdl-1.la $(XML_LIBS)

libgdl_headers = \
	gdl.h \
	gdl-tools.h \
	gdl-dock.h \
	gdl-dock-item.h \
	gdl-dock-notebook.h \
	gdl-dock-paned.h \
<<<<<<< Makefile.am
	gdl-dock-tablabel.h 
=======
	gdl-dock-tablabel.h \
	gdl-dock-layout.h \
	gdl-file-selector-util.h
>>>>>>> 1.11

libgdlincludedir = $(includedir)/libgdl-1.0/gdl
libgdlinclude_HEADERS = \
	$(CORBA_GENERATED_HEADER_FILES) \
	$(libgdl_headers) \
	libgdltypebuiltins.h

gladedir = $(datadir)/gdl/glade
glade_DATA = layout.glade

# layout.glade: layout.glade1 $(LIBGLADE_CONVERT)
#	$(LIBGLADE_CONVERT) layout.glade1 > layout.glade.tmp && \
# 	mv layout.glade.tmp layout.glade

test_dataview_SOURCES = \
	gdl-data-view.h \
	gdl-data-view.c \
	gdl-data-frame.h \
	gdl-data-frame.c \
	gdl-data-model.h \
	gdl-data-model.c \
	gdl-data-model-test.h \
	gdl-data-model-test.c \
	gdl-data-row.h \
	gdl-data-row.c \
	test-dataview.c

test_dataview_LDADD = $(GDL_DEPENDENCIES_LIBS)

# enumerations code generation

libgdltypebuiltins.c: $(libgdl_headers)
	@GLIB_MKENUMS@ \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#include \"libgdltypebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(libgdl_headers) > xgen-gtbec \
	&& cp xgen-gtbec $(@F) \
	&& rm -f xgen-gtbec

libgdltypebuiltins.h: $(libgdl_headers)
	@GLIB_MKENUMS@ \
			--fhead "#ifndef __LIBGDLTYPEBUILTINS_H__\n" \
			--fhead "#define __LIBGDLTYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <gdl/gdl.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __LIBGDLTYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define GDL_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(libgdl_headers) > xgen-gth \
	&& cp xgen-gth $(@F) \
	&& rm -f xgen-gth

libgdlmarshal.h: libgdlmarshal.list
	@GLIB_GENMARSHAL@ \
		--prefix=gdl_marshal libgdlmarshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

libgdlmarshal.c: libgdlmarshal.h
	echo "#include \"libgdlmarshal.h\"" > xgen-gmc \
	&& @GLIB_GENMARSHAL@ \
		--prefix=gdl_marshal libgdlmarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc
