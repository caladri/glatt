.if !defined(PLATFORM) || !defined(CONFIG)
.error "Must define PLATFORM and CONFIG.  Try using the top-level Makefile."
.endif

BUILDDIR?=obj.${PLATFORM}.${CONFIG}

KERNEL_ROOT?=${.CURDIR}

config:
	@rm -rf ${BUILDDIR}
	@mkdir -p ${BUILDDIR}
	@(cd ${BUILDDIR} && mu-config ${KERNEL_ROOT} ${PLATFORM} ${CONFIG})
	@echo 'Build directory has been set up:' ${BUILDDIR}

options:
	@mu-config -s ${KERNEL_ROOT} ${PLATFORM} | grep '[-+]' | xargs
