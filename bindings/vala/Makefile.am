NULL =

vapidir = $(datadir)/vala/vapi

dist_vapi_DATA =	\
	awn.vapi	\
	awn.deps	\
	$(NULL)

dist_noinst_DATA =	\
	awn.metadata	\
	$(NULL)

awn.vapi: $(top_builddir)/libawn/Awn-0.4.gir awn.metadata awn.deps
	$(VAPIGEN) --pkg=atk --pkg=cairo --pkg=pango --pkg=gio-2.0 --pkg=gdk-2.0 --pkg=gdk-pixbuf-2.0 --pkg=gtk+-2.0 --pkg=desktop-agnostic --library=awn $<
