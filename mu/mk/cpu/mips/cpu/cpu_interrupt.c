#include <core/types.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <db/db.h>

void
cpu_hard_interrupt_establish(int interrupt, interrupt_t *func, void *arg)
{
	struct interrupt_handler *ih;

	ASSERT(interrupt >= 0 && interrupt < CPU_HARD_INTERRUPT_COUNT,
	       "invalid interrupt number");
	ih = &PCPU_GET(hard_interrupt)[interrupt];
	ASSERT(ih->ih_func == NULL, "cannot share interrupts");
	ih->ih_func = func;
	ih->ih_arg = arg;
	cpu_write_status(cpu_read_status() |
			 ((1 << interrupt) << CP0_STATUS_INT_HARD_SHIFT));
}

void
cpu_soft_interrupt_establish(int interrupt, interrupt_t *func, void *arg)
{
	struct interrupt_handler *ih;

	ASSERT(interrupt >= 0 && interrupt < CPU_SOFT_INTERRUPT_COUNT,
	       "invalid interrupt number");
	ih = &PCPU_GET(soft_interrupt)[interrupt];
	ASSERT(ih->ih_func == NULL, "cannot share interrupts");
	ih->ih_func = func;
	ih->ih_arg = arg;
	cpu_write_status(cpu_read_status() |
			 ((1 << interrupt) << CP0_STATUS_INT_SOFT_SHIFT));
}

void
cpu_interrupt(void)
{
	panic("interrupt!");
	for (;;) continue;
}

void
cpu_interrupt_initialize(void)
{
	cpu_interrupt_enable();
}

register_t
cpu_interrupt_disable(void)
{
	return (cpu_read_status() & CP0_STATUS_IE);
}

void
cpu_interrupt_enable(void)
{
	cpu_write_status(cpu_read_status() | CP0_STATUS_IE);
}

void
cpu_interrupt_restore(register_t r)
{
	if ((cpu_read_status() & CP0_STATUS_IE) != r)
		cpu_write_status(cpu_read_status() | r);
}
