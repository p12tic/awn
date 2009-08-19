NULL = 

if HAVE_VALA
VALA_BINDINGS = bindings/vala
endif

SUBDIRS = \
	libawn \
	src \
	awn-applet-activation \
	awn-settings \
	applets/taskmanager \
	applets/expander \
	bindings/python \
	bindings/vala \
	po \
	data \
	tests \
	doc \
	themes \
	$(NULL)

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = awn.pc

i18n-update: po/POTFILES.in
	(cd po && for lang in `grep ALL_LINGUAS ../configure.in | cut -d'"' -f2`; do \
		intltool-update $$lang; \
	done)

EXTRA_DIST =				\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	awn.pc.in			\
	autogen.sh

DISTCLEANFILES =			\
	intltool-extract		\
	intltool-merge			\
	intltool-update			\
	doltcompile			\
	doltlibtool
