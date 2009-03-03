animations_lib = libawn-effects-anims.la

noinst_LTLIBRARIES = $(animations_lib)

NULL = 

AM_CFLAGS = $(WARNING_FLAGS)
INCLUDES = $(AWN_CFLAGS)

public_headers = $(NULL)

private_headers = \
        awn-effects-shared.h \
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
        $(NULL)

source_h = $(public_headers) $(private_headers)

source_c = \
        awn-effects-shared.c \
        awn-effect-bounce.c \
        awn-effect-desaturate.c \
        awn-effect-fade.c \
        awn-effect-glow.c \
        awn-effect-simple.c \
        awn-effect-spotlight.c \
        awn-effect-spotlight3d.c \
        awn-effect-squish.c \
        awn-effect-turn.c \
        awn-effect-zoom.c \
        $(NULL)

libawn_effects_anims_la_SOURCES =	\
	$(source_h)			\
	$(source_c)			\
	$(NULL)

libawn_effects_anims_la_LIBADD = $(AWN_LIBS)
