PLATFORM?=testmips
KERNEL_ROOT=${.CURDIR}
CONFIG?=std+db+morder+network

all:
	@(cd ${KERNEL_ROOT}/../config &&				\
		${MAKE} clean mu-config)
	@[ -d obj.${PLATFORM} ] || mkdir -p obj.${PLATFORM}
	@(cd obj.${PLATFORM} &&						\
		${KERNEL_ROOT}/../config/mu-config ${KERNEL_ROOT} ${PLATFORM} ${CONFIG})
	@(cd obj.${PLATFORM} && ${MAKE})

clean:
	@rm -rf obj.${PLATFORM}
