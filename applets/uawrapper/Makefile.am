AM_CPPFLAGS = \
	$(STANDARD_CPPFLAGS) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	$(DISABLE_DEPRECATED_FLAGS) \
	$(NULL)

appletdir = $(APPLETDIR)/uawrapper
applet_LTLIBRARIES = uawrapper.la
uawrapper_la_SOURCES = \
	applet.c \
	awn-ua-wrapper.c \
	awn-ua-wrapper.h \
	$(NULL)
uawrapper_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(NULL)
uawrapper_la_CFLAGS = $(WARNING_FLAGS) $(TASKMANAGER_CFLAGS)
uawrapper_la_LDFLAGS = -module -avoid-version

# Desktop file
desktopdir = $(pkgdatadir)/applets
desktop_DATA = uawrapper.desktop
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_DATA).in.in
CLEANFILES = $(desktop_DATA)
