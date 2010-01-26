## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.shave

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

NULL = 

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=libawn

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../gtk
DOC_SOURCE_DIR=$(top_srcdir)/libawn

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED" 
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/gtk/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/gtk/*.c
HFILE_GLOB=$(DOC_SOURCE_DIR)/*.h
CFILE_GLOB=$(DOC_SOURCE_DIR)/*.c

# Header files to ignore when scanning.
# e.g. IGNORE_HFILES=gtkdebug.h gtkintl.h
# For some bizarre reason, I needed to add nonexistent buffer files to get it
# to work correctly. (malept)
IGNORE_HFILES= \
	awn-effect-bounce.h \
	awn-effect-desaturate.h \
	awn-effect-fade.h \
	awn-effect-glow.h \
	awn-effect-simple.h \
	awn-effect-spotlight.h \
	awn-effect-spotlight3d.h \
	awn-effect-squish.h \
	awn-effect-turn.h \
	awn-effect-zoom.h \
	awn-effects-ops-helpers.h \
	awn-effects-shared.h \
	awn-enum-types.h \
	gseal-transition.h \
	libawn.h \
	libawn-marshal.h \
	stamp-awn-enum-types.h \
	invalid.h

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/gtk/stock-icons/stock_about_24.png
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files=

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. INCLUDES=-I$(top_srcdir) -I$(top_builddir) $(GTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/gtk/$(gtktargetlib)
INCLUDES = \
	$(AWN_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(top_builddir)/libawn	\
	-I$(top_builddir)		\
	$(NULL)

GTKDOC_LIBS = \
	$(AWN_LIBS)							\
	$(top_builddir)/libawn/libawn.la	\
	$(NULL)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

if MAINTAINER_MODE
$(DOC_MODULE).types: $(HFILE_GLOB)
	$(QUIET_GEN) gtkdoc-scan --rebuild-sections --rebuild-types --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="$(IGNORE_HFILES)"
endif

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += 
