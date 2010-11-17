APPLET_NAME = simple-launcher
APPLET_CFLAGS = \
	$(NULL)
VALA_FILES = \
	simple-launcher.vala \
	$(NULL)

AM_VALAFLAGS = \
	--pkg=build \
	--pkg=dbus-glib-1 \
	--pkg=libintl \
        --pkg desktop-agnostic-fdo \
        --pkg desktop-agnostic-ui \
        $(NULL)

include $(top_srcdir)/Makefile.vala-applet
include $(top_srcdir)/Makefile.schemas

# config

schema_in_files = awn-applet-simple-launcher.schema-ini.in

install-data-local: install-gconf-schemas

applet_LTLIBRARIES = simple-launcher.la
simple_launcher_la_SOURCES = \
	$(VALA_C_FILES) \
	$(NULL)
simple_launcher_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(AWN_LIBS) \
	$(NULL)
simple_launcher_la_LDFLAGS = $(APPLET_LINKER_FLAGS)

CLEANFILES += $(schema_DATA) $(schemas_DATA)

EXTRA_DIST += \
        $(schema_in_files) \
        $(NULL)

