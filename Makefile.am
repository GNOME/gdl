SUBDIRS = gdl po

EXTRA_DIST = 			\
	MAINTAINERS		\
	AUTHORS			\
	gdl.spec.in		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdl-1.0.pc 

if HAVE_GNOME
pkgconfig_DATA += gdl-gnome-1.0.pc
endif

CLEANFILES = gdl-1.0.pc intltool-extract intltool-merge intltool-update


