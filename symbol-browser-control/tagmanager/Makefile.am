## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = -I$(srcdir)/include \
	-DDISABLE_REGEX \
	$(GLIB_CFLAGS) -Wall

noinst_LIBRARIES = libctags.a

noinst_HEADERS = debug.h general.h keyword.h options.h parsers.h sort.h vstring.h \
	args.h ctags.h entry.h get.h main.h parse.h read.h strlist.h

libctags_a_SOURCES = \
	tm_workspace.c\
	tm_work_object.c\
	tm_source_file.c\
	tm_symbol.c\
	tm_project.c\
	tm_tag.c\
	args.c\
	args.h\
	c.c\
	ctags.c\
	debug.c\
	entry.c\
	get.c\
	keyword.c\
	options.c\
	parse.c\
	read.c\
	sort.c\
	strlist.c\
	vstring.c

