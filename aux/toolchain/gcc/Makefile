VENDOR=		gnu
PACKAGE=	gcc
VERSION=	4.4.2
DISTFILE=	${PACKAGE}-core-${VERSION}.tar.bz2

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--target=${TOOLCHAIN_TARGET}
CONFIGURE+=	--disable-libssp

USE_GNUMAKE=	true

.include "${GLATT_SRC}/aux/build/vendor.mk"
