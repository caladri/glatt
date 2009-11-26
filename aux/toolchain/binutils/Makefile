VENDOR=		gnu
PACKAGE=	binutils
VERSION=	2.20
DISTFILE=	${PACKAGE}-${VERSION}.tar.bz2
DISTDIR=	${PACKAGE}-${VERSION}

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--target=${TOOLCHAIN_TARGET}

USE_GNUMAKE=	true

.include "${GLATT_SRC}/aux/build/vendor.mk"
