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
	 * Get a physical page, get a virtual address, map it, write to it,
	 * read from it.
	 */
	paddr_t page_addr;
	vaddr_t vaddr;
	volatile uint64_t *p, *q;

	error = page_alloc(&kernel_vm, &page_addr);
	if (error != 0)
		panic("%s: page_alloc failed: %u", __func__, error);
	kcprintf("Allocated page: %p\n", (void *)page_addr);
	error = vm_alloc_address(&kernel_vm, &vaddr, 1);
	if (error != 0)
		panic("%s: vm_alloc_address failed: %u", __func__, error);
	kcprintf("Allocated virtual address: %p\n", (void *)vaddr);
	error = page_map(&kernel_vm, vaddr, page_addr);
	if (error != 0)
		panic("%s: page_map failed: %u", __func__, error);
	q = (volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, page_addr);
	*q = (uint64_t)XKPHYS_MAP(XKPHYS_UC, page_addr);
	ASSERT(*q == (uint64_t)XKPHYS_MAP(XKPHYS_UC, page_addr), "page content valid");
	p = (volatile uint64_t *)vaddr;
	kcprintf("Mapped virtual address: %p\n", (void *)p);
	*p = (uint64_t)XKPHYS_MAP(XKPHYS_UC, page_addr) + 3;
	ASSERT(*p == (uint64_t)XKPHYS_MAP(XKPHYS_UC, page_addr) + 3, "page content valid");
	error = vm_free_address(&kernel_vm, vaddr);
	if (error != 0)
		panic("%s: vm_free_address failed: %u", __func__, error);
	kcprintf("cpu%u: VM appears to work.\n", mp_whoami());

	error = vm_alloc(&kernel_vm, 10 * 1024 * 1024, &vaddr);
	if (error != 0)
		panic("%s: vm_alloc failed: %u", __func__, error);
	kcprintf("Allocated 10MB at %p\n", (void *)vaddr);
	memset((void *)vaddr, 0, 10 * 1024 * 1024);
	kcprintf("And we zeroed it, too!\n");
	error = vm_free(&kernel_vm, 10 * 1024 * 1024, vaddr);
	if (error != 0)
		panic("%s: vm_free failed: %u", __func__, error);

	error = vm_alloc(&kernel_vm, 4 * 1024 * 1024, &vaddr);
	if (error != 0)
		panic("%s: vm_alloc failed: %u", __func__, error);
	kcprintf("Allocated 4MB at %p\n", (void *)vaddr);
	memset((void *)vaddr, 0, 4 * 1024 * 1024);
	kcprintf("And we zeroed it, too!\n");
	error = vm_free(&kernel_vm, 4 * 1024 * 1024, vaddr);
	if (error != 0)
		panic("%s: vm_free failed: %u", __func__, error);

	/* XXX end testcode.  */

	/*
	 * XXX Create a task+thread for us and switch to it.
	 */
	db_enter();
}
