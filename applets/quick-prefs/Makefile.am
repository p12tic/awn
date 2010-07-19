APPLET_NAME = quick-prefs
APPLET_CFLAGS = \
	$(TASKMANAGER_CFLAGS) \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(NULL)
VALA_FILES = \
	applet.vala \
	$(NULL)

AM_VALAFLAGS = \
	--pkg=build \
	--pkg=dbus-glib-1 \
	--pkg=libintl \
	--pkg=libwnck-1.0 \
	$(NULL)

include $(top_srcdir)/Makefile.vala-applet

applet_LTLIBRARIES = quick-prefs.la
quick_prefs_la_SOURCES = \
	$(VALA_C_FILES) \
	$(NULL)
quick_prefs_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(TASKMANAGER_LIBS) \
	$(NULL)
quick_prefs_la_LDFLAGS = $(APPLET_LINKER_FLAGS)
