# $Id: Makefile,v 1.33 2008-06-01 01:17:48 juli Exp $

KERNEL?=	mu-mk

kernel: ${KERNEL}
simulate: simulate-${KERNEL}

.include "config.mk"

PLATFORM_ROOT=	${KERNEL_ROOT}/platform/${PLATFORM}

.include "${PLATFORM_ROOT}/platform.mk"

CPU_ROOT=	${KERNEL_ROOT}/cpu/${CPU}

.include "${CPU_ROOT}/cpu.mk"

KERNEL_CPPFLAGS=-I${KERNEL_ROOT} -I${PLATFORM_ROOT} -I${CPU_ROOT}
KERNEL_CPPFLAGS+=-DCONFIGSTR=${CONFIGSTR}
.if defined(DB)
KERNEL_CPPFLAGS+=-DDB
.endif
.if defined(IPC)
KERNEL_CPPFLAGS+=-DIPC
.endif
.if defined(VERBOSE)
KERNEL_CPPFLAGS+=-DVERBOSE
.endif
.if !defined(MP)
KERNEL_CPPFLAGS+=-DUNIPROCESSOR
.endif
.if !defined(MORDER)
KERNEL_CPPFLAGS+=-DNO_MORDER
.endif

KERNEL_CFLAGS+= -O2 -Wuninitialized
KERNEL_CFLAGS+=	${KERNEL_ABI} ${KERNEL_CPU} -nostdinc
KERNEL_CFLAGS+=	-std=c99 -pedantic
KERNEL_CFLAGS+=	-Werror -W -Wall -Wno-unused -Wformat
KERNEL_CFLAGS+=	-Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch -Wshadow
KERNEL_CFLAGS+=	-Wcast-align -Winline -Wnested-externs -Wredundant-decls
KERNEL_CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
KERNEL_CFLAGS+=	-fstrict-aliasing -fno-builtin
KERNEL_CFLAGS+=	--param inline-unit-growth=100 --param large-function-growth=1000

KERNEL_CFLAGS+=	-Wunused
KERNEL_CFLAGS+=	-Wunused-function
KERNEL_CFLAGS+=	-Wunused-label
KERNEL_CFLAGS+=	-Wno-unused-parameter
KERNEL_CFLAGS+=	-Wunused-value
KERNEL_CFLAGS+=	-Wunused-variable

KERNEL_LDFLAGS=	-e ${KERNEL_ENTRY}
.if defined(KERNEL_TEXT)
KERNEL_LDFLAGS+=-Ttext ${KERNEL_TEXT}
.endif

KERNEL_OBJECTS=	${KERNEL_SOURCES:R:S/$/.o/g}

.if defined(KERNEL_FORMAT)
KERNEL_FORMAT:=--oformat=${KERNEL_FORMAT}
.endif

${KERNEL}: ${KERNEL_OBJECTS}
	${KERNEL_LD} ${KERNEL_LDFLAGS} start.o ${KERNEL_OBJECTS:Nstart.o} \
		${KERNEL_FORMAT} -o $@

clean:
	-rm _as.h _as.h.S
	-rm ${KERNEL_OBJECTS}
	-rm ${KERNEL}

_as.h: ${PLATFORM_ROOT}/platform/_as.sym
	${KERNEL_CC} -S -x c ${KERNEL_CPPFLAGS} ${KERNEL_CFLAGS} -c -o $@.S $>
	grep '@assym@' $@.S | sed 's/.*@assym@ /#/' > $@

.c.o:
	${KERNEL_CC} ${KERNEL_CPPFLAGS} ${KERNEL_CFLAGS} -c -o $@ $<

.S.o: _as.h
	${KERNEL_CC} ${KERNEL_CPPFLAGS} ${KERNEL_CFLAGS} -DASSEMBLER -include ${.OBJDIR}/_as.h -c -o $@ $<

reconfig:
	mu-config ${KERNEL_ROOT} ${PLATFORM} ${CONFIGSTR}