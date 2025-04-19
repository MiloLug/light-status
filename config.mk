# app version
VERSION = 0.0.1
BIN_NAME = light-status

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
DEFFLAGS = -D_DEFAULT_SOURCE -DVERSION=\"${VERSION}\"

RELEASE_CFLAGS = \
	-ffast-math \
    -fno-finite-math-only \
    -march=native \
    -fno-exceptions \
	-O3 \
	-fforce-emit-vtables \
	-faddrsig

CFLAGS = \
	-std=c17 \
	-pedantic \
	-Wall \
	-Wno-deprecated-declarations \
	-I/usr/include \
	-I/usr/include/freetype2

LDFLAGS = \
	-lX11 -lXft \
	-lfontconfig -lfreetype \


# Xinerama, comment if you don't want it
DEFFLAGS += -DUSE_XINERAMA
LDFLAGS += -lXinerama

# compiler and linker
CC = clang
