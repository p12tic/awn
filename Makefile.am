NULL = 

SUBDIRS =				\
	libawn                          \
	src				\
	awn-applet-activation           \
	awn-manager			\
	applets                         \
	po				\
	bindings			\
	data				\
	doc				\
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = awn.pc

EXTRA_DIST =				\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	awn.pc.in			\
	autogen.sh

DISTCLEANFILES =			\
	intltool-extract		\
	intltool-merge			\
	intltool-update
