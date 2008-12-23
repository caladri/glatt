VENDOR=		gnu
PACKAGE=	binutils
VERSION=	2.18
DISTFILE=	${PACKAGE}-${VERSION}.tar.bz2
DISTDIR=	${PACKAGE}-${VERSION}

BINUTILS_TARGET?=	mips64-gxemul-elf

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--target=${BINUTILS_TARGET}

USE_GNUMAKE=	true

.include "${GLATT_SRC}/aux/build/vendor.mk"
