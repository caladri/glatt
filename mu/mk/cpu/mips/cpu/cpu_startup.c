#include <core/types.h>
#include <core/startup.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/tlb.h>
#include <vm/page.h>
#include <vm/vm.h>

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

	/* Start off in 64-bit mode.  */
	cpu_write_status(CP0_STATUS_KX);

	/* Allocate a page for persistent per-CPU data.  */
	error = page_alloc(&kernel_vm, &pcpu_addr);
	if (error != 0)
		panic("cpu%u: page allocate failed: %m", mp_whoami(), error);
	pcpu = (struct pcpu *)XKPHYS_MAP(XKPHYS_CNC, pcpu_addr);

	/* Identify the CPU.  */
	pcpu->pc_cpuinfo = cpu_identify();
	pcpu->pc_flags = PCPU_FLAG_RUNNING;

	/* Clear the TLB and add a wired mapping for my per-CPU data.  */
	tlb_init(pcpu_addr);

	/* Now we can take VM-related exceptions appropriately.  */

	/* Kick off interrupts.  */
	cpu_interrupt_initialize();

	/* Return to the platform code.  */
}
