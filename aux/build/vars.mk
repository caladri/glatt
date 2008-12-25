SHARED_VARS+=	GLATT_SRC

VENDOR_SRC?=	${GLATT_SRC}/../vendor-src
SHARED_VARS+=	VENDOR_SRC

SHARED_VARS+=	PATH

.if defined(PLATFORM)
.if ${PLATFORM} == "testmips" || ${PLATFORM} == "malta"
TOOLCHAIN_TARGET=mips64-glatt-elf
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
