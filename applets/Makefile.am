SUBDIRS = switcher separator notification-area python-test
if USE_GNOME
SUBDIRS += trash
endif

desktopdir = $(libdir)/awn/applets

desktop_DATA = \
	taskman.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" $< > $@

EXTRA_DIST = $(desktop_DATA).in.in
