# Applet shared lib
INCLUDES = \
	-I$(srcdir) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(APPLET_CFLAGS) \
	-DPREFIX=\"$(prefix)\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE

TASKMANAGER_SOURCES= \
	applet.c \
	task-manager.c \
	task-manager.h 

TASKMANANGER_LDADD = \
	$(top_builddir)/libawn/libawn.la \
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
