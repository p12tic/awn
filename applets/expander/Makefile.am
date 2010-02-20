APPLET_NAME = expander
include $(top_srcdir)/Makefile.c-applet

applet_LTLIBRARIES = expander.la
expander_la_SOURCES = \
	applet.c \
	$(NULL)
expander_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(AWN_LIBS) \
	$(NULL)
expander_la_LDFLAGS = $(APPLET_LINKER_FLAGS)
