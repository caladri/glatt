# $Id: platform.mk,v 1.6 2006-01-30 06:43:22 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

KERNEL_SOURCES+=platform_mp.c
KERNEL_SOURCES+=platform_start.c

CPU=		mips

# Load the kernel at physical address 0.
KERNEL_TEXT=	0xa800000000000000
