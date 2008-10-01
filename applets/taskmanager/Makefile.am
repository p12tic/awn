# Applet shared lib
INCLUDES = \
	-I$(srcdir) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(AWN_CFLAGS) \
	$(APPLET_CFLAGS) \
	-DPREFIX=\"$(prefix)\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" 

TASKMANAGER_SOURCES= \
	applet.c

TASKMANANGER_LDADD = \
	$(top_builddir)/libawn/libawn.la
	$(AWN_LIBS) \
  $(APPLET_LIBS)

taskmanagerlibdir = $(libdir)/awn/applets/taskmanager
taskmanagerlib_LTLIBRARIES = taskmanager.la
taskmanager_la_SOURCES = $(TASKMANAGER_SOURCES)
taskmanager_la_LIBADD = $(TASKMANANGER_LDADD)
taskmanager_la_LDFLAGS = -module -avoid-version
taskmanager_la_CFLAGS = 

# Desktop file
desktopdir = $(pkgdatadir)/applets
desktop_DATA = \
	taskmanager.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@LIBDIR\@|$(libdir)|g" $< > $@

EXTRA_DIST = $(desktop_DATA).in.in
clean = $(desktop_DATA) $(desktop_DATA).in
