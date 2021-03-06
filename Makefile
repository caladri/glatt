.if exists(${.CURDIR}/config.mk)
.include "${.CURDIR}/config.mk"
.endif

GLATT_SRC?=	${.CURDIR}

all::
	@echo '>>> Top-level Glatt build system.'
	@echo '>>> If a file named config.mk exists, it will be included.'

###
# mk section
###
MK_CONFIG?=std
MK_FLAGS+=MK_CONFIG="${MK_CONFIG}"
MK_OBJ?=${.OBJDIR}/mk.obj
MK_FLAGS+=BUILDDIR="${MK_OBJ}"
.if defined(PLATFORM)
MK_FLAGS+=PLATFORM="${PLATFORM}"
.endif
.if defined(KERNEL_SIMCPUS)
MK_FLAGS+=KERNEL_SIMCPUS="${KERNEL_SIMCPUS}"
.endif
.if defined(KERNEL_SIMDISKS)
MK_FLAGS+=KERNEL_SIMDISKS="${KERNEL_SIMDISKS}"
.endif
.if defined(KERNEL_SIMRAM)
MK_FLAGS+=KERNEL_SIMRAM="${KERNEL_SIMRAM}"
.endif
.if defined(KERNEL_SIMVERBOSE)
MK_FLAGS+=KERNEL_SIMVERBOSE="${KERNEL_SIMVERBOSE}"
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
	@${GO_MAKE:S,DST,mk,:S,TGT,config,} ${MK_FLAGS}

TARGETS+=mk-build
mk-build-help:
	@echo 'Performs a build of the microkernel that has been configured.'
	@echo 'usage: '${MAKE}' mk-build'
	@echo 'variables:'
	@echo '    MK_OBJ       Directory to build in, e.g.'
	@echo '                 MK_OBJ=/usr/obj/mk.testmips'
mk-build:
	@if [ -e ${MK_OBJ}/Makefile ]; then				\
		${GO_MAKE:S,DST,${MK_OBJ},:S,TGT,kernel,} ${MK_FLAGS} ;	\
	else								\
		echo '>>> Please run '${MAKE}' mk-config first.' ;	\
	fi

TARGETS+=mk-clean
mk-clean-help:
	@echo 'Cleans a build of the microkernel using '${MAKE}' clean.'
	@echo 'usage: '${MAKE}' mk-clean'
	@echo 'variables:'
	@echo '    MK_OBJ       Directory to build in, e.g.'
	@echo '                 MK_OBJ=/usr/obj/mk.testmips'
mk-clean:
	@if [ -e ${MK_OBJ}/Makefile ]; then				\
		${GO_MAKE:S,DST,${MK_OBJ},:S,TGT,clean,} ${MK_FLAGS} ;	\
	else								\
		echo '>>> Already clean.' ;				\
	fi

TARGETS+=mk-options
mk-options-help:
	@echo 'Shows available configuration options for the microkernel.'
	@echo 'usage: '${MAKE}' mk-options'
mk-options:
	@${GO_MAKE:S,DST,mk,:S,TGT,options,} ${MK_FLAGS}

TARGETS+=mk-reconfig
mk-reconfig-help:
	@echo 'Performs a reconfig of the microkernel in its build directory.'
	@echo 'usage: '${MAKE}' mk-reconfig'
	@echo 'variables:'
	@echo '    MK_OBJ       Directory to build in, e.g.'
	@echo '                 MK_OBJ=/usr/obj/mk.testmips'
mk-reconfig:
	@if [ -e ${MK_OBJ}/Makefile ]; then				\
		${GO_MAKE:S,DST,${MK_OBJ},:S,TGT,reconfig,} ${MK_FLAGS} ;\
	else								\
		echo '>>> Please run '${MAKE}' mk-config instead.' ;	\
	fi

TARGETS+=mk-simulate
mk-simulate-help:
	@echo 'Performs a simulation of the microkernel in its build directory.'
	@echo 'usage: '${MAKE}' mk-simulate'
	@echo 'variables:'
	@echo '    MK_OBJ       Directory to build in, e.g.'
	@echo '                 MK_OBJ=/usr/obj/mk.testmips'
	@echo '    KERNEL_SIMDISKS'
	@echo '                 Disk images to use with simulator, e.g.'
	@echo '                 KERNEL_SIMDISKS=/usr/obj/mu.img'
	@echo '    KERNEL_SIMRAM'
	@echo '                 Amount of RAM (in MB) to use with simulator, e.g.'
	@echo '                 KERNEL_SIMRAM=128'
	@echo '    KERNEL_SIMVERBOSE'
	@echo '                 If set, enables verbose simulation.'
mk-simulate:
	@if [ -e ${MK_OBJ}/Makefile ]; then				\
		${GO_MAKE:S,DST,${MK_OBJ},:S,TGT,simulate,} ${MK_FLAGS} ;\
	else								\
		echo '>>> Please run '${MAKE}' mk-config first.' ;	\
	fi

TARGETS+=mk-universe
mk-universe-help:
	@echo 'Attempts to build the microkernel for all platforms.'
	@echo 'usage: '${MAKE}' mk-universe'
mk-universe:
.for _platform in macppc malta octeon testmips
	@cd ${.CURDIR} && ${MAKE} toolchain PLATFORM=${_platform}
.for _config in std std+db std+verbose std+invariants std-ns std+fs+exec
	@cd ${.CURDIR} && ${MAKE} mk PLATFORM=${_platform} MK_CONFIG=${_config}
.endfor
	@cd ${.CURDIR} && ${MAKE} mk-clean
.endfor

.PHONY: mk
TARGETS+=mk
mk-help:
	@echo 'Performs mk-config and then mk-build.'
	@echo 'usage: '${MAKE}' mk'
	@echo 'See mk-config-help and mk-build-help for more information.'
mk: mk-config mk-build

###
# mu section
###
mu-build:
	@${GO_MAKE:S,DST,mu,:S,TGT,all,}

mu-clean:
	@${GO_MAKE:S,DST,mu,:S,TGT,clean,}

.if make(mu-install) && !defined(DESTDIR)
.error "Must set DESTDIR for mu-install."
.endif
mu-install:
	@${GO_MAKE:S,DST,mu,:S,TGT,install,} DESTDIR=${DESTDIR}

mu-image:
	@${GO_MAKE:S,DST,${.CURDIR},:S,TGT,mu-build,}
	(cd ${.CURDIR} && sh mu-image.sh)

.PHONY: mu
TARGETS+=mu
mu-help:
	@echo 'Performs mu-clean and then mu-image.'
	@echo 'usage: '${MAKE}' mu'
	@echo 'See mu-clean-help and mu-image-help for more information.'
mu: mu-clean mu-image


###
# toolchain section
###
TARGETS+=toolchain
toolchain-help:
	@echo 'Builds a complete set of compilers and build tools.'
	@echo 'usage: '${MAKE}' toolchain'
toolchain:
	@mkdir -p ${TOOLCHAIN_ROOT}
	@${GO_MAKE:S,DST,aux/toolchain,:S,TGT,install,} PREFIX=${TOOLCHAIN_ROOT}
	@${GO_MAKE:S,DST,aux/toolchain,:S,TGT,clean,}

.include "${GLATT_SRC}/aux/build/root.mk"
