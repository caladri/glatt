.if defined(TOOLCHAIN_TARGET)
CC=	${TOOLCHAIN_TARGET}-gcc
AR=	${TOOLCHAIN_TARGET}-ar
NM=	${TOOLCHAIN_TARGET}-nm
.endif

CFLAGS+=-W -Wall -Werror
CFLAGS+=-I${GLATT_SRC}/mk
CFLAGS+=-I${GLATT_SRC}/mk/cpu/mips
CFLAGS+=-I${GLATT_SRC}/mu
CFLAGS+=-I${GLATT_SRC}/mu/lib

CFLAGS+=-W -Wall -Wno-main
CFLAGS+=-Werror

CFLAGS+=-mabi=64

CFLAGS+=-fno-builtin

all: lib${LIB}.a

.for _src in ${SRCS}
_obj:=	${_src:T:R:S/$/.o/}
.if defined(OBJS)
OBJS:=	${OBJS} ${_obj}
.else
OBJS:=	${_obj}
.endif
${_obj}: ${_src}
.endfor

lib${LIB}.a: ${OBJS}
	${AR} scq $@ `NM='${NM}' lorder $> | tsort -q`

install:
	@true

clean:
	rm -f lib${LIB}.a ${OBJS}

.c.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

.S.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -DASSEMBLER -c -o $@ $<
