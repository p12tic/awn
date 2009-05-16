SUBDIRS = egg anims

MARSHAL_PREFIX = libawn
include $(top_srcdir)/Makefile.marshal

ENUMFILES = awn-enum-types.c awn-enum-types.h
GIRFILES = Awn-0.4.gir
STAMPFILES = stamp-awn-enum-types.h

GLIB_MKENUMS=`pkg-config --variable=glib_mkenums glib-2.0`

BUILT_SOURCES = $(ENUMFILES) \
	$(GIRFILES) \
	$(typelibs_DATA) \
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
	libawn.h
	$(NULL)

private_headers = \
	awn-effects-ops-new.h \
	awn-effects-ops-helpers.h \
	gseal-transition.h \
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
libawn_la_CFLAGS = $(WARNING_FLAGS)
libawn_la_LIBADD = $(DEPS_LIBS) $(AWN_LIBS)
libawn_la_LDFLAGS = -version-info 0:1:0

libawn_la_LIBADD += \
	$(top_builddir)/libawn/egg/libeggdesktopfile.la \
	$(top_builddir)/libawn/anims/libawn-effects-anims.la \
	$(NULL)

if HAVE_INTROSPECTION
# INTROSPECTION_GIRDIR/INTROSPECTION_TYPELIBDIR aren't the right place to install
# thing - we need to install inside our prefix.
girdir = $(datadir)/gir-1.0
gir_DATA = $(GIRFILES)

Awn-0.4.gir: $(INTROSPECTION_SCANNER) libawn.la
	$(Q)$(INTROSPECTION_SCANNER) \
	-I $(top_srcdir) \
	--include=GObject-2.0 \
	--include=Gio-2.0 \
	--include=GdkPixbuf-2.0 \
	--include=Atk-1.0 \
	--include=cairo-1.0 \
	--include=Pango-1.0 \
	--include=Gdk-2.0 \
	--include=Gtk-2.0 \
	--include=DBus-1.0 \
	--include=DesktopAgnostic-1.0 \
	--library=awn \
	--namespace=Awn --nsversion=0.4 \
	--pkg gobject-2.0 \
	--pkg gio-2.0 \
	--pkg gdk-pixbuf-2.0 \
	--pkg gdk-2.0 \
	--pkg gtk+-2.0 \
	--pkg dbus-glib-1 \
	--pkg desktop-agnostic \
	--c-include=libawn/libawn.h \
	$(source_c) $(public_headers) \
	--output $@
	$(SED) -i -e 's:\(Atk\.Implementor\)Iface:\1:g' $@

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(GIRFILES:.gir=.typelib)

%.typelib: %.gir $(INTROSPECTION_COMPILER)
	$(QUIET_GEN)$(INTROSPECTION_COMPILER) --includedir=$(srcdir) --includedir=. $(INTROSPECTION_COMPILER_OPTS) $< -o $(@F)
endif

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

CLEANFILES = $(STAMPFILES) $(BUILT_SOURCES)

EXTRA_DIST=awn-config-client-shared.c

# vim: set ts=8 sts=8 sw=8 :
