AM_CPPFLAGS = \
	$(STANDARD_CPPFLAGS) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	$(DISABLE_DEPRECATED_FLAGS) \
	$(NULL)

appletdir = $(APPLETDIR)/expander
applet_LTLIBRARIES = expander.la
expander_la_SOURCES = \
	applet.c \
	$(NULL)
expander_la_LIBADD = \
	$(AWN_LIBS) \
	$(top_builddir)/libawn/libawn.la \
	$(NULL)
expander_la_CFLAGS = $(WARNING_FLAGS) $(AWN_CFLAGS)
expander_la_LDFLAGS = -module -avoid-version

# Desktop file
desktopdir = $(pkgdatadir)/applets
desktop_DATA = expander.desktop
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_DATA).in.in
CLEANFILES = $(desktop_DATA)
