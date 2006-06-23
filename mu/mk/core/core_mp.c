#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <db/db_command.h>
#include <io/device/console/console.h>

struct ipi_handler {
	struct spinlock ih_lock;
	unsigned ih_usecnt;
	mp_ipi_handler_t *ih_handler;
	void *ih_arg;
};
#define	IPI_HANDLER_INUSE	(0x0001)

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
		kcprintf("cpu%u: stopped.\n", mp_whoami());
		cpu_halt();
	default:
		panic("%s: unhandled ipi %u", __func__, (unsigned)ipi);
	}
}

static void
mp_db_ipi_list(void)
{
	struct ipi_handler *ih;
	enum ipi_type ipi;

	kcprintf("IPI\tUSECNT\tHANDLER(ARG)\n");
	for (ipi = IPI_FIRST; ipi <= IPI_LAST; ipi++) {
		ih = &ipi_handlers[ipi];
		kcprintf("%u\t%u\t%p(%p)\n", ipi, ih->ih_usecnt,
			 (void *)(uintptr_t)ih->ih_handler, ih->ih_arg);
	}
}
DB_COMMAND(ipi_list, mp_db_ipi_list, "List IPI handlers.");

static void
mp_db_whoami(void)
{
	kcprintf("cpu%u\n", mp_whoami());
}
DB_COMMAND(whoami, mp_db_whoami, "Show which CPU the debugger is on.");
