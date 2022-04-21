# app version
VERSION = 0.0.1

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -DVERSION=\"${VERSION}\"
CFLAGS = \
	-std=c17 \
	-pedantic \
	-Wall \
	-Wno-deprecated-declarations \
	-Os \
	-I/usr/include/freetype2 \
	${CPPFLAGS}

LDFLAGS = \
	-lX11 -lXft \
	-lfontconfig -lfreetype \

# compiler and linker
CC = clang

