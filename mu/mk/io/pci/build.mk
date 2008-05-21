# $Id: build.mk,v 1.1 2008-05-21 06:42:57 juli Exp $

.PATH: ${KERNEL_ROOT}/io/pci

# io/pci
KERNEL_SOURCES+=pci.c
KERNEL_SOURCES+=pcidev.c
