# adapted from vala's vapi/Makefile.am
NULL =

include $(top_srcdir)/Makefile.shave

vapidir = $(datadir)/vala/vapi

dist_vapi_DATA = \
	awn.vapi \
	awn.deps \
	$(NULL)

dist_noinst_DATA = \
	awn.exclude \
	awn.files \
	awn.gi \
	awn.metadata \
	awn.namespace \
	awn-custom.vala \
	$(NULL)

VAPIGEN_INPUT = \
	awn.gi \
	awn-custom.vala \
	$(NULL)

EXTRA_DIST = \
	build.vapi \
	libintl.vapi

if MAINTAINER_MODE
awn.gi: awn.exclude awn.files awn.namespace
	$(QUIET_GEN)$(VALA_GEN_INTROSPECT) awn $(srcdir)

awn.vapi: $(VAPIGEN_INPUT) awn.metadata awn.deps
	$(QUIET_GEN)$(VAPIGEN) --library=awn --vapidir=$(LDA_VAPIDIR) \
	    $(VAPIGEN_INPUT)
endif
