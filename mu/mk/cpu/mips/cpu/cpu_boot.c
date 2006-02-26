#include <core/types.h>
#include <cpu/boot.h>
#include <platform/platform.h>

void
cpu_halt(void)
{
	platform_halt();
}
