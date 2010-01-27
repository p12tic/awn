include $(top_srcdir)/Makefile.shave
MARSHAL_PREFIX = awn
include $(top_srcdir)/Makefile.marshal

AM_CFLAGS = $(WARNING_FLAGS)
AM_CPPFLAGS =				\
	$(STANDARD_CPPFLAGS)		\
	$(DISABLE_DEPRECATED_FLAGS)	\
	$(AWN_CFLAGS)			\
	$(DOCK_CFLAGS)			\
	-DLIBDIR=\""$(libdir)"\"	\
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)			\
	-I$(builddir)			\
	$(NULL)

VALA_FILES = \
	awn-panel-dispatcher.vala \
	awn-app.vala \
	$(NULL)

VALA_GENERATED_FILES = \
	$(VALA_FILES:.vala=.c) \
	$(VALA_FILES:.vala=.h) \
	$(NULL)

VALA_PKGS = \
	--pkg dbus-glib-1 \
	--pkg gtk+-2.0 \
	--pkg desktop-agnostic-cfg \
	$(NULL)

VALA_VAPI_FILES = \
	$(top_builddir)/bindings/vala/awn.vapi \
	awn-panel.vapi \
	$(NULL)

.vala.c:
	$(VALAC) -C -H $(<:.vala=.h) $< $(VALA_VAPI_FILES) $(VALA_PKGS) && \
	$(SED) -i -r 's/^#include <(awn.+h)>$$/#include "\1"/' $(<:.vala=).[ch]

bin_PROGRAMS = avant-window-navigator

avant_window_navigator_LDADD =			\
	$(DOCK_LIBS)				\
	$(top_builddir)/libawn/libawn.la	\
	$(NULL)

avant_window_navigator_SOURCES =	\
	awn-main.c \
	awn-applet-manager.c \
	awn-applet-manager.h \
	awn-applet-proxy.c \
	awn-applet-proxy.h \
	awn-background.c \
	awn-background.h \
	awn-background-flat.c \
	awn-background-flat.h \
	awn-background-3d.c \
	awn-background-3d.h \
	awn-background-curves.c \
	awn-background-curves.h \
	awn-background-edgy.c \
	awn-background-edgy.h \
	awn-background-floaty.c \
	awn-background-floaty.h \
	awn-defines.h \
	$(builddir)/awn-marshal.c \
	$(builddir)/awn-marshal.h \
	awn-monitor.c \
	awn-monitor.h \
	awn-panel.c \
	awn-panel.h \
	awn-separator.c \
	awn-separator.h \
	awn-throbber.c \
	awn-throbber.h \
	awn-ua-alignment.c \
	awn-ua-alignment.h \
	awn-x.h \
	awn-x.c \
	inlinepixbufs.h \
	xutils.h \
	xutils.c \
	$(VALA_GENERATED_FILES) \
	$(NULL)

BUILT_SOURCES = \
	$(VALA_GENERATED_FILES) \
	$(MARSHALFILES) \
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	awn-marshal.list \
	$(NULL)

# vim: set ts=8 sts=8 sw=8 :
