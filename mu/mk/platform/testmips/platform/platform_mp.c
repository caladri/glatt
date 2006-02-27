#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/string.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/tlb.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

#define	TEST_MP_DEV_BASE	0x11000000

#define	TEST_MP_DEV_WHOAMI	0x0000
#define	TEST_MP_DEV_NCPUS	0x0010
#define	TEST_MP_DEV_START	0x0020
#define	TEST_MP_DEV_STARTADDR	0x0030
#define	TEST_MP_DEV_STACK	0x0070
#define	TEST_MP_DEV_MEMORY	0x0090

#define	TEST_MP_DEV_FUNCTION(f)						\
	(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, TEST_MP_DEV_BASE + (f))

static void platform_mp_start_one(cpu_id_t, void (*)(void));
static void platform_mp_startup(void);

size_t
platform_mp_memory(void)
{
	return ((size_t)*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_MEMORY));
}

cpu_id_t
platform_mp_whoami(void)
{
	return (*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_WHOAMI));
}

void
platform_mp_start_all(void)
{
	cpu_id_t cpu;
	uint64_t ncpus;

	ncpus = *TEST_MP_DEV_FUNCTION(TEST_MP_DEV_NCPUS);
	if (ncpus != 1) {
		kcprintf("cpu%u: multiprocessor system detected,"
			 " starting %lu processors.\n", mp_whoami(), ncpus);
		for (cpu = 0; cpu < ncpus; cpu++) {
			if (cpu == mp_whoami()) {
				kcprintf("cpu%u: bootstrap, skipping\n", cpu);
				continue;
			}
			platform_mp_start_one(cpu, platform_mp_startup);
		}
		kcprintf("cpu%u: started processors.\n", mp_whoami());
	} else {
		kcprintf("cpu%u: uniprocessor system detected.\n", mp_whoami());
	}
	platform_mp_start_one(mp_whoami(), platform_mp_startup);
}

static void
platform_mp_start_one(cpu_id_t cpu, void (*startup)(void))
{
	vaddr_t stack;
	int error;

	error = page_alloc_direct(&kernel_vm, &stack);
	if (error != 0)
		panic("%s: page_alloc_direct failed: %u", __func__, error);
	*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_STARTADDR) = (uintptr_t)startup;
	*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_STACK) = stack + PAGE_SIZE;
	*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_START) = cpu;
}

static void
platform_mp_startup(void)
{
	struct pcpu *pcpu;
	paddr_t pcpu_addr;
	int error;

	/* XXX */
	__asm __volatile ("dla $" STRING(gp) ", _gp" : : : "memory");

	/* Allocate a page for persistent per-CPU data.  */
	error = page_alloc(&kernel_vm, &pcpu_addr);
	if (error != 0)
		panic("cpu%u: page allocate failed: %u", mp_whoami(), error);
	pcpu = (struct pcpu *)XKPHYS_MAP(XKPHYS_CNC, pcpu_addr);

	pcpu->pc_vm = &kernel_vm;
	/* Identify the CPU.  */
	pcpu->pc_cpuinfo = cpu_identify();

	/* Clear the TLB and add a wired mapping for my per-CPU data.  */
	tlb_init(pcpu_addr);

	/* Now we can take VM-related exceptions appropriately.  */

	/* Kick off interrupts.  */
	cpu_interrupt_enable();

	/*
	 * XXX Create a task+thread for us and switch to it.
	 */
	db_enter();
}
