KERNEL_CC=	${TOOLCHAIN_TARGET}-gcc
KERNEL_AS=	${TOOLCHAIN_TARGET}-as
KERNEL_LD=	${TOOLCHAIN_TARGET}-ld
KERNEL_NM=	${TOOLCHAIN_TARGET}-nm
KERNEL_SIM=	gxemul

KERNEL_ABI=	-mabi=64
KERNEL_ENTRY=	start
KERNEL_CPU=	-mips3
.if defined(LITTLE_ENDIAN)
KERNEL_FORMAT=	elf64-littlemips
KERNEL_CC+=	-EL
KERNEL_AS+=	-EL
KERNEL_LD+=	-EL
.else
KERNEL_FORMAT=	elf64-bigmips
.endif

# Ask GCC to not use the GP, we may well end up with too much GP-relative
# data to address.  In reality, though, we shouldn't have enough global data
# for this to make sense, and we should be able to turn it back on.
KERNEL_CFLAGS+=	-G0

KERNEL_SIMFLAGS+=-S
KERNEL_SIMFLAGS+=-E ${KERNEL_MACHINE}
.if defined(KERNEL_SUBMACHINE)
KERNEL_SIMFLAGS+=-e ${KERNEL_SUBMACHINE}
.endif
.if defined(KERNEL_SIMCPUS)
KERNEL_SIMFLAGS+=-n ${KERNEL_SIMCPUS}
.if defined(KERNEL_RANDOMBOOTSTRAPCPU)
KERNEL_SIMFLAGS+=-R
.endif
.endif
.if defined(KERNEL_SIMCLOCKHZ)
KERNEL_SIMFLAGS+=-I ${KERNEL_SIMCLOCKHZ}
.endif
.for simdisk in ${KERNEL_SIMDISKS}
KERNEL_SIMFLAGS+=-d ${simdisk}
.endfor
#.if defined(DISPLAY)
#KERNEL_SIMFLAGS+=-X
#.endif
.if defined(KERNEL_SIMVERBOSE)
KERNEL_SIMFLAGS+=-v
.else
KERNEL_SIMFLAGS+=-q
.endif

simulate-${KERNEL}: ${KERNEL}
	${KERNEL_SIM} ${KERNEL_SIMFLAGS} ${KERNEL}

.if make(simulate-${KERNEL}) || make(simulate)
.INTERRUPT:
	killall ${KERNEL_SIM}
.endif
