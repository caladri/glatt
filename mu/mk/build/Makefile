# $Id: Makefile,v 1.6 2006-01-29 10:02:49 juli Exp $

KERNEL?=	mu-mk

kernel: ${KERNEL}
simulate: simulate-${KERNEL}

KERNEL_ROOT?=	${.CURDIR}/..
BUILD_ROOT=	${KERNEL_ROOT}/build

.include "${BUILD_ROOT}/build.mk"

PLATFORM?=	testmips
PLATFORM_ROOT=	${KERNEL_ROOT}/platform/${PLATFORM}

.include "${PLATFORM_ROOT}/platform.mk"

CPU_ROOT=	${KERNEL_ROOT}/cpu/${CPU}

.include "${CPU_ROOT}/cpu.mk"

KERNEL_CPPFLAGS=-I${KERNEL_ROOT} -I${PLATFORM_ROOT} -I${CPU_ROOT}
KERNEL_CFLAGS=	${KERNEL_ABI} ${KERNEL_CPU} -nostdinc -fno-builtin -O2
KERNEL_CFLAGS+=	-W -Wall -Werror -Wno-unused
KERNEL_LDFLAGS=	-e ${KERNEL_ENTRY} -Ttext ${KERNEL_TEXT}

KERNEL_OBJECTS=	${KERNEL_SOURCES:R:S/$/.o/g}

${KERNEL}: ${KERNEL_OBJECTS}
	${KERNEL_LD} ${KERNEL_LDFLAGS} start.o ${KERNEL_OBJECTS:Nstart.o} \
		--oformat=${KERNEL_FORMAT} -o $@

clean:
	-rm ${KERNEL_OBJECTS}
	-rm ${KERNEL}

.c.o:
	${KERNEL_CC} ${KERNEL_CPPFLAGS} ${KERNEL_CFLAGS} -c -o $@ $<

.S.o:
	${KERNEL_CC} ${KERNEL_CPPFLAGS} ${KERNEL_CFLAGS} -DASSEMBLER -c -o $@ $<
