ENUMFILES = awn-enum-types.c awn-enum-types.h
STAMPFILES = stamp-awn-marshal.h stamp-awn-enum-types.h
GLIB_MKENUMS=`pkg-config --variable=glib_mkenums glib-2.0`

BUILT_SOURCES = $(ENUMFILES)

lib_LTLIBRARIES = libawn.la

INCLUDES = \
	$(AWN_CFLAGS)			                \
	$(WARN_CFLAGS)                                  \
	$(DEPS_CFLAGS)					\
	-I$(top_srcdir) 				\
	-I$(top_builddir) 				\
	$(DEPS_CFLAGS)					\
	-DDATADIR=\""$(datadir)"\" 			\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 	        \
	-DLIBDIR=\""$(libdir)"\"                        \
	-shared						\
	$(NULL)	

libawnincdir = $(includedir)/libawn

source_h =	\
	awn-defines.h                                   \
	awn-applet.h                                    \
	awn-applet-dialog.h                             \
	awn-applet-gconf.h				\
	awn-applet-simple.h				\
	awn-cairo-utils.h				\
	awn-gconf.h					\
	awn-effects.h					\
	awn-enum-types.h				\
	awn-plug.h					\
	awn-title.h					

source_c =	\
	awn-applet.c                                    \
	awn-applet-dialog.c				\
	awn-applet-gconf.c                              \
	awn-applet-simple.c				\
	awn-cairo-utils.c				\
	awn-gconf.c					\
	awn-effects.c					\
	awn-enum-types.c				\
	awn-plug.c					\
	awn-title.c

libawninc_HEADERS = \
	$(source_h)	\
	awn-enum-types.h.in
libawn_la_SOURCES = \
	$(source_c)	\
	awn-enum-types.c.in
libawn_la_LIBADD = $(DEPS_LIBS) $(AWN_LIBS)
libawn_la_LDFLAGS = -version-info 0:1:0

awn-enum-types.h: stamp-awn-enum-types.h
	@true
stamp-awn-enum-types.h: $(source_h) Makefile
	( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/awn-enum-types.h.in \
	  $(source_h) ) >> xgen-ceth && \
	(cmp xgen-ceth awn-enum-types.h || cp xgen-ceth awn-enum-types.h) && \
	rm -f xgen-ceth && \
	echo timestamp > $(@F)

awn-enum-types.c: awn-enum-types.h
	( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/awn-enum-types.c.in \
	  $(source_h) ) >> xgen-cetc && \
	cp xgen-cetc awn-enum-types.c && \
	rm -f xgen-cetc

CLEANFILES = $(STAMPFILES)
