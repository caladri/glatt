#include <core/types.h>
#include <core/critical.h>
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
