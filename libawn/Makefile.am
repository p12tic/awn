include $(top_srcdir)/Makefile.shave
MARSHAL_PREFIX = libawn
include $(top_srcdir)/Makefile.marshal

ENUMFILES = awn-enum-types.cc awn-enum-types.h
STAMPFILES = stamp-awn-enum-types.h

GLIB_MKENUMS=`pkg-config --variable=glib_mkenums glib-2.0`

BUILT_SOURCES = $(ENUMFILES) \
	$(builddir)/libawn-marshal.c \
	$(builddir)/libawn-marshal.h \
	$(NULL)

lib_LTLIBRARIES = libawn.la

AM_CPPFLAGS = $(STANDARD_CPPFLAGS) \
	$(DISABLE_DEPRECATED_FLAGS) \
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
	awn-box.h \
	awn-cairo-utils.h \
	awn-config.h \
	awn-dbus-watcher.h \
	awn-desktop-lookup-client.h \
	awn-dialog.h \
	awn-effects.h \
	awn-icon.h \
	awn-icon-box.h \
	awn-pixbuf-cache.h \
	awn-image.h \
	awn-label.h \
	awn-overlay.h \
	awn-overlayable.h \
	awn-overlay-pixbuf.h \
	awn-overlay-pixbuf-file.h \
	awn-overlay-progress.h \
	awn-overlay-progress-circle.h \
	awn-overlay-themed-icon.h \
	awn-overlay-text.h \
	awn-overlay-throbber.h \
	awn-themed-icon.h \
	awn-tooltip.h \
	awn-utils.h \
	vala-utils.h \
	libawn.h
	$(NULL)

anims_headers = \
	anims/awn-effects-shared.h \
	anims/awn-effect-bounce.h \
	anims/awn-effect-desaturate.h \
	anims/awn-effect-fade.h \
	anims/awn-effect-glow.h \
	anims/awn-effect-simple.h \
	anims/awn-effect-spotlight.h \
	anims/awn-effect-spotlight3d.h \
	anims/awn-effect-squish.h \
	anims/awn-effect-turn.h \
	anims/awn-effect-zoom.h \
	$(NULL)

anims_source = \
	anims/awn-effects-shared.cc \
	anims/awn-effect-bounce.cc \
	anims/awn-effect-desaturate.cc \
	anims/awn-effect-fade.cc \
	anims/awn-effect-glow.cc \
	anims/awn-effect-simple.cc \
	anims/awn-effect-spotlight.cc \
	anims/awn-effect-spotlight3d.cc \
	anims/awn-effect-squish.cc \
	anims/awn-effect-turn.cc \
	anims/awn-effect-zoom.cc \
	$(NULL)

private_headers = \
	$(anims_headers) \
	awn-effects-ops-new.h \
	awn-effects-ops-helpers.h \
	gseal-transition.h \
	$(NULL)

source_h = $(public_headers) $(private_headers)

source_c = \
	$(anims_source) \
	awn-alignment.cc \
	awn-applet.cc \
	awn-applet-simple.cc \
	awn-box.cc \
	awn-cairo-utils.cc \
	awn-config.cc \
	awn-dbus-watcher.cc \
	awn-desktop-lookup-client.cc \
	awn-dialog.cc \
	awn-effects.cc \
	awn-effects-ops-new.cc \
	awn-effects-ops-helpers.cc \
	awn-icon.cc \
	awn-icon-box.cc \
	awn-image.cc \
	awn-label.cc \
	awn-overlay.cc \
	awn-overlayable.cc \
	awn-overlay-pixbuf.cc \
	awn-overlay-pixbuf-file.cc \
	awn-overlay-progress.cc \
	awn-overlay-progress-circle.cc \
	awn-overlay-themed-icon.cc \
	awn-overlay-text.cc \
	awn-overlay-throbber.cc \
	awn-pixbuf-cache.cc \
	awn-themed-icon.cc \
	awn-tooltip.cc \
	awn-utils.cc \
	vala-utils.cc \
	$(NULL)

libawninc_HEADERS = \
	$(public_headers) \
	$(NULL)
libawn_la_SOURCES = \
	$(private_headers) \
	$(BUILT_SOURCES) \
	$(source_c) \
	$(NULL)
libawn_la_CFLAGS = $(WARNING_FLAGS)
libawn_la_CXXFLAGS = $(WARNING_FLAGS) -fpermissive -std=c++11
libawn_la_LIBADD = $(DEPS_LIBS) $(AWN_LIBS)
libawn_la_LDFLAGS = -version-info 1:1:0

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

awn-enum-types.cc: awn-enum-types.h
	$(QUIET_GEN)( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/awn-enum-types.cc.in \
	  $(source_h) ) >> xgen-cetc && \
	cp xgen-cetc awn-enum-types.cc && \
	rm -f xgen-cetc

EXTRA_DIST =				\
	awn-enum-types.cc.in		\
	awn-enum-types.h.in		\
	libawn-marshal.list		\
	$(NULL)

CLEANFILES = $(STAMPFILES) $(BUILT_SOURCES)

# vim: set ts=8 sts=8 sw=8 :
