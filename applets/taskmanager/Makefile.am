MARSHAL_PREFIX = taskmanager
include $(top_srcdir)/Makefile.marshal

AM_CPPFLAGS = \
	$(STANDARD_CPPFLAGS) \
	-I$(top_srcdir) \
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
	task-item.c \
	task-item.h \
	task-launcher.c \
	task-launcher.h \
	task-manager.c \
	task-manager.h \
	task-manager-glue.h \
	task-manager-api-wrapper.c \
	task-manager-api-wrapper.h \
	task-manager-api-wrapper-glue.h \
	task-settings.c \
	task-settings.h \
	task-window.c \
	task-window.h \
	$(builddir)/taskmanager-marshal.c \
	$(builddir)/taskmanager-marshal.h \
	xutils.c \
	xutils.h \
	$(NULL)
taskmanager_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(TASKMANAGER_LIBS) \
	$(NULL)
taskmanager_la_CFLAGS = $(WARNING_FLAGS) $(TASKMANAGER_CFLAGS)
taskmanager_la_LDFLAGS = -module -avoid-version

# DBus glue
task-manager-glue.h: task-manager-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=task_manager --mode=glib-server --output=$@ $<

task-manager-api-wrapper-glue.h: task-manager-api-wrapper-dbus.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=task_manager_api_wrapper --mode=glib-server --output=$@ $<

# Desktop file
desktopdir = $(pkgdatadir)/applets
desktop_DATA = taskmanager.desktop
@INTLTOOL_DESKTOP_RULE@

schemadir = $(pkgdatadir)/schemas
schema_DATA = awn-applet-taskmanager.schema-ini

schemasdir       = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = $(schema_DATA:.schema-ini=.schemas.in)
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

%.schemas.in: %.schema-ini
	$(top_srcdir)/data/awn-schema-to-gconf $< $@ "taskmanager"

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-gconf-schemas: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
else
install-gconf-schemas:
endif

install-data-local: install-gconf-schemas

BUILT_SOURCES = \
	task-manager-glue.h \
	task-manager-api-wrapper-glue.h \
	$(MARSHALFILES) \
	$(NULL)

EXTRA_DIST = $(desktop_DATA).in.in $(schema_DATA)
CLEANFILES = $(desktop_DATA) $(schemas_in_files) $(schemas_DATA) $(BUILT_SOURCES)
