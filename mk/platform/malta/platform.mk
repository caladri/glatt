# $Id: platform.mk,v 1.7 2008-06-01 00:06:00 juli Exp $

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

KERNEL_MACHINE=	malta
.if defined(LITTLE_ENDIAN)
KERNEL_SUBMACHINE=	malta
.else
KERNEL_SUBMACHINE=	maltabe
.endif
