# $Id: build.mk,v 1.1 2008-05-21 06:42:57 juli Exp $

.PATH: ${KERNEL_ROOT}/io/device

# io/device
KERNEL_SOURCES+=bus.c
KERNEL_SOURCES+=bus_internal.c
KERNEL_SOURCES+=bus_leaf.c
KERNEL_SOURCES+=bus_root.c
KERNEL_SOURCES+=device_internal.c
