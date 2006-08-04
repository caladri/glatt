#include <core/types.h>
#include <core/startup.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/tlb.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

COMPILE_TIME_ASSERT(sizeof (struct pcpu) <= PAGE_SIZE);

void
cpu_halt(void)
{
	platform_halt();
}

void
cpu_startup(void)
{
	struct pcpu *pcpu;
	paddr_t pcpu_addr;
	int error;

	/*
	 * We don't use the gp, set it to NULL.
	 */
	__asm __volatile ("move $" STRING(gp) ", $" STRING(zero) : : : "memory");

	/*
	 * Set kernel mode.
	 */
	cpu_write_status(KERNEL_STATUS);

	/* Allocate a page for persistent per-CPU data.  */
	error = page_alloc(&kernel_vm, PAGE_FLAG_DEFAULT | PAGE_FLAG_ZERO,
			   &pcpu_addr);
	if (error != 0)
		panic("cpu%u: page allocate failed: %m", mp_whoami(), error);
	pcpu = (struct pcpu *)XKPHYS_MAP(XKPHYS_CNC, pcpu_addr);

	/* Keep our original physical address in the PCPU data, too.  */
	pcpu->pc_physaddr = pcpu;

	/* Identify the CPU.  */
	pcpu->pc_cpuinfo = cpu_identify();
	pcpu->pc_flags = PCPU_FLAG_RUNNING;

	/* Setup the scheduler.  */
	scheduler_cpu_setup(&pcpu->pc_scheduler);

	/* Clear the TLB and add a wired mapping for my per-CPU data.  */
	tlb_init(pcpu_addr);

	/* Now we can take VM-related exceptions appropriately.  */

	/*
	 * XXX push to CPU device attachment.
	 */
	/* Kick off interrupts.  */
	cpu_interrupt_initialize();

	/* Return to the platform code.  */
}
