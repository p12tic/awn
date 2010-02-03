AM_CPPFLAGS = \
	$(STANDARD_CPPFLAGS) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	$(DISABLE_DEPRECATED_FLAGS) \
	$(NULL)

appletdir = $(APPLETDIR)/separator
applet_LTLIBRARIES = separator.la
separator_la_SOURCES = \
	applet.c \
	$(NULL)
separator_la_LIBADD = \
	$(AWN_LIBS) \
	$(top_builddir)/libawn/libawn.la \
	$(NULL)
separator_la_CFLAGS = $(WARNING_FLAGS) $(AWN_CFLAGS)
separator_la_LDFLAGS = -module -avoid-version

# Desktop file
desktopdir = $(pkgdatadir)/applets
desktop_DATA = separator.desktop
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_DATA).in.in
CLEANFILES = $(desktop_DATA)
