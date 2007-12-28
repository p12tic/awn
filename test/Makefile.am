noinst_PROGRAMS = test-config-client

INCLUDES = $(AWN_CFLAGS) -I$(top_srcdir)

test_config_client_SOURCES = test-config-client.c

test_config_client_LDADD = $(top_builddir)/libawn/libawn.la

if HAVE_VALA
noinst_PROGRAMS += vala-test

vala-test: test.vala
	$(VALAC) -C $< --vapidir=$(top_srcdir)/bindings/vala --pkg=awn
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LDFLAGS) $(top_builddir)/libawn/libawn.la
	rm test.c test.h
endif

EXTRA_DIST = test-config-client.py
