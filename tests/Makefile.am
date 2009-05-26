noinst_PROGRAMS = \
	test-applet-simple \
	test-awn-effects \
	test-awn-icon \
	test-awn-icon-box \
	test-config-client \
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

test_config_client_SOURCES = test-config-client.c
test_config_client_LDADD = $(top_builddir)/libawn/libawn.la

test_taskmanager_SOURCES = test-taskmanager.c
test_taskmanager_LDADD = $(top_builddir)/libawn/libawn.la

test_themed_icon_SOURCES = test-themed-icon.c
test_themed_icon_LDADD = $(top_builddir)/libawn/libawn.la

if HAVE_VALA
noinst_PROGRAMS += vala-test

vala-test: test.vala
	$(VALAC) -C $< --vapidir=$(top_srcdir)/bindings/vala --pkg=awn
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LDFLAGS) $(top_builddir)/libawn/libawn.la
	rm test.c test.h
endif

EXTRA_DIST = test-config-client.py
