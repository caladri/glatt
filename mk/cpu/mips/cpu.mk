KERNEL_CC=	${TOOLCHAIN_TARGET}-gcc
KERNEL_AS=	${TOOLCHAIN_TARGET}-as
KERNEL_LD=	${TOOLCHAIN_TARGET}-ld
KERNEL_NM=	${TOOLCHAIN_TARGET}-nm
KERNEL_SIM=	gxemul

MIPS_ABI?=	n64
.if ${MIPS_ABI} == "o64"
MIPS_O64=	1
.elif ${MIPS_ABI} == "n64"
MIPS_N64=	1
.else
.error "Unsupported ABI ${MIPS_ABI}."
.endif

.if defined(MIPS_O64)
KERNEL_ABI=	-mabi=o64 -mlong64
.else
KERNEL_ABI=	-mabi=64
.endif
KERNEL_ENTRY=	start
KERNEL_CPU=	-mips3
.if defined(LITTLE_ENDIAN)
.if defined(MIPS_O64)
KERNEL_FORMAT=	elf32-littlemips
.else
KERNEL_FORMAT=	elf64-littlemips
.endif
KERNEL_CC+=	-EL
KERNEL_AS+=	-EL
KERNEL_LD+=	-EL
.else
.if defined(MIPS_O64)
KERNEL_FORMAT=	elf32-bigmips
.else
KERNEL_FORMAT=	elf64-bigmips
.endif
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
