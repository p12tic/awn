
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

libawninc_HEADERS =	\
	awn-defines.h                                   \
	awn-applet.h                                    \
	awn-applet-dialog.h                             \
	awn-applet-gconf.h				\
	awn-applet-simple.h				\
	awn-cairo-utils.h				\
	awn-gconf.h					\
	awn-title.h					\
	awn-effects.h

libawn_la_SOURCES =	\
	awn-applet.c                                    \
	awn-applet-dialog.c				\
	awn-applet-gconf.c                              \
	awn-applet-simple.c				\
	awn-cairo-utils.c				\
	awn-gconf.c					\
	awn-title.c					\
	awn-effects.c

libawn_la_LIBADD = $(DEPS_LIBS) $(AWN_LIBS)
libawn_la_LDFLAGS = -version-info 0:1:0

