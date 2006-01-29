# $Id: platform.mk,v 1.5 2006-01-29 23:57:09 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

KERNEL_SOURCES+=platform_mp.c
KERNEL_SOURCES+=platform_start.c

CPU=		mips

KERNEL_TEXT=	0xa800000000030000
