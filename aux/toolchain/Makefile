##
# Figure out what components to build.
##
.if !defined(WITHOUT_BINUTILS) && exists(${VENDOR_SRC}/gnu/binutils)
.if !exists(${PREFIX}/bin/${TOOLCHAIN_TARGET}-ld)
WANT_BINUTILS=	t
.endif
.endif

.if !defined(WITHOUT_GCC) && exists(${VENDOR_SRC}/gnu/gcc)
.if !exists(${PREFIX}/bin/${TOOLCHAIN_TARGET}-gcc)
WANT_GCC=	t
.endif
.endif

.if defined(WANT_BINUTILS) || defined(WANT_GCC)
.if !exists(${VENDOR_SRC}/gnu/make)
.error "Building GNU binutils or GCC requires GNU Make!  Update vendor-src."
.endif
.if !exists(${PREFIX}/bin/gnumake)
NEED_GNUMAKE=	t
.endif
.endif

##
# GCC and Binutils require GNU Make, so build it first.
##
.if defined(NEED_GNUMAKE)
SUBDIRS+=	gnumake
.endif

##
# GCC requires Binutils, so build it first.
##
.if defined(WANT_BINUTILS)
SUBDIRS+=	binutils
.endif

##
# Build GCC after its dependencies.
##
.if defined(WANT_BINUTILS)
SUBDIRS+=	gcc
.endif

##
# Build non-vendor toolchain bits.
##
SUBDIRS+=	mu-config

.include "${GLATT_SRC}/aux/build/recurse.mk"
