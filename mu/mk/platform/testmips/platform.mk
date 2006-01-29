# $Id: platform.mk,v 1.4 2006-01-29 22:20:17 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

KERNEL_SOURCES+=platform_mp.c
KERNEL_SOURCES+=startup.c

CPU=		mips

KERNEL_TEXT=	0xa800000000030000
