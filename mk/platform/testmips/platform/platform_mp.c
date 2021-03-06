#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/startup.h>
#include <cpu/tlb.h>
#include <io/bus/bus.h>
#include <vm/vm.h>
#include <vm/vm_page.h>

#define	TEST_MP_DEV_BASE	0x11000000

#define	TEST_MP_DEV_WHOAMI	0x0000
#define	TEST_MP_DEV_NCPUS	0x0010
#define	TEST_MP_DEV_START	0x0020
#define	TEST_MP_DEV_STARTADDR	0x0030
#define	TEST_MP_DEV_STACK	0x0070
#define	TEST_MP_DEV_MEMORY	0x0090
#define	TEST_MP_DEV_IPI_ONE	0x00a0
#define	TEST_MP_DEV_IPI_MANY	0x00b0
#define	TEST_MP_DEV_IPI_READ	0x00c0

#define	TEST_MP_DEV_FUNCTION(f)						\
	(volatile uint64_t *)XKPHYS_MAP(CCA_UC, TEST_MP_DEV_BASE + (f))
#define	TEST_MP_DEV_READ(f)						\
	(volatile uint64_t)*TEST_MP_DEV_FUNCTION(f)
#define	TEST_MP_DEV_WRITE(f, v)						\
	*TEST_MP_DEV_FUNCTION(f) = (v)

#define	TEST_MP_DEV_IPI_INTERRUPT	(6)

static struct bus_instance *platform_mp_bus;
static volatile paddr_t platform_mp_pcpu_addr;

static void platform_mp_attach_cpu(bool);
#ifndef	UNIPROCESSOR
static void platform_mp_ipi_interrupt(void *, int);
static void platform_mp_start_cpu(void);
static void platform_mp_start_one(cpu_id_t);
#endif

#ifndef	UNIPROCESSOR
void
platform_mp_ipi_send(cpu_id_t cpu, enum ipi_type ipi)
{
	TEST_MP_DEV_WRITE(TEST_MP_DEV_IPI_ONE, (ipi << 16) | cpu);
}

void
platform_mp_ipi_send_but(cpu_id_t cpu, enum ipi_type ipi)
{
	TEST_MP_DEV_WRITE(TEST_MP_DEV_IPI_MANY, (ipi << 16) | cpu);
}
#endif

size_t
platform_mp_memory(void)
{
	return ((size_t)TEST_MP_DEV_READ(TEST_MP_DEV_MEMORY));
}

#ifndef	UNIPROCESSOR
unsigned
platform_mp_ncpus(void)
{
	unsigned ncpus;

	ncpus = (unsigned)TEST_MP_DEV_READ(TEST_MP_DEV_NCPUS);
	if (ncpus > MAXCPUS)
		return (MAXCPUS);
	return (ncpus);
}
#endif

void
platform_mp_startup(void)
{
#ifndef	UNIPROCESSOR
	/*
	 * If the MP bus is not set up then we are the boot CPU and must mark
	 * ourselves both present and running.  The boot CPU will mark all other
	 * CPUs present, regardless of whether they run.
	 */
	if (platform_mp_bus == NULL) {
		mp_cpu_present(mp_whoami());
		mp_cpu_running(mp_whoami());
	}
#endif

	/*
	 * Attach a device for the CPU if the bus is set up already.
	 */
	if (platform_mp_bus != NULL)
		platform_mp_attach_cpu(false);
}

cpu_id_t
platform_mp_whoami(void)
{
	cpu_id_t me;

	me = (cpu_id_t)TEST_MP_DEV_READ(TEST_MP_DEV_WHOAMI);
	if (me >= MAXCPUS)
		panic("%s: cpu%u is above the system limit of %u CPUs and cannot run.", __func__, me, MAXCPUS);
	return (me);
}

#ifndef	UNIPROCESSOR
static void
platform_mp_ipi_interrupt(void *arg, int interrupt)
{
	uint64_t ipi;

	/*
	 * XXX
	 * Stab Anders.  The IPI source is lost.  We should encode the cpuid
	 * twice in the future.  Pity, it'd've been easy to store the whole
	 * idata in the pending queue, or something.
	 */
	while ((ipi = *TEST_MP_DEV_FUNCTION(TEST_MP_DEV_IPI_READ)) != IPI_NONE)
		mp_ipi_receive(ipi);
}
#endif

static void
platform_mp_start_all(void *arg)
{
#ifndef	UNIPROCESSOR
	cpu_id_t cpu;
	uint64_t ncpus;

	ncpus = mp_ncpus();
	if (ncpus > MAXCPUS) {
		bus_printf(platform_mp_bus, "Warning: system limit is %u CPUs, but there are %u attached!\n", MAXCPUS, ncpus);
		ncpus = MAXCPUS;
	}

	for (cpu = 0; cpu < ncpus; cpu++)
		if (cpu != mp_whoami())
			mp_cpu_present(cpu);

	if (ncpus != 1) {
		for (cpu = 0; cpu < ncpus; cpu++) {
			if (cpu == mp_whoami())
				platform_mp_attach_cpu(true);
			else
				platform_mp_start_one(cpu);
		}
		while (mp_cpu_running_mask() != mp_cpu_present_mask())
			continue;
	} else {
		platform_mp_attach_cpu(true);
	}
	/*
	 * Once all CPUs are ready to handle PCPU information, switch mp_whoami implementation.
	 */
	mp_whoami = mp_whoami_pcpu;
#else
	platform_mp_attach_cpu(true);
#endif
}
STARTUP_ITEM(platform_mp, STARTUP_MP, STARTUP_SECOND,
	     platform_mp_start_all, NULL);

#ifndef	UNIPROCESSOR
static void
platform_mp_start_cpu(void)
{
	paddr_t pcpu_addr;

	pcpu_addr = platform_mp_pcpu_addr;
	platform_mp_pcpu_addr = 0;

	cpu_startup(pcpu_addr);

	/* Clear BEV.  */
	cpu_write_status(cpu_read_status() & ~CP0_STATUS_BEV);

	cpu_interrupt_setup();

	/* Install an IPI interrupt handler.  */
	cpu_interrupt_establish(TEST_MP_DEV_IPI_INTERRUPT,
				platform_mp_ipi_interrupt, NULL);

	mp_cpu_running(mp_whoami());
	while (mp_cpu_running_mask() != mp_cpu_present_mask())
		continue;

	startup_main();
}

static void
platform_mp_start_one(cpu_id_t cpu)
{
	struct vm_page *pcpu_page;
	vaddr_t stack;
	int error;

#ifdef VERBOSE_DEBUG
	bus_printf(platform_mp_bus, "launching cpu%u...", cpu);
#endif

	error = page_alloc_direct(&kernel_vm, PAGE_FLAG_DEFAULT, &stack);
	if (error != 0)
		panic("%s: page_alloc_direct failed: %m", __func__, error);

	error = page_alloc(PAGE_FLAG_DEFAULT, &pcpu_page);
	if (error != 0)
		panic("%s: page_alloc failed: %m", __func__, error);

	ASSERT(platform_mp_pcpu_addr == 0,
	       "Must not have a page pending for PCPU usage.");
	platform_mp_pcpu_addr = page_address(pcpu_page);

	/*
	 * XXX
	 * Abusing startup_early here is awful.  Checking whether the
	 * CPU is running seems better?
	 */
	startup_early = true;
	TEST_MP_DEV_WRITE(TEST_MP_DEV_STARTADDR,
			  (uintptr_t)platform_mp_start_cpu);
	/*
	 * NB:
	 * Would like to use 'tempstack' for all CPUs, but that means waiting
	 * until each is running threads, not just !startup_early.
	 */
	TEST_MP_DEV_WRITE(TEST_MP_DEV_STACK, stack + PAGE_SIZE);
	TEST_MP_DEV_WRITE(TEST_MP_DEV_START, cpu);

	/*
	 * Loop until startup_early is false again and this CPU is up.
	 */
	while (startup_early)
		continue;

#ifdef VERBOSE_DEBUG
	bus_printf(platform_mp_bus, "launched cpu%u.", cpu);
#endif
}
#endif

static void
platform_mp_attach_cpu(bool bootstrap)
{
	int error;

	ASSERT(platform_mp_bus != NULL, "need mp bus to attach CPUs.");

	if (bootstrap)
		PCPU_SET(flags, PCPU_GET(flags) | PCPU_FLAG_BOOTSTRAP);

	/* Add CPU device.  */
	error = bus_enumerate_child_generic(platform_mp_bus, "cpu");
	if (error != 0)
		panic("%s: bus_enumerate_child_generic failed: %m", __func__,
		      error);

	if (bootstrap) {
		/* Install an IPI interrupt handler.  */
		cpu_interrupt_establish(TEST_MP_DEV_IPI_INTERRUPT,
					platform_mp_ipi_interrupt, NULL);

		error = bus_enumerate_child_generic(platform_mp_bus, "mpbus");
		if (error != 0)
			panic("%s: bus_enumerate_child_generic failed: %m",
			      __func__, error);
	}
}

static int
platform_mp_enumerate(struct bus_instance *bi)
{
	return (0);
}

static int
platform_mp_setup(struct bus_instance *bi)
{
	uint64_t ncpus;

	ASSERT(platform_mp_bus == NULL,
	       "Can only have one mp instance.");
	platform_mp_bus = bi;

	ncpus = TEST_MP_DEV_READ(TEST_MP_DEV_NCPUS);
	if (ncpus == 1)
		bus_set_description(bi, "uniprocessor system");
	else
		bus_set_description(bi, "multiprocessor system with %lu CPUs",
				    ncpus);

	return (0);
}

BUS_INTERFACE(mpif) {
	.bus_enumerate_children = platform_mp_enumerate,
	.bus_setup = platform_mp_setup,
};
BUS_ATTACHMENT(mp, "mainbus", mpif);
