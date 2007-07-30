
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
	-shared																					\
	$(NULL)	

libawnincdir = $(includedir)/libawn

libawninc_HEADERS =	\
	awn-defines.h                                   \
	awn-applet.h                                    \
	awn-applet-gconf.h                              \
	awn-applet-dialog.h								\
	awn-cairo-utils.h								

libawn_la_SOURCES =	\
	awn-applet.c                                    \
	awn-applet-gconf.c                              \
	awn-applet-dialog.c								\
	awn-cairo-utils.c								

libawn_la_LIBADD = $(DEPS_LIBS) $(AWN_LIBS)
libawn_la_LDFLAGS = -version-info 0:1:0

