INCLUDES =				\
	$(AWN_CFLAGS)			\
	$(WARN_CFLAGS)

bin_PROGRAMS = awn-applet-activation

AM_CFLAGS =								\
	-DDATADIR=\""$(datadir)"\"					\
	-DLIBDIR=\""$(libdir)"\"                                        \
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"          \
	-I$(top_builddir)/libawn

awn_applet_activation_LDADD  = $(DEPS_LIBS)	\
        $(AWN_LIBS)                             \
	$(top_builddir)/libawn/libawn.la  

awn_applet_activation_SOURCES =			\
	main.c				        \
	awn-plug.c                              \
	awn-plug.h                              

