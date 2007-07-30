SUBDIRS = switcher trash separator notification-area

desktopdir = $(libdir)/awn/applets

desktop_DATA = \
	taskman.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" $< > $@

extra_dist = $(desktop_DATA).in.in
