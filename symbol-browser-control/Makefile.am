## Process this file with automake to produce Makefile.in

SUBDIRS = tagmanager

INCLUDES = \
	-I$(top_srcdir) \
	-I$(srcdir)/tagmanager/include \
	-I$(top_srcdir)/gdl \
	-DGNOME_DATADIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(gnomelocaledir)"\" \
	$(GNOME_INCLUDEDIR) \
	$(OAF_CFLAGS) \
	$(BONOBO_CFLAGS) \
	$(BONOBOX_CFLAGS) \
	$(GAL_CFLAGS)

## Using exec binary for testing.

#lib_LTLIBRARIES = \
#	libgnome-symbol-browser.la
#
#libgnome_symbol_browser_la_SOURCES = \
#	symbol-browser-cobject.c\
#	symbol-browser-cobject.h\
#	factory.c
#
#libgnome_symbol_browser_la_LIBADD = \
#	$(GNOME_LIBS)     \
#	$(GNOMEUI_LIBS)   \
#	$(BONOBO_LIBS)    \
#	$(BONOBOX_LIBS)   \
#	$(VFS_LIBS)       \
#	$(XML_LIBS)       \
#	ccview/libccview.la

bin_PROGRAMS = gnome-symbol-browser

gnome_symbol_browser_SOURCES = \
	symbol-browser.c\
	symbol-browser.h\
	symbol-browser-cobject.c\
	symbol-browser-cobject.h\
	factory.c

gnome_symbol_browser_LDADD = \
        $(BONOBO_LIBS)  \
	$(BONOBOX_LIBS) \
	../gdl/libgdl.la \
	$(GAL_LIBS)\
	-L$(srcdir)/tagmanager\
	-lctags

noinst_PROGRAMS = test-symbol-browser

test_symbol_browser_SOURCES = \
	test-symbol-browser.c

test_symbol_browser_LDADD = \
	$(GNOME_LIBS) \
	$(BONOBO_LIBS) \
	$(BONOBOX_LIBS) \
	../gdl/libgdl.la 

oaf_in_files = \
	GNOME_Development_SymbolBrowser.oaf.in

oaf_DATA = $(oaf_in_files:.oaf.in=.oaf)
oafdir = $(datadir)/oaf

@XML_I18N_MERGE_OAF_RULE@

uidir = $(datadir)/gnome/ui
ui_DATA = gnome-symbol-browser.xml

EXTRA_DIST = $(oaf_in_files)\
	$(oaf_DATA)\
	$(ui_DATA) 
