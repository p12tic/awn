SUBDIRS = active

desktopdir   = $(datadir)/applications
desktop_DATA = avant-window-navigator.desktop
@INTLTOOL_DESKTOP_RULE@


$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" $< > $@
	
icon_DATA =  \
	avant-window-navigator.svg	\
	avant-window-navigator-24.png	\
	avant-window-navigator-48.png

icondir = $(pkgdatadir)

EXTRA_DIST = $(icon_DATA) $(desktop_DATA)

CLEANFILES = $(desktop_DATA) $(desktop_DATA).in
