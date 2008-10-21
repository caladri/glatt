.if exists(${VENDOR_SRC}/gnu/binutils)
SUBDIRS+=	binutils
.endif
.if exists(${VENDOR_SRC}/gnu/gcc)
SUBDIRS+=	gcc
.endif
SUBDIRS+=	mu-config

.include "${GLATT_SRC}/aux/build/recurse.mk"
