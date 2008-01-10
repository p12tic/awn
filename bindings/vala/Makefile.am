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

awn.vapi: awn.gi awn.metadata
	$(VAPIGEN) --library=awn $<
