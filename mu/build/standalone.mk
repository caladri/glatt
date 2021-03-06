.if defined(TOOLCHAIN_TARGET)
CC=	${TOOLCHAIN_TARGET}-gcc
.endif

CFLAGS+=-W -Wall -Werror
CFLAGS+=-I${GLATT_SRC}/mk
CFLAGS+=-I${GLATT_SRC}/mk/cpu/mips
CFLAGS+=-I${GLATT_SRC}/mu
CFLAGS+=-I${GLATT_SRC}/mu/lib

CFLAGS+=-W -Wall -Wno-main
CFLAGS+=-Werror

CFLAGS+=-mabi=64
LDFLAGS+=-Wl,--oformat=elf64-bigmips

CFLAGS+=-fno-builtin
LDFLAGS+=-nostdlib

all: ${PROGRAM}

.for _src in ${SRCS}
_obj:=	${_src:T:R:S/$/.o/}
.if defined(OBJS)
OBJS:=	${OBJS} ${_obj}
.else
OBJS:=	${_obj}
.endif
${_obj}: ${_src}
.endfor

LDFLAGS+=-L${GLATT_SRC}/mu/lib/libmu
LDADD+=	-lmu

${PROGRAM}: ${GLATT_SRC}/mu/lib/libmu/libmu.a

${PROGRAM}: ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $> ${LDADD}

install:
	@[ -d ${DESTDIR}${BINDIR} ] || mkdir -p ${DESTDIR}${BINDIR}
	cp ${PROGRAM} ${DESTDIR}${BINDIR}/${PROGRAM}
.for _link in ${LINKS}
	ln ${DESTDIR}${BINDIR}/${PROGRAM} ${DESTDIR}${BINDIR}/${_link}
.endfor

clean:
	rm -f ${PROGRAM} ${OBJS}

.c.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

.S.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -DASSEMBLER -c -o $@ $<

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
