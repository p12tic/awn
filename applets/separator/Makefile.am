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

desktopdir = $(libdir)/awn/applets

desktop_DATA = \
	separator.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@LIBDIR\@|$(libdir)|g" $< > $@


EXTRA_DIST = \
	separator.desktop.in.in
