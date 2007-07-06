#include <sk/types.h>
#include <sk/console.h>
#include <sk/sk.h>
#include <sk/string.h>
#include <cpu/sk/memory.h>

extern char __bss_start[], _end[];

static void sk_platform_console_putchar(void *, char);

static struct sk_console sk_platform_console = {
	.skc_name	= "platform",
	.skc_putchar	= sk_platform_console_putchar,
	.skc_private	= XKPHYS_MAP_DEV(0x10000000),
};

void
sk_platform_init(void)
{
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
	 * Tell the supervisor what memory is aailable.
	 */

	/*
	 * Install the supervisor on this CPU as normal.
	 */
	sk_supervisor_install();

	/*
	 * Tell the supervisor what CPUs are present.  It will start them
	 * one by one.
	 */
}

static void
sk_platform_console_putchar(void *private, char ch)
{
	volatile char *putchar = (volatile char *)private;
	*putchar = ch;
}
