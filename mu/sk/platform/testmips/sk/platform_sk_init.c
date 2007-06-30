#include <sk/types.h>
#include <sk/sk.h>
#include <sk/string.h>

extern char __bss_start[], _end[];

void
sk_platform_init(void)
{
	/*
	 * Clear the BSS.
	 */
	memset(__bss_start, 0, _end - __bss_start);
}
