## Process this file with automake to produce Makefile.in

SUBDIRS = tagmanager

INCLUDES = \
	-I$(top_srcdir) \
	-I$(srcdir)/tagmanager/include \
	-I$(top_srcdir)/gdl \
	-DGNOME_DATADIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(gnomelocaledir)"\" \
	-DG_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED \
	$(GNOME_INCLUDEDIR) \
	$(GDL_DEPENDENCIES_CFLAGS) \
	-DSYMBOL_BROWSER_SRCDIR=\""$(srcdir)"/\"

## Using exec binary for testing.

#lib_LTLIBRARIES = \
#	libgnome-symbol-browser.la
#
#libgnome_symbol_browser_la_SOURCES = \
#	symbol-browser.c\
#	symbol-browser.h\
#	symbol-browser-cobject.c\
#	symbol-browser-cobject.h\
#	factory.c
#
#libgnome_symbol_browser_la_LIBADD = \
#	$(GDL_DEPENDENCIES_LIBS) \
#	../gdl/libgdl-1.la

bin_PROGRAMS = gnome-symbol-browser

gnome_symbol_browser_SOURCES = \
	symbol-browser.c\
	symbol-browser.h\
	symbol-browser-cobject.c\
	symbol-browser-cobject.h\
	factory.c

gnome_symbol_browser_LDADD = \
	$(GDL_DEPENDENCIES_LIBS) \
	../gdl/libgdl-1.la \
	-L$(srcdir)/tagmanager\
	-lctags

noinst_PROGRAMS = test-symbol-browser

test_symbol_browser_SOURCES = \
	test-symbol-browser.c

test_symbol_browser_LDADD = \
	$(GDL_DEPENDENCIES_LIBS) \
	../gdl/libgdl-1.la 

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_Development_SymbolBrowser.server.in
server_DATA = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = gnome-symbol-browser.xml

EXTRA_DIST = 
	$(server_DATA) \
	$(ui_DATA)
