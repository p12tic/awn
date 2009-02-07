APP_ICON_NAME = awn-manager
include $(top_srcdir)/Makefile.app-icons

pyfiles_DATA = \
	awnApplet.py		\
	awnDefs.py		\
	awnLauncher.py		\
	awnLauncherEditor.py	\
	awnPreferences.py	\
	awnTheme.py		
pyfilesdir = $(pkgdatadir)/awn-manager

glade_DATA = launcher-editor.glade window.glade
gladedir = $(pkgdatadir)/awn-manager

desktopdir   = $(datadir)/applications
desktop_DATA = awn-manager.desktop
@INTLTOOL_DESKTOP_RULE@

install-exec-local:
	test -e "$(DESTDIR)$(bindir)" || mkdir -p "$(DESTDIR)$(bindir)"
	$(INSTALL) -m 755 awnManager.py "$(DESTDIR)$(bindir)/awn-manager"
	$(INSTALL) -m 755 awn-launcher-editor "$(DESTDIR)$(bindir)"

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/awn-manager
	rm -f $(DESTDIR)$(bindir)/awn-launcher-editor

EXTRA_DIST +=			\
	$(glade_DATA)		\
	*.py			\
	$(desktop_DATA).in.in	\
	$(NULL)

DISTCLEANFILES = awn-manager.desktop

clean:
	rm -f *.pyo *.pyc *.bak *.gladep $(desktop_DATA) $(desktop_DATA).in
