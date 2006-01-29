#include <core/types.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>

register_t
cpu_interrupt_disable(void)
{
	return (cpu_read_status());
}

void
cpu_interrupt_enable(void)
{
}

void
cpu_interrupt_restore(register_t r)
{
	if (cpu_read_status() != r)
		cpu_write_status(r);
}
