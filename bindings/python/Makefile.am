NULL = 

# awn python scripts
pyawndir = $(PYTHON_SITE_PKG)/awn
pyawn_PYTHON = __init__.py

INCLUDES = \
	$(PYTHON_CPPFLAGS) \
	$(PYGTK_CFLAGS) \
	$(PYCAIRO_CFLAGS) \
	$(AWN_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir)/libawn \
	-DHAVE_PYCAIRO \
	$(NULL)

# awn python extensions
pyawnexecdir = $(PYTHON_PLATFORM_PKG)/awn
pyawnexec_LTLIBRARIES = awn.la

common_ldflags = -module -avoid-version

AWN_DEFS = awn.defs
CLEANFILES = awn.c
EXTRA_DIST = $(AWN_DEFS) awn.override
awn.c: $(AWN_DEFS) awn.override

awn_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initawn

awn_la_LIBADD = \
	$(PYTHON_LDFLAGS) \
	$(PYTHON_EXTRA_LIBS) \
	$(PYTHON_EXTRA_LDFLAGS) \
	$(PYGTK_LIBS) \
	$(PYCAIRO_LIBS) \
	$(top_builddir)/libawn/libawn.la \
	$(NULL)

awn_la_SOURCES = awnmodule.c
nodist_awn_la_SOURCES = awn.c

defsdir = $(pkgdatadir)/defs
defs_DATA = $(AWN_DEFS)

# code generator
.defs.c:
	@echo "***INFO*** Generating $*.c from $*.defs" \
	&& ( $(PYGTK_CODEGEN) \
	    -I $(srcdir) \
	    --py_ssize_t-clean \
	    --register $(PYGTK_DEFSDIR)/gdk-types.defs \
	    --register $(PYGTK_DEFSDIR)/gtk-types.defs \
	--override $(srcdir)/$*.override \
	    --prefix py$* $<) > gen-$*.c \
	&& cp -f gen-$*.c $*.c \
	&& rm -f gen-$*.c
