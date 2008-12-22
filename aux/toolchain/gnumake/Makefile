GNUMAKE=make-3.81
GNUMAKE_DIST=${VENDOR_SRC}/gnu/make/${GNUMAKE}.tar.bz2
GNUMAKE_SRC=${.OBJDIR}/work/${GNUMAKE}
GNUMAKE_OBJ=${.OBJDIR}/build/${GNUMAKE}

# Things that belong elsewhere pt 2
pre-configure: ${GNUMAKE_DIST}
	@echo '>>> Unpacking '${GNUMAKE}
	@rm -rf ${GNUMAKE_SRC}
	@mkdir -p ${.OBJDIR}/work
	@cd ${.OBJDIR}/work && tar jxf ${GNUMAKE_DIST}
	exit
	@[ -d ${GNUMAKE_SRC} ]

configure:
	@echo '>>> Configuring '${GNUMAKE}
	@rm -rf ${GNUMAKE_OBJ}
	@mkdir -p ${GNUMAKE_OBJ}
	@cd ${GNUMAKE_OBJ} && ${GNUMAKE_SRC}/configure --prefix=${PREFIX} --program-prefix=gnu

build:
	@echo '>>> Building '${GNUMAKE}
	@cd ${GNUMAKE_OBJ} && make all

install:
	@echo '>>> Installing '${GNUMAKE}
	@cd ${GNUMAKE_OBJ} && make install

clean:
	@echo '>>> Cleaning '${GNUMAKE}
	@rm -rf ${.OBJDIR}/work
	@rm -rf ${.OBJDIR}/build

.include "${GLATT_SRC}/aux/build/vendor.mk"
