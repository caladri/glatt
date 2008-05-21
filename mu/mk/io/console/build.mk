# $Id: build.mk,v 1.1 2008-05-21 06:42:57 juli Exp $

.PATH: ${KERNEL_ROOT}/io/console

# io/console
KERNEL_SOURCES+=console.c
KERNEL_SOURCES+=framebuffer.c
KERNEL_SOURCES+=framebuffer_font.c
