PLATFORM?=testmips
KERNEL_ROOT=${.CURDIR}
.if ${PLATFORM} == "testmips"
CONFIG?=std+db
.elif ${PLATFORM} == "malta"
CONFIG?=std+db+pci+gt
.else
.error "Unknown platform."
.endif

all:
	@(cd ${KERNEL_ROOT}/../config &&				\
		${MAKE} clean mu-config)
	@[ -d obj.${PLATFORM} ] || mkdir -p obj.${PLATFORM}
	@(cd obj.${PLATFORM} &&						\
		${KERNEL_ROOT}/../config/mu-config ${KERNEL_ROOT} ${PLATFORM} ${CONFIG})
	@(cd obj.${PLATFORM} && ${MAKE})

simulate: all
	@(cd obj.${PLATFORM} && ${MAKE} simulate)

clean:
	@rm -rf obj.${PLATFORM}
