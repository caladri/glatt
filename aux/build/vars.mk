SHARED_VARS+=	GLATT_SRC

VENDOR_SRC?=	${GLATT_SRC}/../vendor-src
SHARED_VARS+=	VENDOR_SRC

SHARED_VARS+=	PATH

.for _var in ${SHARED_VARS}
_SHARED_VARS+=${_var}="${${_var}}"
.endfor

GO_MAKE=	@echo '>>> Entering DST.  Running '${MAKE}' 'TGT'.' &&	\
			cd DST && ${MAKE} ${_SHARED_VARS} TGT
