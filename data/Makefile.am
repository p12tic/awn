SUBDIRS = active

desktopdir   = $(datadir)/applications
desktop_DATA = avant-window-navigator.desktop
@INTLTOOL_DESKTOP_RULE@

icon_DATA =  \
	avant-window-navigator.svg	\
	avant-window-navigator-24.png	\
	avant-window-navigator-32.png	\
	avant-window-navigator-48.png

icondir = $(pkgdatadir)
if USE_GCONF
schemasdir       = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = awn.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

awn.schemas.in: awn.schema-ini
	$(top_srcdir)/data/awn-schema-to-gconf $< $@

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
endif
else
schemadir = $(datadir)/avant-window-navigator/schemas
schema_DATA = awn.schema-ini
endif

install-exec-local:
	$(INSTALL) -m 755 awn-schema-to-gconf $(DESTDIR)$(bindir)/awn-schema-to-gconf

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/awn-schema-to-gconf

EXTRA_DIST = $(icon_DATA) $(desktop_DATA).in.in awn.schema-ini awn-schema-to-gconf

CLEANFILES = $(desktop_DATA) $(desktop_DATA).in
