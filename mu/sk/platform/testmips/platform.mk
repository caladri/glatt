# $Id: platform.mk,v 1.1 2007-06-30 02:21:29 juli Exp $

CPU=		mips

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

# Set the machine type for GXemul.
KERNEL_MACHINE=	testmips
