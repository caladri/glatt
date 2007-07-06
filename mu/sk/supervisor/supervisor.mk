# $Id: supervisor.mk,v 1.4 2007-07-06 22:09:58 juli Exp $

.PATH: ${SUPERVISOR_ROOT}

KERNEL_SOURCES+=	supervisor_cpu.c
KERNEL_SOURCES+=	supervisor_memory.c
KERNEL_SOURCES+=	supervisor_exit.c
KERNEL_SOURCES+=	supervisor_functions.c
KERNEL_SOURCES+=	supervisor_install.c
