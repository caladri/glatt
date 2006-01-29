# $Id: build.mk,v 1.3 2006-01-29 10:02:49 juli Exp $

.PATH: ${KERNEL_ROOT}/core
.PATH: ${KERNEL_ROOT}/io/device/console
.PATH: ${KERNEL_ROOT}/vm

# io/devices/console
KERNEL_SOURCES+=console.c
KERNEL_SOURCES+=framebuffer.c
KERNEL_SOURCES+=framebuffer_font.c

# vm
KERNEL_SOURCES+=page.c
KERNEL_SOURCES+=vm.c
