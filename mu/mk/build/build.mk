# $Id: build.mk,v 1.15 2006-02-27 00:55:00 juli Exp $

.PATH: ${KERNEL_ROOT}/core
.PATH: ${KERNEL_ROOT}/db
.PATH: ${KERNEL_ROOT}/io/device/console
.PATH: ${KERNEL_ROOT}/vm

# core
KERNEL_SOURCES+=core_alloc.c
KERNEL_SOURCES+=core_mp.c
KERNEL_SOURCES+=core_spinlock.c
KERNEL_SOURCES+=core_task.c
KERNEL_SOURCES+=core_thread.c

# db
KERNEL_SOURCES+=db.c
KERNEL_SOURCES+=db_panic.c

# io/devices/console
KERNEL_SOURCES+=console.c
KERNEL_SOURCES+=framebuffer.c
KERNEL_SOURCES+=framebuffer_font.c

# vm
KERNEL_SOURCES+=vm.c
KERNEL_SOURCES+=vm_alloc.c
KERNEL_SOURCES+=vm_index.c
KERNEL_SOURCES+=vm_page.c
