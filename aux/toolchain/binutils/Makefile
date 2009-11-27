VENDOR=		gnu
PACKAGE=	binutils
VERSION=	2.20

CONFIGURE+=	--prefix=${PREFIX}
CONFIGURE+=	--target=${TOOLCHAIN_TARGET}
CONFIGURE+=	--disable-werror

CONFIGURE_ENV+=	CFLAGS='-Wno-format'

USE_GNUMAKE=	true

.if ${TOOLCHAIN_TARGET} == "mips64-glatt-elf"
PATCHES+=	${.CURDIR}/binutils-o64.diff
.endif

.include "${GLATT_SRC}/aux/build/vendor.mk"
