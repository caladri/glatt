# $Id: platform.mk,v 1.3 2006-01-29 04:53:54 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

KERNEL_SOURCES+=startup.c

CPU=		mips

KERNEL_TEXT=	0xa800000000030000
