if USE_EGG
desktopfile_lib = libeggdesktopfile.la
endif
if USE_EGG_CHECKSUM
checksum_lib = libeggchecksum.la
endif

noinst_LTLIBRARIES = $(checksum_lib) $(desktopfile_lib)

NULL = 
ENUMFILES = egg-enum-types.c egg-enum-types.h
STAMPFILES = stamp-egg-enum-types.h
GLIB_MKENUMS=`pkg-config --variable=glib_mkenums glib-2.0`

INCLUDES = $(AWN_CFLAGS)

source_h = eggdesktopfile.h

libeggdesktopfile_la_SOURCES =	\
	eggdesktopfile.c			\
	egg-enum-types.c			\
	egg-enum-types.h			\
	$(source_h)					\
	$(NULL)

libeggdesktopfile_la_LIBADD = $(AWN_LIBS)

libeggchecksum_la_SOURCES =	\
	eggchecksum.c			\
	eggchecksum.h

egg-enum-types.h: stamp-egg-enum-types.h
	@true
stamp-egg-enum-types.h: $(source_h) Makefile
	( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/egg-enum-types.h.in \
	  $(source_h) ) >> xgen-ceth && \
	(cmp xgen-ceth egg-enum-types.h || cp xgen-ceth egg-enum-types.h) && \
	rm -f xgen-ceth && \
	echo timestamp > $(@F)

egg-enum-types.c: egg-enum-types.h
	( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/$@.in \
	  $(source_h) ) >> xgen-cetc && \
	cp xgen-cetc $@ && \
	rm -f xgen-cetc

EXTRA_DIST= \
	egg-enum-types.h.in \
	egg-enum-types.c.in \
  eggdesktopfile.c \
	eggdesktopfile.h

CLEANFILES = $(STAMPFILES)

DISTCLEANFILES = egg-enum-types.h egg-enum-types.c
