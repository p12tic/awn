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
	labelled-separator.vala \
	$(NULL)

VALA_C_FILES = \
	$(VALA_FILES:.vala=.c) \
	$(NULL)

VALA_H_FILES = \
	$(VALA_FILES:.vala=.h) \
	$(NULL)

VALA_FLAGS = \
	$(top_srcdir)/bindings/vala/awn.vapi \
	--pkg dbus-glib-1 \
	--pkg libwnck-1.0 \
	--vapidir=$(LDA_VAPIDIR) \
	--pkg desktop-agnostic \
	--pkg desktop-agnostic-cfg \
	$(srcdir)/task-manager.vapi \
	$(NULL)

task-manager.vala.stamp: $(VALA_FILES)
	$(foreach vala_file,$(VALA_FILES),\
	$(VALAC) -C -H $(vala_file:.vala=.h) $(vala_file) $(VALA_FLAGS) || exit 1;)
	touch "$@"

VALA_GENERATED_FILES = \
	$(VALA_C_FILES) \
	$(VALA_H_FILES) \
	$(NULL)

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
	task-manager-api-wrapper.c \
	task-manager-api-wrapper.h \
	task-manager-api-wrapper-glue.h \
	task-manager-dialog.c \
	task-manager-dialog.h \
	task-manager-panel-connector.c \
	task-manager-panel-connector.h \
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
DBUS_XML = task-manager-api-wrapper-dbus.xml

task-manager-api-wrapper-glue.h: task-manager-api-wrapper-dbus.xml Makefile
	$(QUIET_GEN)$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=task_manager_api_wrapper --mode=glib-server --output=$@ $<

# config

schema_in_files = awn-applet-taskmanager.schema-ini.in

install-data-local: install-gconf-schemas

# menus

taskmanager_menusdir = $(applet_datadir)/menus
dist_taskmanager_menus_DATA = \
	$(wildcard menus/*.xml) \
	$(NULL)

# miscellaneous

BUILT_SOURCES = \
	task-manager.vala.stamp \
	task-manager-api-wrapper-glue.h \
	$(MARSHALFILES) \
	$(NULL)

EXTRA_DIST += \
	$(schema_in_files) \
	$(DBUS_XML) \
	task-manager.vala.stamp \
	$(VALA_GENERATED_FILES) \
	taskmanager-marshal.list \
	task-manager.vapi \
	dock-manager-api.vala \
	labelled-separator.vala \
	$(NULL)

CLEANFILES += $(schema_DATA) $(schemas_DATA) $(BUILT_SOURCES)
