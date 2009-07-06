SUBDIRS = active

APP_ICON_NAME = avant-window-navigator
include $(top_srcdir)/Makefile.app-icons

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

dist_pkg_DATA = \
	awn-themed-icon-ui.xml \
	index.theme 

pkgdir = $(pkgdatadir)

# config-related

if GCONF_SCHEMAS_INSTALL
schemasdir       = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = $(schema_DATA:.schema-ini=.schemas.in)
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

%.schemas.in: %.schema-ini
	$(top_srcdir)/data/awn-schema-to-gconf $< $@ $*

@INTLTOOL_SCHEMAS_RULE@

install-gconf-schemas: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
else
install-gconf-schemas:
endif

schemadir = $(pkgdatadir)/schemas
schema_DATA = avant-window-navigator.schema-ini

dist_bin_SCRIPTS = awn-schema-to-gconf awn-applets-migration

# global

install-data-local: install-gconf-schemas install-icons

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/avant-window-navigator.png
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/avant-window-navigator.png

EXTRA_DIST += $(icons) $(text_DATA) $(desktop_DATA).in.in avant-window-navigator.schema-ini awn-themed-icon-ui.glade

CLEANFILES = $(desktop_DATA) $(desktop_DATA).in avant-window-navigator.schemas avant-window-navigator.schemas.in
