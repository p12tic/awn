# Applet shared lib
NULL =
AM_CFLAGS = $(WARNING_FLAGS)
AM_CPPFLAGS = \
	-I$(srcdir) \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(DISABLE_DEPRECATED_FLAGS) \
	$(NULL)

appletdir = $(APPLETDIR)/taskmanager
applet_LTLIBRARIES = taskmanager.la
taskmanager_la_SOURCES = \
	applet.c \
	task-drag-indicator.c \
	task-drag-indicator.h \
	task-icon.c \
	task-icon.h \
	task-launcher.c \
	task-launcher.h \
	task-manager.c \
	task-manager.h \
	task-settings.c \
	task-settings.h \
	task-window.c \
	task-window.h \
	xutils.c \
	xutils.h \
	$(NULL)
taskmanager_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(TASKMANAGER_LIBS) \
	$(NULL)
taskmanager_la_CFLAGS = $(TASKMANAGER_CFLAGS)
taskmanager_la_LDFLAGS = -module -avoid-version

# Desktop file
desktopdir = $(pkgdatadir)/applets
desktop_DATA = taskmanager.desktop
@INTLTOOL_DESKTOP_RULE@

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
CLEANFILES = $(desktop_DATA) $(shemas_in_files) $(schemas_DATA)
