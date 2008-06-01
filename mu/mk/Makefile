PLATFORM?=testmips
KERNEL_ROOT=${.CURDIR}
.if ${PLATFORM} == "testmips"
CONFIG?=std+db
.elif ${PLATFORM} == "malta"
CONFIG?=std+db+pci+gt
.else
.error "Unknown platform."
.endif
BUILDDIR=obj.${PLATFORM}.${CONFIG}

config:
	@rm -rf ${BUILDDIR}
	@(cd ${KERNEL_ROOT}/../config &&				\
		${MAKE} clean mu-config > /dev/null ||			\
		echo 'Could not build mu-config:' $?)
	@mkdir -p ${BUILDDIR}
	@(cd ${BUILDDIR} &&						\
		${KERNEL_ROOT}/../config/mu-config			\
			${KERNEL_ROOT} ${PLATFORM} ${CONFIG})
	@echo 'Build directory has been set up:' ${BUILDDIR}
