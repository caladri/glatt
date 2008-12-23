VENDOR=		gnu
PACKAGE=	gcc
VERSION=	4.2.4
DISTFILE=	${PACKAGE}-core-${VERSION}.tar.bz2
DISTDIR=	${PACKAGE}-${VERSION}

GCC_TARGET?=	mips64-gxemul-elf

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--target=${GCC_TARGET}
CONFIGURE+=	--disable-libssp

USE_GNUMAKE=	true

.include "${GLATT_SRC}/aux/build/vendor.mk"
