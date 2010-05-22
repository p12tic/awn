NULL =
INCLUDES =		\
	-I$(top_srcdir) \
	$(AWN_CFLAGS)	\
	$(PYTHON_CPPFLAGS)	\
	$(PYGTK_CFLAGS) \
	$(NULL)

bin_PROGRAMS = awn-applet

AM_VALAFLAGS = \
	--pkg dbus-glib-1 \
	--pkg desktop-agnostic-fdo \
	--pkg awn \
	../bindings/vala/build.vapi \
	$(NULL)

awn_applet_LDADD =			\
	$(AWN_LIBS)				\
	$(PYTHON_LDFLAGS)			\
	$(PYGTK_LIBS)				\
	$(top_builddir)/libawn/libawn.la	\
	$(NULL)

awn_applet_SOURCES =	\
	awn-applet.vala	\
	execute-wrapper.c	\
	execute-python.c	\
	$(NULL)
# vim: set ts=8 sts=8 sw=8 :
