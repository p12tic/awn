desktopdir = $(pkgdatadir)/applets

desktop_DATA = \
	taskman.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" $< > $@

EXTRA_DIST = $(desktop_DATA).in.in

DISTCLEANFILES = taskman.desktop taskman.desktop.in
