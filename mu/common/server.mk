CC=	${TOOLCHAIN_TARGET}-gcc

CFLAGS+=-I${GLATT_SRC}/mk
CFLAGS+=-I${GLATT_SRC}/mk/cpu/mips
CFLAGS+=-I${GLATT_SRC}/mu

LDFLAGS+=-static -nostdlib

CFLAGS+=-W -Wall -Wno-main
CFLAGS+=-Werror

CFLAGS+=-mabi=64
LDFLAGS+=-Wl,--oformat=elf64-bigmips

CFLAGS+=-fno-builtin

SRCS+=	${GLATT_SRC}/mu/common/syscalls.S
SRCS+=	${GLATT_SRC}/mu/common/util.c
SRCS+=	${GLATT_SRC}/mk/core/core_printf.c

all: ${SERVER}

.for _src in ${SRCS}
_obj:=	${_src:T:R:S/$/.o/}
.if defined(OBJS)
OBJS:=	${OBJS} ${_obj}
.else
OBJS:=	${_obj}
.endif
${_obj}: ${_src}
.endfor

${SERVER}: ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $>

install:
	cp ${SERVER} ${DESTDIR}/mu/servers/${SERVER}

clean:
	rm -f ${SERVER} ${OBJS}

.c.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

.S.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -DASSEMBLER -c -o $@ $<
