#include <core/types.h>
#include <cpu/interrupt.h>

register_t
cpu_interrupt_disable(void)
{
	return (0);
}

void
cpu_interrupt_restore(register_t r)
{
}
