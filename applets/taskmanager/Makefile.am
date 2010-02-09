APPLET_NAME = taskmanager
APPLET_CFLAGS = \
	$(TASKMANAGER_CFLAGS) \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(NULL)
MARSHAL_PREFIX = taskmanager
include $(top_srcdir)/Makefile.c-applet
include $(top_srcdir)/Makefile.marshal
include $(top_srcdir)/Makefile.schemas

VALA_FILES = \
	dock-manager-api.vala \
	$(NULL)

VALA_GENERATED_FILES = \
	$(VALA_FILES:.vala=.c) \
	$(VALA_FILES:.vala=.h) \
	$(NULL)

VALA_PKGS = \
	--pkg dbus-glib-1 \
	--pkg gtk+-2.0 \
	--pkg desktop-agnostic-cfg \
	--pkg libwnck-1.0 \
	$(NULL)

VALA_VAPI_FILES = \
	$(top_builddir)/bindings/vala/awn.vapi \
	task-manager.vapi \
	$(NULL)

.vala.c:
	$(VALAC) -C -H $(<:.vala=.h) $< $(VALA_VAPI_FILES) $(VALA_PKGS) && \
	$(SED) -i -r 's/^#include <(task.+h)>$$/#include "\1"/' $(<:.vala=).[ch]

applet_LTLIBRARIES = taskmanager.la
taskmanager_la_SOURCES = \
	applet.c \
	awn-desktop-lookup.h \
	awn-desktop-lookup-cached.h \
	awn-desktop-lookup-gnome3.h \
	awn-desktop-lookup.c \
	awn-desktop-lookup-cached.c \
	awn-desktop-lookup-gnome3.c \
	task-defines.h \
	task-drag-indicator.c \
	task-drag-indicator.h \
	task-icon.c \
	task-icon.h \
	task-icon-build-context-menus.c \
	task-icon-build-context-menus.h \
	task-icon-private.h \
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
	util.h \
	util.c  \
	$(builddir)/taskmanager-marshal.c \
	$(builddir)/taskmanager-marshal.h \
	xutils.c \
	xutils.h \
	$(VALA_GENERATED_FILES) \
	$(NULL)
taskmanager_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(TASKMANAGER_LIBS) \
	$(NULL)
taskmanager_la_LDFLAGS = $(APPLET_LINKER_FLAGS)

# DBus glue
DBUS_XML = task-manager-dbus.xml task-manager-api-wrapper-dbus.xml

task-manager-glue.h: task-manager-dbus.xml Makefile
	$(QUIET_GEN)$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=task_manager --mode=glib-server --output=$@ $<

task-manager-api-wrapper-glue.h: task-manager-api-wrapper-dbus.xml Makefile
	$(QUIET_GEN)$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=task_manager_api_wrapper --mode=glib-server --output=$@ $<

schema_in_files = awn-applet-taskmanager.schema-ini.in

install-data-local: install-gconf-schemas

BUILT_SOURCES = \
	$(VALA_GENERATED_FILES) \
	task-manager-glue.h \
	task-manager-api-wrapper-glue.h \
	$(MARSHALFILES) \
	$(NULL)

EXTRA_DIST += $(schema_in_files) $(DBUS_XML) taskmanager-marshal.list
CLEANFILES += $(schema_DATA) $(schemas_DATA) $(BUILT_SOURCES)

taskmanager_menusdir = $(applet_datadir)/menus
dist_taskmanager_menus_DATA = \
	$(wildcard menus/*.xml) \
	$(NULL)
