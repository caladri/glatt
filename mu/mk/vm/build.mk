# $Id: build.mk,v 1.1 2008-05-21 06:42:57 juli Exp $

.PATH: ${KERNEL_ROOT}/vm

# vm
KERNEL_SOURCES+=vm.c
KERNEL_SOURCES+=vm_alloc.c
KERNEL_SOURCES+=vm_index.c
KERNEL_SOURCES+=vm_page.c
