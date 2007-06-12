#include <core/types.h>
#include <core/clock.h>
#include <cpu/pcpu.h>

static bool clocks_running;

clock_ticks_t
clock(void)
{
	if (!clocks_running)
		return (0);
	return (PCPU_GET(clock));
}

void
clock_ticks(clock_ticks_t ticks)
{
	clocks_running = true;
	PCPU_SET(clock, PCPU_GET(clock) + ticks);
}
