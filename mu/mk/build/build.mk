# $Id: build.mk,v 1.52 2008-04-28 19:15:06 juli Exp $

.PATH: ${KERNEL_ROOT}/core
.PATH: ${KERNEL_ROOT}/db
.PATH: ${KERNEL_ROOT}/io/console
.PATH: ${KERNEL_ROOT}/io/device
.PATH: ${KERNEL_ROOT}/io/network
.PATH: ${KERNEL_ROOT}/io/pci
.PATH: ${KERNEL_ROOT}/vm

# core
KERNEL_SOURCES+=core_cv.c
KERNEL_SOURCES+=core_ipc.c
KERNEL_SOURCES+=core_malloc.c
.if !defined(NO_MORDER)
KERNEL_SOURCES+=core_morder.c
.endif
.if !defined(UNIPROCESSOR)
KERNEL_SOURCES+=core_mp.c
KERNEL_SOURCES+=core_mp_cpu.c
KERNEL_SOURCES+=core_mp_hokusai.c
KERNEL_SOURCES+=core_mp_ipi.c
.endif
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
KERNEL_SOURCES+=core_ttk.c
KERNEL_SOURCES+=core_vdae.c

# db
KERNEL_SOURCES+=db.c
KERNEL_SOURCES+=db_misc.c
KERNEL_SOURCES+=db_panic.c
KERNEL_SOURCES+=db_show.c

# io/console
KERNEL_SOURCES+=console.c
KERNEL_SOURCES+=framebuffer.c
KERNEL_SOURCES+=framebuffer_font.c

# io/device
KERNEL_SOURCES+=bus.c
KERNEL_SOURCES+=bus_internal.c
KERNEL_SOURCES+=bus_leaf.c
KERNEL_SOURCES+=bus_root.c
KERNEL_SOURCES+=device_internal.c

# io/network
KERNEL_SOURCES+=network_interface.c

# io/pci
KERNEL_SOURCES+=pci.c
KERNEL_SOURCES+=pcidev.c

# vm
KERNEL_SOURCES+=vm.c
KERNEL_SOURCES+=vm_alloc.c
KERNEL_SOURCES+=vm_index.c
KERNEL_SOURCES+=vm_page.c
