APP_ICON_NAME = avant-window-navigator
DESKTOP_BASENAME = avant-window-navigator
desktopdir = $(datarootdir)/applications
include $(top_srcdir)/Makefile.app-icons
include $(top_srcdir)/Makefile.desktop-files
include $(top_srcdir)/Makefile.shave
include $(top_srcdir)/Makefile.schemas

icons = avant-window-navigator-24.png avant-window-navigator-32.png

# icon-related

# I have no idea where else to put it.
fallbackicondir = $(datadir)/icons/hicolor/scalable/apps
fallbackicon_DATA = \
	awn-window-fallback.svg \
	$(NULL)

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
	awn-themed-icon.ui \
	index.theme 

pkgdir = $(pkgdatadir)

# config-related

schema_in_files = avant-window-navigator.schema-ini.in

# global

install-data-local: install-gconf-schemas install-icons

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/avant-window-navigator.png
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/avant-window-navigator.png

EXTRA_DIST += \
	$(icons) \
	$(text_DATA) \
	$(desktop_DATA:.desktop=.desktop.in) \
	$(schema_in_files:.schema-ini.in=.schema-ini.in.in) \
	active/spotlight_png_inline.c \
	awn-window-fallback.svg \
	$(NULL)

CLEANFILES += $(schemas_DATA) $(schema_in_DATA) $(schema_DATA)
