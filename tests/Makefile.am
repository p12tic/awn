noinst_PROGRAMS = \
	test-awn-icon \
	test-config-client \
	test-themed-icon

INCLUDES = $(AWN_CFLAGS) -I$(top_srcdir)

test_awn_icon_SOURCES = test-awn-icon.c
test_awn_icon_LDADD = $(top_builddir)/libawn/libawn.la

test_config_client_SOURCES = test-config-client.c
test_config_client_LDADD = $(top_builddir)/libawn/libawn.la

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
