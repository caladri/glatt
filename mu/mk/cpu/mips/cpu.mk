# $Id: cpu.mk,v 1.27 2007-06-19 02:19:43 juli Exp $

.PATH: ${CPU_ROOT}/cpu
.PATH: ${CPU_ROOT}/page

KERNEL_SOURCES+=cpu_context.S
KERNEL_SOURCES+=cpu_clock_r4k.c
KERNEL_SOURCES+=cpu_critical.c
KERNEL_SOURCES+=cpu_device.c
KERNEL_SOURCES+=cpu_exception.c
KERNEL_SOURCES+=cpu_identify.c
KERNEL_SOURCES+=cpu_interrupt.c
.if !defined(UNIPROCESSOR)
KERNEL_SOURCES+=cpu_mp.c
.endif
KERNEL_SOURCES+=cpu_scheduler.c
KERNEL_SOURCES+=cpu_startup.c
KERNEL_SOURCES+=cpu_task.c
KERNEL_SOURCES+=cpu_thread.c
KERNEL_SOURCES+=cpu_tlb.c
KERNEL_SOURCES+=cpu_vector.S
KERNEL_SOURCES+=page_map.c
KERNEL_SOURCES+=start.S

KERNEL_CC=	mips64-gxemul-elf-gcc
KERNEL_AS=	mips64-gxemul-elf-as
KERNEL_LD=	mips64-gxemul-elf-ld
KERNEL_NM=	mips64-gxemul-elf-nm
KERNEL_SIM=	gxemul

KERNEL_ABI=	-mabi=64
KERNEL_ENTRY=	start
KERNEL_CPU=	-mips3
KERNEL_FORMAT=	elf64-bigmips

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
