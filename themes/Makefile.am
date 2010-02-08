NULL = 

themedir   = $(pkgdatadir)/themes
theme_DATA = \
	dust.awn-theme \
	dark.awn-theme \
	light.awn-theme \
	smoked.awn-theme \
	$(NULL)

thumb_themedir = $(themedir)/thumbnails
thumb_theme_DATA = \
	thumbnails/dust-thumb.png \
	thumbnails/dark-thumb.png \
	thumbnails/light-thumb.png \
	thumbnails/smoked-thumb.png \
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(theme_DATA:.awn-theme:.awn-theme.in) \
	$(thumb_theme_DATA) \
	$(NULL)
