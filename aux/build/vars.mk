VENDOR_SRC?=	${GLATT_SRC}/../vendor-src

GO_MAKE=	@echo '>>> Entering DST.  Running '${MAKE}' 'TGT'.' &&	\
			cd DST && ${MAKE} GLATT_SRC="${GLATT_SRC}" TGT
