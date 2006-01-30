# $Id: build.mk,v 1.7 2006-01-30 00:10:11 juli Exp $

.PATH: ${KERNEL_ROOT}/core
.PATH: ${KERNEL_ROOT}/db
.PATH: ${KERNEL_ROOT}/io/device/console
.PATH: ${KERNEL_ROOT}/vm

# core
KERNEL_SOURCES+=core_mp.c
KERNEL_SOURCES+=core_spinlock.c

# io/devices/console
KERNEL_SOURCES+=console.c
KERNEL_SOURCES+=framebuffer.c
KERNEL_SOURCES+=framebuffer_font.c

# vm
KERNEL_SOURCES+=vm_page.c
KERNEL_SOURCES+=vm.c
