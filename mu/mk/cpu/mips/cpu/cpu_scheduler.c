#include <core/types.h>
#include <cpu/scheduler.h>

void
cpu_scheduler_yield(void)
{
	__asm __volatile ("wait");
}
