VENDOR=		gnu
PACKAGE=	gcc
VERSION=	4.2.4
DISTFILE=	${PACKAGE}-core-${VERSION}.tar.bz2
DISTDIR=	${PACKAGE}-${VERSION}

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--target=${TOOLCHAIN_TARGET}
CONFIGURE+=	--disable-libssp

USE_GNUMAKE=	true

.include "${GLATT_SRC}/aux/build/vendor.mk"
