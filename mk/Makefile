.if !defined(PLATFORM) || !defined(MK_CONFIG)
.error "Must define PLATFORM and MK_CONFIG.  Try using the top-level Makefile."
.endif

BUILDDIR?=obj.${PLATFORM}.${MK_CONFIG}

KERNEL_ROOT?=${.CURDIR}

config:
	@rm -rf ${BUILDDIR}
	@mkdir -p ${BUILDDIR}
	@(cd ${BUILDDIR} && mu-config ${KERNEL_ROOT} ${PLATFORM} ${MK_CONFIG})
	@echo 'Build directory has been set up:' ${BUILDDIR}

options:
	@mu-config -s ${KERNEL_ROOT} ${PLATFORM} ${MK_CONFIG} | grep '[-+]' | xargs
