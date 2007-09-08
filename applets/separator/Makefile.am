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

SEPARATOR_SOURCES =                              \
	applet.c

SEPARATOR_LDADD =                        \
        $(DEPS_LIBS)			        \
        $(AWN_LIBS)                             \
        $(top_builddir)/libawn/libawn.la
        

separatorlibdir = $(libdir)/awn/applets/separator
separatorlib_LTLIBRARIES = separator.la
separator_la_SOURCES = $(SEPARATOR_SOURCES)
separator_la_LIBADD = $(SEPARATOR_LDADD)
separator_la_LDFLAGS = -module -avoid-version
separator_la_CFLAGS =

icondir = $(pkgdatadir)

icon_DATA =  \
        separator.svg

desktopdir = $(libdir)/awn/applets

desktop_DATA = \
	separator.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA): $(desktop_DATA).in
	sed -e "s|\@LIBDIR\@|$(libdir)|g;s|\@ICONDIR\@|$(icondir)|g" $< > $@


EXTRA_DIST = \
	$(icon_DATA) \
        $(desktop_DATA).in

CLEANFILES = $(desktop_DATA)
