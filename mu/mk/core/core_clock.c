#include <core/types.h>
#include <core/clock.h>
#include <cpu/pcpu.h>

clock_ticks_t
clock(void)
{
	return (PCPU_GET(clock));
}

void
clock_ticks(clock_ticks_t ticks)
{
	PCPU_SET(clock, PCPU_GET(clock) + ticks);
}
