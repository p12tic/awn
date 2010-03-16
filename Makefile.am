NULL = 

if HAVE_VALA
VALA_BINDINGS = bindings/vala
endif

SUBDIRS = \
	libawn \
	src \
	applet-activation \
	bindings/python \
	bindings/vala \
	awn-settings \
	applets/expander \
	applets/quick-prefs \
	applets/separator \
	applets/taskmanager \
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

i18n-update: po/POTFILES.in po/POTFILES.skip
	(cd po && for lang in `grep ALL_LINGUAS ../configure.in | cut -d'"' -f2`; do \
		echo -n "Updating $$lang:"; \
		intltool-update $$lang; \
	done)

EXTRA_DIST = \
	autogen.sh \
	awn.pc.in \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(NULL)

DISTCLEANFILES = \
	doltcompile \
	doltlibtool \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(NULL)
