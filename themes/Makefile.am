# desktop file

desktopdir   = $(pkgdatadir)/themes
desktop_DATA = awn-default.desktop

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_DATA:.desktop=.desktop.in)

CLEANFILES = $(desktop_DATA)
