#include <core/types.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <db/db.h>
#include <io/device/console/console.h>

void
cpu_interrupt_establish(int interrupt, interrupt_t *func, void *arg)
{
	struct interrupt_handler *ih;

	/*
	 * XXX
	 * critical section
	 */
	ASSERT(interrupt >= 0 && interrupt < CPU_INTERRUPT_COUNT,
	       "invalid interrupt number");
	ih = &PCPU_GET(interrupt_table)[interrupt];
	ASSERT(ih->ih_func == NULL, "cannot share interrupts");
	ih->ih_func = func;
	ih->ih_arg = arg;
	PCPU_SET(interrupt_mask, PCPU_GET(interrupt_mask) |
		 ((1 << interrupt) << CP0_STATUS_INTERRUPT_SHIFT));
	cpu_write_status((cpu_read_status() & ~CP0_STATUS_INTERRUPT_MASK) |
			 PCPU_GET(interrupt_mask));
#if 0
	kcprintf("cpu%u: %d established (mask %x).\n", mp_whoami(), interrupt,
		 PCPU_GET(interrupt_mask) >> CP0_STATUS_INTERRUPT_SHIFT);
#endif
}

void
cpu_interrupt(void)
{
	struct interrupt_handler *ih;
	unsigned cause, interrupts;
	unsigned interrupt;

	cause = cpu_read_cause();
	interrupts = cause & CP0_CAUSE_INTERRUPT_MASK;
	interrupts &= PCPU_GET(interrupt_mask);
	interrupts >>= CP0_CAUSE_INTERRUPT_SHIFT;
	cause &= ~CP0_CAUSE_INTERRUPT_MASK;
	cpu_write_cause(cause);

	ASSERT(interrupts != 0, "need interrupts to service");

	for (interrupt = 0; interrupt < CPU_INTERRUPT_COUNT; interrupt++) {
		if (interrupts == 0)
			return;
		if ((interrupts & 1) == 0) {
			interrupts >>= 1;
			continue;
		}
		interrupts >>= 1;
		ih = &PCPU_GET(interrupt_table)[interrupt];
		if (ih->ih_func == NULL) {
			kcprintf("cpu%u: stray interrupt %u",
				 mp_whoami(), interrupt);
			kcprintf(" mask&interrupt = %x",
				 PCPU_GET(interrupt_mask) &
				 ((1 << interrupt) <<
				 CP0_STATUS_INTERRUPT_SHIFT));
			kcprintf(" status&interrupt = %x",
				 cpu_read_status() &
				 ((1 << interrupt) <<
				 CP0_STATUS_INTERRUPT_SHIFT));
			kcprintf("\n");
		} else
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
	ASSERT((cpu_read_status() & CP0_STATUS_IE) == 0,
	       "Can't reenable interrupts.");
	cpu_write_status(cpu_read_status() | CP0_STATUS_IE);
}

void
cpu_interrupt_restore(register_t r)
{
	if (r == CP0_STATUS_IE &&
	    (cpu_read_status() & CP0_STATUS_IE) == 0) {
		cpu_write_status((cpu_read_status() &
				  ~CP0_STATUS_INTERRUPT_MASK) |
				 PCPU_GET(interrupt_mask));
		cpu_write_status(cpu_read_status() | r);
	}
}
