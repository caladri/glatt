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

.if !exists(${VENDOR_SRC}/gnu/libiconv)
.error "Building GNU binutils or GCC requires libiconv!  Update vendor-src."
.endif
.if !exists(${PREFIX}/include/iconv.h)
NEED_LIBICONV=	t
.endif
.endif

.if defined(WANT_GCC)
.if !exists(${VENDOR_SRC}/gnu/gmp)
.error "Building GCC requires GMP!  Update vendor-src."
.endif
.if !exists(${PREFIX}/include/gmp.h)
NEED_GMP=	t
.endif

.if !exists(${VENDOR_SRC}/gnu/mpfr)
.error "Building GCC requires MPFR!  Update vendor-src."
.endif
.if !exists(${PREFIX}/include/mpfr.h)
NEED_MPFR=	t
.endif
.endif

##
# GCC and Binutils require GNU Make and libiconv, so build them first.
##
.if defined(NEED_GNUMAKE)
SUBDIRS+=	gnumake
.endif

.if defined(NEED_LIBICONV)
SUBDIRS+=	libiconv
.endif

##
# GCC requires GMP and MPFR, so build them first.
##
.if defined(NEED_GMP)
SUBDIRS+=	gmp
.endif

.if defined(NEED_MPFR)
SUBDIRS+=	mpfr
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
.if defined(WANT_GCC)
SUBDIRS+=	gcc
.endif

##
# Build non-vendor toolchain bits.
##
SUBDIRS+=	mu-config

.include "${GLATT_SRC}/aux/build/recurse.mk"
