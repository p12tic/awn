# Applet shared lib
INCLUDES = \
	-I$(srcdir) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(APPLET_CFLAGS) \
	-DPREFIX=\"$(prefix)\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	-Werror

TASKMANAGER_SOURCES= \
	applet.c \
	task-icon.c \
	task-icon.h \
	task-manager.c \
	task-manager.h \
	task-settings.c \
	task-settings.h \
	task-window.c \
	task-window.h \
	xutils.c \
	xutils.h

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


if USE_GCONF
schemasdir       = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = taskmanager.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

taskmanager.schemas.in: taskmanager.schema-ini
	$(top_srcdir)/data/awn-schema-to-gconf $< $@ "taskmanager"

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-gconf-schemas: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
else
install-gconf-schemas:
endif
else
install-gconf-schemas:
endif
schemadir = $(pkgdatadir)/schemas
schema_DATA = taskmanager.schema-ini

install-data-local: install-gconf-schemas

EXTRA_DIST = $(desktop_DATA).in.in $(schema_DATA)
CLEANFILES = $(desktop_DATA) $(desktop_DATA).in taskmanager.schemas taskmanager.schemas.in
