SUBDIRS = switcher separator notification-area
if USE_GNOME
SUBDIRS += trash
endif

desktopdir = $(libdir)/awn/applets

desktop_DATA = \
	taskman.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" $< > $@

extra_dist = $(desktop_DATA).in.in
