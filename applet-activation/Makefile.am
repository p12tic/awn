NULL =
AM_CPPFLAGS =		\
	-I$(top_srcdir) \
	$(AWN_CFLAGS)	\
	$(NULL)
AM_CXXFLAGS = -fpermissive -std=c++11

bin_PROGRAMS = awn-applet

awn_applet_LDADD =			\
	$(AWN_LIBS)				\
	$(top_builddir)/libawn/libawn.la	\
	$(NULL)

awn_applet_SOURCES =	\
	main.cc			\
	$(NULL)
# vim: set ts=8 sts=8 sw=8 :
