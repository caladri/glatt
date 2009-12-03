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

.if !defined(PACKAGE)
.error "PACKAGE must be defined."
.endif

.if !defined(DISTFILE) || !defined(DISTDIR)
.if defined(VERSION)
DISTDIR?=	${PACKAGE}-${VERSION}
DISTFILE?=	${PACKAGE}-${VERSION}.tar.bz2
.else
.error "DISTFILE and DISTDIR must be defined or inferrable."
.endif
.endif

VENDOR_DIST=	${VENDOR_SRC}/${VENDOR}/${PACKAGE}/${DISTFILE}
WORK_SRC=	${.OBJDIR}/work/${DISTDIR}
BUILD_OBJ=	${.OBJDIR}/build/${PACKAGE}
.if defined(USE_GNUMAKE)
BUILD_MAKE=	gnumake
.else
BUILD_MAKE=	${MAKE}
.endif

pre-configure: ${VENDOR_DIST}
	@echo '>>> Unpacking '${PACKAGE}
	@rm -rf ${WORK_SRC}
	@mkdir -p ${.OBJDIR}/work
.if ${DISTFILE:T} == "bz2"
	@cd ${.OBJDIR}/work && tar jxf ${VENDOR_DIST}
.else
	@cd ${.OBJDIR}/work && tar zxf ${VENDOR_DIST}
.endif
	@[ -d ${WORK_SRC} ]
	@for _patch in ${PATCHES}; do echo '>>> Applying patch '$$_patch; cd ${.OBJDIR}/work && patch -p0 -i$$_patch; done

configure:
	@echo '>>> Configuring '${PACKAGE}
	@rm -rf ${BUILD_OBJ}
	@mkdir -p ${BUILD_OBJ}
	@cd ${BUILD_OBJ} && env ${CONFIGURE_ENV} ${WORK_SRC}/configure ${CONFIGURE}

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
