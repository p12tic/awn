APP_ICON_NAME = awn-settings
include $(top_srcdir)/Makefile.app-icons

pyfiles_DATA = \
	awnClass.py		\
	awnDefs.py		\
	awnSettings.py		\
	awnLauncherEditor.py		
pyfilesdir = $(pkgdatadir)/awn-settings

xml_DATA = 	launcher-editor.xml 	\
		awn-settings.xml

xmldir = $(pkgdatadir)/awn-settings

desktopdir   = $(datadir)/applications
desktop_DATA = 	awn-settings.desktop
@INTLTOOL_DESKTOP_RULE@

install-exec-local:
	test -e "$(DESTDIR)$(bindir)" || mkdir -p "$(DESTDIR)$(bindir)"
	$(INSTALL) -m 755 awnSettings.py "$(DESTDIR)$(bindir)/awn-settings"
	$(INSTALL) -m 755 awn-launcher-editor "$(DESTDIR)$(bindir)"

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/awn-settings
	rm -f $(DESTDIR)$(bindir)/awn-launcher-editor

EXTRA_DIST +=			\
	$(xml_DATA)		\
	*.py			\
	$(desktop_DATA).in.in	\
	$(NULL)

DISTCLEANFILES = awn-settings.desktop

clean:
	rm -f *.pyo *.pyc *.bak $(desktop_DATA) $(desktop_DATA).in
