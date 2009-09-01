APP_ICON_NAME = awn-settings
include $(top_srcdir)/Makefile.app-icons

pyfiles_DATA = \
	awnClass.py		\
	awnDefs.py		\
	awnSettings.py		\
	awnSettingsHelper.py	\
	awnLauncherEditor.py		
pyfilesdir = $(pkgdatadir)/awn-settings

ui_DATA = \
	launcher-editor.ui \
	awn-settings.ui \
	$(NULL)

uidir = $(pkgdatadir)/awn-settings

desktopdir   = $(datadir)/applications
desktop_DATA = 	awn-settings.desktop
@INTLTOOL_DESKTOP_RULE@

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
	$(INSTALL) -m 755 awn-launcher-editor "$(DESTDIR)$(bindir)"

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/awn-settings
	rm -f $(DESTDIR)$(bindir)/awn-launcher-editor

EXTRA_DIST +=			\
	$(ui_DATA)		\
	*.py			\
	$(desktop_DATA).in	\
	$(NULL)

DISTCLEANFILES = awn-settings.desktop

clean:
	rm -f *.pyo *.pyc *.bak $(desktop_DATA)
