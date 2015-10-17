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
	$(VALAC) -C -H $(vala_file:.vala=.h) $(srcdir)/$(vala_file) $(VALA_FLAGS) || exit 1;)
	touch "$@"

VALA_GENERATED_FILES = \
	$(VALA_C_FILES) \
	$(VALA_H_FILES) \
	$(NULL)

applet_LTLIBRARIES = taskmanager.la
taskmanager_la_SOURCES = \
	applet.cc \
	awn-desktop-lookup.h \
	awn-desktop-lookup-cached.h \
	awn-desktop-lookup-gnome3.h \
	awn-desktop-lookup.cc \
	awn-desktop-lookup-cached.cc \
	awn-desktop-lookup-gnome3.cc \
	dock-manager-api.cc	\
	dock-manager-api.h	\
	task-defines.h \
	task-drag-indicator.cc \
	task-drag-indicator.h \
	task-icon.cc \
	task-icon.h \
	task-icon-build-context-menus.cc \
	task-icon-build-context-menus.h \
	task-icon-private.h \
	task-item.cc \
	task-item.h \
	task-launcher.cc \
	task-launcher.h \
	task-manager.cc \
	task-manager.h \
	task-manager-api-wrapper.cc \
	task-manager-api-wrapper.h \
	task-manager-api-wrapper-glue.h \
	task-manager-dialog.cc \
	task-manager-dialog.h \
	task-manager-panel-connector.cc \
	task-manager-panel-connector.h \
	task-settings.cc \
	task-settings.h \
	task-window.cc \
	task-window.h \
	util.h \
	util.cc  \
        $(builddir)/taskmanager-marshal.c \
	$(builddir)/taskmanager-marshal.h \
	xutils.cc \
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
	$(wildcard $(srcdir)/menus/*.xml) \
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
	labelled-separator.vala \
	$(NULL)

CLEANFILES += $(schema_DATA) $(schemas_DATA) $(BUILT_SOURCES) $(VALA_GENERATED_FILES)
