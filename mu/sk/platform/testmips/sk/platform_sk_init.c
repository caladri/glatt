#include <sk/types.h>
#include <sk/console.h>
#include <sk/sk.h>
#include <sk/string.h>
#include <supervisor/cpu.h>
#include <supervisor/memory.h>
#include <cpu/sk/memory.h>

extern char __bss_start[], _end[];

static void sk_platform_console_putchar(void *, char);

static struct sk_console sk_platform_console = {
	.skc_name	= "platform",
	.skc_putchar	= sk_platform_console_putchar,
	.skc_private	= XKPHYS_MAP_DEV(0x10000000),
};

#define	MP_READ64(offset)						\
	(*(volatile uint64_t *)XKPHYS_MAP_DEV(0x11000000 | (offset)))

void
sk_platform_init(void)
{
	cpu_id_t cpu, whoami;
	size_t membytes;
	uint64_t ncpus;
	paddr_t base;

	/*
	 * Clear the BSS.
	 */
	memset(__bss_start, 0, _end - __bss_start);

	/*
	 * Initialize a console.
	 */
	sk_console_set(&sk_platform_console);

	sk_puts("\n");
	sk_puts("Glatt Supervised Kernel.\n");
	sk_puts("Copyright (c) 2005-2007 The Positry.  All rights reserved.\n");
	sk_puts("\n");

	/*
	 * Tell the supervisor what memory is available.
	 *
	 * We start at 5MB, to leave 1MB for exception vectors and 4MB for
	 * the kernel.  XXX Start at _end.
	 *
	 * XXX Check that there is enough memory.
	 */
	membytes = MP_READ64(0x90);
	base = 5ul * 1024 * 1024;
	membytes -= base;
	supervisor_memory_insert(base, membytes, MEMORY_GLOBAL);

	/*
	 * Tell the supervisor how many CPUs are available.
	 */
	whoami = MP_READ64(0x00);
	ncpus = MP_READ64(0x10);

	supervisor_cpu_add(whoami);
	for (cpu = 0; cpu < ncpus; cpu++) {
		if (cpu != whoami)
			supervisor_cpu_add_child(whoami, cpu);
	}

	/*
	 * Install the supervisor on this CPU as normal.
	 */
	sk_supervisor_install();
}

static void
sk_platform_console_putchar(void *private, char ch)
{
	volatile char *putchar = (volatile char *)private;
	*putchar = ch;
}
