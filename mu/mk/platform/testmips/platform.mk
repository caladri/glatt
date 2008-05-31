# $Id: platform.mk,v 1.18 2008-05-31 23:44:50 juli Exp $

# XXX Find a way to get this from CONFIG.
CPU=		mips

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

# Set the machine type for GXemul.
KERNEL_MACHINE=	testmips
