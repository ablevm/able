BIN?=able
SRCS=trap.c libable_shim.c term.c able.c
LIBS=-lpthread -lable

.include "config.mk"

OBJS+=${SRCS:N*.h:R:S/$/.o/}

.PHONY: build clean install uninstall

build: ${BIN}

clean:
	-rm -vf ${OBJS} ${BIN}
	-rm -vf ${BIN}.core

install: ${BIN}
	@mkdir -p ${BINDIR}
	install -m 0755 ${BIN} ${BINDIR}/${BIN}

uninstall:
	-rm -vf ${BINDIR}/${BIN}

${BIN}: ${OBJS}
	${LD} ${LDFLAGS} -o ${BIN} ${OBJS} ${LIBS}
