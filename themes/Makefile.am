# desktop file

desktopdir   = $(datadir)/avant-window-navigator/themes
desktop_DATA = awn-default.desktop

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@LIBDIR\@|$(libdir)|g" $< > $@

@INTLTOOL_DESKTOP_RULE@


EXTRA_DIST = $(desktop_DATA).in.in

CLEANFILES = $(desktop_DATA) $(desktop_DATA).in
