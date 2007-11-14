SUBDIRS = active

desktopdir   = $(datadir)/applications
desktop_DATA = avant-window-navigator.desktop
@INTLTOOL_DESKTOP_RULE@


$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" $< > $@
	
icon_DATA =  \
	avant-window-navigator.svg	\
	avant-window-navigator-24.png	\
	avant-window-navigator-32.png	\
	avant-window-navigator-48.png

icondir = $(pkgdatadir)
if USE_GCONF
awn.schemas: awn.schemas.in

awn.schemas.in: awn.schema-ini
	$(top_srcdir)/data/awn-schema-to-gconf avant-window-navigator $< $@
else
schemadir = $(datadir)/awn/schemas
schema_DATA = awn.schema-ini
endif

EXTRA_DIST = $(icon_DATA) $(desktop_DATA).in.in

CLEANFILES = $(desktop_DATA) $(desktop_DATA).in
