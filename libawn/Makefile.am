SUBDIRS = egg anims

# shave-related
V         = @
Q         = $(V:1=)
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

NULL = 

ENUMFILES = awn-enum-types.c awn-enum-types.h
MARSHALFILES = libawn-marshal.c libawn-marshal.h
STAMPFILES = \
	stamp-awn-marshal.h \
	stamp-awn-enum-types.h \
	stamp-libawnmarshal.c \
	stamp-libawnmarshal.h \
	$(NULL)

GLIB_MKENUMS=`pkg-config --variable=glib_mkenums glib-2.0`

BUILT_SOURCES = $(ENUMFILES) $(MARSHALFILES)

lib_LTLIBRARIES = libawn.la

AM_CFLAGS = $(WARNING_FLAGS) $(DISABLE_DEPRECATED_FLAGS)
INCLUDES = \
	$(AWN_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DDATADIR=\""$(datadir)"\" \
	-DPKGDATADIR=\""$(pkgdatadir)"\"\
	$(NULL)	

libawnincdir = $(includedir)/libawn

public_headers = \
	awn-defines.h \
	awn-alignment.h \
	awn-applet.h \
	awn-applet-simple.h \
	awn-cairo-utils.h \
	awn-config-bridge.h \
	awn-config-client.h \
	awn-desktop-item.h \
	awn-dialog.h \
	awn-effects.h \
	awn-icon.h \
	awn-icon-box.h \
	awn-themed-icon.h \
	awn-tooltip.h \
	awn-utils.h \
	awn-vfs.h \
	libawn.h
	$(NULL)

private_headers = \
	awn-effects-ops-new.h \
	awn-effects-ops-helpers.h \
	$(NULL)

source_h = $(public_headers) $(private_headers)

source_c = \
	awn-alignment.c \
	awn-applet.c \
	awn-applet-simple.c \
	awn-cairo-utils.c \
	awn-config-bridge.c \
	awn-dialog.c \
	awn-effects.c \
	awn-effects-ops-new.c \
	awn-effects-ops-helpers.c \
	awn-icon.c \
	awn-icon-box.c \
	awn-themed-icon.c \
	awn-tooltip.c \
	awn-utils.c \
	awn-vfs.c \
	$(NULL)

source_c += awn-desktop-item-egg.c

if USE_GCONF
source_c += awn-config-client-gconf.c
else
source_c += awn-config-client-gkeyfile.c
endif

libawninc_HEADERS = \
	$(public_headers) \
	$(NULL)
libawn_la_SOURCES = \
	$(private_headers) \
	$(BUILT_SOURCES) \
	$(source_c) \
	$(NULL)
libawn_la_LIBADD = $(DEPS_LIBS) $(AWN_LIBS)
libawn_la_LDFLAGS = -version-info 0:1:0

libawn_la_LIBADD += \
	$(top_builddir)/libawn/egg/libeggdesktopfile.la \
	$(top_builddir)/libawn/anims/libawn-effects-anims.la \
	$(NULL)

# glib-genmarshal (from WebKit)
LIBAWN_MARSHAL = $(builddir)/libawn-marshal
LIBAWN_MARSHAL_LIST = $(srcdir)/libawnmarshal.list
LIBAWN_GENMARSHAL = \
	$(GLIB_GENMARSHAL) --prefix=libawn_marshal $(LIBAWN_MARSHAL_LIST)

$(LIBAWN_MARSHAL).c: stamp-libawnmarshal.c
	@true

$(LIBAWN_MARSHAL).h: stamp-libawnmarshal.h
	@true

stamp-libawnmarshal.c: $(LIBAWN_MARSHAL_LIST) stamp-libawnmarshal.h
	$(QUIET_GEN)( \
		echo '#include "libawn-marshal.h"' > $(LIBAWN_MARSHAL).c && \
		$(LIBAWN_GENMARSHAL)  --body >> $(LIBAWN_MARSHAL).c && \
		echo timestamp > $(@F) \
	)

stamp-libawnmarshal.h: $(LIBAWN_MARSHAL_LIST)
	$(QUIET_GEN)( \
		$(LIBAWN_GENMARSHAL) --header > $(LIBAWN_MARSHAL).h && \
		echo timestamp > $(@F) \
	)

awn-enum-types.h: stamp-awn-enum-types.h
	@true
stamp-awn-enum-types.h: $(source_h) Makefile
	$(QUIET_GEN)( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/awn-enum-types.h.in \
	  $(source_h) ) >> xgen-ceth && \
	(cmp xgen-ceth awn-enum-types.h || cp xgen-ceth awn-enum-types.h) && \
	rm -f xgen-ceth && \
	echo timestamp > $(@F)

awn-enum-types.c: awn-enum-types.h
	$(QUIET_GEN)( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/awn-enum-types.c.in \
	  $(source_h) ) >> xgen-cetc && \
	cp xgen-cetc awn-enum-types.c && \
	rm -f xgen-cetc

CLEANFILES = $(STAMPFILES)

EXTRA_DIST=awn-config-client-shared.c

# vim: set ts=8 sts=8 sw=8 :
