#include <core/types.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <db/db.h>
#include <io/device/console/console.h>

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
	struct interrupt_handler *ih;
	unsigned cause, interrupts;
	unsigned interrupt;

	cause = cpu_read_cause();
	interrupts = (cause & CP0_CAUSE_INT_MASK) >> CP0_CAUSE_INT_SOFT_SHIFT;
	cause &= ~CP0_CAUSE_INT_MASK;
	cpu_write_cause(cause);

	for (interrupt = 0; interrupt < CPU_SOFT_INTERRUPT_COUNT; interrupt++) {
		if (interrupts == 0)
			return;
		if ((interrupts & 1) == 0) {
			interrupts >>= 1;
			continue;
		}
		interrupts >>= 1;
		ih = &PCPU_GET(soft_interrupt)[interrupt];
		if (ih->ih_func == NULL)
			kcprintf("cpu%u: stray soft interrupt %u.\n",
				 mp_whoami(), interrupt);
		else
			ih->ih_func(ih->ih_arg, interrupt);
	}
	for (interrupt = 0; interrupt < CPU_HARD_INTERRUPT_COUNT; interrupt++) {
		if (interrupts == 0)
			return;
		if ((interrupts & 1) == 0) {
			interrupts >>= 1;
			continue;
		}
		interrupts >>= 1;
		ih = &PCPU_GET(hard_interrupt)[interrupt];
		if (ih->ih_func == NULL)
			kcprintf("cpu%u: stray hard interrupt %u.\n",
				 mp_whoami(), interrupt);
		else
			ih->ih_func(ih->ih_arg, interrupt);
	}
	ASSERT(interrupts == 0, "must handle all interrupts");
}

void
cpu_interrupt_initialize(void)
{
	cpu_interrupt_enable();
}

register_t
cpu_interrupt_disable(void)
{
	register_t status;

	status = cpu_read_status();
	if ((status & CP0_STATUS_IE) != 0)
		cpu_write_status(status & ~CP0_STATUS_IE);
	return (status & CP0_STATUS_IE);
}

void
cpu_interrupt_enable(void)
{
	cpu_write_status(cpu_read_status() | CP0_STATUS_IE);
}

void
cpu_interrupt_restore(register_t r)
{
	if (r == CP0_STATUS_IE &&
	    (cpu_read_status() & CP0_STATUS_IE) == 0)
		cpu_write_status(cpu_read_status() | r);
}
