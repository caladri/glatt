.if exists(${.CURDIR}/config.mk)
.include "${.CURDIR}/config.mk"
.endif

GLATT_SRC?=	${.CURDIR}
TOOLCHAIN_ROOT?=${.CURDIR}/../toolchain-root
PATH:=		${PATH}:${TOOLCHAIN_ROOT}/bin

all::
	@echo '>>> Top-level Glatt build system.'

###
# mk section
###
.if defined(MK_CONFIG)
MK_FLAGS+=CONFIG="${MK_CONFIG}"
.endif
MK_OBJ?=${.OBJDIR}/mk.obj
MK_FLAGS+=BUILDDIR="${MK_OBJ}"
.if defined(PLATFORM)
MK_FLAGS+=PLATFORM="${PLATFORM}"
.endif

TARGETS+=mk-config
mk-config-help:
	@echo 'Configures a build of the microkernel.'
	@echo 'usage: '${MAKE}' mk-config'
	@echo 'variables:'
	@echo '    MK_CONFIG    Configuration string for kernel, e.g.'
	@echo '                 MK_CONFIG=std+pci'
	@echo '    MK_OBJ       Directory to build in, e.g.'
	@echo '                 MK_OBJ=/usr/obj/mk.testmips'
	@echo '    PLATFORM     Platform to build for, e.g.'
	@echo '                 PLATFORM=testmips'
mk-config:
	${GO_MAKE:S,DST,mk,:S,TGT,config,} ${MK_FLAGS}

TARGETS+=mk-build
mk-build-help:
	@echo 'Performs a build of the microkernel that has been configured.'
	@echo 'usage: '${MAKE}' mk-build'
	@echo 'variables:'
	@echo '    MK_OBJ       Directory to build in, e.g.'
	@echo '                 MK_OBJ=/usr/obj/mk.testmips'
mk-build:
	${GO_MAKE:S,DST,${MK_OBJ},:S,TGT,kernel,} ${MK_FLAGS}

.PHONY: mk
TARGETS+=mk
mk-help:
	@echo 'Performs mk-config and then mk-build.'
	@echo 'usage: '${MAKE}' mk'
	@echo 'See mk-config-help and mk-build-help for more information.'
mk: mk-config mk-build

###
# toolchain section
###
TARGETS+=toolchain
toolchain-help:
	@echo 'Builds a complete set of compilers and build tools.'
	@echo 'usage: '${MAKE}' toolchain'
toolchain:
	@mkdir -p ${TOOLCHAIN_ROOT}
	${GO_MAKE:S,DST,aux/toolchain,:S,TGT,install,} PREFIX=${TOOLCHAIN_ROOT}
	${GO_MAKE:S,DST,aux/toolchain,:S,TGT,clean,}

.include "${GLATT_SRC}/aux/build/root.mk"
