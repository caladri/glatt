VENDOR=		gnu
PACKAGE=	mpc
VERSION=	0.9
DISTFILE=	${PACKAGE}-${VERSION}.tar.gz

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--with-gmp=${PREFIX}
CONFIGURE+=	--with-mpfr=${PREFIX}

USE_GNUMAKE=	true

.include "${GLATT_SRC}/aux/build/vendor.mk"
