SUBDIRS = idl gdl scintilla-control po intl

m4datadir = $(datadir)/aclocal
m4data_DATA = gdl.m4

confexecdir = $(libdir)
confexec_DATA = gdlConf.sh 

gdlConf.sh: $(top_srcdir)/gdlConf.sh.in Makefile
	sed -e 's?\@GDL_LIBDIR\@?$(GDL_LIBDIR)?g' \
	    -e 's?\@GDL_INCLUDEDIR\@?$(GDL_INCLUDEDIR)?g' \
	    -e 's?\@VERSION\@?$(VERSION)?g' \
	    -e 's?\@GDL_LIBS\@?$(GDL_LIBS)?g' \
	      < $(top_srcdir)/gdlConf.sh.in > gdlConf.tmp \
	  && mv gdlConf.tmp gdlConf.sh

EXTRA_DIST = 		\
	gdl.m4          \
	gdlConf.sh.in	\
	MAINTAINERS	\
	AUTHORS		\
	gdl.spec.in	\
	gdl.spec	\
	xml-i18n-extract.in \
	xml-i18n-merge.in \
	xml-i18n-update.in

CLEANFILES = gdlConf.sh 
