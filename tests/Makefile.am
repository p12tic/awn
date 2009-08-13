include $(top_srcdir)/Makefile.shave

noinst_PROGRAMS = \
	test-applet-simple \
	test-awn-effects \
	test-awn-icon \
	test-awn-icon-box \
	test-taskmanager \
	test-themed-icon

AM_CPPFLAGS = $(STANDARD_CPPFLAGS) $(DISABLE_DEPRECATED_FLAGS)
AM_CFLAGS = $(WARNING_FLAGS)
INCLUDES = $(AWN_CFLAGS) -I$(top_srcdir)

test_applet_simple_SOURCES = test-applet-simple.c
test_applet_simple_LDADD = $(top_builddir)/libawn/libawn.la

test_awn_effects_SOURCES = test-awn-effects.c
test_awn_effects_LDADD = $(top_builddir)/libawn/libawn.la

test_awn_icon_SOURCES = test-awn-icon.c
test_awn_icon_LDADD = $(top_builddir)/libawn/libawn.la

test_awn_icon_box_SOURCES = test-awn-icon-box.c
test_awn_icon_box_LDADD = $(top_builddir)/libawn/libawn.la

test_taskmanager_SOURCES = test-taskmanager.c
test_taskmanager_LDADD = $(top_builddir)/libawn/libawn.la

test_themed_icon_SOURCES = test-themed-icon.c
test_themed_icon_LDADD = $(top_builddir)/libawn/libawn.la

if HAVE_VALA
noinst_PROGRAMS += test-vala-awn-dialog

test_vala_awn_dialog_SOURCES = test-vala-awn-dialog.c
test_vala_awn_dialog_CFLAGS = -Wall -Wno-redundant-decls
test_vala_awn_dialog_LDADD = $(top_builddir)/libawn/libawn.la

.vala.c:
	$(QUIET_GEN)$(VALAC) -C $< --vapidir=$(top_srcdir)/bindings/vala \
		--vapidir=$(LDA_VAPIDIR) --pkg=awn

CLEANFILES = test-vala-awn-dialog.c
endif
