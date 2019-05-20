#include <core/types.h>
#include <core/critical.h>
#include <core/startup.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>

void
cpu_critical_enter(void)
{
	register_t s;

	ASSERT(!startup_early,
	       "Cannot enter critical section during early startup.");

	s = cpu_interrupt_disable();

	if (PCPU_GET(critical_count) == 0) {
		PCPU_SET(critical_section, s);
		PCPU_SET(critical_count, 1);
	} else {
		PCPU_SET(critical_count, PCPU_GET(critical_count) + 1);
	}
	ASSERT(PCPU_GET(critical_count) > 0,
	       "Must be in a critical section after entering one.");
}

void
cpu_critical_exit(void)
{
	register_t s;

	ASSERT(!startup_early,
	       "Cannot exit critical section during early startup.");

	s = PCPU_GET(critical_section);

	ASSERT(PCPU_GET(critical_count) > 0,
	       "Must be in a critical section to exit one.");
	if (PCPU_GET(critical_count) == 1) {
		PCPU_SET(critical_section, 0);
		PCPU_SET(critical_count, 0);
		cpu_interrupt_restore(s);
	} else {
		PCPU_SET(critical_count, PCPU_GET(critical_count) - 1);
	}
}

bool
cpu_critical_section(void)
{
	ASSERT(!startup_early,
	       "Cannot check critical section during early startup.");
	return (PCPU_GET(critical_count) > 0);
}
