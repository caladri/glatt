# $Id: platform.mk,v 1.2 2007-06-30 09:11:35 juli Exp $

.PATH: ${PLATFORM_ROOT}/sk
.PATH: ${PLATFORM_ROOT}/supervisor

KERNEL_SOURCES+=platform_sk_init.c

CPU=		mips

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

# Set the machine type for GXemul.
KERNEL_MACHINE=	testmips
