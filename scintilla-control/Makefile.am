SUBDIRS=scintilla

INCLUDES = \
	-DPLAT_GTK=1 \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) $(GNOME_INCLUDEDIR) -DGDL_DATADIR=\""$(datadir)"\" \
	$(GNOME_CFLAGS) \
	$(BONOBO_CFLAGS) \
	$(BONOBOX_CFLAGS) \
	$(VFS_CFLAGS) \
	$(OAF_CFLAGS)

uidir = $(datadir)/gnome/ui
ui_DATA = scintilla-ui.xml

lib_LTLIBRARIES = libscintilla-control.la
noinst_PROGRAMS = test-scintilla

test_scintilla_SOURCES = test.c
libscintilla_control_la_SOURCES = \
	scintilla-control.c \
	scintilla-highlight.c \
	scintilla-highlight.h \
	scintilla-persist-file.c \
	scintilla-persist-file.h \
	scintilla-persist-stream.c \
	scintilla-persist-stream.h \
	scintilla-editor-buffer.c \
	scintilla-editor-buffer.h \
	scintilla-editor-gutter.c \
	scintilla-editor-gutter.h \
	scintilla-find.c \
	scintilla-find.h

LEXER_OBJS = \
	scintilla/LexCPP.o     \
	scintilla/LexLua.o     \
	scintilla/LexPerl.o    \
	scintilla/LexPython.o  \
	scintilla/LexSQL.o     \
	scintilla/LexHTML.o    \
	scintilla/LexOthers.o  \
	scintilla/LexVB.o

libscintilla_control_la_LIBADD = \
	$(GNOME_LIBS)     \
	$(GNOMEUI_LIBS)   \
	$(BONOBO_LIBS)    \
	$(BONOBOX_LIBS)   \
	$(VFS_LIBS)       \
	scintilla/libscintilla-lexers.la \
	scintilla/libscintilla-widget.la

test_scintilla_LDADD =  \
	$(GNOME_LIBS)   \
	$(GNOMEUI_LIBS) \
        $(BONOBO_LIBS)  \
	$(BONOBOX_LIBS) \
	../gdl/libgdl.la

oafdir=$(datadir)/oaf
oaf_in_files=Bonobo_Control_Scintilla.oaf.in
oaf_DATA = $(oaf_in_files:.oaf.in=.oaf)

@XML_I18N_MERGE_OAF_RULE@

EXTRA_DIST=$(oaf_in_files) $(oaf_DATA) $(ui_DATA)