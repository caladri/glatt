# $Id: platform.mk,v 1.16 2008-03-01 10:46:07 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

# device
KERNEL_SOURCES+=testmips_console.c
KERNEL_SOURCES+=testmips_ether.c
KERNEL_SOURCES+=testmips_framebuffer.c
KERNEL_SOURCES+=testmips_mpbus.c

# platform
KERNEL_SOURCES+=platform_clock.c
KERNEL_SOURCES+=platform_mainbus.c
KERNEL_SOURCES+=platform_mp.c
KERNEL_SOURCES+=platform_start.c
KERNEL_SOURCES+=platform_startup.c

CPU=		mips

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

# Set the machine type for GXemul.
KERNEL_MACHINE=	testmips
