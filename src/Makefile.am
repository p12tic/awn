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
	awn-background.c \
	awn-background.h \
	awn-background-flat.c \
	awn-background-flat.h \
	awn-defines.h \
	awn-monitor.c \
	awn-monitor.h \
	awn-panel.c \
	awn-panel.h \
	awn-x.h \
	awn-x.c \
	inlinepixbufs.h \
	xutils.h \
	xutils.c \
	$(NULL)

# DBus glue
awn-app-glue.h: awn-app-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_app --mode=glib-server --output=$@ $<


# vim: set ts=8 sts=8 sw=8 :
