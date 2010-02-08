APP_ICON_NAME = awn-settings
DESKTOP_BASENAME = awn-settings
desktopdir = $(datarootdir)/applications
include $(top_srcdir)/Makefile.app-icons
include $(top_srcdir)/Makefile.desktop-files

python_files = \
	awnClass.py \
	awnSettingsHelper.py \
	$(NULL)

generated_python_files = \
	awnDefs.py \
	awnSettings.py \
	$(NULL)

pyfiles_DATA = \
	$(python_files) \
	$(generated_python_files) \
	$(NULL)
pyfilesdir = $(pkgdatadir)/awn-settings

ui_DATA = \
	awn-settings.ui \
	$(NULL)

uidir = $(pkgdatadir)/awn-settings

# icon-related

categoryicondir = $(datadir)/icons/hicolor/scalable/categories
dist_categoryicon_DATA = \
	awn-plugins.svg \
	$(NULL)

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

install-exec-local:
	test -e "$(DESTDIR)$(bindir)" || mkdir -p "$(DESTDIR)$(bindir)"
	$(INSTALL) -m 755 awnSettings.py "$(DESTDIR)$(bindir)/awn-settings"

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/awn-settings

EXTRA_DIST += \
	$(python_files) \
	$(generated_python_files:.py=.py.in) \
	$(ui_DATA) \
	$(desktop_DATA:.desktop=.desktop.in) \
	$(NULL)
