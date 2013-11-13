include $(top_srcdir)/Makefile.shave

noinst_PROGRAMS = \
	test-applet-simple \
	test-awn-effects \
	test-awn-icon \
	test-awn-icon-box \
	test-taskmanager \
	test-themed-icon

AM_CPPFLAGS = $(STANDARD_CPPFLAGS) $(DISABLE_DEPRECATED_FLAGS) $(AWN_CFLAGS) -I$(top_srcdir)
AM_CFLAGS = $(WARNING_FLAGS)
AM_CXXFLAGS = $(WARNING_FLAGS) -fpermissive -std=c++11

test_applet_simple_SOURCES = test-applet-simple.cc
test_applet_simple_LDADD = 	\
						$(top_builddir)/libawn/libawn.la \
						$(AWN_LIBS)

test_awn_effects_SOURCES = test-awn-effects.cc
test_awn_effects_LDADD = \
						$(top_builddir)/libawn/libawn.la \
						$(AWN_LIBS)

test_awn_icon_SOURCES = test-awn-icon.cc
test_awn_icon_LDADD = \
					$(top_builddir)/libawn/libawn.la \
					$(AWN_LIBS)

test_awn_icon_box_SOURCES = test-awn-icon-box.cc
test_awn_icon_box_LDADD = \
						$(top_builddir)/libawn/libawn.la \
						$(AWN_LIBS)

test_taskmanager_SOURCES = test-taskmanager.cc
test_taskmanager_LDADD = \
	$(AWN_LIBS) \
	$(top_builddir)/libawn/libawn.la \
	$(NULL)

test_themed_icon_SOURCES = test-themed-icon.cc
test_themed_icon_LDADD = \
						$(top_builddir)/libawn/libawn.la \
						$(AWN_LIBS)

EXTRA_DIST = 	test-awn-dialog.py 	\
		test-awn-tooltip.py	\
		test-effects.py		\
		test-effects-scaling.py	\
		test-overlays.py	\
		test-taskmanager-dnd.py	\
		test-taskmanager-windows.py

noinst_PROGRAMS += test-vala-awn-dialog

VALA_FILES = \
	test-vala-awn-dialog.vala \
	$(NULL)

test_vala_awn_dialog_SOURCES = test-vala-awn-dialog.c
test_vala_awn_dialog_CFLAGS = -Wall -Wno-redundant-decls
test_vala_awn_dialog_LDADD = \
							$(top_builddir)/libawn/libawn.la \
							$(AWN_LIBS)

.vala.c:
	$(QUIET_GEN)$(VALAC) -C $< --vapidir=$(top_srcdir)/bindings/vala \
		--vapidir=$(LDA_VAPIDIR) --pkg=awn

EXTRA_DIST += $(VALA_FILES)
CLEANFILES = test-vala-awn-dialog.c
