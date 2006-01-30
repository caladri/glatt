#include <core/types.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <io/device/console/console.h>

register_t
cpu_interrupt_disable(void)
{
	return (cpu_read_status() & CP0_STATUS_IE);
}

void
cpu_interrupt_enable(void)
{
	kcprintf("cpu%u: interrupts enabled.\n", mp_whoami());
	cpu_write_status(cpu_read_status() | CP0_STATUS_IE);
}

void
cpu_interrupt_restore(register_t r)
{
	if ((cpu_read_status() & CP0_STATUS_IE) != r)
		cpu_write_status(cpu_read_status() | r);
}
