#include <core/types.h>
#include <core/mp.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/pcpu.h>
#include <cpu/startup.h>
#ifdef DB
#include <db/db_command.h>
#endif
#include <vm/vm.h>
#include <vm/vm_page.h>

#ifdef DB
DB_COMMAND_TREE(cpu, root, cpu);
#endif

static struct pcpu pcpu_array[MAXCPUS];

void
cpu_break(void)
{
#if 0
	asm volatile ("break 7" : : : "memory");
#else
	platform_halt();
#endif
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

	/*
	 * Set kernel mode.
	 */

	/* Set up PCPU.  */
	pcpu = &pcpu_array[mp_whoami()];
	memset(pcpu, 0, sizeof *pcpu);
	asm volatile ("mtsprg 0, %[pcpu]" : [pcpu] "=&r"(pcpu));

	/* Identify the CPU.  */
	/* XXX */

	/* Actually set up the TLB.  */
	/* XXX */

	/* Now we can take VM-related exceptions appropriately.  */
	pcpu->pc_flags = PCPU_FLAG_RUNNING;
	startup_early = false;

	/* Set up interrupts.  */
#if 0
	cpu_interrupt_setup();
#endif
}

void
cpu_startup_thread(void *arg)
{
	void (*callback)(void *);

	callback = (void (*)(void *))(uintptr_t)arg;
	platform_startup_thread();
	callback(NULL);
}

void
cpu_wait(void)
{
	/* Nothing.  */
}
