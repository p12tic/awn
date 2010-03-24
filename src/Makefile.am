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

bin_PROGRAMS = avant-window-navigator

avant_window_navigator_LDADD =			\
	$(DOCK_LIBS)				\
	$(top_builddir)/libawn/libawn.la	\
	$(NULL)

avant_window_navigator_SOURCES =	\
	awn-main.c \
	awn-app.c \
	awn-app.h \
	awn-app-glue.h \
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
	awn-panel-glue.h \
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
	$(NULL)

# DBus glue
awn-app-glue.h: awn-app-dbus.xml Makefile
	$(QUIET_GEN)$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_app --mode=glib-server --output=$@ $<

awn-panel-glue.h: awn-panel-dbus.xml Makefile
	$(QUIET_GEN)$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_panel --mode=glib-server --output=$@ $<

BUILT_SOURCES = \
	awn-app-glue.h \
	awn-panel-glue.h \
	$(MARSHALFILES) \
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	awn-app-dbus.xml \
	awn-panel-dbus.xml \
	awn-marshal.list \
	$(NULL)

# vim: set ts=8 sts=8 sw=8 :
