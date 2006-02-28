#include <core/types.h>
#include <core/startup.h>
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
	cpu_interrupt_initialize();

	/* Return to the platform code.  */
}
