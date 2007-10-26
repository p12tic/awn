pyfiles_DATA = \
	awnApplet.py		\
	awnDefs.py		\
	awnLauncher.py		\
	awnPreferences.py	\
	awnTheme.py		
pyfilesdir = $(pkgdatadir)/awn-manager

glade_DATA = window.glade
gladedir = $(pkgdatadir)/awn-manager

desktopdir   = $(datadir)/applications
desktop_DATA = awn-manager.desktop
@INTLTOOL_DESKTOP_RULE@


$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" $< > $@

awnDefs.py: awnDefs.in.py
	sed -e "s|\@PREFIX\@|$(prefix)|g" awnDefs.in.py > awnDefs.py

install-exec-local:
	sed -e "s|\@DATADIR\@|$(datadir)|g" awnManager.in.py > awnManager.py
	$(INSTALL) -m 755 awnManager.py $(DESTDIR)$(bindir)/awn-manager

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/awn-manager

EXTRA_DIST = $(glade_DATA) *.py $(desktop_DATA).in.in

clean:
	rm -f *.pyo *.pyc *.bak *.gladep $(desktop_DATA) $(desktop_DATA).in
