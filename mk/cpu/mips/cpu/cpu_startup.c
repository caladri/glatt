#include <core/types.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/startup.h>
#include <cpu/tlb.h>
#ifdef DB
#include <db/db_command.h>
#endif
#include <vm/vm.h>
#include <vm/vm_page.h>

#ifdef DB
DB_COMMAND_TREE(cpu, root, cpu);
#endif

volatile bool startup_early = true;

COMPILE_TIME_ASSERT(sizeof (struct pcpu) <= PAGE_SIZE);

void
cpu_break(void)
{
	asm volatile ("break 7" : : : "memory");
	for (;;) continue;
}

void
cpu_halt(void)
{
	platform_halt();
}

void
cpu_startup(void)
{
	struct pcpu *pcpu;
	struct vm_page *pcpu_page;
	paddr_t pcpu_addr;
	int error;

	/*
	 * We don't use the gp, set it to NULL.
	 */
	asm volatile ("move $" STRING(gp) ", $" STRING(zero) : : : "memory");

	/*
	 * Set kernel mode.
	 */
	cpu_write_status(KERNEL_STATUS);

	/* Allocate a page for persistent per-CPU data.  */
	error = page_alloc(PAGE_FLAG_DEFAULT | PAGE_FLAG_ZERO, &pcpu_page);
	if (error != 0)
		panic("cpu%u: page allocate failed: %m", mp_whoami(), error);
	pcpu_addr = page_address(pcpu_page);
	pcpu = (struct pcpu *)XKPHYS_MAP(XKPHYS_CNC, pcpu_addr);

	/* Keep our original physical address in the PCPU data, too.  */
	pcpu->pc_physaddr = pcpu;

	/* Identify the CPU.  */
	pcpu->pc_cpuinfo = cpu_identify();
	pcpu->pc_cpuid = mp_whoami();

	/* Clear the TLB and add a wired mapping for my per-CPU data.  */
	tlb_init(kernel_vm.vm_pmap, pcpu_addr);

	/* Now we can take VM-related exceptions appropriately.  */
	pcpu->pc_flags = PCPU_FLAG_RUNNING;
	startup_early = false;

	/* Return to the platform code.  */
}

void
cpu_startup_thread(void *arg)
{
	void (*callback)(void *);

	callback = (void (*)(void *))(uintptr_t)arg;
	platform_startup_thread();
	callback(NULL);
}
