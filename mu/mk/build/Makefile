# $Id: Makefile,v 1.8 2006-02-01 10:05:42 juli Exp $

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
KERNEL_CFLAGS=	${KERNEL_ABI} ${KERNEL_CPU} -nostdinc
KERNEL_CFLAGS+=	-std=c99 -pedantic -fno-builtin -W -Wall -Werror -Wno-unused
KERNEL_CFLAGS+=	-O2 -fstrict-aliasing
# Ask GCC to not use the GP, we may well end up with too much GP-relative
# data to address.  In reality, though, we shouldn't have enough global data
# for this to make sense, and we should be able to turn it back on.
# XXX MIPS only.
KERNEL_CFLAGS+=	-G0
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
