###
# Defines build targets for vendor-provided software that uses its own build
# system.
###

.if !defined(GLATT_SRC)
.error "Must define GLATT_SRC!"
.endif

.include "${GLATT_SRC}/aux/build/vars.mk"

all::
	@echo '>>> Building vendor software...'

# XXX Check that variables are defined.

VENDOR_DIST=${VENDOR_SRC}/${VENDOR}/${PACKAGE}/${DISTFILE}
WORK_SRC=${.OBJDIR}/work/${DISTDIR}
BUILD_OBJ=${.OBJDIR}/build/${PACKAGE}
.if defined(USE_GNUMAKE)
BUILD_MAKE=gnumake
.else
BUILD_MAKE=make
.endif

pre-configure: ${VENDOR_DIST}
	@echo '>>> Unpacking '${PACKAGE}
	@rm -rf ${WORK_SRC}
	@mkdir -p ${.OBJDIR}/work
	@cd ${.OBJDIR}/work && tar jxf ${VENDOR_DIST}
	@[ -d ${WORK_SRC} ]

configure:
	@echo '>>> Configuring '${PACKAGE}
	@rm -rf ${BUILD_OBJ}
	@mkdir -p ${BUILD_OBJ}
	@cd ${BUILD_OBJ} && ${WORK_SRC}/configure ${CONFIGURE}

build:
	@echo '>>> Building '${PACKAGE}
	@cd ${BUILD_OBJ} && ${BUILD_MAKE} all

install:
	@echo '>>> Installing '${PACKAGE}
	@cd ${BUILD_OBJ} && ${BUILD_MAKE} install

clean:
	@echo '>>> Cleaning '${PACKAGE}
	@rm -rf ${.OBJDIR}/work
	@rm -rf ${.OBJDIR}/build

.for _target in configure build install clean
pre-${_target}:
${_target}: pre-${_target}
post-${_target}: ${_target}
.PHONY: pre-${_target} ${_target} post-${_target}
.endfor

build: post-configure

install: post-build

all:: build
	@echo '>>> Finished building vendor software.'
