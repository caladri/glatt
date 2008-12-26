#include <core/types.h>
#include <cpu/interrupt.h>

register_t
cpu_interrupt_disable(void)
{
	panic("%s: not yet implemented.", __func__);
}

void
cpu_interrupt_restore(register_t r)
{
	panic("%s: not yet implemented.", __func__);
}
