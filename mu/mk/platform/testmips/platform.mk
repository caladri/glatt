# $Id: platform.mk,v 1.7 2006-01-30 09:13:46 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

KERNEL_SOURCES+=platform_mp.c
KERNEL_SOURCES+=platform_start.c

CPU=		mips

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000
