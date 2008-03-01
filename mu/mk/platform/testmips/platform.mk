# $Id: platform.mk,v 1.15 2008-03-01 07:00:00 juli Exp $

.PATH: ${PLATFORM_ROOT}/platform
.PATH: ${PLATFORM_ROOT}/device

# device
KERNEL_SOURCES+=testmips_console.c
.if defined(OLDDRIVER)
KERNEL_SOURCES+=testmips_ether.c
KERNEL_SOURCES+=testmips_framebuffer.c
KERNEL_SOURCES+=testmips_mpbus.c
.endif

# platform
KERNEL_SOURCES+=platform_clock.c
KERNEL_SOURCES+=platform_mp.c
KERNEL_SOURCES+=platform_start.c
KERNEL_SOURCES+=platform_startup.c

CPU=		mips

# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

# Set the machine type for GXemul.
KERNEL_MACHINE=	testmips
