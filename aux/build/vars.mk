SHARED_VARS+=	GLATT_SRC

VENDOR_SRC?=	${GLATT_SRC}/aux/vendor-src
SHARED_VARS+=	VENDOR_SRC

.if !defined(TOOLCHAIN_ROOT)
TOOLCHAIN_ROOT=	${GLATT_SRC}/../toolchain-root

PATH:=		${PATH}:${TOOLCHAIN_ROOT}/bin
.if defined(LD_LIBRARY_PATH)
LD_LIBRARY_PATH:=${LD_LIBRARY_PATH}:${TOOLCHAIN_ROOT}/lib
.else
LD_LIBRARY_PATH:=/lib:/usr/lib:/usr/local/lib:${TOOLCHAIN_ROOT}/lib
.endif
.endif

SHARED_VARS+=	TOOLCHAIN_ROOT
SHARED_VARS+=	PATH
SHARED_VARS+=	LD_LIBRARY_PATH

.if defined(PLATFORM)
.if ${PLATFORM} == "testmips" || ${PLATFORM} == "malta" || ${PLATFORM} == "octeon"
TOOLCHAIN_TARGET=mips64-glatt-elf
.elif ${PLATFORM} == "macppc"
TOOLCHAIN_TARGET=powerpc-glatt-elf
.else
.error "Target not supported."
.endif
.else
.if !defined(TOOLCHAIN_TARGET)
.error "Must have PLATFORM or TOOLCHAIN_TARGET."
.endif
.endif
SHARED_VARS+=	TOOLCHAIN_TARGET

.for _var in ${SHARED_VARS}
_SHARED_VARS+=${_var}="${${_var}}"
.endfor

GO_MAKE=	echo '>>> Entering DST.  Running '${MAKE}' 'TGT'.' &&	\
			cd DST && ${MAKE} ${_SHARED_VARS} TGT
