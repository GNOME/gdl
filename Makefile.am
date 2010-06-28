SUBDIRS = gdl docs po

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

EXTRA_DIST = 			\
	MAINTAINERS		\
	AUTHORS			\
	gdl.spec.in		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	m4/introspection.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdl-3.0.pc 

CLEANFILES = gdl-3.0.pc intltool-extract intltool-merge intltool-update


