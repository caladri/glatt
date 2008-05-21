# $Id: build.mk,v 1.1 2008-05-21 06:42:57 juli Exp $

.PATH: ${KERNEL_ROOT}/core

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
KERNEL_SOURCES+=core_thread.c
KERNEL_SOURCES+=core_ttk.c
KERNEL_SOURCES+=core_vdae.c
