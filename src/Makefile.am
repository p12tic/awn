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
	-I$(top_srcdir)			\
	-I$(builddir)			\
	$(NULL)

VALA_FILES = \
	awn-panel-dispatcher.vala \
	awn-app.vala \
	$(NULL)

VALA_FLAGS = \
	$(top_srcdir)/bindings/vala/awn.vapi \
	$(top_srcdir)/src/awn-panel.vapi \
	--pkg dbus-glib-1 \
	--vapidir=$(LDA_VAPIDIR) \
	--pkg gtk+-2.0 \
	--pkg desktop-agnostic-cfg \
	$(NULL)

awn-core.vala.stamp: $(VALA_FILES)
	$(foreach vala_file,$?,\
	$(VALAC) -C -H $(notdir $(vala_file:.vala=.h)) $(vala_file) $(VALA_FLAGS) && \
	$(SED) -i -r 's/^#include <(awn.+h)>$$/#include "\1"/' $(notdir $(vala_file:.vala=.))[ch] || exit 1;)
	touch "$@"

VALA_GENERATED_FILES = \
	$(VALA_FILES:.vala=.c) \
	$(VALA_FILES:.vala=.h) \
	$(NULL)

bin_PROGRAMS = avant-window-navigator

avant_window_navigator_LDADD =			\
	$(DOCK_LIBS)				\
	$(AWN_LIBS)				\
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
	awn-background-null.c \
	awn-background-null.h \
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
	awn-background-lucido.c \
	awn-background-lucido.h \
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
	awn-core.vala.stamp \
	$(MARSHALFILES) \
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(VALA_GENERATED_FILES) \
	$(NULL)

EXTRA_DIST = \
	awn-marshal.list \
	awn-app.vala \
	awn-panel-dispatcher.vala \
	awn-panel.vapi  \
	$(VALA_GENERATED_FILES) \
	$(NULL)

# vim: set ts=8 sts=8 sw=8 :
