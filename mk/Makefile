PLATFORM?=testmips
KERNEL_ROOT=${.CURDIR}
.if ${PLATFORM} == "testmips"
CONFIG?=std
.elif ${PLATFORM} == "malta"
CONFIG?=std+pci+gt
.else
.error "Unknown platform."
.endif
BUILDDIR?=obj.${PLATFORM}.${CONFIG}

config:
	@rm -rf ${BUILDDIR}
	@mkdir -p ${BUILDDIR}
	@(cd ${BUILDDIR} && mu-config ${KERNEL_ROOT} ${PLATFORM} ${CONFIG})
	@echo 'Build directory has been set up:' ${BUILDDIR}
