# $Id: cpu.mk,v 1.19 2006-03-07 02:23:27 juli Exp $

.PATH: ${CPU_ROOT}/cpu
.PATH: ${CPU_ROOT}/page

KERNEL_SOURCES+=cpu_context.S
KERNEL_SOURCES+=cpu_critical.c
KERNEL_SOURCES+=cpu_exception.c
KERNEL_SOURCES+=cpu_identify.c
KERNEL_SOURCES+=cpu_interrupt.c
KERNEL_SOURCES+=cpu_mp.c
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
KERNEL_MACHINE=	testmips

KERNEL_SIMFLAGS=-E ${KERNEL_MACHINE} -q
.if defined(DISPLAY)
KERNEL_SIMFLAGS+=-X
.endif

simulate-${KERNEL}: ${KERNEL}
	${KERNEL_SIM} ${KERNEL_SIMFLAGS} ${KERNEL}

.if make(simulate-${KERNEL}) || make(simulate)
.INTERRUPT: 
	killall ${KERNEL_SIM}
.endif
