INCLUDES =				\
	$(AWN_CFLAGS)			\
	$(DOCK_CFLAGS)			\
	-DLIBDIR=\""$(libdir)"\"	\
	-I$(top_srcdir)/libawn		\
	-Werror \
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
	awn-config-bridge.c \
	awn-config-bridge.h \
	awn-defines.h \
	awn-monitor.c \
	awn-monitor.h \
	awn-panel.c \
	awn-panel.h \
	awn-panel-glue.h \
	awn-x.h \
	awn-x.c \
	inlinepixbufs.h \
	xutils.h \
	xutils.c \
	$(NULL)

# DBus glue
awn-app-glue.h: awn-app-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_app --mode=glib-server --output=$@ $<

awn-panel-glue.h: awn-panel-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_panel --mode=glib-server --output=$@ $<

BUILT_SOURCES = \
	awn-app-glue.h \
	awn-panel-glue.h \
	$(NULL)

EXTRA_DIST = \
	awn-app-dbus.xml \
	awn-panel-dbus.xml \
	$(NULL)

# vim: set ts=8 sts=8 sw=8 :
