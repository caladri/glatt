# $Id: build.mk,v 1.1 2008-05-21 06:42:57 juli Exp $

.PATH: ${KERNEL_ROOT}/db

# db
KERNEL_SOURCES+=db.c
KERNEL_SOURCES+=db_misc.c
KERNEL_SOURCES+=db_panic.c
KERNEL_SOURCES+=db_show.c
