#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <io/device/console/console.h>

/*
 * XXX
 * All IPIs are fast interrupts for now and run in a stolen context.  They
 * are not threads and may not block, or hilarity will ensue.
 */

struct ipi_handler {
	struct spinlock ih_lock;
	unsigned ih_usecnt;
	mp_ipi_handler_t *ih_handler;
	void *ih_arg;
};

static struct ipi_handler ipi_handlers[IPI_LAST + 1];

static void mp_ipi_handler(void *, enum ipi_type);

static void
mp_ipi_handlers_init(void *arg)
{
	struct ipi_handler *ih;
	enum ipi_type ipi;

	for (ipi = IPI_FIRST; ipi <= IPI_LAST; ipi++) {
		ih = &ipi_handlers[ipi];
		spinlock_init(&ih->ih_lock, "ipi");
		ih->ih_usecnt = 0;
		ih->ih_handler = NULL;
		ih->ih_arg = NULL;
	}
	mp_ipi_register(IPI_STOP, mp_ipi_handler, NULL);
}
STARTUP_ITEM(mp_ipi_handlers, STARTUP_MP, STARTUP_BEFORE,
	     mp_ipi_handlers_init, NULL);

void
mp_ipi_receive(enum ipi_type ipi)
{
	struct ipi_handler *ih;

	ASSERT(ipi >= IPI_FIRST || ipi <= IPI_LAST, "invalid IPI");
	ih = &ipi_handlers[ipi];
	spinlock_lock(&ih->ih_lock);
	if (ih->ih_handler == NULL) {
		spinlock_unlock(&ih->ih_lock);
		panic("stray IPI %u", ipi);
	}
	ih->ih_usecnt++;
	spinlock_unlock(&ih->ih_lock);
	/*
	 * XXX
	 * use sleepq and a thread.
	 */
	ih->ih_handler(ih->ih_arg, ipi);
	/*
	 * XXX atomic increment/decrement.
	 */
	spinlock_lock(&ih->ih_lock);
	ih->ih_usecnt--;
	spinlock_unlock(&ih->ih_lock);
}

void
mp_ipi_register(enum ipi_type ipi, mp_ipi_handler_t handler, void *arg)
{
	struct ipi_handler *ih;

	ASSERT(ipi >= IPI_FIRST || ipi <= IPI_LAST, "invalid IPI");
	ih = &ipi_handlers[ipi];
	spinlock_lock(&ih->ih_lock);
	ASSERT(ih->ih_handler == NULL, "cannot share ipi");
	ASSERT(ih->ih_usecnt == 0, "cannot register handler for in-use ipi");
	ih->ih_handler = handler;
	ih->ih_arg = arg;
	spinlock_unlock(&ih->ih_lock);
}

void
mp_ipi_send(cpu_id_t cpu, enum ipi_type ipi)
{
	ASSERT(ipi >= IPI_FIRST || ipi <= IPI_LAST, "invalid IPI");
	if (cpu == mp_whoami()) {
		mp_ipi_receive(ipi);
	} else {
		cpu_mp_ipi_send(cpu, ipi);
	}
}

void
mp_ipi_send_all(enum ipi_type ipi)
{
	ASSERT(ipi >= IPI_FIRST || ipi <= IPI_LAST, "invalid IPI");
	mp_ipi_send_but(mp_whoami(), ipi);
	mp_ipi_send(mp_whoami(), ipi);
}

void
mp_ipi_send_but(cpu_id_t cpu, enum ipi_type ipi)
{
	ASSERT(ipi >= IPI_FIRST || ipi <= IPI_LAST, "invalid IPI");
	cpu_mp_ipi_send_but(cpu, ipi);
}

static void
mp_ipi_handler(void *arg, enum ipi_type ipi)
{
	switch (ipi) {
	case IPI_STOP:
		/* XXX Is this the right place to do it?  */
		mp_cpu_stopped(mp_whoami());
		kcprintf("cpu%u: stopped.\n", mp_whoami());
		for (;;)
			continue;
#if 0
		cpu_halt();
#endif
	default:
		panic("%s: unhandled ipi %u", __func__, (unsigned)ipi);
	}
}
