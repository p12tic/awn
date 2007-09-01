INCLUDES =							\
	-I$(srcdir)						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"		\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(AWN_CFLAGS)					\
	-DPREFIX=\"$(prefix)\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"				\
	-DDATADIR=\"$(datadir)\"				\
	-DLIBDIR=\"$(libdir)\"                                  \
	-I$(top_builddir)/libawn

NAREA_SOURCES =					\
	applet.c				\
	eggtraymanager.c			\
	eggtraymanager.h			\
	eggmarshalers.c				\
	eggmarshalers.h

NAREA_LDADD =                        \
        $(DEPS_LIBS)			        \
        $(AWN_LIBS)                             \
        $(top_builddir)/libawn/libawn.la
        

notification_arealibdir = $(libdir)/awn/applets/notification-area/
notification_arealib_LTLIBRARIES = notification-area.la
notification_area_la_SOURCES = $(NAREA_SOURCES)
notification_area_la_LIBADD = $(NAREA_LDADD)
notification_area_la_LDFLAGS = -module -avoid-version
notification_area_la_CFLAGS =

icondir = $(pkgdatadir)

icon_DATA =  \
        notification.svg

desktopdir = $(libdir)/awn/applets/

desktop_DATA = \
	notification-area.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@LIBDIR\@|$(libdir)|g;s|\@ICONDIR\@|$(icondir)|g" $< > $@

EXTRA_DIST = \
	$(icon_DATA) \
        $(desktop_DATA).in.in

CLEANFILES = $(desktop_DATA)
