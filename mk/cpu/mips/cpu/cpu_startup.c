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
cpu_startup(paddr_t pcpu_addr)
{
	struct pcpu *pcpu;

	/*
	 * We don't use the gp, set it to NULL.
	 */
	asm volatile ("move $" STRING(gp) ", $" STRING(zero) : : : "memory");

	/*
	 * Set kernel mode.  We're still using BEVs.
	 *
	 * XXX Unless we're being called for a second CPU.
	 */
	cpu_write_status(KERNEL_STATUS | CP0_STATUS_BEV);

	/* Direct-map the PCPU data until the TLB is up.  */
	pcpu = (struct pcpu *)XKPHYS_MAP(CCA_CNC, pcpu_addr);
	memset(pcpu, 0, sizeof *pcpu);

	/* Identify the CPU.  */
	cpu_identify(&pcpu->pc_cpuinfo);
	pcpu->pc_cpuid = mp_whoami();

	/* Clear the TLB and add a wired mapping for my per-CPU data.  */
	tlb_init(pcpu_addr, pcpu->pc_cpuinfo.cpu_ntlbs);

	/* Now we can take VM-related exceptions appropriately.  */
	pcpu->pc_flags = PCPU_FLAG_RUNNING;
	startup_early = false;
}

void
cpu_startup_thread(void *arg)
{
	void (*callback)(void *);

	callback = (void (*)(void *))(uintptr_t)arg;
	platform_startup_thread();
	callback(NULL);
}
