# $Id: build.mk,v 1.8 2006-01-30 00:20:37 juli Exp $

.PATH: ${KERNEL_ROOT}/core
.PATH: ${KERNEL_ROOT}/db
.PATH: ${KERNEL_ROOT}/io/device/console
.PATH: ${KERNEL_ROOT}/vm

# core
KERNEL_SOURCES+=core_mp.c
KERNEL_SOURCES+=core_spinlock.c

# db
KERNEL_SOURCES+=db_panic.c

# io/devices/console
KERNEL_SOURCES+=console.c
KERNEL_SOURCES+=framebuffer.c
KERNEL_SOURCES+=framebuffer_font.c

# vm
KERNEL_SOURCES+=vm_page.c
KERNEL_SOURCES+=vm.c
