# $Id: platform.mk,v 1.2 2006-08-05 00:44:43 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

# device
KERNEL_SOURCES+=malta_console.c

# platform
KERNEL_SOURCES+=platform_mp.c
KERNEL_SOURCES+=platform_start.c
KERNEL_SOURCES+=platform_startup.c

CPU=		mips

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

# We're building big-endian.
KERNEL_MACHINE=	malta
KERNEL_SUBMACHINE=	maltabe
