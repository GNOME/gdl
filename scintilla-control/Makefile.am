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

oafdir = $(datadir)/oaf
oaf_DATA = scintilla.oafinfo

uidir = $(datadir)/gnome/ui
ui_DATA = scintilla-ui.xml

EXTRA_DIST = $(oaf_DATA) $(ui_DATA)

bin_PROGRAMS = scintilla-control
noinst_PROGRAMS = test-scintilla

test_scintilla_SOURCES = test.c

scintilla_control_SOURCES = \
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
	scintilla-editor-gutter.h

LEXER_OBJS = \
	scintilla/LexCPP.o     \
	scintilla/LexLua.o     \
	scintilla/LexPerl.o    \
	scintilla/LexPython.o  \
	scintilla/LexSQL.o     \
	scintilla/LexHTML.o    \
	scintilla/LexOthers.o  \
	scintilla/LexVB.o

scintilla_control_LDADD = \
	$(GNOME_LIBS)     \
	$(GNOMEUI_LIBS)   \
	$(BONOBO_LIBS)    \
	$(BONOBOX_LIBS)   \
	$(VFS_LIBS)       \
	$(LEXER_OBJS)     \
	../gdl/libgdl.la  \
        scintilla/libscintilla-widget.a 

test_scintilla_LDADD =  \
	$(GNOME_LIBS)   \
	$(GNOMEUI_LIBS) \
        $(BONOBO_LIBS)  \
	$(BONOBOX_LIBS) \
	../gdl/libgdl.la
