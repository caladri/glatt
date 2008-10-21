# Things that belong elsewhere pt 1
GNUMAKE?=gnumake

GCC=gcc-4.2.4
GCC_DIST=${VENDOR_SRC}/gnu/gcc/${GCC:S/-/-core-/}.tar.bz2
GCC_SRC=${.OBJDIR}/work/${GCC}
GCC_OBJ=${.OBJDIR}/build/${GCC}
GCC_TARGET=mips64-gxemul-elf

# Things that belong elsewhere pt 2
pre-configure: ${GCC_DIST}
	@echo '>>> Unpacking '${GCC}
	@rm -rf ${GCC_SRC}
	@mkdir -p ${.OBJDIR}/work
	@cd ${.OBJDIR}/work && tar jxf ${GCC_DIST}
	@[ -d ${GCC_SRC} ]

configure:
	@echo '>>> Configuring '${GCC}
	@rm -rf ${GCC_OBJ}
	@mkdir -p ${GCC_OBJ}
	@cd ${GCC_OBJ} && ${GCC_SRC}/configure --prefix=${PREFIX} --target=${GCC_TARGET} --disable-libssp

build:
	@echo '>>> Building '${GCC}
	@cd ${GCC_OBJ} && ${GNUMAKE} all

install:
	@echo '>>> Installing '${GCC}
	@cd ${GCC_OBJ} && ${GNUMAKE} install

clean:
	@echo '>>> Cleaning '${GCC}
	@rm -rf ${.OBJDIR}/work
	@rm -rf ${.OBJDIR}/build

.include "${GLATT_SRC}/aux/build/vendor.mk"
