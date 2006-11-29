# $Id: build.mk,v 1.27 2006-11-29 04:39:41 juli Exp $

.PATH: ${KERNEL_ROOT}/core
.PATH: ${KERNEL_ROOT}/db
.PATH: ${KERNEL_ROOT}/io/device
.PATH: ${KERNEL_ROOT}/io/device/console
.PATH: ${KERNEL_ROOT}/vm

# core
KERNEL_SOURCES+=core_malloc.c
KERNEL_SOURCES+=core_mp.c
KERNEL_SOURCES+=core_mutex.c
KERNEL_SOURCES+=core_pool.c
KERNEL_SOURCES+=core_printf.c
KERNEL_SOURCES+=core_scheduler.c
KERNEL_SOURCES+=core_sleepq.c
KERNEL_SOURCES+=core_spinlock.c
KERNEL_SOURCES+=core_startup.c
KERNEL_SOURCES+=core_task.c
KERNEL_SOURCES+=core_test.c
KERNEL_SOURCES+=core_thread.c
KERNEL_SOURCES+=core_vdae.c

# db
KERNEL_SOURCES+=db.c
KERNEL_SOURCES+=db_panic.c
KERNEL_SOURCES+=db_show.c

# io/device
KERNEL_SOURCES+=device.c
KERNEL_SOURCES+=device_root.c
KERNEL_SOURCES+=driver.c
KERNEL_SOURCES+=driver_generic.c

# io/device/console
KERNEL_SOURCES+=console.c
KERNEL_SOURCES+=framebuffer.c
KERNEL_SOURCES+=framebuffer_font.c

# vm
KERNEL_SOURCES+=vm.c
KERNEL_SOURCES+=vm_alloc.c
KERNEL_SOURCES+=vm_index.c
KERNEL_SOURCES+=vm_page.c
