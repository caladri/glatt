# $Id: cpu.mk,v 1.1.1.1 2006-01-29 01:52:41 juli Exp $

KERNEL_CC=	mips64-gxemul-elf-gcc
KERNEL_AS=	mips64-gxemul-elf-as
KERNEL_LD=	mips64-gxemul-elf-ld
KERNEL_SIM=	gxemul

KERNEL_ABI=	-mabi=64
KERNEL_TEXT=	0xa800000000030000
KERNEL_ENTRY=	main
KERNEL_CPU=	-mips3
KERNEL_FORMAT=	elf64-bigmips
KERNEL_MACHINE=	testmips

KERNEL_SIMFLAGS=-E ${KERNEL_MACHINE} -q
.if defined(DISPLAY)
KERNEL_SIMFLAGS+=-X
.endif

simulate-${KERNEL}: ${KERNEL}
	${KERNEL_SIM} ${KERNEL_SIMFLAGS} ${KERNEL}

