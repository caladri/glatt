#include <core/types.h>
#include <core/mp.h>
#include <core/pool.h>
#include <core/startup.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <core/console.h>

static struct pool interrupt_handler_pool;

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
	if (!STAILQ_EMPTY(&PCPU_GET(interrupt_table)[interrupt])) {
		kcprintf("cpu%u: sharing interrupt %d.\n", mp_whoami(),
			 interrupt);
	}
	ih = pool_allocate(&interrupt_handler_pool);
	ih->ih_func = func;
	ih->ih_arg = arg;
	STAILQ_INSERT_TAIL(&PCPU_GET(interrupt_table)[interrupt], ih, ih_link);
	PCPU_SET(interrupt_mask, PCPU_GET(interrupt_mask) |
		 ((1 << interrupt) << CP0_STATUS_INTERRUPT_SHIFT));
	cpu_write_status((cpu_read_status() & ~CP0_STATUS_INTERRUPT_MASK) |
			 PCPU_GET(interrupt_mask));
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
		if (STAILQ_EMPTY(&PCPU_GET(interrupt_table)[interrupt])) {
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
			continue;
		}
		STAILQ_FOREACH(ih, &PCPU_GET(interrupt_table)[interrupt],
			       ih_link) {
			ih->ih_func(ih->ih_arg, interrupt);
		}
	}
	ASSERT(interrupts == 0, "must handle all interrupts");
}

register_t
cpu_interrupt_disable(void)
{
	register_t status;

	status = cpu_read_status();
	if ((status & CP0_STATUS_IE) != 0) {
		cpu_write_status(status & ~CP0_STATUS_IE);
		PCPU_SET(interrupt_enable, 0);
	}
	return (status & CP0_STATUS_IE);
}

void
cpu_interrupt_restore(register_t r)
{
	ASSERT(r != CP0_STATUS_IE || (cpu_read_status() & CP0_STATUS_IE) == 0,
	       "Cannot re-enable interrupts twice!");
	if (r == CP0_STATUS_IE &&
	    (cpu_read_status() & CP0_STATUS_IE) == 0) {
		PCPU_SET(interrupt_enable, 1);
		cpu_write_status((cpu_read_status() &
				  ~CP0_STATUS_INTERRUPT_MASK) |
				 PCPU_GET(interrupt_mask) | r);
	}
}

void
cpu_interrupt_setup(void)
{
	unsigned interrupt;

	PCPU_SET(interrupt_enable, 1);
	PCPU_SET(interrupt_mask, 0);
	for (interrupt = 0; interrupt < CPU_INTERRUPT_COUNT; interrupt++)
		STAILQ_INIT(&PCPU_GET(interrupt_table)[interrupt]);
	ASSERT((cpu_read_status() & CP0_STATUS_IE) == 0,
	       "Can't reenable interrupts.");
	cpu_write_cause(0);
	cpu_write_status((cpu_read_status() & ~CP0_STATUS_INTERRUPT_MASK) |
			 PCPU_GET(interrupt_mask));
	cpu_write_status(cpu_read_status() | CP0_STATUS_IE);
}

static void
cpu_interrupt_init(void *arg)
{
	int error;

	error = pool_create(&interrupt_handler_pool, "INTERRUPT HANDLER",
			    sizeof (struct interrupt_handler), POOL_DEFAULT);
	if (error != 0)
		panic("%s: pool_created failed: %m", __func__, error);
}
STARTUP_ITEM(cpu_interrupt, STARTUP_POOL, STARTUP_FIRST, cpu_interrupt_init,
	     NULL);
