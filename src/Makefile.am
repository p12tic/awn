NULL = 
INCLUDES =				\
	$(AWN_CFLAGS)			\
	$(DOCK_CFLAGS)			\
	-DDATADIR=\""$(datadir)"\"	\
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)/libawn		\
	$(NULL)

if BUILD_EXPERIMENTAL
experimental_programs = awn
endif

bin_PROGRAMS = avant-window-navigator $(experimental_programs)

avant_window_navigator_LDADD =			\
	$(DOCK_LIBS)				\
	$(top_builddir)/libawn/libawn.la	\
	$(NULL)

avant_window_navigator_SOURCES =	\
	main.c				\
	awn-applet-manager.h            \
	awn-applet-manager.c            \
	awn-applet-manager-glue.h       \
	awn-applet-proxy.c              \
	awn-applet-proxy.h              \
	awn-bar.h			\
	awn-bar.c   			\
	awn-dbus-glue.h			\
	awn-hotspot.h			\
	awn-hotspot.c			\
	awn-marshallers.h		\
	awn-marshallers.c		\
	awn-task.h			\
	awn-task.c			\
	awn-task-manager.h		\
	awn-task-manager.c		\
	awn-window.h			\
	awn-window.c	 		\
	awn-x.h				\
	awn-x.c				\
	awn-utils.h			\
	awn-utils.c			\
	inlinepixbufs.h			\
	xutils.h			\
	xutils.c			\
	$(NULL)

icon_test_SOURCES =	\
	icon-test.c	\
	xutils.h	\
	xutils.c	\
	$(NULL)

icon_test_LDADD =	\
	$(DOCK_LIBS)	\
	$(NULL)

awn-dbus-glue.h: awn-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_task_manager --mode=glib-server --output=$@ $<

awn-applet-manager-glue.h: awn-applet-manager-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_applet_manager --mode=glib-server --output=$@ $<

awn-marshallers.c: awn-marshallers.list
	$(GLIB_GENMARSHAL) --prefix _awn_marshal --body $< > $@

awn-marshallers.h: awn-marshallers.list
	$(GLIB_GENMARSHAL) --prefix _awn_marshal --header $< > $@

BUILT_SOURCES =                         \
        awn-dbus-glue.h                 \
        awn-applet-manager-glue.h       \
        awn-marshallers.h               \
        awn-marshallers.c								\
				awn-app-glue.h									\
				$(NULL)

EXTRA_DIST =                            \
        awn-dbus.xml                    \
        awn-applet-manager-dbus.xml     \
        awn-marshallers.list						\
				awn-app-dbus.xml
	$(NULL)

DISTCLEANFILES = awn-applet-manager-glue.h awn-dbus-glue.h awn-app-glue.h

noinst_PROGRAMS =	\
	icon-test	\
	$(NULL)

awn_SOURCES = \
	awn-main.c \
	awn-app.c \
	awn-app.h \
	awn-defines.h \
	awn-monitor.c \
	awn-monitor.h \
	awn-panel.c \
	awn-panel.h \
	$(NULL)

awn_LDADD =			\
	$(DOCK_LIBS)				\
	$(top_builddir)/libawn/libawn.la	\
	$(NULL)

awn-app-glue.h: awn-app-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=awn_app --mode=glib-server --output=$@ $<


# vim: set ts=8 sts=8 sw=8 :
