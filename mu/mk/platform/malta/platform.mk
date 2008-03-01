# $Id: platform.mk,v 1.5 2008-03-01 10:46:07 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

# device
KERNEL_SOURCES+=malta_console.c

# platform
KERNEL_SOURCES+=platform_mainbus.c
KERNEL_SOURCES+=platform_start.c
KERNEL_SOURCES+=platform_startup.c

CPU=		mips
UNIPROCESSOR=	t

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

KERNEL_MACHINE=	malta
.if defined(LITTLE_ENDIAN)
KERNEL_SUBMACHINE=	malta
.else
KERNEL_SUBMACHINE=	maltabe
.endif
