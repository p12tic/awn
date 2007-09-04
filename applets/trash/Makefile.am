INCLUDES =							\
	-I$(srcdir)						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"		\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(AWN_CFLAGS)					\
	-DPREFIX=\"$(prefix)\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"				\
	-DDATADIR=\"$(datadir)\"				\
	-DLIBDIR=\"$(libdir)\"                                  \
	-I$(top_builddir)/libawn				\
	-DGLADEDIR=\"$(libdir)/awn/applets/trash\"

TRASH_SOURCES =                              \
	applet.c                                \
	trashapplet.c                           \
	trashapplet.h                           \
	trash-monitor.c                         \
	trash-monitor.h                         \
	xstuff.c                                \
	xstuff.h

TRASH_LDADD =                        \
        $(DEPS_LIBS)			        \
        $(AWN_LIBS)                             \
        $(top_builddir)/libawn/libawn.la
        

trashlibdir = $(libdir)/awn/applets/trash
trashlib_LTLIBRARIES = trash.la
trash_la_SOURCES = $(TRASH_SOURCES)
trash_la_LIBADD = $(TRASH_LDADD)
trash_la_LDFLAGS = -module -avoid-version
trash_la_CFLAGS =

schemasdir   = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = trash.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
endif

desktopdir = $(libdir)/awn/applets

desktop_DATA = \
	trash.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@LIBDIR\@|$(libdir)|g" $< > $@

gladedir = $(libdir)/awn/applets/trash
glade_DATA = trashapplet.glade

EXTRA_DIST = \
	trash.desktop.in.in                     \
	$(schemas_in_files)			\
	trashapplet.glade
