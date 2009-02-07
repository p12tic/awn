SUBDIRS = egg

NULL = 

ENUMFILES = awn-enum-types.c awn-enum-types.h
STAMPFILES = stamp-awn-marshal.h stamp-awn-enum-types.h
GLIB_MKENUMS=`pkg-config --variable=glib_mkenums glib-2.0`

BUILT_SOURCES = $(ENUMFILES)

lib_LTLIBRARIES = libawn.la

INCLUDES = \
	$(AWN_CFLAGS)			\
	-I$(top_srcdir) 		\
	-I$(top_builddir) 		\
	-DDATADIR=\""$(datadir)"\" 	\
	$(NULL)	

libawnincdir = $(includedir)/libawn

public_headers =			\
	awn-defines.h		\
	awn-applet.h		\
	awn-applet-dialog.h	\
	awn-applet-simple.h	\
	awn-cairo-utils.h	\
	awn-config-client.h	\
	awn-desktop-item.h	\
	awn-effects.h		\
	awn-enum-types.h	\
	awn-icons.h		\
	awn-plug.h		\
	awn-settings.h		\
	awn-title.h		\
	awn-vfs.h		\
	$(NULL)

private_headers = \
	awn-effect-bounce.h		\
	awn-effect-desaturate.h		\
	awn-effect-fade.h		\
	awn-effect-glow.h		\
	awn-effect-spotlight.h		\
	awn-effect-spotlight3d.h	\
	awn-effect-squish.h		\
	awn-effect-turn.h		\
	awn-effect-zoom.h		\
	awn-effects-ops.h		\
	awn-effects-shared.h		\
	$(NULL)

source_h = $(public_headers) $(private_headers)

source_c =				\
	awn-applet.c			\
	awn-applet-dialog.c		\
	awn-applet-simple.c		\
	awn-cairo-utils.c		\
	awn-effect-bounce.c		\
	awn-effect-desaturate.c		\
	awn-effect-fade.c		\
	awn-effect-glow.c		\
	awn-effect-spotlight.c		\
	awn-effect-spotlight3d.c	\
	awn-effect-squish.c		\
	awn-effect-turn.c		\
	awn-effect-zoom.c		\
	awn-effects.c			\
	awn-effects-ops.c		\
	awn-effects-shared.c		\
	awn-enum-types.c		\
	awn-icons.c			\
	awn-plug.c			\
	awn-settings.c			\
	awn-title.c			\
	awn-vfs.c			\
	$(NULL)

if USE_GNOME
source_c += awn-desktop-item-gnome.c
else
source_c += awn-desktop-item-egg.c
endif

if USE_GCONF
source_c += awn-config-client-gconf.c
else
source_c += awn-config-client-gkeyfile.c
endif

libawninc_HEADERS =		\
	$(public_headers)	\
	$(NULL)
libawn_la_SOURCES =		\
	$(private_headers)	\
	$(source_c)		\
	$(NULL)
libawn_la_LIBADD = $(DEPS_LIBS) $(AWN_LIBS)
libawn_la_LDFLAGS = -version-info 0:1:0

if USE_EGG
libawn_la_LIBADD +=					\
	$(top_builddir)/libawn/egg/libeggdesktopfile.la \
	$(NULL)
endif
if USE_EGG_CHECKSUM
libawn_la_LIBADD +=					\
	$(top_builddir)/libawn/egg/libeggchecksum.la	\
	$(NULL)
endif

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

EXTRA_DIST =				\
	awn-config-client-shared.c	\
	awn-enum-types.c.in		\
	awn-enum-types.h.in		\
	$(NULL)

DISTCLEANFILES = awn-enum-types.h awn-enum-types.c

# vim: set ts=8 sts=8 sw=8 :
