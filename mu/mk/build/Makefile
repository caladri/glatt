# $Id: Makefile,v 1.2 2006-01-29 02:28:51 juli Exp $

KERNEL?=	mu.mk

kernel: ${KERNEL}
simulate: simulate-${KERNEL}

KERNEL_ROOT?=	${.CURDIR}/..
.PATH: ${KERNEL_ROOT}/core ${KERNEL_ROOT}/io

PLATFORM?=	testmips
PLATFORM_ROOT=	${KERNEL_ROOT}/platform/${PLATFORM}

.include "${PLATFORM_ROOT}/platform.mk"

CPU_ROOT=	${KERNEL_ROOT}/cpu/${CPU}

.include "${CPU_ROOT}/cpu.mk"

KERNEL_CPPFLAGS=-I${KERNEL_ROOT} -I${PLATFORM_ROOT} -I${CPU_ROOT}
KERNEL_CFLAGS=	${KERNEL_ABI} ${KERNEL_CPU} -nostdinc -fno-builtin -O2
KERNEL_CFLAGS+=	-W -Wall -Werror
KERNEL_LDFLAGS=	-e ${KERNEL_ENTRY} -Ttext ${KERNEL_TEXT}

KERNEL_OBJECTS=	${KERNEL_SOURCES:R:S/$/.o/g}

${KERNEL}: ${KERNEL_OBJECTS}
	${KERNEL_LD} ${KERNEL_LDFLAGS} ${KERNEL_OBJECTS} \
		--oformat=${KERNEL_FORMAT} -o $@

clean:
	-rm ${KERNEL_OBJECTS}
	-rm ${KERNEL}

.c.o:
	${KERNEL_CC} ${KERNEL_CPPFLAGS} ${KERNEL_CFLAGS} -c -o $@ $<

.S.o:
	${KERNEL_CC} ${KERNEL_CPPFLAGS} ${KERNEL_CFLAGS} -DASSEMBLER -c -o $@ $<
