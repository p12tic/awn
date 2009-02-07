APP_ICON_NAME = avant-window-navigator
include $(top_srcdir)/Makefile.app-icons

SUBDIRS =		\
        active		\
        default-theme	\
        $(NULL)

# desktop file

desktopdir   = $(datadir)/applications
desktop_DATA = avant-window-navigator.desktop
@INTLTOOL_DESKTOP_RULE@

icons = avant-window-navigator-24.png avant-window-navigator-32.png

# icon-related

install-icons:
	mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps
	mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps
	$(INSTALL) -m644 $(srcdir)/avant-window-navigator-24.png $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/avant-window-navigator.png
	$(INSTALL) -m644 $(srcdir)/avant-window-navigator-32.png $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/avant-window-navigator.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

text_DATA = \
	index.theme

textdir = $(pkgdatadir)

# config-related

if USE_GCONF
schemasdir       = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = awn.schemas.in awn-applets-shared.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

awn.schemas.in: awn.schema-ini
	$(top_srcdir)/data/awn-schema-to-gconf $< $@

awn-applets-shared.schemas.in: shared.schema-ini
	$(top_srcdir)/data/awn-schema-to-gconf $< $@ shared

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-gconf-schemas: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
else
install-gconf-schemas:
endif
else
install-gconf-schemas:
endif
schemadir = $(pkgdatadir)/schemas
schema_DATA = awn.schema-ini shared.schema-ini

dist_bin_SCRIPTS = awn-schema-to-gconf awn-autostart awn-applets-migration

# global

install-data-local: install-gconf-schemas install-icons

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/avant-window-navigator.png
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/avant-window-navigator.png

EXTRA_DIST += $(icons) $(text_DATA) $(desktop_DATA).in.in awn.schema-ini shared.schema-ini

CLEANFILES = $(desktop_DATA) $(desktop_DATA).in

DISTCLEANFILES = awn.schemas.in awn.schemas awn-applets-shared.schemas.in awn-applets-shared.schemas 
