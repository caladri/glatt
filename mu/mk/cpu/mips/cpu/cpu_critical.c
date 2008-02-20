#include <core/types.h>
#include <core/critical.h>
#include <core/startup.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <db/db.h>

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
	ASSERT(!startup_early,
	       "Cannot check critical section during early startup.");
#if 0
	return ((cpu_read_status() & CP0_STATUS_IE) == 0);
#endif
	return (PCPU_GET(interrupt_enable) == 0);
}
