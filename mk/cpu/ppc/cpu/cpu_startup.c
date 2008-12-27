#include <core/types.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/pcpu.h>
#include <cpu/startup.h>
#ifdef DB
#include <db/db_command.h>
#endif
#include <vm/page.h>
#include <vm/vm.h>

#ifdef DB
DB_COMMAND_TREE(cpu, root, cpu);
#endif

volatile bool startup_early = true;

COMPILE_TIME_ASSERT(sizeof (struct pcpu) <= PAGE_SIZE);

void
cpu_break(void)
{
#if 0
	__asm __volatile ("break 7" : : : "memory");
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
	struct vm_page *pcpu_page;
	paddr_t pcpu_addr;
	int error;

	/*
	 * Set kernel mode.
	 */

	/* Allocate a page for persistent per-CPU data.  */
	error = page_alloc(PAGE_FLAG_DEFAULT | PAGE_FLAG_ZERO, &pcpu_page);
	if (error != 0)
		panic("cpu%u: page allocate failed: %m", mp_whoami(), error);
	pcpu_addr = page_address(pcpu_page);
	pcpu = (struct pcpu *)(void *)pcpu_page; /* XXX*/

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
