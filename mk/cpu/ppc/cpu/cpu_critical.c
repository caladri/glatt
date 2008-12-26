#include <core/types.h>
#include <core/critical.h>
#if 0
#include <core/startup.h>
#endif
#include <cpu/interrupt.h>

critical_section_t
cpu_critical_enter(void)
{
	return (cpu_interrupt_disable());
}

void
cpu_critical_exit(critical_section_t crit)
{
	cpu_interrupt_restore(crit);
}

bool
cpu_critical_section(void)
{
	panic("Cannot check critical section yet.");
#if 0
	ASSERT(!startup_early,
	       "Cannot check critical section during early startup.");
	if ((cpu_read_status() & CP0_STATUS_IE) == 0) {
		ASSERT(PCPU_GET(interrupt_enable) == 0,
		       "Interrupt enable bit not set but flag is.");
		return (true);
	}
	ASSERT(PCPU_GET(interrupt_enable) == 1,
	       "Interrupt enable bit set but flag is not.");
	return (false);
#endif
}
