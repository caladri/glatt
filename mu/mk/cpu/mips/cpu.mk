# $Id: cpu.mk,v 1.9 2006-01-30 07:37:18 juli Exp $

.PATH: ${CPU_ROOT}/cpu
.PATH: ${CPU_ROOT}/page

KERNEL_SOURCES+=cpu_critical.c
KERNEL_SOURCES+=cpu_exception.c
KERNEL_SOURCES+=cpu_identify.c
KERNEL_SOURCES+=cpu_interrupt.c
KERNEL_SOURCES+=page_map.c
KERNEL_SOURCES+=start.S

KERNEL_CC=	mips64-gxemul-elf-gcc
KERNEL_AS=	mips64-gxemul-elf-as
KERNEL_LD=	mips64-gxemul-elf-ld
KERNEL_SIM=	gxemul

KERNEL_ABI=	-mabi=64
KERNEL_ENTRY=	start
KERNEL_CPU=	-mips3
KERNEL_FORMAT=	elf64-bigmips
KERNEL_MACHINE=	testmips

KERNEL_SIMFLAGS=-E ${KERNEL_MACHINE} -q
.if defined(DISPLAY)
KERNEL_SIMFLAGS+=-X
.endif

simulate-${KERNEL}: ${KERNEL}
	${KERNEL_SIM} ${KERNEL_SIMFLAGS} ${KERNEL}

