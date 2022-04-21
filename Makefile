# light-status - linux event listener
# See LICENSE file for copyright and license details.

include config.mk

SRC = main.c util.c drw.c
OBJ = ${SRC:.c=.o}
BIN_NAME = light-status

all: options ${BIN_NAME}

options:
	@echo crealtime build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

${BIN_NAME}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f ${BIN_NAME} ${OBJ} ${BIN_NAME}-${VERSION}.tar.gz

dist: clean
	mkdir -p ${BIN_NAME}-${VERSION}
	cp -R LICENSE Makefile README config.mk\
		drw.h util.h ${SRC} ${BIN_NAME}-${VERSION}
	tar -cf ${BIN_NAME}-${VERSION}.tar ${BIN_NAME}-${VERSION}
	gzip ${BIN_NAME}-${VERSION}.tar
	rm -rf ${BIN_NAME}-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BIN_NAME} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${BIN_NAME}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${BIN_NAME}

.PHONY: all options clean dist install uninstall

