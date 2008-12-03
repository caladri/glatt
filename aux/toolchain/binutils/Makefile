BINUTILS=binutils-2.18
BINUTILS_DIST=${VENDOR_SRC}/gnu/binutils/${BINUTILS}.tar.bz2
BINUTILS_SRC=${.OBJDIR}/work/${BINUTILS}
BINUTILS_OBJ=${.OBJDIR}/build/${BINUTILS}
BINUTILS_TARGET=mips64-gxemul-elf

# Things that belong elsewhere pt 2
pre-configure: ${BINUTILS_DIST}
	@echo '>>> Unpacking '${BINUTILS}
	@rm -rf ${BINUTILS_SRC}
	@mkdir -p ${.OBJDIR}/work
	@cd ${.OBJDIR}/work && tar jxf ${BINUTILS_DIST}
	@[ -d ${BINUTILS_SRC} ]

configure:
	@echo '>>> Configuring '${BINUTILS}
	@rm -rf ${BINUTILS_OBJ}
	@mkdir -p ${BINUTILS_OBJ}
	@cd ${BINUTILS_OBJ} && ${BINUTILS_SRC}/configure --prefix=${PREFIX} --target=${BINUTILS_TARGET}

build:
	@echo '>>> Building '${BINUTILS}
	@cd ${BINUTILS_OBJ} && ${GNUMAKE} all

install:
	@echo '>>> Installing '${BINUTILS}
	@cd ${BINUTILS_OBJ} && ${GNUMAKE} install

clean:
	@echo '>>> Cleaning '${BINUTILS}
	@rm -rf ${.OBJDIR}/work
	@rm -rf ${.OBJDIR}/build

.include "${GLATT_SRC}/aux/build/vendor.mk"
