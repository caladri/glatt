VENDOR=		gnu
PACKAGE=	gcc
VERSION=	4.8.1
DISTFILE=	${PACKAGE}-${VERSION}.tar.bz2

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--target=${TOOLCHAIN_TARGET}
CONFIGURE+=	--disable-libssp
CONFGIURE+=	--with-gmp=${PREFIX}
CONFGIURE+=	--with-mpfr=${PREFIX}
CONFGIURE+=	--with-mpc=${PREFIX}
CONFIGURE+=	--enable-languages=c

# XXX ac_compile for the gmp and mpfr tests is broken.
CONFIGURE_ENV+=	CFLAGS="-I${PREFIX}/include"
CONFIGURE_ENV+=	LDFLAGS="-L${PREFIX}/lib"

# XXX portability issues.
CONFIGURE_ENV+=	MAKEINFO=missing

USE_GNUMAKE=	true

.include "${GLATT_SRC}/aux/build/vendor.mk"
