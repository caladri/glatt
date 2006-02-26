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

static void platform_mp_start_one(void);

static struct spinlock startup_lock;

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
	vaddr_t stack;
	int error;

	spinlock_init(&startup_lock, "startup");

	ncpus = *TEST_MP_DEV_FUNCTION(TEST_MP_DEV_NCPUS);

	kcprintf("cpu%u: Starting %lu processors.\n", mp_whoami(), ncpus);
	*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_STARTADDR) =
		(uintptr_t)&platform_mp_start_one;
	spinlock_lock(&startup_lock);
	for (cpu = 0; cpu < ncpus; cpu++) {
		if (cpu == mp_whoami()) {
			kcprintf("cpu%u: bootstrap.\n", cpu);
			continue;
		}
		error = page_alloc_direct(&kernel_vm, &stack);
		if (error != 0)
			panic("%s: stack allocation failed: %u", __func__,
			      error);
		spinlock_unlock(&startup_lock);
		*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_STACK) = stack + PAGE_SIZE;
		*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_START) = cpu;
		spinlock_lock(&startup_lock);
	}
	spinlock_unlock(&startup_lock);
	kcprintf("cpu%u: Started processors, continuing.\n", mp_whoami());
	platform_mp_start_one();
}

static void
platform_mp_start_one(void)
{
	struct pcpu *pcpu;
	paddr_t pcpu_addr;
	int error;

	/* XXX */
	__asm __volatile ("dla $" STRING(gp) ", _gp" : : : "memory");

	spinlock_lock(&startup_lock);

	/* Allocate a page for persistent per-CPU data.  */
	error = page_alloc(&kernel_vm, &pcpu_addr);
	if (error != 0)
		panic("cpu%u: page allocate failed: %u", mp_whoami(), error);
	pcpu = (struct pcpu *)XKPHYS_MAP(XKPHYS_CNC, pcpu_addr);

	pcpu->pc_vm = &kernel_vm;
	/* Identify the CPU.  */
	pcpu->pc_cpuinfo = cpu_identify();

	spinlock_unlock(&startup_lock);

	/* WARNING: Everything below here is not serialized.  */

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
