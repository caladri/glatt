# $Id: cpu.mk,v 1.1 2007-06-30 02:21:29 juli Exp $

#KERNEL_SOURCES+=start.S

KERNEL_CC=	mips64-gxemul-elf-gcc
KERNEL_AS=	mips64-gxemul-elf-as
KERNEL_LD=	mips64-gxemul-elf-ld
KERNEL_NM=	mips64-gxemul-elf-nm
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

KERNEL_SIMFLAGS+=-E ${KERNEL_MACHINE}
.if defined(KERNEL_SUBMACHINE)
KERNEL_SIMFLAGS+=-e ${KERNEL_SUBMACHINE}
.endif
.if defined(KERNEL_SIMCPUS)
KERNEL_SIMFLAGS+=-n ${KERNEL_SIMCPUS}
KERNEL_SIMFLAGS+=-R
.endif
.if defined(KERNEL_SIMCLOCKHZ)
KERNEL_SIMFLAGS+=-I ${KERNEL_SIMCLOCKHZ}
.endif
.if defined(DISPLAY)
KERNEL_SIMFLAGS+=-X
.endif
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
