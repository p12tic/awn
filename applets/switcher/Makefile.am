INCLUDES =							\
	-I$(srcdir)						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"		\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(AWN_CFLAGS)					\
	-DPREFIX=\"$(prefix)\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"				\
	-DDATADIR=\"$(datadir)\"				\
	-DLIBDIR=\"$(libdir)\"                                  \
	-I$(top_builddir)/libawn

SWITCHER_SOURCES =                              \
	applet.c

SWITCHER_LDADD =                        \
        $(DEPS_LIBS)			        \
        $(AWN_LIBS)                             \
        $(top_builddir)/libawn/libawn.la
        

switcherlibdir = $(libdir)/awn/applets/switcher
switcherlib_LTLIBRARIES = switcher.la
switcher_la_SOURCES = $(SWITCHER_SOURCES)
switcher_la_LIBADD = $(SWITCHER_LDADD)
switcher_la_LDFLAGS = -module -avoid-version
switcher_la_CFLAGS =

desktopdir = $(libdir)/awn/applets

desktop_DATA = \
	switcher.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@LIBDIR\@|$(libdir)|g" $< > $@

schemasdir   = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = switcher.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
endif

EXTRA_DIST = \
	switcher.desktop.in.in			\
	switcher.schemas.in
