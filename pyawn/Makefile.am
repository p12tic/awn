# python scripts & extension modules
pkgpythondir = $(pyexecdir)
pkgpyexecdir = $(pyexecdir)

# awn python extension modules
pyawnexecdir = $(pkgpyexecdir)/awn

# clutter python scripts
pyawndir = $(pkgpythondir)/awn
pyawn_PYTHON = __init__.py

INCLUDES = \
	$(PYTHON_INCLUDES) \
	$(PYGTK_CFLAGS) \
	$(PYCAIRO_CFLAGS) \
	$(AWN_CFLAGS) \
	-DHAVE_PYCAIRO

pyawnexec_LTLIBRARIES = awn.la

common_ldflags = -module -avoid-version

CLUTTERGTK_DEFS = awn.defs
CLEANFILES = awn.c
EXTRA_DIST = $(CLUTTERGTK_DEFS) awn.override
awn.c: $(CLUTTERGTK_DEFS) awn.override

awn_la_CFLAGS = \
	$(CLUTTERGTK_CFLAGS) \
	$(PYCAIRO_CFLAGS) \
	$(PYGTK_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir)/libawn \
	$(AWN_CFLAGS)

awn_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initawn

awn_la_LIBADD = \
	$(PYGTK_LIBS) \
	$(PYCAIRO_CFLAGS) \
	$(CLUTTERGTK_LIBS) \
	$(top_builddir)/libawn/libawn.la  

awn_la_SOURCES = awnmodule.c
nodist_awn_la_SOURCES = awn.c

defsdir = $(pkgdatadir)/$(PLATFORM_VERSION)/defs
defs_DATA = $(CLUTTERGTK_DEFS)

# code generator
.defs.c:
	@echo "***INFO*** Generating $*.c from $*.defs" \
	&& ( $(PYGTK_CODEGEN) \
	    -I $(srcdir) \
	    --py_ssize_t-clean \
	    --register $(PYGTK_DEFSDIR)/gdk-types.defs \
	    --register $(PYGTK_DEFSDIR)/gtk-types.defs \
	    --register $(PYGTK_DEFSDIR)/gconf.defs \
	--override $(srcdir)/$*.override \
	    --prefix py$* $<) > gen-$*.c \
	&& cp -f gen-$*.c $*.c \
	&& rm -f gen-$*.c
