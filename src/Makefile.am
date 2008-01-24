NULL = 
INCLUDES =				\
	$(AWN_CFLAGS)			\
	$(DOCK_CFLAGS)			\
	-DLIBDIR=\""$(libdir)"\"	\
	-I$(top_srcdir)/libawn		\
	$(NULL)

bin_PROGRAMS = avant-window-navigator

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
        awn-marshallers.c		\
	$(NULL)

EXTRA_DIST =                            \
        awn-dbus.xml                    \
        awn-applet-manager-dbus.xml     \
        awn-marshallers.list		\
	$(NULL)

noinst_PROGRAMS =	\
	icon-test	\
	$(NULL)
# vim: set ts=8 sts=8 sw=8 :
