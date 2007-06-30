# $Id: supervisor.mk,v 1.3 2007-06-30 09:27:53 juli Exp $

.PATH: ${SUPERVISOR_ROOT}

KERNEL_SOURCES+=	supervisor_exit.c
KERNEL_SOURCES+=	supervisor_functions.c
KERNEL_SOURCES+=	supervisor_install.c
