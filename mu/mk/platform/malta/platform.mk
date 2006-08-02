# $Id: platform.mk,v 1.1 2006-08-02 19:55:47 juli Exp $

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
