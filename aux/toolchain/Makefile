##
# GCC and Binutils require GNU Make, so build it first.
##
.if exists(${VENDOR_SRC}/gnu/make)
SUBDIRS+=	gnumake
.endif

##
# GCC requires Binutils, so build it first.
##
.if exists(${VENDOR_SRC}/gnu/binutils)
SUBDIRS+=	binutils
.endif

##
# Build GCC after its dependencies.
##
.if exists(${VENDOR_SRC}/gnu/gcc)
SUBDIRS+=	gcc
.endif

##
# Build non-vendor toolchain bits.
##
SUBDIRS+=	mu-config

.include "${GLATT_SRC}/aux/build/recurse.mk"
