SUBDIRS = idl gdl scintilla-control po intl

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
	gdlConf.sh.in	\
	MAINTAINERS	\
	AUTHORS		

CLEANFILES = gdlConf.sh 
