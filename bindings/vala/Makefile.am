# adapted from vala's vapi/Makefile.am
NULL =

vapidir = $(datadir)/vala/vapi

dist_vapi_DATA =	\
	awn.vapi	\
	awn.deps	\
	$(NULL)

dist_noinst_DATA =	\
	awn.files		\
	awn.gi			\
	awn.metadata	\
	awn.namespace	\
	$(NULL)

if MAINTAINER_MODE
awn.gi: awn.files awn.namespace
	$(VALA_GEN_INTROSPECT) awn $(srcdir)

awn.vapi: awn.gi awn.metadata awn.deps
	$(VAPIGEN) --library=awn $<
endif
