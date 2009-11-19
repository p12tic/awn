include $(top_srcdir)/Makefile.shave

AM_CPPFLAGS = \
	$(STANDARD_CPPFLAGS) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	-DLOCALEDIR=\""$(datadir)/locale"\"
	$(DISABLE_DEPRECATED_FLAGS) \
	$(NULL)

VALA_FILES = \
	applet.vala \
	$(NULL)
AM_VALAFLAGS = --pkg=libwnck-1.0 --pkg=dbus-glib-1

#APPLET_NAME = quick-prefs
appletdir = $(APPLETDIR)/quick-prefs
applet_LTLIBRARIES = quick-prefs.la

# parts taken from awn-extras' Makefile.vala-applet

# for shave
QUIET_INSTALL = $(Q:@=@echo '  INSTALL '$@;)
QUIET_VALA = $(Q:@=@echo '  VALAC '$@;)

BUILT_SOURCES = quick-prefs.vala.stamp

VALA_C_FILES = $(VALA_FILES:.vala=.c)

quick-prefs.vala.stamp: $(VALA_FILES)
	$(QUIET_VALA)$(VALAC) -C $^ \
		--vapidir=$(top_srcdir)/bindings/vala --pkg=awn \
		--pkg=libintl --pkg=build \
		--vapidir=$(LDA_VAPIDIR) \
		$(AM_VALAFLAGS) $(VALAFLAGS)
	touch "$@"

quick_prefs_la_SOURCES = \
	$(VALA_C_FILES) \
	$(NULL)
quick_prefs_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(TASKMANAGER_LIBS) \
	$(NULL)
quick_prefs_la_CFLAGS = $(WARNING_FLAGS) $(TASKMANAGER_CFLAGS)
quick_prefs_la_LDFLAGS = -module -avoid-version

# Desktop file
desktopdir = $(pkgdatadir)/applets
desktop_DATA = quick-prefs.desktop
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_DATA).in.in $(VALA_FILES)
CLEANFILES = $(desktop_DATA) $(BUILT_SOURCES) $(VALA_C_FILES)
