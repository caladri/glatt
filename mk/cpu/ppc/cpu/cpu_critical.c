#include <core/types.h>
#include <core/critical.h>
#include <core/startup.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>

void
cpu_critical_enter(void)
{
	ASSERT(!startup_early,
	       "Cannot enter critical section during early startup.");

	if (PCPU_GET(critical_count) == 0)
		PCPU_SET(critical_section, cpu_interrupt_disable());
	PCPU_SET(critical_count, PCPU_GET(critical_count) + 1);
}

void
cpu_critical_exit(void)
{
	ASSERT(!startup_early,
	       "Cannot exit critical section during early startup.");

	PCPU_SET(critical_count, PCPU_GET(critical_count) - 1);
	if (PCPU_GET(critical_count) == 0)
		cpu_interrupt_restore(PCPU_GET(critical_section));
}

bool
cpu_critical_section(void)
{
	ASSERT(!startup_early,
	       "Cannot check critical section during early startup.");
	return (PCPU_GET(critical_count) > 0);
}
