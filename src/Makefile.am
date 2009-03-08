# shave-related
V         = @
Q         = $(V:1=)
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = $(WARNING_FLAGS) $(DISABLE_DEPRECATED_FLAGS)
INCLUDES =				\
	$(AWN_CFLAGS)			\
	$(DOCK_CFLAGS)			\
	-DLIBDIR=\""$(libdir)"\"	\
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)/libawn		\
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
	awn-defines.h \
	$(builddir)/awn-marshal.c \
	$(builddir)/awn-marshal.h \
	awn-monitor.c \
	awn-monitor.h \
	awn-panel.c \
	awn-panel.h \
	awn-panel-glue.h \
	awn-throbber.c \
	awn-throbber.h \
	awn-x.h \
	awn-x.c \
	inlinepixbufs.h \
	xutils.h \
	xutils.c \
	$(NULL)

# glib-genmarshal (from WebKit)
AWN_MARSHAL = $(builddir)/awn-marshal
AWN_MARSHAL_LIST = $(srcdir)/awnmarshal.list
AWN_GENMARSHAL = $(GLIB_GENMARSHAL) --prefix=awn_marshal $(AWN_MARSHAL_LIST)

$(AWN_MARSHAL).c: stamp-awnmarshal.c
	@true

$(AWN_MARSHAL).h: stamp-awnmarshal.h
	@true

stamp-awnmarshal.c: $(AWN_MARSHAL_LIST) stamp-awnmarshal.h
	$(QUIET_GEN)( \
		echo '#include "awn-marshal.h"' > $(AWN_MARSHAL).c && \
		$(AWN_GENMARSHAL)  --body >> $(AWN_MARSHAL).c && \
		echo timestamp > $(@F) \
	)

stamp-awnmarshal.h: $(AWN_MARSHAL_LIST)
	$(QUIET_GEN)( \
		$(AWN_GENMARSHAL) --header > $(AWN_MARSHAL).h && \
		echo timestamp > $(@F) \
	)

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

DISTCLEANFILES = \
	stamp-awnmarshal.c \
	stamp-awnmarshal.h \
	$(NULL)

# vim: set ts=8 sts=8 sw=8 :
