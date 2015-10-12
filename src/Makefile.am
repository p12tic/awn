include $(top_srcdir)/Makefile.shave
MARSHAL_PREFIX = awn
include $(top_srcdir)/Makefile.marshal

AM_CFLAGS = $(WARNING_FLAGS)
AM_CXXFLAGS = $(WARNING_FLAGS) -fpermissive -std=c++11
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
	awn-app.c \
	awn-app.h \
	awn-main.cc \
	awn-applet-manager.cc \
	awn-applet-manager.h \
	awn-applet-proxy.cc \
	awn-applet-proxy.h \
	awn-background.cc \
	awn-background.h \
	awn-background-null.cc \
	awn-background-null.h \
	awn-background-flat.cc \
	awn-background-flat.h \
	awn-background-3d.cc \
	awn-background-3d.h \
	awn-background-curves.cc \
	awn-background-curves.h \
	awn-background-edgy.cc \
	awn-background-edgy.h \
	awn-background-floaty.cc \
	awn-background-floaty.h \
	awn-background-lucido.cc \
	awn-background-lucido.h \
	awn-defines.h \
	$(builddir)/awn-marshal.c \
	$(builddir)/awn-marshal.h \
	awn-monitor.cc \
	awn-monitor.h \
	awn-panel-dispatcher.cc \
	awn-panel-dispatcher.h \
	awn-panel.cc \
	awn-panel.h \
	awn-separator.cc \
	awn-separator.h \
	awn-throbber.cc \
	awn-throbber.h \
	awn-ua-alignment.cc \
	awn-ua-alignment.h \
	awn-x.h \
	awn-x.cc \
	inlinepixbufs.h \
	xutils.h \
	xutils.cc \
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
	awn-panel.vapi  \
	$(VALA_GENERATED_FILES) \
	$(NULL)

# vim: set ts=8 sts=8 sw=8 :
