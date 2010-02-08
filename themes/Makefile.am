NULL = 

themedir   = $(pkgdatadir)/themes
theme_DATA = dark.awn-theme

thumb_themedir = $(themedir)/thumbnails
thumb_theme_DATA = thumbnails/dark-thumb.png

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(theme_DATA:.awn-theme:.awn-theme.in) \
	$(thumb_theme_DATA) \
	$(NULL)
