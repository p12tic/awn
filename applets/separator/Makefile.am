APPLET_NAME = separator
include $(top_srcdir)/Makefile.c-applet

applet_LTLIBRARIES = separator.la
separator_la_SOURCES = \
	applet.c \
	$(NULL)
separator_la_LIBADD = \
	$(top_builddir)/libawn/libawn.la \
	$(AWN_LIBS) \
	$(NULL)
separator_la_LDFLAGS = $(APPLET_LINKER_FLAGS)
